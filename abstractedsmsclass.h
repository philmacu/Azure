#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <QMainWindow>
#include <QDebug>
#include <QTimer>

#include <iostream>
#include <string.h>
#include <string>

#include <stdio.h>
// headers needed for serial
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

using namespace std;

// serial defines
#define BAUDRATE B9600            
#define MODEMDEVICE "/dev/ttyS2"
#define _POSIX_SOURCE 1 // POSIX compliant source
#define RESET_MODEM 1 // reset flags and AT commands
#define AT_COMMANDS_ONLY 2 // just clear AT commands etc
#define FLAGS_ONLY 3 // just reset flags
#define TX_BUFFER_MAX 500
#define RESPONSE_OK 1
#define RESPONSE_ERR 2
#define RESPONSE_GTHAN 3
#define RESPONSE_TEXT_SENT_OK 4
#define RESPONSE_TEXT_SENT_FAIL 5
#define RESPONSE_SIM_UNLOCKED_OK 6
#define RESPONSE_SIM_IS_LOCKED 7
#define RESPONSE_NETWORK_REG 8
#define RESPONSE_NETWORK_ERROR 9
#define RESPONSE_SIG_LVL 10
#define SMS_STACK_SIZE 10
#define SMS_READ_STACK_SIZE 15

// names for AT commands
#define ECHO_OFF 0
#define TEXT_MODE_ON 1
#define VERBOSE_ON 2
#define DISABLE_AUTO_ANS 3
#define TEST_SIM_PIN 4
#define REGISTERED_ON_NET 5
#define SIGNAL_STRENGTH 6
#define CALLER_ID 7
#define ENABLE_SMS_NOTIFICATION 8
#define EXTENDED_INFO_OFF 9
#define CLEAR_SMS_IN 10 // this is only sent on a reboot
#define REPEAT_AT_CYCLE 100 // when this value is reached the commands reissue
#define MODEM_REBOOTS_AFTER_MINS 2 // if no comms reboot after this time

#define STAGE_1 0
#define STAGE_2 1
#define STAGE_3 2
// sms error check errors
#define MIN_NUMBER_DIGITS 13 // eg + 353 85 1234 567 
#define NO_NUMBER 1
#define NO_BODY 2
#define TOO_FEW_DIGITS 4
#define ILLEGAL_NUMBER 8

#define AT_RESPONSE_TIMEOUT 20000 // mseconds
#define CTS_RELEASE_TIME 30000 // will arm if cts gets 'stuck'
#define SCHEDULER_RATE 100  // rate at which the stack is depleted
#define MIN_TIME_BETWEEN_SMS 2000 // time between consecutive texts
#define DELAY_AFTER_SEND 1000

#define PAUSE_SYMBOL '#'    // causes a standard delay in sending
#define PAUSE_DURATION 20000   // number of miliseconds to pause

#define ENG_LOG	"/home/sts/embedded/logs/smsEngLog.txt"
#define SHUTDOWN_MESSAGE "killall 1234567"

class phoneBookClass;

struct gsmFlagStructure{
	int sendText; // indicates which phase/stage of the AT command we are at
	int ATE0;	// set to !0 if we just sent AT command, used when we get a response, and to set the stage
	int ATV1;	// set to !0 if we just sent AT command, used when we get a response
	int CMGF1;	// PDU mode off
	int ATS0;	// disables auto answer
	int CPIN;	// test SIM is ready
	int CREG;	// registered OK?
	int CSQ;	// signal strength
	int CLIP;	// incoming caller ID
	int CNMI;	// enable receipt of SMS
	int CRC0;	// disable extended reports
	int CMGD;	// delete all stored SMS

	int readMsg; // not implemented yet -- will contain stages of reading a message
	int responseType; // used to indicate what we got back from modem, eg OK, Error etc
	char callerID[25];
	bool responseExpected; // used if we have multiple statements
	bool CTS; // clear to send --- or not!
	bool echoOff; // the echo is off
	bool verboseON;
	bool autoAnswerOff;
	bool SIMpinOK;
	bool TextModeON; // PDU has been disabled
	bool registeredOK;
	bool modemBeingRung;
	bool callerIDon;
	bool capturedCallerID;
	bool CNMIisOn;
	bool extendedReportsOff;
	bool storedSMSdeleted;
	bool deferReadSMS;
	bool sendingAtext;	// will use this to block a text
    bool CMGDx;      // delet msg x
	bool stackIsBusy;
	bool allowDeleteAll; // controls if we can send a delete all command
	bool hangUpRequested;
	int signalStrengthLvL;
	int smsSendFailed; // number of times message send has failed
};

