#pragma once
#include <QtWidgets>
#include <QTcpSocket>
#include "StartDialog.h"
#include "Commands.h"

class ChatClient : public QWidget
{
    Q_OBJECT

private:
    QTcpSocket* m_socket;
    QTextEdit* m_messages;
    QLineEdit* m_message;
    QLabel* m_name;
    quint16 m_nextBlockSize;

    QString m_host;
    int m_port;

    void showMessage();
    void showServerMessage();
    void sendClientInfo();

public:
    ChatClient(const QString& host = "localhost", int port = 2323, QWidget *parent = nullptr);

public slots:
    void socketReadReady();
    void socketConnected();
    void socketDisconnected();
    void socketError(QAbstractSocket::SocketError error);
    void sendMessage();
    void connectToServer();
};
