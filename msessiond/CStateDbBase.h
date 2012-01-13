/*
 * CStateDbBase.h
 *
 *  Created on: 31 Jul 2010
 *      Author: martin
 */

#ifndef CSTATEDBBASE_H_
#define CSTATEDBBASE_H_

#include <string>
#include <sqlite3.h>

#include "sessionEx.h"

class CMediaItem;
class CPlaylistItem;

namespace muroa {
	class CSession;
}

class CStateDbBase {
public:
	CStateDbBase(std::string dbFileName);
	virtual ~CStateDbBase();

	virtual int open();
	virtual int close();

	std::string getValue(std::string key, bool &found);
	int getIntValue(std::string key, bool &found);
	void setValue(std::string key, std::string value);
	void setValue(std::string key, int value);

    void updateCollectionRevItem( int pos, int hash, int rev );

    int getNumMediaItemsByHash(unsigned hash);
    CMediaItem* getMediaItemByHash(unsigned hash);
    unsigned getHashByFilename(const std::string& filename, bool& found) const;
    bool hashPresent();
    CMediaItem* getMediaItemByPos(int colPos, int colRev);

    int rowIDofColRevEntry(int colPos, int colHash, int colRev);

protected:
    sqlite3 *m_db;

    int updateMediaItem( CMediaItem* item );
    CMediaItem* getMediaItemFromStmt(sqlite3_stmt *pStmt);

    void beginTansaction() throw(CApiMisuseException);
    void endTransaction() throw(CApiMisuseException);

	void prepareStmt(sqlite3_stmt** stmt, std::string sqlStmt);
	void finalizeStmt(sqlite3_stmt** stmt);

    void createTable(std::string name, std::string schema);

private:
	std::string m_dbFileName;

    /** Revision table is very simple: rev_id , rev_nr
     *  There is a revision table for the collection, playlist and nextlist.
     */
    void createGeneralTable();
    void createCollectionTable();
    void createCollectionRevisionsTable();


	sqlite3_stmt *m_beginTransactionStmt;
	sqlite3_stmt *m_endTransactionStmt;

	sqlite3_stmt *m_updateMediaItemStmt;
	void prepareUpdateMediaItemStmt();
	void finalizeUpdateMediaItemStmt();

	sqlite3_stmt *m_updateColRevStmt;
	void prepareUpdateColRevStmt();
	void finalizeUpdateColRevStmt();

	sqlite3_stmt *m_hashByFilenameStmt;
	void prepareHashByFilenameStmt();
	void finalizeHashByFilenameStmt();

	sqlite3_stmt *m_getMediaItemByPosStmt;
	void prepareGetMediaItemByPosStmt();
	void finalizeGetMediaItemByPosStmt();

	sqlite3_stmt *m_selectMediaItemStmt;
	void prepareSelectMediaItemStmt();
	void finalizeSelectMediaItemStmt();

	sqlite3_stmt *m_selectColRevStmt;
	void prepareSelectColRevStmt();
	void finalizeSelectColRevStmt();

};

#endif /* CSTATEDBBASE_H_ */
