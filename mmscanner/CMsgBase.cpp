/*
 * CMsgBase.cpp
 *
 *  Created on: 31 Oct 2010
 *      Author: martin
 */

#include "CMsgBase.h"
#include <sessionEx.h>
#include <sstream>

#include "CMsgError.h"
#include "CMsgResponse.h"
#include "CMsgQuit.h"
#include "CMsgOpenDb.h"
#include "CMsgScanDir.h"
#include "CMsgFinished.h"
#include "CMsgProgress.h"
#include "CMsgCollectionChanged.h"

#include <assert.h>

using namespace std;

int CMsgBase::m_last_id = 1;

CMsgBase::CMsgBase(): m_serialisationBuffer(0), m_serialisationBufferSize(0) {
}

CMsgBase::~CMsgBase() {
	if(m_serialisationBuffer != 0) {
		delete [] m_serialisationBuffer;
		m_serialisationBuffer = 0;
		m_serialisationBufferSize = 0;
	}
}


bool CMsgBase::equalTo(const CMsgBase &other) {
	return ( m_msgType == other.m_msgType && m_msgID == other.m_msgID && m_payloadSize == other.m_payloadSize );
}


CMsgBase* CMsgBase::msgFactory(char*& buffer, int& size) {
	CMsgBase* basePtr = 0;
	int msgSize = 0;

	const int headerSize = 3 * sizeof(uint32_t);

	if( size < headerSize) {
		stringstream ss;
		ss << "Need more data: message too small to decode header.";
		throw(NeedMoreDataException(ss.str(), 1));
	}

	uint32_t* u32Ptr = reinterpret_cast<uint32_t*>(buffer);
	uint32_t type        = u32Ptr[0];
	uint32_t payloadSize = u32Ptr[2];


	if( size < headerSize + payloadSize ) {
		stringstream ss;
		ss << "Need more data: message payload indicated by header is larger than buffer.";
		throw(NeedMoreDataException(ss.str(), 2));
	}


	switch( type ) {
	    case E_MSG_ERR:
	    	basePtr = new CMsgError( buffer, size );
	    	break;

		case E_MSG_RESP:
			basePtr = new CMsgResponse( buffer, size );
			break;

		case E_MSG_QUIT:
			basePtr = new CMsgQuit( buffer, size );
			break;

		case E_MSG_OPEN_DB:
			basePtr = new CMsgOpenDb( buffer, size );
			break;

		case E_MSG_SCAN_DIR:
			basePtr = new CMsgScanDir( buffer, size );
			break;

		case E_MSG_FINISHED:
			basePtr = new CMsgFinished( buffer, size );
			break;

		case E_MSG_PROGRESS:
			basePtr = new CMsgProgress( buffer, size );
			break;

		case E_MSG_COLLECTION_CHANGED:
			basePtr = new CMsgCollectionChanged( buffer, size );
			break;

		default:
		{
			stringstream ss;
			ss << "unknown message type: " << type;
			throw(InvalidMsgException(ss.str()));
		}
	}
	msgSize = basePtr->getSize();
	size -= msgSize;
	buffer += msgSize;

	return basePtr;
}



uint32_t CMsgBase::checkHeader(char* buffer, int size, enum msg_types type) {
	uint32_t* u32Ptr = reinterpret_cast<uint32_t*>(buffer);
	uint32_t typeInHeader = u32Ptr[0];
	uint32_t msgID = u32Ptr[1];
	m_payloadSize = u32Ptr[2];

	if(type != typeInHeader) {
		stringstream ss;
		ss << "CMsgBase::checkHeader ( id=" << msgID << ")type is " << typeInHeader << " but expected " << type;
		throw InvalidMsgException(ss.str());
	}
	// reallocSerialisationBuffer(m_payloadSize);
	return msgID;
}

void CMsgBase::serializeHeader() {
	assert( m_serialisationBufferSize >= getHeaderSize() );
	uint32_t* u32Ptr = reinterpret_cast<uint32_t*>(m_serialisationBuffer);
	u32Ptr[0] = m_msgType;
	u32Ptr[1] = m_msgID;
	u32Ptr[2] = m_payloadSize;

}


int CMsgBase::reallocSerialisationBuffer( int payloadSize ) {
	if(m_serialisationBuffer != 0) {
		delete [] m_serialisationBuffer;
		m_serialisationBuffer = 0;
		m_serialisationBufferSize = 0;
	}
	int size = payloadSize + getHeaderSize();
	m_serialisationBuffer = new char[size];
	m_serialisationBufferSize = size;

	return m_serialisationBufferSize;
}
