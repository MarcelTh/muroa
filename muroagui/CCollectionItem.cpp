/*
 * CCollectionItem.cpp
 *
 *  Created on: 17 Oct 2009
 *      Author: martin
 */

#include "CCollectionItem.h"
#include <QString>

CCollectionItem::CCollectionItem(QString itemStr) : CItemBase(itemStr)
{
	// TODO parse string here
	bool ok;
	setFilename( itemStr.section(',', 0, 0) );
	setArtist( itemStr.section(',', 1, 1) );
	setAlbum( itemStr.section(',', 2, 2) );
	setTitle( itemStr.section(',', 3, 3) );
	setYear( itemStr.section(',', 4, 4).toInt(&ok) );
	setLengthInSec( itemStr.section(',', 5, 5).toInt(&ok) );

	m_hash = itemStr.section(',', 6, 6).toUInt(&ok);

}

CCollectionItem::~CCollectionItem() {

}

QVariant CCollectionItem::data(int column) const
{
	QVariant data;
	switch(column)
	{
		case 0:
			data = m_artist;
			break;
		case 1:
			data = m_album;
			break;

		case 2:
			data = m_title;
			break;

		case 3:
			data = m_year;
			break;

		case 4:
		{
			data = QString("%1:%2").arg(m_length_in_s / 60).arg(m_length_in_s % 60);
			break;
		}
		case 5:
			data = m_filename;
			break;

		default:
			return QString("wrong column index!");
	}
	return data;

}

QString CCollectionItem::getTitle(int col)
{
	QString title;
	switch(col)
	{
		case 0:
			title = "Artist";
			break;
		case 1:
			title = "Album";
			break;

		case 2:
			title = "Title";
			break;

		case 3:
			title = "Year";
			break;

		case 4:
			title = "Length";
			break;

		default:
			return QString("Hä?");

	}
	return title;
}
