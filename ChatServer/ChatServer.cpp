#include "ChatServer.h"

//створення макету та елементів взаємодії класом QWidget
ChatServer::ChatServer(QWidget* parent) : QWidget(parent), m_nextBlockSize(0)
{
    m_server = new QTcpServer;
    m_console = new QTextEdit;
    m_command = new QLineEdit;

    m_console->setReadOnly(true);

    QLabel* commandLbl = new QLabel("Командний рядок");

    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addWidget(commandLbl);
    hLayout->addWidget(m_command, 3);
    QVBoxLayout* vLayout = new QVBoxLayout;
    vLayout->addWidget(m_console, 4);
    vLayout->addLayout(hLayout);
    setLayout(vLayout);

    setMinimumSize(350, 250);
    connect(m_server, SIGNAL(newConnection()), this, SLOT(slotNewConnection()));
    connect(m_command, SIGNAL(returnPressed()), this, SLOT(serverCommand()));
}

// Запуск сервера
void ChatServer::startServer()
{
    if(m_server->isListening())
      /* isListening() - булева функція QTcpServer
         Вказує серверу прослуховувати вхідні з’єднання за адресою та портом.
         Якщо порт дорівнює 0, порт вибирається автоматично.
      */
    {
        // для виводу часу операції в термінал застосовується QTime::currentTime().toString()
        m_console->append(QTime::currentTime().toString() + " Сервер вже працює!");
        return;
    }
//f
    if(m_server->listen(QHostAddress::Any, 2323))
        // Якщо адресою є QHostAddress::Any, сервер слухатиме всі мережеві інтерфейси. Дефолтний порт 2323
        // реалізувати вибір порту

        m_console->append(QTime::currentTime().toString() + " Сервер запущено!");
    else
        m_console->append(QTime::currentTime().toString() + " Неможливо запустити сервер: " + m_server->errorString());
}

//Нове підключення
void ChatServer::slotNewConnection()
{
    QTcpSocket* clientSocket = m_server->nextPendingConnection();
    Client* client = new Client(clientSocket, this);
    m_clients.append(client);

    connect(clientSocket, SIGNAL(disconnected()), this, SLOT(deleteSocket())); //
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(readClient())); //
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
            sendServerMessage(m_clients.at(i)->name() + " вийшов із чату"); //пов клієнтам
            m_console->append(QTime::currentTime().toString() + " " + m_clients.at(i)->name() + " було відєднано"); //пов в термінал
            m_clients.removeAt(i);
        }
    m_mutex.unlock();

    snd->deleteLater(); //видалення користувача після завершення потоку
}

// ВЗАЄМОДІЯ З КЛІЄНТОМ
void ChatServer::readClient()
{
    //знаходження відправника
    QTcpSocket* clientSocket = (QTcpSocket*)sender();
    QDataStream in(clientSocket);
    in.setVersion(QDataStream::Qt_6_0);

    //Перевірка цілісності повідомлення (серіалізація відбувається з допомогою QDataSteam)
    // https://doc.qt.io/qt-5/qdatastream.html#details
    if(m_nextBlockSize == 0)
    {
        if(clientSocket->bytesAvailable() < sizeof(quint16)) //quint16 встановлює розрядність 16 біт для даних
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

//авторизація клієнта
void ChatServer::authClient(QTcpSocket *client)
{
    QDataStream in(client);
    in.setVersion(QDataStream::Qt_6_0);
    QString name;
    in >> name;

    //Перевірка на наявність клієнта з таким же ім'ям
    m_mutex.lock();
    foreach(Client* authClient, m_clients)
        if(authClient->name() == name)
        {
            //якщо такий клієнт існує = відмовити
            authFailed(client);
            m_mutex.unlock();
            return;
        }

    //якщо такого клієнта не існує = впустити
    for(int i = m_clients.size() - 1; i >= 0; i--)
        if(m_clients.at(i)->socket() == client)
            m_clients.at(i)->setName(name);

    authSuccess(client);
    m_mutex.unlock();
}

void ChatServer::authFailed(QTcpSocket *client)
{
    //Повідомлення клієнта про невдалу аутентифікацію
    sendServerMessage("Authentication failed", client);

    //усунення клієнта
    for(int i = m_clients.size() - 1; i >= 0; i--)
        if(m_clients.at(i)->socket() == client)
            m_clients.removeAt(i);
    //Консольне повід про невдалу аутентифікацію
    m_console->append(QTime::currentTime().toString() + " " +
                      QString::number(client->socketDescriptor()) + ": аутентифікація невдала.");

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
    sendServerMessage("Користувач " + name + " увійшов у чат");

    m_console->append(QTime::currentTime().toString() + " " + name +
                      "(" + QString::number(client->socketDescriptor()) + ") " + " успішно приєднано!");
}

//Надсилання серверних повідомлень клієнту
        // Серіалізація даних для TCP зєднання відбувається за допомогою QDataStream
void ChatServer::sendServerMessage(QString message, QTcpSocket *client)
{
    QByteArray data;
/* QByteArray створює байтний масив з даних.
   Разом із серіалізацією QDataStream та перевіркою цілісності забезпечує надійність передачі даних.
*/
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

//Надсилання повідомлення одному клієнту
void ChatServer::sendToClient(QByteArray data, QTcpSocket *client)
{
    client->write(data);
}

//розсилка повідомлення всім клієнтам
void ChatServer::sendToAllClients(QByteArray data)
{
    foreach(Client* client, m_clients)
        client->socket()->write(data);
}

// завершення виконання програми
void ChatServer::stopServer()
{
    this->close();
}



//визначення та виконання серверних команд
void ChatServer::serverCommand()
{
    //зчитування команд з командного рядка
    QString consoleCmd = m_command->text();
    if(consoleCmd.isNull() || (consoleCmd.remove(' ') == ""))
        return;
    // start\stop сервера
    if(consoleCmd == "stop")
        stopServer();
    if(consoleCmd == "start")
        startServer();


    m_command->setText("");
}
