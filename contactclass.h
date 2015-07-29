#ifndef CONTACTCLASS_H
#define CONTACTCLASS_H

#include <QMainWindow>
#include <QObject>
#include <QString>
#include <QChar>
#include <QMultiMap>
#include <QStack>
#include <QList>
#include <QSet>
#include <QDebug>


class ContactClass : public QMainWindow
{
    Q_OBJECT

public:
    explicit ContactClass(QWidget *parent = 0);
    ~ContactClass();
    int addContact(QChar group, QString name, QString number);
    int checkValidGroup(QChar group);
    QStack<QString> getContactsForGroup(QChar group); // returns Stack of numbers to service
    QMultiMap<QChar,QString> contactNumbers;
    QStack<QString> toCall; // this is a list of numbers in a group

private:

signals:

public slots:
};

#endif // CONTACTCLASS_H
