#pragma once
#include <QTime>
#include <QMutex>
#include <QtWidgets>
#include <QTcpServer>
#include <QTcpSocket>
#include "Client.h"
#include "Commands.h"

class ChatServer : public QWidget
{
    Q_OBJECT
private:
    QList<Client*> m_clients;
    QTcpServer* m_server;
    QMutex m_mutex;
    quint16 m_nextBlockSize;

    QTextEdit* m_console;
    QLineEdit* m_command;

    void sendToAllClients(QByteArray data);
    void sendToClient(QByteArray data, QTcpSocket* client);
    void sendMessage(QTcpSocket* sender);
    void sendServerMessage(QString message, QTcpSocket* client = nullptr);
    void authClient(QTcpSocket* client);
    void authFailed(QTcpSocket* client);
    void authSuccess(QTcpSocket* client);
    void stopServer();

public:
    ChatServer(QWidget* parent = nullptr);

public slots:
    void startServer();
    void slotNewConnection();
    void deleteSocket();
    void readClient();
    void serverCommand();
};
