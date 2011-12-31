/*
 * CStateDB.cpp
 *
 *  Created on: 3 Nov 2010
 *      Author: martin
 */

#include "CRootItem.h"

#include <CMediaItem.h>
#include <CPlaylistItem.h>
#include <CNextlistItem.h>

#include "CStateDB.h"
#include "CSession.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace muroa {

CStateDB::CStateDB(std::string dbFileName): CStateDbBase(dbFileName),
                                            m_updateMediaItemStmt(0),
                                            m_selectMediaItemStmt(0),
                                            m_getMediaItemByPosStmt(0),
                                            m_updatePlaylistItemStmt(0),
                                            m_selectPlaylistItemStmt(0),
                                            m_selectPlaylistRevStmt(0),
                                            m_updateNextlistItemStmt(0),
                                            m_selectNextlistItemStmt(0),
                                            m_selectNextlistRevStmt(0)
{
}

CStateDB::~CStateDB() {
}

int CStateDB::open() {
	int rc = CStateDbBase::open();
	if( rc == 0 ){

		prepareUpdateMediaItemStmt();

		createPlaylistRevisionsTable();
		createNextlistRevisionsTable();

		prepareSelectMediaItemStmt();
		prepareGetMediaItemByPosStmt();

		prepareSelectPlaylistRevStmt();
		prepareUpdatePlaylistItemStmt();
		prepareSelectPlaylistItemStmt();

		prepareSelectNextlistRevStmt();
		prepareUpdateNextlistItemStmt();
		prepareSelectNextlistItemStmt();
	}
	return rc;
}

int CStateDB::close() {
	finalizeUpdateNextlistItemStmt();
	finalizeSelectNextlistItemStmt();
	finalizeSelectNextlistRevStmt();

	finalizeUpdatePlaylistItemStmt();
	finalizeSelectPlaylistItemStmt();
	finalizeSelectPlaylistRevStmt();

	finalizeSelectMediaItemStmt();
	finalizeGetMediaItemByPosStmt();
	finalizeUpdateMediaItemStmt();

	int rc = CStateDbBase::close();
	return rc;
}

void CStateDB::saveSession(CSession const * const session) {
	int colMinRevVal = session->getMinMediaColRev();
	int colMaxRevVal = session->getMaxMediaColRev();
	setValue("MinMediaColRev", colMinRevVal);
	setValue("MaxMediaColRev", colMaxRevVal);
	updateMediaColTable(session);

	int minPlRevVal = session->getMinPlaylistRev();
	int maxPlRevVal = session->getMaxPlaylistRev();
	setValue("MinPlaylistRev", minPlRevVal);
	setValue("MaxPlaylistRev", maxPlRevVal);
	updatePlaylistRevsTable(session);

	int minNlRevVal = session->getMinNextlistRev();
	int maxNlRevVal = session->getMaxNextlistRev();
	setValue("MinNextlistRev", minNlRevVal);
	setValue("MaxNextlistRev", maxNlRevVal);
    updateNextlistRevsTable(session);
}

void CStateDB::restoreSession(CSession * const session) {
	restoreMediaCols(session);
	restorePlaylists(session);
	//restoreNextlists(session);
	repairSession(session);
}

void CStateDB::updatePlaylistRevsTable(CSession const * const session, int minrev, int maxrev ) {
	if(minrev == -1) minrev = session->getMinPlaylistRev();
	if(maxrev == -1) maxrev = session->getMaxPlaylistRev();

	for(int rev = minrev; rev <= maxrev; rev++) {
		CRootItem* playlist = session->getPlaylist(rev);

		CRootItem::iterator it(playlist->begin());
		for(int i=0; it != playlist->end(); it++, i++ ) {
			CItemBase* item_b = *it;
			if(item_b->type() == CItemType::E_PLAYLISTITEM) {
                CPlaylistItem* item = reinterpret_cast<CPlaylistItem*>(item_b);
                updatePlaylistItem(i, item, rev, 0);
			}
		}
	}
}

void CStateDB::updateNextlistRevsTable(CSession const * const session, int minrev, int maxrev ) {

}



