/*
 * CmdEditNextlist.cpp
 *
 *  Created on: 28 Dec 2011
 *      Author: martin
 */

#include "CmdEditNextlist.h"

using namespace std;

namespace muroa {

CmdEditNextlist::CmdEditNextlist() : Cmd(Cmd::EDIT_NEXTLIST) {
	// TODO Auto-generated constructor stub

}

CmdEditNextlist::CmdEditNextlist(unsigned  fromRev, std::string diff)
                              : Cmd(Cmd::EDIT_NEXTLIST),
                                m_fromRev(fromRev),
                                m_diff(diff)
{

}

CmdEditNextlist::~CmdEditNextlist() {
	// TODO Auto-generated destructor stub
}

    string CmdEditNextlist::getDiff() const
    {
        return m_diff;
    }

    void CmdEditNextlist::setDiff(string diff)
    {
        m_diff = diff;
    }

    unsigned CmdEditNextlist::getFromRev() const
    {
        return m_fromRev;
    }

    void CmdEditNextlist::setFromRev(unsigned  fromRev)
    {
        m_fromRev = fromRev;
    }

} /* namespace muroa */