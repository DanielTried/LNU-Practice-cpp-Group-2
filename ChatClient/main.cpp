#include <QApplication>
#include "ChatClient.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ChatClient w;
    w.show();
    return a.exec();
}