void CStateDB::restoreMediaCols(CSession * const session) {
	if(m_db == 0) throw(CApiMisuseException("Calling restoreCollection(), but DB not opened."));
	bool found;

	int minRev =  getIntValue("CollectionRevMin", found);
	assert(found);
	int maxRev = getIntValue("CollectionRevMax", found);
	assert(found);

	session->setMinMediaColRev( minRev );

	for(int rev = minRev; rev <= maxRev; rev++) {
		CRootItem* ri = getMediaColRev(rev);
		session->addMediaColRev(ri);
	}
}

CRootItem* CStateDB::getMediaColRev(int rev) {
	CRootItem* mediaCol = new CRootItem();

	CMediaItem* item = (CMediaItem*)0xffFFffFF;
	int pos = 0;
	do {
		item = getMediaItemByPos(pos, rev, mediaCol );
		pos++;
		if(item) {

			ostringstream oss;
			oss << "/" << item->getArtist() << "/" << item->getAlbum();

			cerr << oss.str();

			CCategoryItem* parent = mediaCol->getItemPtr(oss.str());
			if(parent == 0) {
				parent = mediaCol->mkPath(oss.str());
			}
			mediaCol->addContentItem(item, parent);
		}
	} while(item != 0);

	return mediaCol;
}


void CStateDB::updateMediaColTable( CSession const * const session, int minrev, int maxrev ) {

	beginTansaction();

	if(minrev == -1) minrev = session->getMinMediaColRev();
	if(maxrev == -1) maxrev = session->getMaxMediaColRev();

	for(int rev = minrev; rev <= maxrev; rev++) {
		CRootItem* mediaCol = session->getMediaCol(rev);

		CRootItem::iterator it(mediaCol->begin());

		for(; it != mediaCol->end(); it++) {
			CItemBase* item_b = *it;
			if(item_b->type() == CItemType::E_MEDIAITEM) {
                CMediaItem* item = reinterpret_cast<CMediaItem*>(item_b);
                updateMediaItem( item );
			}
		}
	}
	endTransaction();
}

void CStateDB::restorePlaylists(CSession * const session) {
	if(m_db == 0) throw(CApiMisuseException("Calling restorePlaylists(), but DB not opened."));
	bool found;

	int minRev = getIntValue("PlaylistRevMin", found);
	assert(found);
	int maxRev = getIntValue("PlaylistRevMax", found);
	assert(found);

	session->setMinPlaylistRev( minRev );

	for(int rev = minRev; rev <= maxRev; rev++) {
		CRootItem* playlist = new CRootItem();

		CPlaylistItem* item;
		int pos = 0;
		do {
			item = getPlaylistItemByPos(pos, rev, playlist);
			pos++;
			if(item) {
				CCategoryItem* parent = playlist->getItemPtr();
				playlist->addContentItem(item, parent);
			}
		} while(item != 0);
		session->addPlaylistRev(playlist);
	}
}

void CStateDB::restoreNextlists(CSession * const session) {
	if(m_db == 0) throw(CApiMisuseException("Calling restoreNextlists(), but DB not opened."));
	bool found;

	int minRev = getIntValue("NextlistRevMin", found);
	assert(found);
	int maxRev = getIntValue("NextlistRevMax", found);
	assert(found);

	session->setMinNextlistRev( minRev );

	for(int rev = minRev; rev < maxRev; rev++) {
		CRootItem* nextlist = new CRootItem();

		CNextlistItem* item;
		int pos = 0;
		do {
			item = getNextlistItemByPos(pos, rev, nextlist);
			pos++;
			if(item) {
				CCategoryItem* parent = nextlist->getItemPtr();
				nextlist->addContentItem(item, parent);
			}
		} while(item != 0);
		session->addNextlistRev(nextlist);
	}
}

/**
 *   check for playlist entries not in collection. If any, remove them and create new playlist revision.
 */
