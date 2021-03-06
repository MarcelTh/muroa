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
#include "cstreamserver.h"

using namespace std;
using namespace boost::posix_time;
using namespace muroa;

CStreamServer::CStreamServer(int session_id, int transport_buffer_size_in_ms) : 
            m_first_send_time(not_a_date_time), 
            m_send_time(not_a_date_time), 
            m_last_send_time(not_a_date_time) {

  m_session_id = session_id;
  m_stream_id = 0;

  m_payload_size = 1024;
  m_audio_bytes_per_frame = 2 * sizeof(short);

  m_transport_buffer_duration = millisec(transport_buffer_size_in_ms);

  m_payload_duration_sum = microseconds(0);
  m_total_play_time = microseconds(0);
  
  m_rtp_packet = new CRTPPacket(m_session_id, m_stream_id, m_payload_size);


  m_frames_in_sync_period = 0;


}


CStreamServer::~CStreamServer()
{
}




/*!
    \fn CStreamServer::open()
 */
int CStreamServer::open(int audio_bytes_per_second)
{
  m_last_send_time = microsec_clock::universal_time();
  m_last_payload_duration = not_a_date_time;
  
  ptime now = m_last_send_time;

  m_stream_id++;

  m_syncobj.setTimeToNow();
  m_syncobj.addDuration(m_transport_buffer_duration);
  m_syncobj.frameNr(0);
  m_syncobj.streamId(m_stream_id);  
  m_syncobj.sessionId(m_session_id);
  m_syncobj.serialize();

  m_rtp_packet->sessionID(m_session_id);
  m_rtp_packet->streamID(m_stream_id);

  m_rtp_packet->seqNum(m_rtp_packet->seqNum() + 1);
  m_rtp_packet->payloadType(PAYLOAD_SYNC_OBJ);
  m_rtp_packet->copyInPayload(m_syncobj.getSerialisationBufferPtr(), m_syncobj.getSerialisationBufferSize());

  m_audio_bytes_per_second = audio_bytes_per_second;
  
  m_frames_in_sync_period = 0;
  m_payload_duration_sum = microseconds(0);
  cerr << "CStreamServer::open: audiobytes/s = " << audio_bytes_per_second 
       << " session/stream id = (" << m_session_id << "/" << m_stream_id << ")" 
       << " time = " << now << endl;

  sendToAllClients(m_rtp_packet);

  return 0;
}


/*!
    \fn CStreamServer::close()
 */
void CStreamServer::close()
{
    cerr << "CStreamServer::close()" << endl;

    /// @todo implement me
    // send end of stream packet
}


int CStreamServer::write(char* buffer, int length) {
    int sum = 0;
    int left = length;
    int offset = 0;

   while( left > 0 ) {
       if(left >= 1024) {
           sum += sendPacket(buffer + offset, 1024);
           offset += 1024;
           left -= 1024;
       }
       else {
           sum += sendPacket(buffer + offset, left);
           offset += left;
           left = 0;
       }
   }

   return sum;
}

/*!
    \fn CStreamServer::write(char* buffer, int length)
 */
