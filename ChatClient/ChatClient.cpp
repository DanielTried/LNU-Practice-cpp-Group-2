#include "ChatClient.h"

//визначення елементів та створення макету з допомогою QWidget
ChatClient::ChatClient(const QString& host, int port, QWidget *parent)
    : QWidget(parent), m_socket(nullptr), m_nextBlockSize(0)
{
    m_host = host;
    m_port = port;

    m_messages = new QTextEdit;
    m_messages->setReadOnly(true);
    m_message = new QLineEdit;
    m_name = new QLabel("*Ваше ім'я*");

    // кнопки
    QPushButton* sendButton = new QPushButton("Надіслати");
    connect(sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
    connect(m_message, SIGNAL(returnPressed()), this, SLOT(sendMessage()));

    QPushButton* connectButton = new QPushButton("Увійти до кімнати");
    connect(connectButton, SIGNAL(clicked()), this, SLOT(connectToServer()));


  QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addWidget(m_name);
    hLayout->addWidget(m_message, 3);
    hLayout->addWidget(sendButton);
   QVBoxLayout* vLayout = new QVBoxLayout;
    vLayout->addWidget(new QLabel("TCPChat"));
    vLayout->addWidget(m_messages);
    vLayout->addLayout(hLayout);
    vLayout->addWidget(connectButton);
    setLayout(vLayout);
}

// Приєднання
void ChatClient::connectToServer()
{
    if(m_socket && (m_socket->state() == QTcpSocket::ConnectedState))
        return;

// Вікно приєднання до сервера
    StartDialog* startDialog = new StartDialog;
    if(startDialog->exec() == QDialog::Accepted)
    {
        m_socket = new QTcpSocket(this);
        m_socket->connectToHost(m_host, m_port);
        m_name->setText(startDialog->name());


        connect(m_socket, SIGNAL(readyRead()), this, SLOT(socketReadReady()));
        connect(m_socket, SIGNAL(connected()), this, SLOT(socketConnected()));
        connect(m_socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
        connect(m_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));

        if(m_socket->waitForConnected(1000))
            sendClientInfo();
    }
    // Закриття вікна
    delete startDialog;
}

void ChatClient::socketConnected()
{
//    повідомлення про час підєднання;
//    m_messages->append(time.currentTime().toString() + " Приєднано");
}

void ChatClient::socketDisconnected()
{
    m_socket->deleteLater();
}

void ChatClient::socketReadReady()
{
    QDataStream in(m_socket);
    in.setVersion(QDataStream::Qt_6_0);

    //перевірка цілісності повідомлення
    if(m_nextBlockSize == 0)
    {
        if(m_socket->bytesAvailable() < sizeof(quint16))
            return;
        in >> m_nextBlockSize;
    }

    if(m_socket->bytesAvailable() < m_nextBlockSize)
        return;

    //Визначення команди
    quint8 command;
    in >> command;

    //Виконання команди
    switch (command)
    {
    case message:
    {
        showMessage();
        break;
    }
    case serverMessage:
    {
        showServerMessage();
        break;
    }
    }

    m_nextBlockSize = 0;
}

//Відображення повідомлень інших клієнтів
void ChatClient::showMessage()
{
    QDataStream in(m_socket);
    in.setVersion(QDataStream::Qt_6_0);

    QTime time;
    QString msg;
    QString name;
    in >> time >> name >> msg;

    m_messages->append(time.toString() + " <" + name + "> " + msg);
}

//Показ повідомлень сервера
void ChatClient::showServerMessage()
{
    QDataStream in(m_socket);
    in.setVersion(QDataStream::Qt_6_0);

    QTime time;
    QString msg;
    in >> time >> msg;

    m_messages->append(time.toString() + " " + msg);
}

//Надсилання повідомлення
void ChatClient::sendMessage()
{
    if((m_message->text().remove(' ') != "") && !m_message->text().isEmpty() && m_socket->state() == QAbstractSocket::ConnectedState)
    {
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_0);
        out << quint16(0) << quint8(message) << QTime::currentTime() << m_name->text() << m_message->text();

        out.device()->seek(0);
        out << quint16(data.size() - sizeof(quint16));

        m_socket->write(data);
        m_message->setText("");
    }
}

//інформація авторизації серверу
void ChatClient::sendClientInfo()
{
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint16(0) << quint8(clientInfo) << m_name->text();

    out.device()->seek(0);
    out << quint16(data.size() - sizeof(quint16));

    m_socket->write(data);
}

//Display connection errors
void ChatClient::socketError(QAbstractSocket::SocketError error)
{
    QString strError = " Error: " + (error == QAbstractSocket::HostNotFoundError ? "Хост не знайдено." :
                                    error == QAbstractSocket::RemoteHostClosedError ? "Відалений хост закритий" :
                                    error == QAbstractSocket::ConnectionRefusedError ? "Відмовлено" :
                                    QString(m_socket->errorString()));
    m_messages->append(QTime::currentTime().toString() + strError);

    m_socket->deleteLater();
    m_socket = nullptr;
}