void CStateDB::repairSession(CSession* session) {
	CRootItem* mediaCol = session->getMediaCol();
	CRootItem* playlist = session->getPlaylist();
	CRootItem* nextlist = session->getNextlist();

	CRootItem* newPlaylist = new CRootItem();
	bool unknownItemInPlaylist = false;

	CRootItem::iterator it(playlist->begin());
	for(; it != playlist->end(); it++) {

		CCategoryItem* plci = (*it)->getParent();
		CCategoryItem* nci = newPlaylist->getItemPtr( plci->getPath() );

		CPlaylistItem* plItem = new CPlaylistItem(newPlaylist, nci );

		unsigned hash = plItem->getMediaItemHash();
		CItemBase* item_b = mediaCol->getContentPtr(CItemType::E_MEDIAITEM, hash);
		CMediaItem* item = reinterpret_cast<CMediaItem*>(item_b);
		if( item == 0 ) {
			unknownItemInPlaylist = true;
		}
		else {
			newPlaylist->addContentItem(plItem, nci);
		}
	}

	if( unknownItemInPlaylist ) {
		session->addPlaylistRev( newPlaylist );
		updatePlaylistRevsTable(session, session->getMaxPlaylistRev(), session->getMaxPlaylistRev());
	}
	else {
		delete newPlaylist;
	}

}

void CStateDB::updateMediaItem( CMediaItem* item ) {
	assert(m_updateMediaItemStmt != 0);

	int retval = sqlite3_bind_int(m_updateMediaItemStmt, 1, item->getHash());
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'hash' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_text(m_updateMediaItemStmt, 2, item->getFilename().c_str(), -1, SQLITE_TRANSIENT);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'filename' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_text(m_updateMediaItemStmt, 3, item->getArtist().c_str(), -1, SQLITE_TRANSIENT);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'Artist' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_text(m_updateMediaItemStmt, 4, item->getAlbum().c_str(), -1, SQLITE_TRANSIENT);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'album' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_text(m_updateMediaItemStmt, 5, item->getTitle().c_str(), -1, SQLITE_TRANSIENT);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'title' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_int(m_updateMediaItemStmt, 6, item->getYear());
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'year' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_int(m_updateMediaItemStmt, 7, item->getDuration());
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'duration' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_int(m_updateMediaItemStmt, 8, 0);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'num_played' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_int(m_updateMediaItemStmt, 9, 0);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'num_skipped' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_int(m_updateMediaItemStmt, 10, 0);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'num_repeated' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_int(m_updateMediaItemStmt, 11, 0);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'rating' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}

	retval = sqlite3_step( m_updateMediaItemStmt );

	if(retval != SQLITE_DONE) {
		cerr << "Error stepping m_updateColItemStmt: " << sqlite3_errmsg(m_db);
	}

	retval = sqlite3_reset(m_updateMediaItemStmt);
	if(retval != SQLITE_OK) {
		cerr << "Error resetting m_updateColItemStmt statement: " << sqlite3_errmsg(m_db);
	}
}


CMediaItem* CStateDB::getMediaItemByPos(int colPos, int colRev, CRootItem* ri) {
	CMediaItem* item = 0;

	assert(m_getMediaItemByPosStmt != 0);

	int retval = sqlite3_bind_int(m_getMediaItemByPosStmt, 1, colPos);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'colPos' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_int(m_getMediaItemByPosStmt, 2, colRev);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'colRev' to getColItemByPos statement: " << sqlite3_errmsg(m_db) << endl;
	}

	int rowid = 0;
	int num_found = 0;
	do {
		retval = sqlite3_step( m_getMediaItemByPosStmt );
		switch(retval) {
		case SQLITE_ROW:
			item = getMediaItemFromStmt(m_getMediaItemByPosStmt, ri);
			num_found++;
 			break;

		case SQLITE_DONE:
			// no more rows match search
			break;

		default:
			cerr << "Error during step command: " << sqlite3_errmsg(m_db) << endl;
			break;
		}
	}while (retval == SQLITE_ROW);

	if(retval != SQLITE_DONE) {
		cerr << "Error stepping getColItemByPos: " << sqlite3_errmsg(m_db);
	}
	retval = sqlite3_reset(m_getMediaItemByPosStmt);
	if(retval != SQLITE_OK) {
		cerr << "Error resetting getColItemByPos statement: " << sqlite3_errmsg(m_db);
	}
	return item;
}

