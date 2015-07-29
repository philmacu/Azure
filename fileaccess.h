#ifndef FILEACCESS_H
#define FILEACCESS_H

#include <QMainWindow>
#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QString>

class ScenarioThread;
class ContactClass;

class FileAccess : public QMainWindow
{
    Q_OBJECT
public:
    explicit FileAccess(QWidget *parent = 0);
    ~FileAccess();
    int loadFile(QString &fileName, ScenarioThread *scenario);
    int loadContacts(QString &fileName, ContactClass *contactBook);
    ScenarioThread *m_loadedScenario; // used to copy stuff loaded back out
    ContactClass *m_loadedContacts;
    // after loading give us a ref to the phoneBooks
    ContactClass *pcsPhoneBook;
    ContactClass *smsPhoneBook;
    ContactClass *dmrPhoneBook;
    int taskErrors; // number of errors found when loading tasks
    int scenarioErrors;
private:
    QString m_fileText;
    QFile *file;
    QFile *contactsFile;
    void removeComments(void);// removes CR,LF and comments
    int loadScenarioConfig(void);
    int loadTaskConfig(void);
    int loadUserInfo(void); // this parses the user info
signals:
    void statusUpdate(QString message);
public slots:
};

#endif // FILEACCESS_H
