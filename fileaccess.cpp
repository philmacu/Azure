#include "fileaccess.h"
#include "scenarothread.h"
#include "contactclass.h"
#include "mainwindow.h"


/*
 *
 *
 * Class to handle loading of files into memory
 *
 *
 */

/*
 * Due to the fact that we are using pointers to Objs, we need to be carefull
 * a=b doesn't copy the data from b to a, it just changes the pointer
 * so that they point to same thing, so later on if we change b, a will
 * also change. OK, to solve that i created a pointer c int the main code
 * a reference to that is passed in, a local pointer (which we dont
 * care about) then points to this m_loade... = scena... so any changes
 * made to m_loade.. go back to the pointer taht we referenced in
 * i.e. the actual pointer in Q.
 */

FileAccess::FileAccess(QWidget *parent) : QMainWindow(parent)
{
    file = new QFile;
    contactsFile = new QFile;
}

FileAccess::~FileAccess()
{
    delete file;
    delete contactsFile;
}

int FileAccess::loadFile(QString &fileName,ScenarioThread *scenario)
{
    // dump any old values
    m_loadedScenario = scenario;
    m_loadedScenario->cleanObject();

    // copies the file into a text string
    file->setFileName(fileName);
    // check if it exists
    if(!file->exists())
    {
        qDebug() << "No such file " << fileName;
        emit statusUpdate("File not found!!! " + fileName);
        return 0;
    }
    else
    {
        // try read all of the data
        file->open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream fileStream(file);
        m_fileText = fileStream.readAll();
        file->close();
        emit statusUpdate("File loaded OK: " + fileName);
        // now start handling the data in the file
        removeComments();
        if (loadScenarioConfig())
        {
            emit statusUpdate("Scenario Loaded OK");
            if (loadTaskConfig())
            {
                emit statusUpdate("Task List OK");
                return 1;
            }
        }
    }
    return 0;
}

int FileAccess::loadContacts(QString &fileName, ContactClass *contactBook)
{
    // point the local reference to the phonebook
    m_loadedContacts = contactBook;

    // read the contacts file from memory

    // copies the file into a text string
    file->setFileName(fileName);
    // check if it exists
    if(!file->exists())
    {
        qDebug() << "No such file " << fileName;
        emit statusUpdate("File not found!!! " + fileName);
        return 0;
    }
    else
    {
        // try read all of the data
        file->open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream fileStream(file);
        m_fileText = fileStream.readAll();
        file->close();
        emit statusUpdate("File loaded OK: " + fileName);
        // now start handling the data in the file
        removeComments();
        if (loadUserInfo())
        {
            emit statusUpdate("Contacts Loaded OK");
            /*if (loadTaskConfig())
            {
                emit statusUpdate("Task List OK");
                return 1;
            }*/
        }
        else
        {
            emit statusUpdate("Contacts Load error");
            return 0;
        }
    }
    return 1;
}

int FileAccess::loadUserInfo()
{
    // parse the data in the file
    // Group, Name, Number



    // find first occurence of 'END'
    int stringLen = m_fileText.length();
    int contactsFound = 0;
    int firstCommaPosition = 0;
    int secondCommaPosition = 0;
    int thirdCommaPosition = 0;

    // find task list sequence
    while ((firstCommaPosition++) < stringLen)
    {
        QChar contactGroup;
        QString contactEntry;
        firstCommaPosition = m_fileText.indexOf(',',firstCommaPosition);
        secondCommaPosition = m_fileText.indexOf(',',(firstCommaPosition+1));
        thirdCommaPosition = m_fileText.indexOf(',',(secondCommaPosition+1));
        qDebug() << firstCommaPosition << secondCommaPosition << thirdCommaPosition;
        if ((firstCommaPosition > -1) & (secondCommaPosition > -1)
                & (thirdCommaPosition > -1))
        {
            // group char is before this comma
            contactGroup = m_fileText.mid((firstCommaPosition-1),1)[0];
            qDebug() << contactGroup;
            contactEntry = m_fileText.mid((firstCommaPosition +1),(thirdCommaPosition-firstCommaPosition-1));
            qDebug() << contactEntry;
            m_loadedContacts->contactNumbers.insertMulti(contactGroup,contactEntry);
            firstCommaPosition = thirdCommaPosition;
            contactsFound++;
        }
        else
        {
            emit statusUpdate("End of file");
            firstCommaPosition = stringLen;
        }
    }

    emit statusUpdate("Contacts Loaded: " + QString::number(contactsFound));




    return 1;
}

void FileAccess::removeComments(void)
{
    // remove CR/LF and comments
    m_fileText.replace('\r',' ');
    m_fileText.replace('\n',' ');
    // lets dump the comments
    int start = 0 , end =0;
    while (end != -1)
    {
        start = m_fileText.indexOf('*',start);
        end = m_fileText.indexOf('*',(start+1));
        m_fileText.remove((start),(end-start+1));
        start++;
    }
    // get rid of the extra spaces
    m_fileText = m_fileText.simplified();
}