CMediaItem* CStateDB::getMediaItemByHash(unsigned hash, CRootItem* ri) {
	CMediaItem* item;

	int retval = sqlite3_bind_int(m_selectMediaItemStmt, 1, hash);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'hash' to selectColItemStmt statement: " << sqlite3_errmsg(m_db) << endl;
	}

	do {
		retval = sqlite3_step( m_selectMediaItemStmt );
		switch(retval) {
		case SQLITE_ROW:
			item = getMediaItemFromStmt(m_selectMediaItemStmt, ri);
			break;

		case SQLITE_DONE:
			// no more rows match search
			break;

		default:
			cerr << "Error during step command: " << sqlite3_errmsg(m_db);
			break;
		}

	}while (retval == SQLITE_ROW);
	if(retval != SQLITE_DONE) {
		cerr << "Error stepping selectColItemStmt: " << sqlite3_errmsg(m_db);
	}

	retval = sqlite3_reset(m_selectMediaItemStmt);
	if(retval != SQLITE_OK) {
		cerr << "Error resetting selectColItemStmt: " << sqlite3_errmsg(m_db);
	}
	return item;
}

CMediaItem* CStateDB::getMediaItemFromStmt(sqlite3_stmt *pStmt, CRootItem* ri) {
	CMediaItem* item = new CMediaItem(ri);
	// (hash INTEGER, file TEXT, artist TEXT, album TEXT, title TEXT, duration INTEGER, num_played INTEGER, num_skipped INTEGER, num_repeated INTEGER, rating INTEGER)";
	int numCol = sqlite3_column_count(pStmt);
	// Scerr << "result has " << numCol << " columns" << endl;

	int hash = sqlite3_column_int(pStmt, 0);

	const unsigned char *filename = sqlite3_column_text(pStmt, 1);
	int filenameSize = sqlite3_column_bytes(pStmt, 1);
	item->setFilename( (const char*)filename );

	const unsigned char *artist = sqlite3_column_text(pStmt, 2);
	int artistSize = sqlite3_column_bytes(pStmt, 2);
	item->setArtist( (const char*)artist );

	const unsigned char *album = sqlite3_column_text(pStmt, 3);
	int albumSize = sqlite3_column_bytes(pStmt, 3);
	item->setAlbum( (const char*)album );

	const unsigned char *title = sqlite3_column_text(pStmt, 4);
	int titleSize = sqlite3_column_bytes(pStmt, 4);
	item->setTitle( (const char*)title );

	int year = sqlite3_column_int(pStmt, 5);
	item->setYear( year );

	int duration = sqlite3_column_int(pStmt, 6);
	item->setDuration( duration );

//	cerr << "SELECT: " << filename << " " << artist << " " << album << " " << title << endl;
	return item;
}


void CStateDB::updatePlaylistItem( int plPos, CPlaylistItem* item, int plRev, int colRev ) {
	int rowid = rowIDofPlRevEntry(plPos, item->getMediaItemHash(), plRev, colRev);
	if(rowid == 0) {  // this entry does not exist yet
		int retval = sqlite3_bind_int(m_updatePlaylistItemStmt, 1, plPos);
		if(retval != SQLITE_OK) {
			cerr << "Error binding value 'plPos' to updatePlaylistItem statement: " << sqlite3_errmsg(m_db) << endl;
		}

		retval = sqlite3_bind_int(m_updatePlaylistItemStmt, 2, item->getMediaItemHash());
		if(retval != SQLITE_OK) {
			cerr << "Error binding value 'hash' to updatePlaylistItem statement: " << sqlite3_errmsg(m_db) << endl;
		}
		retval = sqlite3_bind_int(m_updatePlaylistItemStmt, 3, plRev);
		if(retval != SQLITE_OK) {
			cerr << "Error binding value 'plRev' to updatePlaylistItem statement: " << sqlite3_errmsg(m_db) << endl;
		}
		retval = sqlite3_bind_int(m_updatePlaylistItemStmt, 4, colRev);
		if(retval != SQLITE_OK) {
			cerr << "Error binding value 'colRev' to updatePlaylistItem statement: " << sqlite3_errmsg(m_db) << endl;
		}

		do {
			retval = sqlite3_step( m_updatePlaylistItemStmt );
		}while (retval == SQLITE_ROW);

		if(retval != SQLITE_DONE) {
			cerr << "Error stepping updatePlaylistItemStmt: " << sqlite3_errmsg(m_db);
		}
		retval = sqlite3_reset( m_updatePlaylistItemStmt );
		if(retval != SQLITE_OK) {
			cerr << "Error resetting m_updatePlaylistItemStmt statement: " << sqlite3_errmsg(m_db);
		}
	}
}