int CStreamServer::sendPacket(char* buffer, int length) {

    time_duration payload_duration = microseconds((length * 1000000) / m_audio_bytes_per_second);

    m_rtp_packet->copyInPayload(buffer, length);
    

    m_rtp_packet->seqNum( m_rtp_packet->seqNum() + 1 ); 
    m_rtp_packet->timestamp( m_frames_in_sync_period );
    m_rtp_packet->payloadType(PAYLOAD_PCM);

//    measure the quality of the below usleep calculation
    
    //now = microsec_clock::universal_time();
    //interval = now - m_last_send_time;
    //cerr << "interval: " << interval << endl;


    sendToAllClients(m_rtp_packet);

    m_frames_in_sync_period += length / m_audio_bytes_per_frame;

//    m_last_send_time = now;    

    if(m_last_payload_duration.is_special() == true) {
      // if this is the first packet, assume the "last" with same duration 
      // for sleep calculation below. 
      
      cerr << " CStreamSource::readAndSend: ============ STREAM RESET! =============" << endl;
      m_last_payload_duration = payload_duration;
      m_time_since_last_packet = m_last_payload_duration;

      m_first_send_time = microsec_clock::universal_time();
      m_send_time = m_first_send_time;
      
      m_payload_duration_sum = payload_duration;
      // cerr << " payload_duration sum = " << m_payload_duration_sum;

    }
    else {
      m_send_time = microsec_clock::universal_time();
      m_payload_duration_sum += payload_duration;                  
      // cerr << " payload_duration sum=" << m_payload_duration_sum;
    }

    m_total_play_time = m_send_time - m_first_send_time;
    // cerr << " total play time=" << total_play_time;

    time_duration sleep_duration = m_payload_duration_sum - m_total_play_time;
    long sleep_duration_in_us = sleep_duration.total_microseconds();    

    // cerr << " -> sleeping " << sleep_duration_in_us << " us before sending next packet." << endl;
    
    // only sleep, if there is time to sleep. especially on negative values, we are much too late, so don't sleep at all.
    if(sleep_duration_in_us > 0)
      usleep(sleep_duration_in_us);
    else {
      // cerr << " Sleep duration was < 0!" << endl;
    }

    m_last_payload_duration = payload_duration;
}


/*!
    \fn CStreamServer::flush()
 */
void CStreamServer::flush()
{
    /// @todo implement me
}


list<CStreamConnection*>::iterator CStreamServer::addStreamConnection(CStreamConnection* conn) {

  list<CStreamConnection*>::iterator iter;

  // do some checks on the socket here.

  m_connection_list_mutex.Lock();
  iter = m_connection_list.insert(m_connection_list.end(), conn);
  m_connection_list_mutex.UnLock();

}


CStreamConnection* CStreamServer::removeStreamConnection(list<CStreamConnection*>::iterator conn_iterator) {

  CStreamConnection* conn;  
  conn = *conn_iterator;
  
  m_connection_list_mutex.Lock();
  m_connection_list.erase(conn_iterator);
  m_connection_list_mutex.UnLock();

  return conn;
}



/*!
    \fn CStreamServer::sendToAllClients(CRTPPacket* packet)
 */
void CStreamServer::sendToAllClients(CRTPPacket* packet)
{
    list<CStreamConnection*>::iterator iter;
    for(iter = m_connection_list.begin(); iter != m_connection_list.end(); iter++ ) {
      (*iter)->send(packet->bufferPtr(), packet->usedBufferSize()); 
    }
}


/*!
    \fn CStreamServer::addClient(CIPv4Address* addr)
 */
list<CStreamConnection*>::iterator CStreamServer::addClient(CIPv4Address* addr, const string& name)
{
    bool known = false;

    list<CStreamConnection*>::iterator iter;
    for(iter = m_connection_list.begin(); iter != m_connection_list.end(); iter++ ) {
      CIPv4Address* knownaddr = (*iter)->getClientAddress();
      if( *knownaddr == *addr ) {
          known = true;
      }
    }

    if( known == false ) {
        CStreamConnection* conn = new CStreamConnection(this, name);
        conn->connect(addr);
        return addStreamConnection(conn);
    }
    else
        return m_connection_list.end();

}

void CStreamServer::adjustReceiverList(std::vector<ServDescPtr> receivers)
{
    list<CStreamConnection*>::iterator iter;
    for(iter = m_connection_list.begin(); iter != m_connection_list.end(); iter++ ) {

        bool found = false;
        for(int i=0; i < receivers.size(); i++) {
            string servicename = receivers[i]->getServiceName();
            if( servicename.compare( (*iter)->getName() ) == 0 ) {
                found = true;
                receivers.erase(receivers.begin() + i);
                break;
            }
        }
        if( found = false) {
            removeClient(iter);
        }
    }
    // add oaa receivers left in receivers list
    for(int i=0; i < receivers.size(); i++) {
        ServDescPtr srv_desc_ptr = receivers[i];
        CIPv4Address addr(srv_desc_ptr->getHostName(), srv_desc_ptr->getPortNr());
        addClient(&addr, srv_desc_ptr->getServiceName());
    }
}

/*!
    \fn CStreamServer::removeClient(CIPv4Address* addr);
 */
