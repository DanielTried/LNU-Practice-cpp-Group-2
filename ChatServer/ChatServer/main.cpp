#include <QApplication>
#include "ChatServer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ChatServer server;
    server.startServer();
    server.show();

    return a.exec();
}
