//
//
// C++ Implementation: $MODULE$
//
// Description:
//
//
// Author: Martin Runge <martin.runge@web.de>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "xmlCommands.h"


using namespace std;


xmlCommands::xmlCommands()
{
}


xmlCommands::~xmlCommands()
{
}

const std::string xmlCommands::open = string("<muroa>\n");
const std::string xmlCommands::close = string("</muroa>\n");

const std::string xmlCommands::sessionList = string("sessionList");

const std::string xmlCommands::sessionDesc = string("sessionDesc");

const std::string xmlCommands::joinSession = string("session");
const std::string xmlCommands::leaveSession = string("</session>\n");
const std::string xmlCommands::leave = string("leave");

const std::string xmlCommands::play = string("play");
const std::string xmlCommands::pause = string("pause");
const std::string xmlCommands::stop = string("stop");
const std::string xmlCommands::next = string("next");
const std::string xmlCommands::prev = string("prev");
const std::string xmlCommands::progress = string("progress");
const std::string xmlCommands::scanCollection = string("scanCollection");
const std::string xmlCommands::stateChanged = string("stateChanged");
const std::string xmlCommands::finished = string("finished");
const std::string xmlCommands::error = string("error");
const std::string xmlCommands::XML_ver = string("<?xml version=\"1.0\"?>\n");
const std::string xmlCommands::getCollection = string("getCollection");
const std::string xmlCommands::getPlaylist = string("getPlaylist");
const std::string xmlCommands::getNextlist = string("getNextlist");
const std::string xmlCommands::getSessionState = string("getSessionState");
const std::string xmlCommands::editCollection = string("editCollection");
const std::string xmlCommands::editPlaylist = string("editPlaylist");
const std::string xmlCommands::editNextlist = string("editNextlist");
const std::string xmlCommands::editSessionState = string("editSessionState");
