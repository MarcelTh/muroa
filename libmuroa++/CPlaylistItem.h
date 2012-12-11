/***************************************************************************
 *
 *   CPlaylistItem.h
 *
 *   This file is part of libmuroa++                                  *
 *   Copyright (C) 2011 by Martin Runge <martin.runge@web.de>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef CPLAYLISTITEM_H_
#define CPLAYLISTITEM_H_

#include "IContentItem.h"
#include "IIdProvider.h"
#include "MuroaExceptions.h"

namespace muroa {

class CMediaItem;

class CPlaylistItem: public IContentItem {
public:
    CPlaylistItem(const CPlaylistItem& other, CRootItem* root_item, CCategoryItem* parent);
	CPlaylistItem(uint32_t mediaItemHash, uint32_t plID = 0);
	CPlaylistItem(CRootItem *root_item, CCategoryItem*  parent, int posInParent = -1);
	CPlaylistItem(CRootItem *root_item, std::string text, CCategoryItem*  parent, int posInParent = -1) throw(ExMalformedPatch);
	virtual ~CPlaylistItem();

	// CPlaylistItem* clone(const IContentItem& other, CRootItem *root_item, CCategoryItem*  parent);

	void setParent(CRootItem *root_item, CCategoryItem*  parent, int posInParent = -1);

	bool operator==(const IContentItem& other);
	inline bool operator!=(const IContentItem& other){ return !operator==(other); };

	std::string serialize(bool asDiff = false);

	inline uint32_t getMediaItemHash() { return m_mediaitem_hash; };
	void setMediaItemHash(uint32_t hash);
	void setMediaItem(CMediaItem* mediaitem);

	static void setIDProvider(muroa::IIdProvider* idProvider);
	static muroa::IIdProvider* getIDProvider();

private:
	uint32_t m_mediaitem_hash;

	static uint32_t getNextFreeID();
	static void setNextFreeID(uint32_t next_free_id);
	static muroa::IIdProvider* m_id_provider;
	static uint32_t m_next_free_id;

	void assembleText();
};
} // namespace muroa
#endif /* CPLAYLISTITEM_H_ */
