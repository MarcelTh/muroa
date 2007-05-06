/***************************************************************************
 *   Copyright (C) 2005 by Martin Runge   *
 *   martin.runge@web.de   *
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "cmuroad.h"
#include "cplayer.h"

using namespace std;

int main(int argc, char *argv[])
{
  int num;
  char c;
  

  try
  {
    Cmuroad muroad(argc, argv);
    muroad.daemonize();

    

    CPlayer player(4001, muroad.audioDevice());
    player.start();

    cin >> c;

  }
  catch(string except)
  {
    if(except.compare("help") == 0)
      exit(0);
    else
      cerr << except << endl;
  }

  return EXIT_SUCCESS;
}