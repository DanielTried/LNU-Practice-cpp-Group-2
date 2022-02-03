QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    StartDialog.cpp \
    main.cpp \
    ChatClient.cpp

HEADERS += \
    ChatClient.h \
    Commands.h \
    StartDialog.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Des.qml \
    DesForm.ui.qml