void CStateDB::updateNextlistItem( int nlPos, CPlaylistItem* item, int nlRev, int plRev ) {

}

int CStateDB::rowIDofPlRevEntry(int plPos, int colHash, int plRev, int colRev) {
	int retval = sqlite3_bind_int(m_selectPlaylistRevStmt, 1, plPos);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'plPos' to selectPlRev statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_int(m_selectPlaylistRevStmt, 2, colHash);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'colHash' to selectPlRev statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_int(m_selectPlaylistRevStmt, 3, plRev);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'plRev' to selectPlRev statement: " << sqlite3_errmsg(m_db) << endl;
	}
	retval = sqlite3_bind_int(m_selectPlaylistRevStmt, 4, colRev);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'colRev' to selectPlRev statement: " << sqlite3_errmsg(m_db) << endl;
	}

	int rowid = 0;
	int num_found = 0;
	do {
		retval = sqlite3_step( m_selectPlaylistRevStmt );
		switch(retval) {
		case SQLITE_ROW:
			rowid =	sqlite3_column_int(m_selectPlaylistRevStmt, 0);
			num_found++;
 			break;

		case SQLITE_DONE:
			// no more rows match search
			break;

		default:
			cerr << "Error during step command: " << sqlite3_errmsg(m_db) << endl;
			break;
		}

	}while (retval == SQLITE_ROW);

	if(retval != SQLITE_DONE) {
		cerr << "Error stepping table playlistRevs: " << sqlite3_errmsg(m_db);
	}
	retval = sqlite3_reset(m_selectPlaylistRevStmt);
	if(retval != SQLITE_OK) {
		cerr << "Error resetting statement: " << sqlite3_errmsg(m_db);
	}
	return rowid;
}


CPlaylistItem* CStateDB::getPlaylistItemFromStmt(sqlite3_stmt *pStmt, CRootItem* ri) {
	unsigned hash = sqlite3_column_int(pStmt, 0);
	CPlaylistItem* item = new CPlaylistItem( hash );
	return item;
}

CPlaylistItem* CStateDB::getPlaylistItemByPos(int pos, int rev, CRootItem* ri) {
	CPlaylistItem* item = 0;

	int retval = sqlite3_bind_int(m_selectPlaylistItemStmt, 1, pos);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'pos' to selectPlaylistItemStmt statement: " << sqlite3_errmsg(m_db) << endl;
	}

	retval = sqlite3_bind_int(m_selectPlaylistItemStmt, 2, rev);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'rev' to selectPlaylistItemStmt statement: " << sqlite3_errmsg(m_db) << endl;
	}

	retval = sqlite3_bind_int(m_selectPlaylistItemStmt, 3, 0);
	if(retval != SQLITE_OK) {
		cerr << "Error binding value 'colRev' to selectPlaylistItemStmt statement: " << sqlite3_errmsg(m_db) << endl;
	}

	do {
		retval = sqlite3_step( m_selectPlaylistItemStmt );
		switch(retval) {
		case SQLITE_ROW:
			item = getPlaylistItemFromStmt(m_selectPlaylistItemStmt, ri);
			break;

		case SQLITE_DONE:
			// no more rows match search
			break;

		default:
			cerr << "Error during step command: " << sqlite3_errmsg(m_db);
			break;
		}

	}while (retval == SQLITE_ROW);
	if(retval != SQLITE_DONE) {
		cerr << "Error stepping selectPlaylistItemStmt: " << sqlite3_errmsg(m_db);
	}

	retval = sqlite3_reset(m_selectPlaylistItemStmt);
	if(retval != SQLITE_OK) {
		cerr << "Error resetting selectPlaylistItemStmt: " << sqlite3_errmsg(m_db);
	}
	return item;
}

