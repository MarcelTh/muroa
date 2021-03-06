/*
 * CMediaScannerTest.cpp
 *
 *  Created on: 16 Nov 2010
 *      Author: martin
 */

#include "CMediaScannerTest.h"

#include "../mediascanner/CMediaScanner.h"
#include "../mediascanner/CFsScanner.h"
#include "../mediascanner/CStateDbUpdater.h"

#include "../mediascanner/CMsgOpenDb.h"
#include "../mediascanner/CMsgScanDir.h"
#include "../mediascanner/CMsgQuit.h"

#include <iostream>
using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION( CMediaScannerTest );

CMediaScannerTest::CMediaScannerTest() {

}

CMediaScannerTest::~CMediaScannerTest() {
	if(m_thread.joinable()) {
		m_thread.join();
	}
}


void CMediaScannerTest::setUp() {
	m_media_scanner = new CMediaScanner();
	m_fs_scanner = new CFsScanner(m_media_scanner);

	vector<string> types;
	types.push_back(".mp3");
	types.push_back(".ogg");

	m_fs_scanner->setFileTypes( types );
}

void CMediaScannerTest::tearDown() {
	delete m_media_scanner;
	delete m_fs_scanner;

	if(m_thread.joinable()) {
		m_thread.join();
	}
}

void CMediaScannerTest::testEventLoop() {

	// m_fs_scanner->scanDir("/home/martin");
	run();
	sleep(30);
	CMsgQuit* quitMsg = new CMsgQuit();
	m_media_scanner->postEvent(quitMsg);
	join();
	CPPUNIT_ASSERT( 1 );
}


void CMediaScannerTest::testScanDirEvent() {
	CMsgOpenDb* openDbMsg = new CMsgOpenDb("testDB.mysql");
	CMsgScanDir* msg = new CMsgScanDir("/home/martin");

	run();

	m_media_scanner->postEvent(openDbMsg);
	m_media_scanner->postEvent(msg);
	sleep(30);
	CMsgQuit* quitMsg = new CMsgQuit();
	m_media_scanner->postEvent(quitMsg);
	join();

}

void CMediaScannerTest::testDbUpdater() {
	CMsgOpenDb* openDbMsg = new CMsgOpenDb("TestDB.sqlite");
	CMsgScanDir* scanDirMsg = new CMsgScanDir("/home/martin");

	run();
	m_media_scanner->postEvent(openDbMsg);

	sleep(1);

	bool found;
	int old_max_rev = m_media_scanner->m_stateDbUpdater->getIntValue("MaxMediaColRev", found);
	CPPUNIT_ASSERT_MESSAGE("'MaxMediaColRev' not found in general table.", found );

	m_media_scanner->postEvent(scanDirMsg);

	sleep(1);
	while(m_media_scanner->getProgress() < 100 ) {
		sleep(1);
	}

	int max_rev, i = 0;
	do
	{
		max_rev = m_media_scanner->m_stateDbUpdater->getIntValue("MaxMediaColRev", found);
		CPPUNIT_ASSERT(found );
		sleep(1);
		i++;
	}while( max_rev == old_max_rev && i < 20 );

	CMsgQuit* quitMsg = new CMsgQuit();
	m_media_scanner->postEvent(quitMsg);
	join();

	CPPUNIT_ASSERT_MESSAGE("Faild to append exactly one revision to collection.", max_rev == old_max_rev + 1 );
	CPPUNIT_ASSERT_MESSAGE("Appending exactly one revision to collection took longer than 20 s.", i < 20 );
}


void CMediaScannerTest::run() {
	m_thread = std::thread(&CMediaScanner::run, m_media_scanner);
}

void CMediaScannerTest::join() {
	if(m_thread.joinable()) {
		m_thread.join();
	}
}
