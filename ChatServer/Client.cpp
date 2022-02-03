#include "Client.h"

Client::Client(QTcpSocket* socket, QObject *parent) : QObject(parent), m_socket(socket)
{

}

void Client::setName(QString name)
{
    m_name = name;
}

QString Client::name()
{
    return m_name;
}

QTcpSocket* Client::socket()
{
    return m_socket;
}