CNextlistItem* CStateDB::getNextlistItemByPos(int pos, int rev, CRootItem* ri) {

}


void CStateDB::createPlaylistRevisionsTable() {
	createTable("playlistRevs","(entry_id INTEGER PRIMARY KEY AUTOINCREMENT, plPos INTEGER, colHash INTEGER, plRev INTEGER, ColRev INTEGER, FOREIGN KEY(colHash) REFERENCES collection(hash))");
}

void CStateDB::createNextlistRevisionsTable() {
	createTable("nextlistRevs","(entry_id INTEGER PRIMARY KEY AUTOINCREMENT, nlPos INTEGER, plPos INTEGER, nlRev INTEGER, plRev INTEGER)");
}



void CStateDB::prepareUpdatePlaylistItemStmt() {
	prepareStmt(&m_updatePlaylistItemStmt, "INSERT OR REPLACE INTO playlistRevs " \
			"( plPos, colHash, plRev, colRev) " \
			" VALUES " \
		    "(?,?,?,?)");
}

void CStateDB::prepareUpdateMediaItemStmt() {
	prepareStmt(&m_updateMediaItemStmt, "INSERT OR REPLACE INTO collection " \
			"( hash, file, artist, album, title, year, duration, num_played, num_skipped, num_repeated, rating) " \
			" VALUES " \
		    "(?,?,?,?,?,?,?,?,?,?,?)");
}

void CStateDB::finalizeUpdateMediaItemStmt() {
	finalizeStmt( &m_updateMediaItemStmt );
}


void CStateDB::prepareSelectMediaItemStmt() {
	prepareStmt(&m_selectMediaItemStmt, "SELECT * FROM collection WHERE hash=?");
}

void CStateDB::finalizeSelectMediaItemStmt() {
	finalizeStmt(&m_selectMediaItemStmt);
}

void CStateDB::prepareGetMediaItemByPosStmt() {
	prepareStmt(&m_getMediaItemByPosStmt, "SELECT * FROM collection INNER JOIN collectionRevs ON collection.hash=collectionRevs.colHash WHERE collectionRevs.colPos=? AND collectionRevs.colRev=?");
}

void CStateDB::finalizeGetMediaItemByPosStmt() {
	finalizeStmt( &m_getMediaItemByPosStmt );
}

void CStateDB::finalizeUpdatePlaylistItemStmt() {
	finalizeStmt( &m_updatePlaylistItemStmt );
}

void CStateDB::prepareSelectPlaylistRevStmt() {
	prepareStmt(&m_selectPlaylistRevStmt, "SELECT rowid FROM playlistRevs WHERE plPos=? AND colHash=? AND plRev=? AND colRev=?");
}

void CStateDB::finalizeSelectPlaylistRevStmt() {
	finalizeStmt( &m_selectPlaylistRevStmt );
}

void CStateDB::prepareSelectPlaylistItemStmt() {
	prepareStmt(&m_selectPlaylistItemStmt, "SELECT colHash FROM playlistRevs WHERE plPos=? AND plRev=? AND colRev=?");
}

void CStateDB::finalizeSelectPlaylistItemStmt() {
	finalizeStmt( &m_selectPlaylistItemStmt );
}

void CStateDB::prepareUpdateNextlistItemStmt() {

}

void CStateDB::finalizeUpdateNextlistItemStmt() {
	finalizeStmt( &m_updateNextlistItemStmt );
}

void CStateDB::prepareSelectNextlistItemStmt() {

}

void CStateDB::finalizeSelectNextlistItemStmt() {
	finalizeStmt( &m_selectNextlistItemStmt );
}

void CStateDB::prepareSelectNextlistRevStmt() {

}

void CStateDB::finalizeSelectNextlistRevStmt() {
	finalizeStmt( &m_selectNextlistRevStmt );
}

}
