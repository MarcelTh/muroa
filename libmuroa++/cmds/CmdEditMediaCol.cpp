/*
 * CmdColChanged.cpp
 *
 *  Created on: 28 Dec 2011
 *      Author: martin
 */

#include "CmdEditMediaCol.h"

using namespace std;

namespace muroa {

CmdEditMediaCol::CmdEditMediaCol() : Cmd(Cmd::EDIT_MEDIACOL) {
}

CmdEditMediaCol::CmdEditMediaCol(unsigned  fromRev, unsigned toRev, std::string diff)
                              : Cmd(Cmd::EDIT_MEDIACOL),
                                m_fromRev(fromRev),
                                m_toRev(toRev),
                                m_diff(diff)
{

}

CmdEditMediaCol::~CmdEditMediaCol() {
}

    string CmdEditMediaCol::getDiff() const
    {
        return m_diff;
    }

    void CmdEditMediaCol::setDiff(string diff)
    {
        m_diff = diff;
    }

    unsigned CmdEditMediaCol::getFromRev() const
    {
        return m_fromRev;
    }

    void CmdEditMediaCol::setFromRev(unsigned  fromRev)
    {
        m_fromRev = fromRev;
    }

    unsigned CmdEditMediaCol::getToRev() const
    {
        return m_toRev;
    }

    void CmdEditMediaCol::setToRev(unsigned  toRev)
    {
        m_toRev = toRev;
    }
} /* namespace muroa */
