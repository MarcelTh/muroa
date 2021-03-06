/***************************************************************************
 *   Copyright (C) 2005 by Martin Runge                                    *
 *   martin.runge@web.de                                                   *
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


#include "cstreamserver.h"

#include <iostream>
#include <string>

CStreamServer streamserver(1, 600);


char* tmp_buffer;
int tmp_length;
int tmp_offset;


extern "C" {


int ds_cpp_init(char** clients) {

  tmp_length = 8192;
  tmp_buffer = (char*)malloc(tmp_length);
  tmp_offset = 0;

  std::string str;

  streamserver.stdClientPort(4001);  

  for(int i=0; clients[i] != NULL; i++) {
    str = clients[i];
    if(str.size() > 0) {
      std::cerr << "client " << i << " is " << clients[i] << std::endl;
      CIPv4Address *localaddr = new CIPv4Address(clients[i], streamserver.stdClientPort());
      streamserver.addClient(localaddr);
    }
  }
  
  return 0;
}


int ds_cpp_config_ok(char** clients) {
  std::list<std::string> clients_list;
  std::string str;

  for(int i=0; clients[i] != NULL; i++) {
    str = clients[i];
    if(str.size() > 0)
      clients_list.push_back(clients[i]);    
  }    
  streamserver.adjustClientListTo(clients_list);  
}

int ds_cpp_open(int audio_bytes_per_second) {
  // streamserver = new CStreamServer( 1, 300 );
  std::cerr << "ds_cpp_open / audio bytes per sec:" << audio_bytes_per_second << std::endl;

  streamserver.listClients( );

  streamserver.open(audio_bytes_per_second); 

  return 0;
}

void ds_cpp_close() {
  streamserver.close();
}


int ds_cpp_write(char* buffer, int length) {

  int offset2;
  int write_length;

  // std::cerr << "ds_cpp_write length " << length ;

  offset2 = 0;  
  write_length = 1024;

  memcpy(tmp_buffer + tmp_offset, buffer, length);
  tmp_length = tmp_offset + length;

  while (write_length + offset2 < tmp_length) {
    streamserver.write(tmp_buffer + offset2, write_length);
    offset2 += write_length;
    // std::cerr << " write: " << write_length;
  } 
  // std::cerr << std::endl;
  // copy  rest to begining of buffer
    
  memcpy(tmp_buffer, tmp_buffer + offset2, tmp_length - offset2);
  tmp_offset = tmp_length - offset2;

  return 0;
}



void ds_cpp_flush() {
  std::cerr << "ds_cpp_flush()" << std::endl;
  streamserver.flush();
}

void ds_cpp_pause() {
  std::cerr << "ds_cpp_pause()" << std::endl;
  // streamserver->pause();
}
}
