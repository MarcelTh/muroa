/*
 * CMediaItem.h
 *
 *  Created on: 16 Oct 2009
 *      Author: martin
 */

#ifndef CMEDIAITEM_H_
#define CMEDIAITEM_H_

// #include "CItemBase.h"
#include "IContentItem.h"

#include <string>

namespace muroa {

class CMediaItem : public IContentItem {
public:
	CMediaItem(CRootItem *root_item = 0);
	CMediaItem(CRootItem *root_item, CCategoryItem*  parent, int posInParent = -1);
	CMediaItem(CRootItem *root_item, std::string text, CCategoryItem*  parent, int posInParent = -1) throw(ExMalformedPatch);
	virtual ~CMediaItem();

	void setParent(CRootItem *root_item, CCategoryItem*  parent, int posInParent = -1);

    inline std::string getAlbum() const { return m_album; }
    inline std::string getArtist() const { return m_artist; }
    inline std::string getFilename() const { return m_filename; }
    inline int getDuration() const { return m_duration_in_s; }
    inline std::string getTitle() const { return m_title; }
    inline int getYear() const  { return m_year; }

    void setAlbum(std::string album);

    void setArtist(std::string artist);
    void setFilename(std::string filename);
    void setDuration(int duration);
    void setTitle(std::string title);
    void setYear(int year);


	bool operator==(const CMediaItem& other);
	inline bool operator!=(const CMediaItem& other){ return !operator==(other); };

// 	void addChild(IContentItem* newMediaItem, int pos = -1);

	std::string serialize(bool asDiff = false);

private:

    std::string m_filename;
    std::string m_artist;
    std::string m_album;
    std::string m_title;
	int m_year;
	int m_duration_in_s;

	void assembleText();
	void rehash();
};
} // namespace muroa
#endif /* CCOLLECTIONITEM_H_ */
