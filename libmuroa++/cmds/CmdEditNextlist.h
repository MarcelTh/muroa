/*
 * CmdEditNextlist.h
 *
 *  Created on: 28 Dec 2011
 *      Author: martin
 */

#ifndef CMD_EDIT_NEXTLIST_H_
#define CMD_EDIT_NEXTLIST_H_

#include "Cmd.h"
#include <string>

namespace muroa {

class CmdEditNextlist: public muroa::Cmd {
public:
	CmdEditNextlist();
	CmdEditNextlist(unsigned  fromRev, unsigned toRev, std::string diff);
    virtual ~CmdEditNextlist();

    std::string getDiff() const;
    void setDiff(std::string diff);

    unsigned getFromRev() const;
    void setFromRev(unsigned  fromRev);

    unsigned getToRev() const;
    void setToRev(unsigned  fromRev);
private:
    unsigned m_fromRev;
    unsigned m_toRev;
    std::string m_diff;

};

} /* namespace muroa */
#endif /* CMD_EDIT_NEXTLIST_H_ */
