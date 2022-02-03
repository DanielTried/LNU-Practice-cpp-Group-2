#include "StartDialog.h"

StartDialog::StartDialog(QWidget* parent) : QDialog(parent)
{
    m_nickname = new QLineEdit;
    QLabel* lblName = new QLabel("Name:");
    QPushButton* okBtn = new QPushButton("Ok");

    connect(okBtn, SIGNAL(clicked()), SLOT(nameCompleted()));
    connect(this, SIGNAL(inputCompleted()), SLOT(accept()));

    QRegularExpressionValidator validator(QRegularExpression("[0-9a-zA-z]"));
    m_nickname->setValidator(&validator);

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(lblName);
    layout->addWidget(m_nickname, 3);
    layout->addWidget(okBtn);
    setLayout(layout);
}

QString StartDialog::name() const
{
    return m_nickname->text();
}

void StartDialog::nameCompleted()
{
    if((m_nickname->text().remove(' ') != "") && !m_nickname->text().isEmpty())
        emit inputCompleted();
}