void CStreamServer::removeClient(const string& name)
{
    list<CStreamConnection*>::iterator iter;
    for(iter = m_connection_list.begin(); iter != m_connection_list.end(); iter++ ) {
        CStreamConnection* sc = *iter;
        if( name.compare( sc->getName() ) == 0 ) {
            removeClient(iter);
            iter--;
        }
    }
}


void CStreamServer::removeClient(list<CStreamConnection*>::iterator iter) {
  removeStreamConnection(iter);
}

/*!
    \fn CStreamServer::getSyncObj(uint32_t session_id, uint32_t stream_id)
 */
CSync* CStreamServer::getSyncObj(uint32_t session_id, uint32_t stream_id)
{
    if(m_syncobj.sessionId() == session_id && m_syncobj.streamId() == stream_id) {
      m_syncobj.serialize();
      return &m_syncobj;
    }
    else
      return 0;
}

//
//void CStreamServer::adjustClientListTo(std::list<std::string> clients)
//{
//  cerr << "CStreamServer::adjustClientListTo:" << endl;
//  CIPv4Address* connection_list_addr;
//  CIPv4Address tmp_addr;
//  list<CStreamConnection*>::iterator conn_iter, tmp_conn_iter;
//  list<string>::iterator clients_iter, clients_tmp_iter;
//
//  for(conn_iter = m_connection_list.begin(); conn_iter != m_connection_list.end(); conn_iter++ ) {
//
//
//    connection_list_addr = (*conn_iter)->getClientAddress();
//    cerr << "checking if " << *connection_list_addr << " is still in the list of clients..." ;
//    bool found = false;
//
//    for(clients_iter = clients.begin(); clients_iter != clients.end(); clients_iter++ ) {
//      tmp_addr = *clients_iter;
//      if(tmp_addr.port() == 0)
//        tmp_addr.port(m_std_client_port);
//
//      if(tmp_addr == *connection_list_addr) {
//        found = true;
//        break;
//      }
//    }
//    if (!found) {
//      cerr << " not found -> remove!" << endl;
//      if(conn_iter == m_connection_list.begin()) {
//        removeStreamConnection(conn_iter);
//        conn_iter = m_connection_list.begin();
//      }
//      else {
//        tmp_conn_iter = conn_iter;
//        --tmp_conn_iter;
//        removeStreamConnection(conn_iter);
//        conn_iter = tmp_conn_iter;
//      }
//    }
//    else {
//      cerr << "found -> keep!" << endl;
//    }
//  }
//  // now, all connections to clients not in the list any more have been removed.
//
//
//  for(clients_iter = clients.begin(); clients_iter != clients.end(); clients_iter++ ) {
//    cerr << "checking if " << *clients_iter << " is already in the list of connections ..." ;
//
//    tmp_addr = *clients_iter;
//    if(tmp_addr.port() == 0)
//      tmp_addr.port(m_std_client_port);
//
//    bool found = false;
//
//    for(conn_iter = m_connection_list.begin(); conn_iter != m_connection_list.end(); conn_iter++ ) {
//      connection_list_addr = (*conn_iter)->getClientAddress();
//
//      if(tmp_addr == *connection_list_addr) {
//        found = true;
//        break;
//      }
//    }
//    if (!found) {
//      cerr << " not found -> add!" << endl;
//      addClient(&tmp_addr);
//    }
//    else {
//      cerr << "found -> already present in connection list!" << endl;
//    }
//  }
//  // now, the lists are in sync
//
//}
//

/*!
    \fn CStreamServer::stdClientPort(int port)
 */
void CStreamServer::stdClientPort(int port)
{
  m_std_client_port = port;
}


/*!
    \fn CStreamServer::stdClientPort(void)
 */
int CStreamServer::stdClientPort(void)
{
  return m_std_client_port;
}


/*!
    \fn CStreamServer::listClients(void)
 */
void CStreamServer::listClients(void)
{
  list<CStreamConnection*>::iterator iter;
  cerr << "List of clients in session: " << endl;
  for(iter = m_connection_list.begin(); iter != m_connection_list.end(); iter++ ) {
    cerr << *(*iter)->getClientAddress() << endl;
  }
}