int FileAccess::loadScenarioConfig()
{
    // load scenario specific info
    // priority etc
    QChar yesOrNo;
    QString priorityVal;
    // Get Name
    int fileNameStart = m_fileText.indexOf("SCENARIO:'");
    if (fileNameStart > -1)
    {
        // lets find the name
        fileNameStart += 10;
        int fileNameEnd = m_fileText.indexOf("'",fileNameStart);
        if ((fileNameEnd-fileNameStart) > 50)
        {
            emit statusUpdate("Scenario name too long!!!");
            m_loadedScenario->setName("Scenario name too long, should be less than 50 characters");
        }
        else
            m_loadedScenario->setName(m_fileText.mid(fileNameStart,(fileNameEnd-fileNameStart)));
    }
    else
    {
        emit statusUpdate("Scenario name missing from config file");
        m_loadedScenario->setName("Scenario name missing from config file");
    }

    // Get priority stuff
    int indexFoundKeyAt = m_fileText.indexOf("PRIORITY:");
    if (indexFoundKeyAt > -1)
    {
        // find end of number
        int startAt = indexFoundKeyAt+9;
        int endsAt = m_fileText.indexOf(" ",startAt);
        int numberLen = (endsAt-startAt);
        if (numberLen < 3)
        {
            priorityVal = m_fileText.mid((indexFoundKeyAt+9),numberLen);
            m_loadedScenario->setPriority(priorityVal.toInt());
        }
        else
        {
            emit statusUpdate("Please check the priority value in config file...");
            m_loadedScenario->setPriority(0);
        }
    }
    else
        m_loadedScenario->setPriority(0);

    indexFoundKeyAt = m_fileText.indexOf("IS KILLED:");
    if (indexFoundKeyAt > -1) yesOrNo = m_fileText[(indexFoundKeyAt+10)];
    if ((yesOrNo=='y')|(yesOrNo=='Y'))
        m_loadedScenario->setHighPriorityKill(true);
    else
        m_loadedScenario->setHighPriorityKill(false);

    indexFoundKeyAt = m_fileText.indexOf("IS STORED:");
    if (indexFoundKeyAt > -1) yesOrNo = m_fileText[(indexFoundKeyAt+10)];
    if ((yesOrNo=='y')|(yesOrNo=='Y'))
        m_loadedScenario->setHighPriorityStores(true);
    else
        m_loadedScenario->setHighPriorityStores(false);

    // read and set the latch and reset stuff
    indexFoundKeyAt = m_fileText.indexOf("IS LATCHED:");
    if (indexFoundKeyAt > -1)
        yesOrNo = m_fileText[(indexFoundKeyAt+11)];
    if ((yesOrNo=='y')|(yesOrNo=='Y'))
        m_loadedScenario->setIfLatchable(true);
    else
        m_loadedScenario->setIfLatchable(false);

    // now get the chras for the reset groups
    indexFoundKeyAt = m_fileText.indexOf("RESET BY:");
    if (indexFoundKeyAt > -1)
        m_loadedScenario->setResetBy(m_fileText[(indexFoundKeyAt+9)]);

    indexFoundKeyAt = m_fileText.indexOf("RESET GRP:");
    if (indexFoundKeyAt > -1)
        m_loadedScenario->setResetsGroup(m_fileText[(indexFoundKeyAt+10)]);


    return 1;
}


