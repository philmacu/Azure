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
#include <QTimer>
#include <QTimerEvent>
#include <QString>

using namespace std;
// serial defines
#define BAUDRATE B9600            
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 // POSIX compliant source
#define START_CHAR '>'
#define END_CHAR 0x0d	
#define START_CHAR_FLAG	0	// START char flag bit position
#define END_CHAR_FLAG	1	// END char flag bit position
#define SPECIAL_CHAR	0xfe	// notifier char replace with ' '
#define SEND_NAK_TIMES 3	// in event of an unknown ask for a resend, up to a max amount
#define SERIAL_IN_BUFFER 255

// timer defines
#define HEARTBEAT_RATE 5000 // mSeconds
#define READ_INPUT_BUFFER_RATE 15
#define SERIAL_ERROR_TIEMOUT_VALUE 10000

// serial message types
#define NAK 0
#define ACK 1
#define HEARTBEAT 2
#define GET_PANEL_TIME 3	// request that the panel sends us time
#define EVENT_MSG 'E'	// event message
#define STATUS_MSG 'S'	// status message
#define INFO_MSG 'F'	// information message

// alarm panel defines
#define MSG_TYPE_OFFSET 2 // what type of message is it?
#define PANEL_OFFSET 3	// 2 CHARS
#define EVENT_OFFSET 5	// 3 CHARS
#define EVENT_TIME_OFFSET 9 // hhmmss 6 bytes
#define LOOP_OFFSET	15	// 2 chars
#define ZONE_OFFSET 17	// 5 chars
#define SENSOR_OFFSET 22 // S(ensor) M(odule)
#define SENS_ADDR_OFFSET 23 // 2 chars
#define ANALOGUE_VAL_OFFSET 30 // 3 chars
#define OFFSET_END 33
#define SYSTEM_DATE_OFFSET 14 
#define SYSTEM_DATE_END 33
#define TEXT_START 36
#define EVENT_BEFORE_RESET 0 // number of events that can be processed

#define CUSTOM_LABEL_LEN 15

// files
#define PANEL_EVENT_LOG "/home/sts/embedded/logs/panelLog.txt"
#define PANEL_PARAMETERS "/home/sts/embedded/files/notifier.txt"
#define DEVICE_NOT_FITTED "NONE"

struct panelEventStruct{
	char panel[3];
	char event[11];
	char time[7];
	char date[11];
	char loop[3];
	char zone[6];
	char sensor[2];
	char sensorAddr[3];
	char analogVal[4];
	char text[80];//65 is protocol max
	char custom[CUSTOM_LABEL_LEN];
};

struct buildSMSflags{
	bool ID;
	bool loop;
	bool zone;
	bool panelTime;
	bool PanelDate;
	bool panelText;
	bool sensor;
	bool sensorAddr;
	bool sensorVal;
	bool customText;
};


class notifierPanel : public QMainWindow
{
	Q_OBJECT

public:
	notifierPanel();
	~notifierPanel();
	char SMStext[150];
	bool isFitted;
	int deleteAll(void);
	bool isAlive;
	bool inAlarm;
private:
	panelEventStruct panelEvent;
	int openNonCanonicalUART(void);
	void closeNonCanonicalUART(void);
	struct termios oldtio, newtio;
	int fd;
	bool serialTXtokenFree;
	int sendSerialMessage(int messageCode);
	QTimer *heartbeatTimer;
	QTimer *serialInTimer;
	QTimer *serialErrorTimer;
	int serialDataFlags;
	int serialInindex;
	char serialInBuffer[SERIAL_IN_BUFFER];
	void parseReceivedData(void);
	bool systemTimeUpdate;
	int loadSettings(void);
	buildSMSflags includeInSMS;
	int eventCount;
private slots:
	void sendHeartbeat(void);
	void testForSerialIn(void);
	void timerSerialTimeOut(void);

signals:
	void callFirePanelEvent(QString,int); // text and number of alarms
	void callResetPanelEvent(QString); // DTGof reset?
	void callFaultPanelEvent(QString);
	void callSilencePanelEvent(QString);
};
