/*
 * IContentItem.cpp
 *
 *  Created on: 29 Jun 2011
 *      Author: martin
 */

#include "IContentItem.h"

#include "CMediaItem.h"
#include "CPlaylistItem.h"
#include "CNextlistItem.h"


IContentItem::IContentItem(CRootItem *root_item, CCategoryItem*  parent, const item_type_t type) :
   CItemBase(root_item, parent, type)
{

}

IContentItem::~IContentItem() {

}

IContentItem* IContentItem::itemFactory(const CItemType itemType,
		                                CRootItem *root_item,
		                                CCategoryItem *parent,
		                                const unsigned posInParent)
{
	const CItemType::item_type_t type_enum = itemType.getType();
	switch(type_enum) {
	case CItemType::E_ROOT:
		return 0;
		break;
	case CItemType::E_CAT:
		return 0;
		break;
	case CItemType::E_MEDIAITEM:
		return new CMediaItem(root_item, parent, posInParent);
		break;
	case CItemType::E_PLAYLISTITEM:
		return new CPlaylistItem(root_item, parent, posInParent);
		break;
	case CItemType::E_NEXTLISTITEM:
		return new CNextlistItem(root_item, parent, posInParent);
		break;
	default:
		return 0;
		break;
	}
}

IContentItem* IContentItem::itemFactory(const CItemType itemType,
		                                CRootItem *root_item,
		                                std::string text,
		                                CCategoryItem *parent,
		                                const unsigned posInParent) throw(ExMalformedPatch)
{
	const CItemType::item_type_t type_enum = itemType.getType();
	switch(type_enum) {
	case CItemType::E_ROOT:
		return 0;
		break;
	case CItemType::E_CAT:
		return 0;
		break;
	case CItemType::E_MEDIAITEM:
		return new CMediaItem(root_item, text, parent, posInParent);
		break;
	case CItemType::E_PLAYLISTITEM:
		return new CPlaylistItem(root_item, text, parent, posInParent);
		break;
	case CItemType::E_NEXTLISTITEM:
		return new CNextlistItem(root_item, text, parent, posInParent);
		break;
	default:
		return 0;
		break;
	}
}

IContentItem* IContentItem::itemFactory(CMediaItem* item,
		                                CRootItem *root_item,
		                                CCategoryItem *parent,
		                                const unsigned posInParent)
{
		 CMediaItem* newItem = new CMediaItem(root_item, parent, posInParent);
         newItem->setArtist(item->getArtist());
		 newItem->setAlbum(item->getAlbum());
		 newItem->setTitle(item->getTitle());
		 newItem->setDuration(item->getDuration());
		 newItem->setYear(item->getYear());
		 newItem->setFilename(item->getFilename());
		 return newItem;
}


bool IContentItem::operator==(const IContentItem& other) {
	return true;
}

uint32_t IContentItem::hash( std::string stdstr ) {
	uint32_t hash = 0;
	int c;
	const char* str = stdstr.c_str();

	while (c = *str++)
		hash = c + (hash << 6) + (hash << 16) - hash;

	return hash;
}

