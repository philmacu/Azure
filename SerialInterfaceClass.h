#pragma once

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <fcntl.h>
#include <termios.h>

#include <QMainWindow>
#include <QDebug>
#include <QString>
#include <QTimer>

#define MODEMDEVICE "/dev/ttyS3"
#define BAUDRATE B9600            
#define _POSIX_SOURCE 1 // POSIX compliant source
#define ACK_CHAR 0x06 // interface ok
#define NAK_CHAR 0x15 // interface problem
#define TX_FAIL 0x17 // tx fail  
#define TX_OK 0x04 // tx went ok
#define END_CHAR 0x0a	
#define START_CHAR_FLAG	0	// START char flag bit position
#define END_CHAR_FLAG	1	// END char flag bit position
#define SERIAL_IN_BUFFER 255
#define FAST_READ_RATE 10 // can read the serial buffer at 2 rates
#define SLOW_READ_RATE 100
#define INITALISING_RATE 100
#define DEFAULT_ZONE "HYT zon 1\n"
#define DEFAULT_CHN "HYT chn 1\n"
#define RETRANSMIT_ATTEMPTS 3

struct serialInterfaceFlags
{
	bool sentPRV;
	bool sentGRP;
	bool sentZON;
	bool sentCHN;
	bool sentTXnotificationON;
	bool gotACK;
	bool gotNAK;
	bool gotTXOK;
	bool gotTXfail;
	bool gotResponse;
	bool TXonConfirmed;
	bool fatalTXproblem;
};

class SerialInterfaceClass : public QMainWindow
{
	Q_OBJECT

public:
	SerialInterfaceClass();
	~SerialInterfaceClass();
	int sendHYTmessage(QString radioGrpCode, QString body);
	int sendPCSmessage(QString code, QString body);
	int sendSLCmessage(QString code, QString body);
	bool isTextCycleFinished(void);
	void killText(void);
	serialInterfaceFlags hytFlags;
private:
	struct termios oldtio, newtio;
	int fd;
	bool m_textSentOK;
	bool m_killNow;
	void closeNonCanonicalUART(void);
	int openNonCanonicalUART(void);
	int sendSerial(QString s);
	int serialInindex;
	char serialInBuffer[SERIAL_IN_BUFFER];
	QTimer *readSerial;
	void parseSerial(void);
	void clearBuffer(void);
	void initalise(void);
	void clearStateFlags(void); // general clear before send
	//QTimer *ackFailTimer;
	//QTimer *privateDeliveryFailTimer;
	QTimer *initalisationTimer;
	int sendPrivateMessage(QString s);
	int sendGroupMessage(QString s);
	bool m_CTS;
	int m_initalisationLevel;
	bool m_systemInitalised;
	bool m_initalisingRadio;
	QString lastTransmission;
	int numberOfRetries;
private slots:
	//void ackFail(void);
	//void privateDeliveryFail(void);
	void getSerial(void);
	void initalisationStages(void);
signals:
	void pleaseLogThis(QString);
	void updateRadioFlags(void);
};

