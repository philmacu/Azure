#pragma once

#include <QMainWindow>
#include <QDebug>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QDate>

#define PATH_USER_LOG "/home/sts/logs/user_log.txt"
#define PATH_ENG_LOG "/home/sts/logs/eng_log.txt"

class LogClass : public QMainWindow
{

	Q_OBJECT

public:
	LogClass();
	~LogClass();

private:
	QFile *m_userLog;
	QFile *m_engLog;

public slots:
	int writeToUserLog(QString logEntry);
	int writeToEngLog(QString logEntry);
};

