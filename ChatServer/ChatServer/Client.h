#pragma once
#include <QObject>
#include <QTcpSocket>

class Client : public QObject
{
    Q_OBJECT
private:
    QString m_name;
    QTcpSocket* m_socket;

public:
    Client(QTcpSocket* socket, QObject *parent = nullptr);

    void setName(QString name);

    QString name();
    QTcpSocket* socket();
};
