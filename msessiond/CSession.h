/*
 * CSession.h
 *
 *  Created on: 31 Oct 2011
 *      Author: martin
 */

#ifndef CSESSION_H_
#define CSESSION_H_

#include <string>
#include <vector>
#include <set>
#include <map>
#include <CConnection.h>
#include <CRootItem.h>

#include "sessionEx.h"

namespace muroa {

class CCmdBase;
class CTcpServer;

class CSession {
public:
	CSession(std::string name);
	virtual ~CSession();

	std::string getName() { return m_name; };

	void addConnection(CConnection* ptr);
	void removeConnection(CConnection* ptr);

	CRootItem*  getMediaCol(int revision = -1) const;
	CRootItem*  getPlaylist(int revision = -1) const;
	CRootItem*  getNextlist(int revision = -1) const;

	const std::string getMediaColDiff(unsigned fromRevision, int toRevision = -1) const;
	const std::string getPlaylistDiff(unsigned fromRevision, int toRevision = -1) const;
	const std::string getNextlistDiff(unsigned fromRevision, int toRevision = -1) const;

	inline int getCollectionRevision() const { return m_maxMediaColRev; };
	inline int getPlaylistRevision() const { return m_maxPlaylistRev; };
	inline int getNextlistRevision() const { return m_maxNextlistRev; };

	inline int getMinMediaColRev() const { return m_minMediaColRev; };
	inline int getMinPlaylistRev() const { return m_minPlaylistRev; };
	inline int getMinNextlistRev() const { return m_minNextlistRev; };


	void addMediaColRev(CRootItem* ri);
	void addMediaColRev(const std::string& mediaCol );
	void addPlaylistRev(CRootItem* ri);
	void addPlaylistRev(const std::string& playlist);
	void addNextlistRev(CRootItem* ri);
	void addNextlistRev(const std::string& nextlist);

	int addMediaColRevFromDiff(const std::string& mediaColDiff, unsigned diffFromRev) throw(InvalidMsgException);
	int addPlaylistRevFromDiff(const std::string& playlistDiff, unsigned diffFromRev) throw(InvalidMsgException);
	int addNextlistRevFromDiff(const std::string& nextlistDiff, unsigned diffFromRev) throw(InvalidMsgException);

	int addNextlistRevFromNextCmd();
	int addNextlistRevFromPrevCmd();

	void setMinMediaColRevision(int rev) throw();
	void setMinPlaylistRevision(int rev) throw();
	void setMinNextlistRevision(int rev) throw();




	void toAll( CCmdBase* cmd );

private:
	bool hasConnection(CConnection* conn);

	CRootItem* getRev(const std::map<unsigned, CRootItem*>& map,
			          const unsigned rev,
			          const std::string& message) const throw(InvalidMsgException);

	// CTcpServer* m_tcp_server;
	std::string m_name;
	std::set<CConnection*> m_connections;

	std::map<unsigned, CRootItem*> m_mediaColRevs;
	std::map<unsigned, CRootItem*> m_playlistRevs;
	std::map<unsigned, CRootItem*> m_nextlistRevs;

    unsigned m_maxMediaColRev;
    unsigned m_maxPlaylistRev;
    unsigned m_maxNextlistRev;

    unsigned m_minMediaColRev;
    unsigned m_minPlaylistRev;
    unsigned m_minNextlistRev;

    unsigned m_playlistPos;

    std::string m_stateDB;

};

} /* namespace muroa */
#endif /* CSESSION_H_ */