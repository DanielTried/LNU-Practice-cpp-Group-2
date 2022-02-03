#include "ChatServer.h"


void ChatServer::startServer()
{
    if(m_server->isListening())
    {
        m_console->append(QTime::currentTime().toString() + " Server is already started");
        return;
    }

    if(m_server->listen(QHostAddress::Any, 2323))
        m_console->append(QTime::currentTime().toString() + " Server started!");
    else
        m_console->append(QTime::currentTime().toString() + " Unable to start the server: " + m_server->errorString());
}

//Нове підключення
void ChatServer::slotNewConnection()
{
    QTcpSocket* clientSocket = m_server->nextPendingConnection();
    Client* client = new Client(clientSocket, this);
    m_clients.append(client);

    connect(clientSocket, SIGNAL(disconnected()), this, SLOT(deleteSocket()));
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(readClient()));
}

//Знайти та роз'єднати неактивного клієнта
//Розсилання сповіщення до сервера та інших клієнтів
void ChatServer::deleteSocket()
{
    QTcpSocket* snd = (QTcpSocket*)sender();

    m_mutex.lock();
    for(int i = 0; i < m_clients.size(); i++)
        if(m_clients.at(i)->socket() == snd)
        {
            sendServerMessage(m_clients.at(i)->name() + " leave the chat");
            m_console->append(QTime::currentTime().toString() + " " + m_clients.at(i)->name() + " disconnected");
            m_clients.removeAt(i);
        }
    m_mutex.unlock();

    snd->deleteLater();
}

void ChatServer::readClient()
{
    //знаходження відправника
    QTcpSocket* clientSocket = (QTcpSocket*)sender();
    QDataStream in(clientSocket);
    in.setVersion(QDataStream::Qt_6_0);

    //Перевірка цілісності повідомлення
    if(m_nextBlockSize == 0)
    {
        if(clientSocket->bytesAvailable() < sizeof(quint16))
            return;
        in >> m_nextBlockSize;
    }

    if(clientSocket->bytesAvailable() < m_nextBlockSize)
        return;

    //визначення команди
    quint8 command;
    in >> command;

    //виконання команди
    switch (command)
    {
    case message:
    {
        sendMessage(clientSocket);
        break;
    }
    case clientInfo:
    {
        authClient(clientSocket);
        break;
    }
    }
    m_nextBlockSize = 0;
}

//Надсилання повідомлення клієнта іншим клієнтам
void ChatServer::sendMessage(QTcpSocket* sender)
{
    QByteArray data;

    QDataStream in(sender);
    in.setVersion(QDataStream::Qt_6_0);
    QTime time;
    QString name, msg;
    in >> time >> name >> msg;

    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint16(0) << quint8(message) << time << name << msg;

    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));
    sendToAllClients(data);
}

void ChatServer::authClient(QTcpSocket *client)
{
    QDataStream in(client);
    in.setVersion(QDataStream::Qt_6_0);
    QString name;
    in >> name;

    //Перевірка клієнта
    m_mutex.lock();
    foreach(Client* authClient, m_clients)
        if(authClient->name() == name)
        {
            //якщо клієнт існує
            authFailed(client);
            m_mutex.unlock();
            return;
        }

    //якщо клієнта не існує
    for(int i = m_clients.size() - 1; i >= 0; i--)
        if(m_clients.at(i)->socket() == client)
            m_clients.at(i)->setName(name);

    authSuccess(client);
    m_mutex.unlock();
}

void ChatServer::authFailed(QTcpSocket *client)
{
    //Повідомлення клієнда про невдалу аутентифікацію
    sendServerMessage("Authentication failed", client);

    //усунення клієнта
    for(int i = m_clients.size() - 1; i >= 0; i--)
        if(m_clients.at(i)->socket() == client)
            m_clients.removeAt(i);
    //Консольне повід про невдалу аутентифікацію
    m_console->append(QTime::currentTime().toString() + " " +
                      QString::number(client->socketDescriptor()) + ": authentication failed");

    client->disconnectFromHost();
}

void ChatServer::authSuccess(QTcpSocket* client)
{
    //Визначення клієнта
    QString name;
    for(int i = m_clients.size() - 1; i >= 0; i--)
        if(m_clients.at(i)->socket() == client)
            name = m_clients.at(i)->name();

    //Сповіщення клієнту про приєднання
    sendServerMessage(name + " joined to the chat");

    m_console->append(QTime::currentTime().toString() + " " + name +
                      "(" + QString::number(client->socketDescriptor()) + ") " + " successfully connected");
}

//Надсилання серверних повідомлень клієнту
void ChatServer::sendServerMessage(QString message, QTcpSocket *client)
{
    QByteArray data;

    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    out << quint16(0) << quint8(serverMessage) << QTime::currentTime() << " " + message;

    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));

    if(client)
    {
        sendToClient(data, client);
        return;
    }
    sendToAllClients(data);
}

//Надсилання повідомлення клієнту
void ChatServer::sendToClient(QByteArray data, QTcpSocket *client)
{
    client->write(data);
}

//розсилка повідомлення
void ChatServer::sendToAllClients(QByteArray data)
{
    foreach(Client* client, m_clients)
        client->socket()->write(data);
}

void ChatServer::stopServer()
{
    this->close();
}

//визначення та виконання серверних команд
void ChatServer::serverCommand()
{
    QString consoleCmd = m_command->text();
    if(consoleCmd.isNull() || (consoleCmd.remove(' ') == ""))
        return;

    if(consoleCmd == "stop")
        stopServer();
    if(consoleCmd == "start")
        startServer();

    m_command->setText("");
}
