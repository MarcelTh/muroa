/*
 * CSessionCommand.cpp
 *
 *  Created on: 12 Jun 2010
 *      Author: martin
 */

#include "CSessionCommand.h"

CSessionCommand::CSessionCommand( int knownRev, QString name , QObject * parent ) : CCommandBase(knownRev) {
	m_cmdName = name;
	m_cmdData = QString();
}

CSessionCommand::~CSessionCommand() {
}


void CSessionCommand::timeout() {
	CCommandBase::timeout();
}
