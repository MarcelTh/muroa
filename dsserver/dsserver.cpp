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

#include <iostream>
#include <sys/time.h>
#include <getopt.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "libsock++.h"
#include "libdsaudio.h"

#include "cstreamconnection.h"
#include "csocket.h"
#include "csync.h"
#include "crtppacket.h"

#include "cipv4address.h"
#include "cstreamserver.h"

using namespace std;
using namespace boost::posix_time;

int cppserver(vector<string> clients, int sessionID, int streamID);
int cserver(int argc, char *argv[]);

void usage(string progname)
{
    cout << "usage: " << progname << " [options]" << endl
         << "options: " << endl
         << "  --client  -c <client1:port>   a render client in the form hostname:port, option can be repeated for two or more clients"  << endl
         << "  --sessionid -S <num>   a numerical session id" << endl
         << "  --streamid  -s <num>   a numerical stream  id" << endl;


}

int main(int argc, char *argv[]) {
//  cserver(argc, argv);
    char c;
    int verbose_flag = 0;

    vector<string> clients;
    int sessionID = -1;
    int streamID = -1;

    while (1)
      {
        static struct option long_options[] =
          {
            /* These options set a flag. */
            {"verbose", no_argument,       &verbose_flag, 1},
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {"clients",     required_argument, 0, 'c'},
            {"sessionid",   required_argument, 0, 'S'},
            {"streamid",    required_argument, 0, 's'},
            {0, 0, 0, 0}
          };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "c:S:s:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
          break;

        switch (c)
          {
          case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
              break;
            printf ("option %s", long_options[option_index].name);
            if (optarg)
              printf (" with arg %s", optarg);
            printf ("\n");
            break;

          case 'c':
            cerr <<"option -c with value '" <<  optarg << "'"<< endl;
            clients.push_back(optarg);
            break;

          case 'S':
            sessionID = strtol(optarg, NULL, 10);
            cerr << "option -S with value '"<< sessionID << "'" << endl;
            break;

          case 's':
            streamID = strtol(optarg, NULL, 10);
            cerr << "option -s with value '"<< streamID << "'" << endl;
            break;

          case 'h':
          default:
            usage(argv[0]);
            exit(0);
          }
      }


  cppserver(clients, sessionID, streamID);
  return 0;
}

int cppserver(vector<string> clients, int sessionID, int streamID) {

  unsigned long num;
  int buffersize = 1024;
  char *buffer = new char[buffersize];


  CIPv4Address *addr;
  CStreamServer streamserver(sessionID);

//  clients[0] = "localhost";
//  sockets[0] = new CSocket(SOCK_DGRAM);
//  sockets[0]->connect("localhost", 4001);


  for(int i = 0; i < clients.size(); i++) {
    addr = new CIPv4Address(clients[i], 4001);
    streamserver.addClient(addr, clients[i]);
  }

  streamserver.open();

  FILE* in_fd = fopen("infile.raw", "r");
  do {
    num = fread((void*)buffer, buffersize, 1, in_fd); 
    streamserver.write(buffer, num * buffersize);    


  } while(num != 0);

  streamserver.close();

  delete [] buffer;

  return 0;
}