int FileAccess::loadTaskConfig()
{
    // find first occurence of 'END'
    int stringLen = m_fileText.length();
    int firstEndAt = m_fileText.indexOf("#END#");
    int currentIndex = 0;
    int tasksFound = 0;
    int start = -1 ,end = -1;
    int indexPCS,indexDLY, indexSMS, indexSLC, indexOUT,
            indexAUD, indexDMR;

    // find task list sequence
    while (currentIndex < stringLen)
    {
        int lowestVal = stringLen;
        indexPCS = m_fileText.indexOf("#PCS#",currentIndex);
        if (indexPCS > -1) lowestVal = indexPCS;
        indexSMS = m_fileText.indexOf("#SMS#",currentIndex);
        if ((indexSMS > -1) & (indexSMS < lowestVal)) lowestVal = indexSMS;
        indexDLY = m_fileText.indexOf("#DLY#",currentIndex);
        if ((indexDLY > -1) & (indexDLY < lowestVal)) lowestVal = indexDLY;
        indexSLC = m_fileText.indexOf("#SLC#",currentIndex);
        if ((indexSLC > -1) & (indexSLC < lowestVal)) lowestVal = indexSLC;
        indexOUT = m_fileText.indexOf("#OUT#",currentIndex);
        if ((indexOUT > -1) & (indexOUT < lowestVal)) lowestVal = indexOUT;
        indexAUD = m_fileText.indexOf("#AUD#",currentIndex);
        if ((indexAUD > -1) & (indexAUD < lowestVal)) lowestVal = indexAUD;
        indexDMR = m_fileText.indexOf("#DMR#",currentIndex);
        if ((indexDMR > -1) & (indexDMR < lowestVal)) lowestVal = indexDMR;

        if (lowestVal > firstEndAt) break;

        if (end > lowestVal) // this occurs if the second ' is missing
            emit statusUpdate("Please check for missing ' at end!");

        // now find the task!!! that was first!!
        if (lowestVal == indexPCS)
        {
            emit statusUpdate("Task PCS");
            // need to further break it down
            // look for group
            //qDebug() << m_fileText[lowestVal+6];
            QChar callGroup = m_fileText[lowestVal+6];
            // test if this matches a char in the phone book
            if (!pcsPhoneBook->contactNumbers.contains(callGroup))
                emit statusUpdate("Unknown group in PCS call list");

            // now find message, first ' should be at a fixed position
            if (m_fileText[lowestVal+8] == '\'')
            {
                start = lowestVal+9;
                // have a start lets look for end
                end = m_fileText.indexOf('\'',start+1);
                if(end < 0)
                {
                    emit statusUpdate("Message error");
                }
                else
                {
                    // extract message
                    QString message = m_fileText.midRef(start,(end-start)).toString();
                    qDebug() << message;
                    // now put it all back togeteher? --- has to be better as a structure!?
                    // push it onto the the task list
                    QString taskMember = "#PCS#";
                    //taskMember += "," + message;
                    // load other vals
                    TaskData taskInfo;
                    taskInfo.protocol = taskMember;
                    taskInfo.messageGroup = callGroup;
                    taskInfo.messageBody = message;
                    // push it

                    m_loadedScenario->taskVector.push_back(taskInfo);
                }
            }
            else emit statusUpdate("Please check for missing ' at start");

            tasksFound++;
        }
        else if (lowestVal == indexSMS)
        {
            emit statusUpdate("Task SMS");
            QChar callGroup = m_fileText[lowestVal+6];
            // test if this matches a char in the phone book
            if (!smsPhoneBook->contactNumbers.contains(callGroup))
                emit statusUpdate("Unknown group in PCS call list");

            // now find message, first ' should be at a fixed position
            if (m_fileText[lowestVal+8] == '\'')
            {
                start = lowestVal+9;
                // have a start lets look for end
                end = m_fileText.indexOf('\'',start+1);
                if(end < 0)
                {
                    emit statusUpdate("Message error");
                }
                else
                {
                    // extract message
                    QString message = m_fileText.midRef(start,(end-start)).toString();
                    qDebug() << message;
                    QString taskMember = "#SMS#";
                    // load other vals
                    TaskData taskInfo;
                    taskInfo.protocol = taskMember;
                    taskInfo.messageGroup = callGroup;
                    taskInfo.messageBody = message;
                    // push it

                    m_loadedScenario->taskVector.push_back(taskInfo);
                }
            }
            else emit statusUpdate("Please check for missing ' at start");
            tasksFound++;
        }
        else if (lowestVal == indexDLY)
        {
            emit statusUpdate("Task DLY");
            tasksFound++;
        }
        else if (lowestVal == indexSLC)
        {
            emit statusUpdate("Task SLC");
            tasksFound++;
        }
        else if (lowestVal == indexOUT)
        {
            emit statusUpdate("Task OUT");
            tasksFound++;
        }
        else if (lowestVal == indexAUD)
        {
            emit statusUpdate("Task AUD");
            tasksFound++;
        }
        else if (lowestVal == indexDMR)
        {
            emit statusUpdate("Task DMR");
            /*QChar callGroup = m_fileText[lowestVal+6];
            // test if this matches a char in the phone book
            if (!dmrPhoneBook->contactNumbers.contains(callGroup))
                emit statusUpdate("Unknown group in PCS call list");

            // now find message, first ' should be at a fixed position
            if (m_fileText[lowestVal+8] == '\'')
            {
                start = lowestVal+9;
                // have a start lets look for end
                end = m_fileText.indexOf('\'',start+1);
                if(end < 0)
                {
                    emit statusUpdate("Message error");
                }
                else
                {
                    // extract message
                    QString message = m_fileText.midRef(start,(end-start)).toString();
                    qDebug() << message;
                    QString taskMember = "#DMR#";
                    // load other vals
                    TaskData taskInfo;
                    taskInfo.protocol = taskMember;
                    taskInfo.messageGroup = callGroup;
                    taskInfo.messageBody = message;
                    // push it

                    m_loadedScenario->taskVector.push_back(taskInfo);
                }
            }
            else emit statusUpdate("Please check for missing ' at start");
            */
            tasksFound++;
        }
        currentIndex = lowestVal + 1;
    }

    emit statusUpdate(QString::number(tasksFound));


    return 1;
}

