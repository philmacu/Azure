/*
 * This class handles,
 * storing, adding, checking and retrieving
 * names, numbers, group from 'phone book'
 *
 */

#include "contactclass.h"

ContactClass::ContactClass(QWidget *parent) : QMainWindow(parent)
{

}

ContactClass::~ContactClass()
{

}

int ContactClass::addContact(QChar group, QString name, QString number)
{
    // now do the addition
    // combinr name and number into one string
    QString nameNumber = name + number;
    contactNumbers.insertMulti(group,nameNumber);
    return 1;
}

QStack<QString> ContactClass::getContactsForGroup(QChar group)
{
    // dump the old call list
    toCall.clear();
    QSet<QChar> keys = QSet<QChar>::fromList(contactNumbers.keys());
    int numberStacked = 0;
    // now iterate over the complete List using an iterator
    QMultiMap<QChar,QString>::ConstIterator ii = contactNumbers.find(group);
    while(ii!=contactNumbers.end() && ii.key()== group)
    {
        toCall.push(ii.value());
        ++ii;
        numberStacked++;
    }
    return toCall;
}