struct serialString{
	int MaxLength;
	char buffer[TX_BUFFER_MAX];
	int index;
};

struct powerCharsIn{
	bool gotGTHAN;
	int gotCRLF; // 0, 1 =<CR>, 2=<CR><LF>
};

struct smsStackitem{
    char number[30];
    char body[300];
};

struct stackVars{
    int inPointer;
    int outPointer;
    bool stackInUse;
};

class AbstractedSmsClass : public QMainWindow
{
	Q_OBJECT
public:
	AbstractedSmsClass();
	~AbstractedSmsClass();
	char smsNumber[20];
	char smsBody[250];

	int ATE0(void);
	int ATV1(void);
	int CMGF1(void);
	int ATS0(void);
	int CPIN(void);
	int CREG(void);
	int CSQ(void);
	int CLIP(void);
	int CNMI(void);
	int CRC0(void);
	int CMGD_all(void);
	int errorCheckSMS(char *number, char *body);
	int smsSendProgress;
	int pushSMSstack(const char *number, char *body);
	int hangUp(void);
	bool ATcommandTimeout;
	bool gotAtext;
	int deleteAll(void); // this is used to completely erase the queue
	struct gsmFlagStructure gsmFlags;
	bool isAlive;
	char smsNumberBody[300]; // [0] contains number and DTG [1] contains body
	phoneBookClass *phone_book;	// an instance of our phonebook
	void writeToEngLog(std::string logThisPlease);
	int sendText(char*, char*);
	// some getters and setters
	int getSignalLevel(void);
	void killText(void);
	bool isTextCycleFinished(void);
	bool getCTSstate(void);
private:
	bool m_textCycleFinished;
    int openNonCanonicalUART(void);
    void closeNonCanonicalUART(void);
    void resetModemFlags(void);
    int parseSMS(void);
	std::string tempLog;
    int pushReadSMSstack(char *messageID);
    int pushSMScommandStack(char *message);
    int textCommand;	// what the user wanted us to do
	int fd;
	FILE *logFile;	// eng log file descriptor
	struct termios oldtio, newtio;
	struct serialString TxResponseBuffer;
	struct serialString RxEventBuffer;
    struct powerCharsIn rxedChars;
    struct smsStackitem smsStackElement[SMS_STACK_SIZE+1];
    
    void handleExpectedResponse(void);
    void sendTextError(void);
    void clearBuffer(void);
    void clearRxBuffer(void);
    void clearSMSflags(void);
    int popSMSstack(void);
    int popReadSMSstack(void);
    int popSMScommandStack(void);
    int readText(void);
    int deleteSMS(void);
    char messageStack[SMS_READ_STACK_SIZE+1][300];
    char messageId[SMS_READ_STACK_SIZE+1][3];
    char deleteSMSstack[SMS_READ_STACK_SIZE+1][3];
    bool smsRxOverrun;
    QTimer *schedulerTimer;     // will handle events in the stack
    QTimer *ATresponseTimer;
	QTimer *SmsCTSwd;
    char smsSoredInMemory;
    bool  readingSMS;
    bool routinelyDeleteAll; // delete memory
    int numberOfCR;
    int smsStackIndex;
    int readSMSstackOutIndex; // last postion in Q
    int readSMSinStackIndex; // new sms MSGID into Q
    int smsCommandStackIndex; // last postion
    int smsCommandOutStackIndex;
    int deleteSMSstackIndex;    // the total message Q
    int deleteSMSoutStackIndex; // the message being deleted
    int smsSendingStackIndex;
    int smsToGoStackIndex;
	bool allowCR; // allows CR's we need this when we are getting Msg Id
    struct stackVars smsDeleteStack;
    struct stackVars smsInStack;    // to be implemented!!!
    struct stackVars smsReadStack;
    struct stackVars smsCommandStack;
	// these variables handle if its group or single texts
	bool groupText;
	bool singleText;
	bool getNextFromPBook;
	int getAndSendSMSfromStack(void);
    bool lockoutDueToPause;
	int groupScanPointer; // this scans across group string sending to particular groups
	int groupIndexPosition;
	std::string shutdownMessage;
	int ATcommand; // which AT command to send
	bool skipDeleteSMSmemory;
	int signalLevel;
public slots:
	void ATcommandResponseTimeout(void);
	void readSerial(void);
	void sendNextATcommand(void);


private slots:
    void handleTheStack(void);
	void forcereleaseCTS(void);

signals:
	int dataForLog(std::string logThis);
	void requestCompleteReset(std::string identifier);
	void signalLevelIs(int i);
	void parsedIncomingSMS(QString DTG, QString callerId, QString body);
};

#endif 

