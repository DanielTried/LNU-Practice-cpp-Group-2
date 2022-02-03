#pragma once
#include <QtWidgets>

class StartDialog : public QDialog
{
    Q_OBJECT
private:
    QLineEdit* m_nickname;

public:
    StartDialog(QWidget* parent = nullptr);

    QString name() const;

signals:
    void inputCompleted();

public slots:
    void nameCompleted();
};
