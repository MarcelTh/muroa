#include "CNetwork.h"
#include <QString>
#include <QDebug>

#include <exception>

CNetwork::CNetwork()
{
    qDebug() << QString("CNetwork::CNetwork()");
    m_server = new QTcpServer();
    qDebug() << QString("new QTcpServer() returned 0x%1").arg((ulong)m_server, 8, 16, QLatin1Char('0'));

    connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

CNetwork::~CNetwork()
{
    delete m_server;
}

int CNetwork::listen(int portNr)
{
    if (!m_server->listen(QHostAddress::Any, portNr )) {
    //    qDebug() << QString("throwing exception!");
    //    throw std::exception();
        return -1;
     }
     else {
    	 m_port_nr = portNr;
    	 return m_port_nr;
     }
}


void CNetwork::newConnection()
{
    // qDebug() << QString("new connection requested");
    QTcpSocket* socket = m_server->nextPendingConnection();
    qDebug() << QString("new connection requested: socket: 0x%1").arg((ulong)socket, 8, 16, QLatin1Char('0'));

    //connect(m_socket, SIGNAL(connected()), this, SLOT(connected()));
    //connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    //connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));

    emit newConnection(socket);

    //m_server->close();
}


//void CNetwork::connected()
//{
//    qDebug() << QString("socket connected");
//
//}

//void CNetwork::disconnected()
//{
//    qDebug() << QString("socket disconnected");
//
//    disconnect(m_socket, SIGNAL(connected()), this, SLOT(connected()));
//    disconnect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
//    disconnect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));
//
//    //delete m_socket;
//
//    emit connectionClosed(m_socket);
//
//    m_socket = 0;
//
//    listen();
//}
//
//void CNetwork::error(QAbstractSocket::SocketError socketError)
//{
//    qDebug() << QString("socket error %1").arg(m_socket->errorString());
//
//}

