#include "LogClass.h"


LogClass::LogClass()
{
	m_userLog = new QFile;
	m_engLog = new QFile;
	// copies the file into a text string
	m_userLog->setFileName(PATH_USER_LOG);
	m_engLog->setFileName(PATH_ENG_LOG);
	// check if it exists, create otherwise
	m_userLog->open(QIODevice::Append | QIODevice::Text);
	QTextStream temp_userLogStream(m_userLog);
	temp_userLogStream << "\nSystem Reboot...\n";
	m_userLog->close();
	m_engLog->open(QIODevice::Append | QIODevice::Text);
	QTextStream temp_engLogStream(m_engLog);
	QTime time = QTime::currentTime();
	QDate date = QDate::currentDate();
	temp_engLogStream << "\nSyetem Reboot at " << time.toString() 
		<< ' ' << date.toString() << " (system time)\n";
	m_engLog->close();
}


LogClass::~LogClass()
{
	delete m_userLog;
	delete m_engLog;
}

int LogClass::writeToUserLog(QString logEntry)
{
	m_userLog->open(QIODevice::Append | QIODevice::Text);
	QTextStream temp_userLogStream(m_userLog);
	QTime time = QTime::currentTime();
	QDate date = QDate::currentDate();
	temp_userLogStream << '\n' << "System Time " << time.toString()
		<< ' ' << date.toString() << ' ' << logEntry << "\n------------------------------------";
	m_userLog->close();
	return 1;
}

int LogClass::writeToEngLog(QString logEntry)
{
	m_engLog->open(QIODevice::Append | QIODevice::Text);
	QTextStream temp_engLogStream(m_engLog);
	QTime time = QTime::currentTime();
	QDate date = QDate::currentDate();
	temp_engLogStream << '\n' << "System Time " << time.toString() 
		<< ' ' << date.toString() << ' ' << logEntry << "\n------------------------------------";
	m_engLog->close();
	return 1;
}