int cserver(int argc, char *argv[]) {

  unsigned long num;
  int payload_size = 1024;

  int m_num_clients;
  int i;

  /** the actual system time when the last packet was sent */
  struct timeval m_send_time;

  /** system time when the first packet was sent */
  unsigned long long m_first_send_time = 0;
  unsigned long long send_time = 0;
  unsigned long long m_payload_duration_sum = 0;
  unsigned long long total_play_time = 0;

  string clients[10];
  CSocket *sockets[10];
  
  if(argc > 1 && strcmp(argv[1], "--help") == 0) {
    cout << "usage:" << endl 
         << "dsserver [client1] [client2] [client3] ... [client10]"  << endl
         << "client0 will always be 'localhost', the others are optional." << endl;

    exit(0);

  } 


//  clients[0] = "localhost";
//  sockets[0] = new CSocket(SOCK_DGRAM);
//  sockets[0]->connect("localhost", 4001);


  for(i=1; i < argc; i++) {
    clients[i-1] = argv[i];  

    sockets[i-1] = new CSocket(SOCK_DGRAM);
    sockets[i-1]->connect(clients[i-1], 4001);
  }

  m_num_clients = argc - 1;

  ptime m_last_send_time = microsec_clock::universal_time();
  ptime now = m_last_send_time;

  time_duration interval;

  int session_id = 1;
  int stream_id = 1;

  CRTPPacket rtp_packet(session_id, stream_id, payload_size);
  
  cout << "testserver!!!" << endl;

  unsigned long payload_duration_in_us = 0;
  long time_since_last_packet_in_us;
  long audio_bytes_per_second = 44100 * 2 * 2;

  int m_last_payload_duration_in_us = 0;
        

  FILE* in_fd = fopen("infile.raw", "r");
  
  int transport_buffer_size_in_ms = 1500;

  CSync syncobj;
  time_duration buffersize = millisec(transport_buffer_size_in_ms);

  syncobj.addDuration(buffersize);
  syncobj.frameNr(0);
  syncobj.streamId(stream_id);  
  syncobj.sessionId(session_id);
  syncobj.serialize();

  rtp_packet.payloadType(PAYLOAD_SYNC_OBJ);

  rtp_packet.copyInPayload(syncobj.getSerialisationBufferPtr(), syncobj.getSerialisationBufferSize());

//  rtp_packet.sessionID(session_id);
//  rtp_packet.streamID(stream_id);

          
  for(i=0; i < m_num_clients; i++) {
    sockets[i]->write(rtp_packet.bufferPtr(), rtp_packet.usedBufferSize());
  }

  long m_num_multi_channel_samples_before = 0;

  do {
    num = fread((void*)rtp_packet.payloadBufferPtr(), rtp_packet.payloadBufferSize(), 1, in_fd); 
    num *= rtp_packet.payloadBufferSize();
    rtp_packet.usedPayloadBufferSize(num);

    payload_duration_in_us = (num * 1000000) / audio_bytes_per_second;

    rtp_packet.seqNum( rtp_packet.seqNum() + 1 ); 
    rtp_packet.timestamp( m_num_multi_channel_samples_before );
    rtp_packet.payloadType(PAYLOAD_PCM);

//    measure the quality of the below usleep calculation
//    now = microsec_clock::universal_time();
//    interval = now - m_last_send_time;
//    cerr << "interval: " << interval << endl;

    

    for(i=0; i < m_num_clients; i++) {
      char *buffer = rtp_packet.bufferPtr();
      int buffersize = rtp_packet.bufferSize();
      sockets[i]->write(buffer, buffersize); 
    }

    m_num_multi_channel_samples_before += num / (2 * sizeof(short));

//    m_last_send_time = now;    

    if(m_last_payload_duration_in_us == 0) {
      // if this is the first packet, assume the "last" with same duration 
      // for sleep calculation below. 
      
      cerr << " CStreamSource::readAndSend: ============ STREAM RESET! =============" << endl;
      m_last_payload_duration_in_us = payload_duration_in_us;
      time_since_last_packet_in_us = m_last_payload_duration_in_us;

      gettimeofday(&m_send_time, NULL);
      m_first_send_time = m_send_time.tv_sec * 1000000 + m_send_time.tv_usec;
      send_time = m_first_send_time;
      
      m_payload_duration_sum = payload_duration_in_us;
      // cerr << " payload_duration sum = " << m_payload_duration_sum;

    }
    else {
      gettimeofday(&m_send_time, NULL);
      send_time = m_send_time.tv_sec * 1000000 + m_send_time.tv_usec;
                       
      m_payload_duration_sum += payload_duration_in_us;                  
      
      // cerr << " payload_duration sum=" << m_payload_duration_sum;
                       
    }

    total_play_time = send_time - m_first_send_time;
    // cerr << " total play time=" << total_play_time;

    long sleep_duration_in_us = m_payload_duration_sum - total_play_time;
    
    // cerr << " -> sleeping " << sleep_duration_in_us << " us before sending next packet." << endl;
    
    // only sleep, if there is time to sleep. especially on negative values, we are much too late, so don't sleep at all.
    if(sleep_duration_in_us > 0)
      usleep(sleep_duration_in_us);
    else {
      cerr << " Sleep duration was < 0!" << endl;
    }

    m_last_payload_duration_in_us = payload_duration_in_us;
  
  } while(num != 0);
  
  
  
  fclose(in_fd);
  return EXIT_SUCCESS;

}

