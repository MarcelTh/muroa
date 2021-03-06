/*
 * CMediaScannerCtrl.cpp
 *
 *  Created on: 24 Oct 2010
 *      Author: martin
 */

#include "CMediaScannerCtrl.h"

#include "CMuroaServer.h"

#include <signal.h>
#include <stdint.h>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include "mediascanner/CMsgError.h"


void sigusr1_handler(int signum) {
	// fprintf(stderr, "sigusr1 handler!!!!!\n");
	return;
}

using namespace std;

CMediaScannerCtrl::CMediaScannerCtrl(CMuroaServer *parent) : m_thread(0),
		                                                     m_child_running(false),
		                                                     m_pid(0),
		                                                     m_parent(parent) {

	struct sigaction new_action;

    /* Set up the structure to specify the new action. */
    new_action.sa_handler = sigusr1_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;

    int retval = sigaction(SIGUSR1, &new_action, &m_old_action);
    if(retval != 0)
    	perror("sigaction");
}

CMediaScannerCtrl::~CMediaScannerCtrl() {
    sigaction (SIGUSR1, &m_old_action, NULL);
}


void CMediaScannerCtrl::start() {

	if(running() || m_pid != 0 ) return;

	vector<string> args;

	int sv[2];
	int retval = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	if(retval != 0) {
		perror("CMediaScannerCtrl::start:");
	}

	setSocket( sv[0] );

	stringstream ss;
	ss << sv[1];
	args.push_back(ss.str());

	m_pid = m_subProcess.start("./build/mediascanner/mediascanner" , args ,0 ,0);

	if(m_pid == 0) {
		// child pid is still 0, maybe execve did mot work,but we are in child process.
		// send exit msg to parent process.

		// following lines are equal to sendEvent(msg) but in reverse order (other socket)
		CMsgError *msg = new CMsgError( 1, m_subProcess.getLastErrorMsg() );
		ssize_t written;
		int size;
		const char * buffer = msg->serialize(size);
		written = write(sv[1], buffer, size);
		// end of reverse sendEvent(msg)

		exit(1);
	}
	else {

		m_child_running = true;
		m_waitthread = new thread( &CMediaScannerCtrl::waitPid, this);
		m_thread = new thread( &CEventLoop::run, this);
	}
}

void CMediaScannerCtrl::stop() {
	CMsgQuit quitmsg;

	sendEvent(&quitmsg);
	terminate();
}

void CMediaScannerCtrl::terminate() {
	CMsgQuit quitmsg;
	postEvent(&quitmsg);

	std::unique_lock<std::mutex> lk(m_mutex);
	int tries = 0;
	while(m_child_running) {
		cv_status cv_return_cause = m_cond_var.wait_for(lk, std::chrono::milliseconds(100));

		cerr << "m_cond_var.wait_for returned due to ";
		if(cv_return_cause == cv_status::timeout) {
			switch (tries) {
				case 0:
				case 1:
					cerr << "timeout -> send SIGTERM to mediascanner process" << endl;
					kill(m_pid, SIGTERM);
					break;
				case 2:
				default:
					cerr << "timeout -> send SIGKILL to mediascanner process" << endl;
					kill(m_pid, SIGKILL);
			}
		}
		else {
			cerr << "notification from waitpid -> mediascanner process exited." << endl;
		}
		tries++;
	}

	// interrupt blocking recv in loop() out
	pthread_kill(m_thread->native_handle(), SIGUSR1);
	if(m_thread != 0) {
		m_thread->join();
		delete m_thread;
		m_thread = 0;
	}

	if(m_waitthread != 0) {
		m_waitthread->join();
		delete m_waitthread;
		m_waitthread = 0;
	}
}


bool CMediaScannerCtrl::handleMsg(CMsgBase* msg) {
	bool rc = true;
	int type = msg->getType();
	switch(type) {
	    case E_MSG_ERR:
	    {
			CMsgError* error = reinterpret_cast<CMsgError*>(msg);
			cerr << "CMediaScannerCtrl::handleMsg Error: " << error->getMessage() << endl;
			m_parent->reportError(error->getErrorCode(), error->getMessage());
			break;
	    }
		case E_MSG_RESP:
		{
			CMsgResponse* reponse = reinterpret_cast<CMsgResponse*>(msg);
			m_parent->response(reponse->getRequestID(), reponse->getReturnCode(), reponse->getMessage());
			break;
		}
		case E_MSG_COLLECTION_CHANGED:
		{
			CMsgCollectionChanged* colChMsg = reinterpret_cast<CMsgCollectionChanged*>(msg);
			m_parent->collectionChanged(colChMsg->getNewRev(), colChMsg->getMinRev(), colChMsg->getMaxRev());
			break;
		}
		case E_MSG_PROGRESS:
		{
			CMsgProgress* progMsg = reinterpret_cast<CMsgProgress*>(msg);
			m_parent->scanProgress(progMsg->getProgress());
			break;
		}
		case E_MSG_FINISHED:
		{
			CMsgFinished* finiMsg = reinterpret_cast<CMsgFinished*>(msg);
			m_parent->jobFinished(finiMsg->getJobID());
			break;
		}
		case E_MSG_QUIT:
		{
			CMsgQuit* quitMsg = reinterpret_cast<CMsgQuit*>(msg);
			rc = false;
			break;
		}

	}
	return rc;
}


void CMediaScannerCtrl::waitPid() {
	int status;
	cerr << "waiting for process to exit" << endl;

	pid_t retval = -1;
	while(retval == -1) {
		errno = 0;
		cerr << "waitpid: waiting for pid:" << m_pid << endl;
		retval = waitpid(m_pid, &status, 0);
		if(retval == -1) {
			cerr << "waitpid returned -1: " << endl;
			cerr << strerror(errno) << endl;
		}
		if(retval > 0) {
		    std::lock_guard<std::mutex> lk(m_mutex);
			cerr << "process " << retval << " changed state. Was waiting for " << m_pid << endl;
			cerr << strerror(errno) << endl;
			m_child_running = false;
			m_pid = 0;
		    m_cond_var.notify_one();
		    m_parent->stopMediaScannerAsync();

		}
	}
	cerr << "mediascanner exited. Status: " << status << endl;
}

