QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SOURCES += \
        ChatServer.cpp \
        Client.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ChatServer.h \
    Client.h \
    Commands.h
