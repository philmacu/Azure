#include "serialPort.h"
#include "phoneBookClass.h"

serialPort::serialPort()
{
	phone_book = new phoneBookClass;
	openNonCanonicalUART();
	resetModemFlags();
	// allocate resources for a timer to monitor replies
	// one shot and is only set to run when we send an AT command
	ATresponseTimer = new QTimer;
	CTSneverReleasedTimer = new QTimer;
    schedulerTimer = new QTimer;
	delayAfterSendTimer = new QTimer;
    lockoutPauseTimer = new QTimer;
	QMainWindow::connect(ATresponseTimer, SIGNAL(timeout()), 
		this, SLOT(ATcommandResponseTimeout()));
	QMainWindow::connect(CTSneverReleasedTimer, SIGNAL(timeout()),
		this, SLOT(CTSneverReleased()));
    QMainWindow::connect(schedulerTimer, SIGNAL(timeout()),
		this, SLOT(handleTheStack()));
	QMainWindow::connect(delayAfterSendTimer, SIGNAL(timeout()),
		this, SLOT(delayAfterSend()));
    QMainWindow::connect(lockoutPauseTimer, SIGNAL(timeout()),
        this, SLOT(releaseLockoutPause()));
    // reset the stacks
    smsStackIndex = -1;
	deleteSMSstackIndex = -1;
    deleteSMSoutStackIndex = -1;
	smsCommandStackIndex = -1;
    smsCommandOutStackIndex = -1;
    readSMSstackOutIndex = -1;
    readSMSinStackIndex = -1;

	// free running timer
	schedulerTimer->start(SCHEDULER_RATE);
	// single shots
	ATresponseTimer->setSingleShot(true);
	ATresponseTimer->stop();
	CTSneverReleasedTimer->setSingleShot(true);
	CTSneverReleasedTimer->stop();
	delayAfterSendTimer->setSingleShot(true);
	delayAfterSendTimer->stop();
    lockoutPauseTimer->setSingleShot(true);
    lockoutPauseTimer->stop();
	gsmFlags.smsSendFailed = 0; // no failed sms sends
	smsSendProgress = 0;
	smsSendingStackIndex = -1;
	smsToGoStackIndex = -1;
	ATcommandTimeout = false;
    routinelyDeleteAll = false;
	gsmFlags.stackIsBusy = false;
	shutdownMessage = SHUTDOWN_MESSAGE;
}

serialPort::~serialPort()
{
	closeNonCanonicalUART();
	delete phone_book;
	delete ATresponseTimer;
	delete CTSneverReleasedTimer;
	delete schedulerTimer;
	delete delayAfterSendTimer;
	delete lockoutPauseTimer;
	qDebug() << "SMS destrucor completed OK/n";
}

void serialPort::handleTheStack()
{

}

int serialPort::getAndSendSMSfromStack()
{

	
}

void serialPort::releaseLockoutPause()
{

}

int serialPort::pushSMSstack(const char *number, char *body)
{
    return -1;
}

int serialPort::popSMSstack()
{

}

int serialPort::pushReadSMSstack(char *smsMessageID)
{
    if (readSMSinStackIndex < SMS_READ_STACK_SIZE ) {
        // we have space in the stack
        readSMSinStackIndex ++;
        strcpy(messageId[readSMSinStackIndex],smsMessageID);
        return readSMSinStackIndex;
    }
    else return -1;
        /*readSMSstackIndex++;
	if (readSMSstackIndex > SMS_STACK_SIZE) readSMSstackIndex = 0;
	// place message ID onto stack
	messageId[readSMSstackIndex] = singleChar;
        return smsStackIndex;*/
}

void serialPort::ATcommandResponseTimeout(void)
{
	// we sent an AT command but never got a response in time, we reset our flags
	qDebug() << "Modem response timedout";

	// if sending a text retry
	if (gsmFlags.sendText) {
		gsmFlags.smsSendFailed++;
		clearSMSflags();
		sendText(smsNumber, smsBody);
	}
	else resetModemFlags();
	// notify the rest of the code
	ATcommandTimeout = true;
}

void serialPort::CTSneverReleased(void)
{
	// CTS didn't clear and we want to send an sms
	clearSMSflags();
}

char serialPort::readSerial()
{
	if (gsmFlags.responseExpected)
	{
		// so we are waiting dor a response
		char buffer[] = "\0", testSting[20];
		bool gotWhatWeWanted = false;
		//int readResult = read(fd, buffer, 1);
		if (int readResult = read(fd, buffer, 1) > 0)
		{
			if (((buffer[0] == '\r') | (buffer[0] == '\n'))& !readingSMS) {
				clearBuffer();
				// we got something so serial is up
				isAlive = true;
		}
		else 
		{
			//qDebug() << buffer[0];
			//qDebug() << TxResponseBuffer.index;
			TxResponseBuffer.buffer[TxResponseBuffer.index] = buffer[0];
			TxResponseBuffer.index++;
			TxResponseBuffer.buffer[TxResponseBuffer.index] = '\0'; // need this so compare works OK
			// lets do some testing
			//if (TxResponseBuffer.buffer[0] == '>')
			strcpy(testSting, ">");
			if (!strncmp(TxResponseBuffer.buffer, testSting, strlen(testSting)))
			{
				gsmFlags.responseType = RESPONSE_GTHAN;// got a GTHAN
				gotWhatWeWanted = true;
			}
			strcpy(testSting, "+CMGS");
			if (!strncmp(TxResponseBuffer.buffer, testSting, strlen(testSting)))
			{
				gsmFlags.responseType = RESPONSE_TEXT_SENT_OK; // got an ok
				gotWhatWeWanted = true;
				gsmFlags.CTS = true;
			}
			strcpy(testSting, "+CMSE"); // error on send text
			if (!strncmp(TxResponseBuffer.buffer, testSting, strlen(testSting)))
			{
				gsmFlags.responseType = RESPONSE_TEXT_SENT_FAIL; // got an ok
				gotWhatWeWanted = true;
				gsmFlags.CTS = true;
			}
			// better to use a string so we can make sure of correct length
			strcpy(testSting, "OK");
			if (!strncmp(TxResponseBuffer.buffer, testSting, strlen(testSting)))
			{
				gsmFlags.responseType = RESPONSE_OK; // got an ok
				gotWhatWeWanted = true;
				gsmFlags.CTS = true;
			}
			strcpy(testSting, "ERROR");
			if (!strncmp(TxResponseBuffer.buffer, testSting, strlen(testSting)))
			{
				gsmFlags.responseType = RESPONSE_ERR; // got an ok
				gotWhatWeWanted = true;
				gsmFlags.CTS = true;
			}
			strcpy(testSting, "+CPIN: READY");
			if (!strncmp(TxResponseBuffer.buffer, testSting, strlen(testSting)))
			{
				gsmFlags.responseType = RESPONSE_SIM_UNLOCKED_OK; // got an ok
				gotWhatWeWanted = true;
				gsmFlags.CTS = true;
			}
			strcpy(testSting, "+CME ERROR");
			if (!strncmp(TxResponseBuffer.buffer, testSting, strlen(testSting)))
			{
				gsmFlags.responseType = RESPONSE_SIM_IS_LOCKED; // got an ok
				gotWhatWeWanted = true;
				gsmFlags.CTS = true;
			}
			// we need an extra char 
			if (TxResponseBuffer.index > 9){
				strcpy(testSting, "+CREG:");
				if (!strncmp(TxResponseBuffer.buffer, testSting, strlen(testSting)))
				{
					// so its a reply to a registration request
					if (TxResponseBuffer.buffer[9] == '1') {
						gsmFlags.responseType = RESPONSE_NETWORK_REG;
						gotWhatWeWanted = true;
						gsmFlags.CTS = true;
					}
					else {
						// not registered
						gsmFlags.responseType = RESPONSE_NETWORK_ERROR;
						gotWhatWeWanted = true;
						gsmFlags.CTS = true;
					}
				}
			}
			if (TxResponseBuffer.index > 7){
				strcpy(testSting, "+CSQ:");
				if (!strncmp(TxResponseBuffer.buffer, testSting, strlen(testSting)))
				{
					char level[3];
					gsmFlags.signalStrengthLvL = 0;
					level[0] = TxResponseBuffer.buffer[6];
					level[1] = TxResponseBuffer.buffer[7];
					if ((level[1] > 47) & (level[1] < 58)) gsmFlags.signalStrengthLvL = (int)level[1] - 48;
					if ((level[0] > 47) & (level[0] < 58)) gsmFlags.signalStrengthLvL += ((int)level[0] - 48) * 10;
					gsmFlags.responseType = RESPONSE_SIG_LVL;
					gotWhatWeWanted = true;
					gsmFlags.CTS = true;
				}
			}
            strcpy(testSting, "+CMGR:");
            if (!strncmp(TxResponseBuffer.buffer, testSting, strlen(testSting)))
            {
                readingSMS = true;
                // need to count number of \r
                if (buffer[0] == '\r') {
                        numberOfCR++;
                }
                if (numberOfCR > 2)
                {
                    // lets try put the message onto the stack
                    if (smsCommandStackIndex < SMS_READ_STACK_SIZE){
                        smsCommandStackIndex++;
                        strcpy(messageStack[smsCommandStackIndex], TxResponseBuffer.buffer);
                        qDebug() << "Copied sms onto stack";
                    }
                    else qDebug() << "Command stack full";
                    numberOfCR = 0;
                    readingSMS = false;
                    gsmFlags.CTS = true;
                    gsmFlags.responseExpected = false;
                    // now push the message ID onto delete stack
                    if (deleteSMSstackIndex < SMS_READ_STACK_SIZE){
                        deleteSMSstackIndex++;
                        strcpy(deleteSMSstack[deleteSMSstackIndex],messageId[readSMSstackOutIndex]);
                        qDebug() << messageId[readSMSstackOutIndex] << "Queued for deletion";
                    }
                    else qDebug() << "Delete Stack full";
                    popReadSMSstack();
                    qDebug() << "SMS Read Removed from stack";
                    clearBuffer();
					numberOfCR = 0;
                }
                }
            }
        }
        if (gotWhatWeWanted) handleExpectedResponse();
	}
	else
	{
		// check for ring in etc, COULD ALSO BE GETTING echoes at start
		char buffer[] = "\0", testSting[20];
		if (int readResult = read(fd, buffer, 1) > 0)
		{
			if (((buffer[0] == '\r') | (buffer[0] == '\n'))&!allowCR) {
				clearRxBuffer();
			}
			else
			{
				RxEventBuffer.buffer[RxEventBuffer.index] = buffer[0];
				RxEventBuffer.index++;
				RxEventBuffer.buffer[RxEventBuffer.index] = '\0';
				strcpy(testSting, "RING");
				if (!strncmp(RxEventBuffer.buffer, testSting, strlen(testSting)))
				{
					// someone is ringing the modem
					gsmFlags.modemBeingRung = true;
				}
				if (RxEventBuffer.index > 20){
					strcpy(testSting, "+CLIP");
					if (!strncmp(RxEventBuffer.buffer, testSting, strlen(testSting)))
					{
						// grab caller ID, get rid of +CLIP and junk at end
						for (int i = 8; i < 21; i++){
							gsmFlags.callerID[i - 8] = RxEventBuffer.buffer[i];
							if ((RxEventBuffer.buffer[i] == 34) | (RxEventBuffer.buffer[i] == '\0')){
								gsmFlags.callerID[i - 8] = '\0'; // terminate it early
								i = 21;
							}
						}
						gsmFlags.callerID[13] = '\0';
						gsmFlags.capturedCallerID = true;
					}
				}
				strcpy(testSting, "+CMTI:");
				if (!strncmp(RxEventBuffer.buffer, testSting, strlen(testSting)))
				{
					allowCR = true;
					char tempString[3];
					if ((RxEventBuffer.index == 14) & (RxEventBuffer.buffer[13] == '\r')) {
						// we have a single digit
						tempString[0] = RxEventBuffer.buffer[12];
						tempString[1] = '\0';
						pushReadSMSstack(tempString);
						qDebug() << "About to read incoming SMS " << tempString;
						//clearBuffer();
					}
					if ((RxEventBuffer.index == 15) & (RxEventBuffer.buffer[14] == '\r')) {
						// sms number is > 10
						tempString[0] = RxEventBuffer.buffer[12];
						tempString[1] = RxEventBuffer.buffer[13];
						tempString[2] = '\0';
						pushReadSMSstack(tempString);
						qDebug() << "About to read incoming SMS " << tempString;
						//clearBuffer();
					}
				}
				else allowCR = false;
			}
		}
	}
	return '-';
}

int serialPort::parseSMS()
{
    // lets deconstruct sms
    qDebug() << "Message :" << messageStack[smsCommandStackIndex] ;
	writeToEngLog(messageStack[smsCommandStackIndex]);
    // parse it
	/* 
	we need to scan through the string, the first "" is REC read/unread - not interested
	The second "" is the caller ID
	The third is the DTG
	Then after <LF> is the message test <LF>
	*/
	int doubleQuotes = 0, textInBlockIndex = 0;
	char callerId[20], callerDTG[30], callerText[300];
	for (int i = 0; i < strlen(messageStack[smsCommandStackIndex]); i++)
	{	
		char stringElement = messageStack[smsCommandStackIndex][i];
		if (stringElement == 34)
		{
			doubleQuotes++;
			textInBlockIndex = 0;
		}
		switch (doubleQuotes)
		{
		case 3:
			if (stringElement != 34)
			{
				callerId[textInBlockIndex] = stringElement;
				textInBlockIndex++;
				callerId[textInBlockIndex] = '\0';
			}
			break;
		case 5:
			if (stringElement != 34)
			{
				callerDTG[textInBlockIndex] = stringElement;
				textInBlockIndex++;
				callerDTG[textInBlockIndex] = '\0';
			}
			break;
		case 6:
			// from now on we should have the text body
			if ((stringElement != 34) & (stringElement != 13) & (stringElement != 10))
			{
				callerText[textInBlockIndex] = stringElement;
				textInBlockIndex++;
				callerText[textInBlockIndex] = '\0';
			}
			break;
		default:
			break;
		}
		
	}
    // if all handled reset the stack
    if (smsCommandOutStackIndex >= smsCommandStackIndex){
        smsCommandOutStackIndex = -1;
        smsCommandStackIndex = -1;
    }
	// test for a magic text
	if (!strcmp(callerText, shutdownMessage.c_str()))
	{
		// we have received the code
		std::string requestFrom(callerId);
		writeToEngLog("Received a shutdown message from: " + requestFrom);
		requestCompleteReset(requestFrom);
	}
}

int serialPort::openNonCanonicalUART()
{
	
	int result = false;
	fd = open(MODEMDEVICE, O_RDWR | O_NONBLOCK | O_NOCTTY | O_NDELAY);
	tcgetattr(fd, &oldtio); // save current serial port settings 
	bzero(&newtio, sizeof(newtio)); // clear struct for new port settings 
	
	/*
	BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
	CS8     : 8n1 (8bit,no parity,1 stopbit)
	CLOCAL  : local connection, no modem contol - ignore Carrier detect
	CREAD   : enable receiving characters
	*/
	
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	// if we want to map CR to LF then c_iflag | ICRNL
	// otherwise leave it alone and noncanonically test for CR
	newtio.c_iflag = IGNPAR;//IGNPAR  : ignore bytes with parity errors
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;// ECHO off
	// settings only used in Non-can
	newtio.c_cc[VTIME] = 0;     // blocking time
	newtio.c_cc[VMIN] = 0;     // or block until numb of chars

	tcflush(fd, TCIFLUSH);	// flush the buffer
	tcsetattr(fd, TCSANOW, &newtio);	// get current attributes

	// output some welcome stuff
	char welcomeMsg[] = "Hello, 9600n81 UEXT2\n";

	/*int n = write(fd, welcomeMsg, strlen(welcomeMsg));
	if (n<0)
	{
		qDebug() << "we got a problem, port not open\n";
	}
	else {
		qDebug() << "Port Open: " << n << " \n";
	}*/
	if (fd <0) { perror(MODEMDEVICE); return (-1); }
	result = true;
	

	return result;
}

void serialPort::closeNonCanonicalUART()
{
	// do a flush of both RX and TX sata
	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &oldtio);
}

void serialPort::handleExpectedResponse()
{
	// this is called if we got a response in, generally will be used to 
	// send second part of command
	// disable the one shot alarm timer
	ATresponseTimer->stop();
	switch (gsmFlags.responseType)
	{
	case RESPONSE_GTHAN:
		// this means we already sent number to send text to, now send the body
		// disable response timeout
		ATresponseTimer->stop();
		sendText(smsNumber, smsBody);
		break;
	case RESPONSE_OK:
		// disable response timeout
		ATresponseTimer->stop();
		// what was the OK in response to!
        //if (gsmFlags.sendText) sendText(smsNumber, smsBody); // just in case modem sends OK instead
		if (gsmFlags.ATE0) {
			gsmFlags.ATE0 = 0;
			gsmFlags.echoOff = true;
			gsmFlags.responseExpected = false;
			qDebug() << "Echo is now OFF\n";
		}
		if (gsmFlags.ATV1){
			gsmFlags.ATV1 = 0;
			gsmFlags.verboseON = true;
			gsmFlags.responseExpected = false;
			qDebug() << "Verbose is now set ON\n";
		}
		if (gsmFlags.CMGF1){
			gsmFlags.CMGF1 = 0;
			gsmFlags.TextModeON = true;
			gsmFlags.responseExpected = false;
			qDebug() << "PDU mode disabled\n";
		}
		if (gsmFlags.ATS0){
			gsmFlags.ATS0 = 0;
			gsmFlags.autoAnswerOff = true;
			gsmFlags.responseExpected = false;
			qDebug() << "Auto Answer is now OFF\n";
		}
		if (gsmFlags.CLIP){
			gsmFlags.CLIP = 0;
			gsmFlags.callerIDon = true;
			gsmFlags.responseExpected = false;
			qDebug() << "Incoming Caller ID is ON\n";
		}
		if (gsmFlags.CNMI){
			gsmFlags.CNMI = 0;
			gsmFlags.CNMIisOn = true;
			gsmFlags.responseExpected = false;
			qDebug() << "We will be notified of incoming SMS\n";
		}
		if (gsmFlags.CRC0){
			gsmFlags.CRC0 = 0;
			gsmFlags.extendedReportsOff = true;
			gsmFlags.responseExpected = false;
			qDebug() << "Extended reports are off\n";
		}
		if (gsmFlags.CMGD){
			gsmFlags.CMGD = 0;
			gsmFlags.storedSMSdeleted = true;
			gsmFlags.responseExpected = false;
			qDebug() << "Deleted any stored SMS in modem memory\n";
		}
        if (gsmFlags.CMGDx){
            gsmFlags.CMGDx = false;
			gsmFlags.responseExpected = false;
            qDebug() << "Message Deleted";
            // decrement the stack
            //deleteSMSstackIndex--;
        }
		if (gsmFlags.hangUpRequested)
		{
			gsmFlags.hangUpRequested = false;
			gsmFlags.responseExpected = false;
			qDebug() << "We hung up\n";
			writeToEngLog("Hung up, see user log for more info\n");
		}
		
		break;
	case RESPONSE_ERR:
		// disable response timeout
		ATresponseTimer->stop();
		if (gsmFlags.sendText) sendTextError();
		break;
	case RESPONSE_TEXT_SENT_OK:
		if (gsmFlags.sendText) sendText(smsNumber, smsBody);
		break;
	case RESPONSE_TEXT_SENT_FAIL:
		ATresponseTimer->stop();
		if (gsmFlags.sendText) sendTextError();
		break;
	case RESPONSE_SIM_UNLOCKED_OK:
		if (gsmFlags.CPIN){
			gsmFlags.CPIN = 0;
			gsmFlags.SIMpinOK = true;
			gsmFlags.responseExpected = false;
			qDebug() << "SIM is unlocked\n";
		}
		break;
	case RESPONSE_SIM_IS_LOCKED:
		if (gsmFlags.CPIN){
			gsmFlags.CPIN = 0;
			gsmFlags.SIMpinOK = false;
			gsmFlags.responseExpected = false;
			qDebug() << "SIM is LOCKED\n";
		}
		break;
	case RESPONSE_NETWORK_REG:
		if (gsmFlags.CREG){
			gsmFlags.CREG = 0;
			gsmFlags.registeredOK = true;
			gsmFlags.responseExpected = false;
			qDebug() << "Registered on the network\n";
		}
		break;
	case RESPONSE_NETWORK_ERROR:
		if (gsmFlags.CREG){
			gsmFlags.CREG = 0;
			gsmFlags.registeredOK = false;
			gsmFlags.responseExpected = false;
			qDebug() << "Not registered yet\n";
		}
		break;
	case RESPONSE_SIG_LVL:
		if (gsmFlags.CSQ){
			gsmFlags.CSQ = 0;
			gsmFlags.responseExpected = false;
			qDebug() << gsmFlags.signalStrengthLvL << " LEVEL\n";
		}
	default:
		break;
	}
	// clearbuffer
	TxResponseBuffer.buffer[0] = '\0';
	gsmFlags.responseType = -1;
	//gsmFlags.responseExpected = false;
	//gsmFlags.CTS = true;
}

int serialPort::sendText(char *destNumber, char *messageToGo){
	// MULTIPART SEQUENCE	
	char ATmessageString[200];
	int result;
	smsSendProgress++;
	switch (gsmFlags.sendText)
	{
	case STAGE_1:
		// haven't sent anything yet
		// store number and message
		//textTo = destNumber;
		//textMessage = messageToGo;
		// try and copy pointer into string
		strcpy(smsNumber, destNumber);
		strcpy(smsBody, messageToGo);
		strcpy(ATmessageString, "AT+CMGS=\"");
		strcat(ATmessageString, smsNumber);
		strcat(ATmessageString, "\"\r"); // "<CR>
		// send
		result = write(fd, ATmessageString, strlen(ATmessageString));
		if (result > 0)
		{
			//resetTxResponseBuffer();
			clearBuffer();
			gsmFlags.sendText++;
			gsmFlags.responseExpected = true;
			gsmFlags.CTS = false;
			qDebug() << "Sending Started\n";
			std::string numberLog = "Sent SMS to: ";
			std::string numberIs(smsNumber);
			std::string bodyIs(smsBody);
			numberLog += numberIs + " " + bodyIs;
			dataForLog(numberLog);
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
		}
		else gsmFlags.sendText = 0;
		break;
	case STAGE_2:
		// ok so we must of received the correct >
		gsmFlags.CTS = false;
		strcpy(ATmessageString, "\"");	// "
		strcat(ATmessageString, smsBody);
		strcat(ATmessageString, "\"\x1a"); // "<sub>
		result = write(fd, ATmessageString, strlen(ATmessageString));
		if (result > 0)
		{
			clearBuffer();
			gsmFlags.sendText++;
			gsmFlags.responseExpected = true;
			qDebug() << "Sending Body " << smsBody;
			//tempLog(smsBody);
			//std::string logString(smsBody);
			
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
		}
		else gsmFlags.sendText = STAGE_1;
		break;
	case STAGE_3:
		// what we do next depends on if this is a single or multiple sends
        // pop the element from the stack,
		if (singleText){
			popSMSstack();
		}
		else if (groupText){
			// so how do we handle group!!!
			// need to get next number from phone book
			getNextFromPBook = true; // this will trigger next send
		}
		//dataForLog("Text Sent");
	default:
		gsmFlags.sendText = STAGE_1;
		smsSendProgress = 0;
		break;
	}
	return gsmFlags.sendText;
}

void serialPort::delayAfterSend(void)
{
	delayAfterSendTimer->stop();
	gsmFlags.sendingAtext = false;
}

void serialPort::sendTextError(void) //error could of occurred at any time how do we handle it
{
	clearBuffer();
	gsmFlags.sendText = 0;
	sendText(smsNumber, smsBody);
}

void serialPort::clearBuffer(void)
{
	// clear the buffer
	TxResponseBuffer.buffer[0] = '\0';
	TxResponseBuffer.index = 0;
	TxResponseBuffer.MaxLength = TX_BUFFER_MAX;
	allowCR = false;
}

void serialPort::clearRxBuffer(void)
{
	// clear the buffer
	RxEventBuffer.buffer[0] = '\0';
	RxEventBuffer.index = 0;
	RxEventBuffer.MaxLength = TX_BUFFER_MAX;
}

int serialPort::hangUp()
{
	if (gsmFlags.CTS)
	{
		write(fd, "ATH\r", 4);
		gsmFlags.capturedCallerID = false;
		gsmFlags.callerID[0] = '\0';
		// clear the rx buffer
		clearRxBuffer();
		gsmFlags.responseExpected = true;
		gsmFlags.hangUpRequested = true;
		return 1;
	}
	else return 0;
}

int serialPort::ATE0(void)
{
	int result = 0;
	// only send if ATE0 state is unknown
	if (!gsmFlags.echoOff)
	{
		if (gsmFlags.CTS)
		{
			char ATmessageString[] = "ATE0\r";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.ATE0 = 1;
			// lets test if serial port is active, will be set when response gets back to us
			isAlive = false;
		}
	}
	return result;
}

int serialPort::ATV1(void)
{
	int result = 0;
	// only send if ATE0 state is unknown
	if (!gsmFlags.ATV1)
	{
		if (gsmFlags.CTS)
		{
			char ATmessageString[] = "ATV1\r";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.ATV1 = 1;
		}
	}
	return result;
}

int serialPort::CMGF1(void)
{
	int result = 0;
	if (!gsmFlags.CMGF1)
	{
		if (gsmFlags.CTS)
		{
			char ATmessageString[] = "AT+CMGF=1\r";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.CMGF1 = 1;
		}
	}
	return result;
}

int serialPort::CSQ(void)
{
	int result = 0;
	if (!gsmFlags.CSQ)
	{
		if (gsmFlags.CTS)
		{
			char ATmessageString[] = "AT+CSQ\r\n";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.CSQ = 1;
		}
	}
	return result;
}

int serialPort::ATS0(void)
{
	int result = 0;
	if (!gsmFlags.ATS0)
	{
		if (gsmFlags.CTS)
		{
			char ATmessageString[] = "ATS0=0\r\n";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.ATS0 = 1;
		}
	}
	return result;
}

int serialPort::CPIN(void)
{
	int result = 0;
	if (!gsmFlags.CPIN)
	{
		if (gsmFlags.CTS)
		{
			char ATmessageString[] = "AT+CPIN?\r\n";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.CPIN = 1;
		}
	}
	return result;
}

int serialPort::CREG(void)
{
	int result = 0;
	if (!gsmFlags.CREG)
	{
		if (gsmFlags.CTS)
		{
			char ATmessageString[] = "AT+CREG?\r\n";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.CREG = 1;
		}
	}
	return result;
}

int serialPort::CLIP(void)
{
	int result = 0;
	if (!gsmFlags.CLIP)
	{
		if (gsmFlags.CTS)
		{
			char ATmessageString[] = "AT+CLIP=1\r\n";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.CLIP = 1;
		}
	}
	return result;
}

int serialPort::CNMI(void)
{
	int result = 0;
	if (!gsmFlags.CNMI)
	{
		if (gsmFlags.CTS)
		{
			char ATmessageString[] = "AT+CNMI=2,1\r\n";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.CNMI = 1;
		}
	}
	return result;
}

int serialPort::CRC0(void)
{
	int result = 0;
	if (!gsmFlags.CRC0)
	{
		if (gsmFlags.CTS)
		{
			char ATmessageString[] = "AT+CRC=0\r\n";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.CRC0 = 1;
		}
	}
	return result;
}

int serialPort::CMGD_all(void)
{
	int result = 0;
	if (!gsmFlags.CMGD)
	{
		if (gsmFlags.CTS)
		{
			if (smsRxOverrun) smsRxOverrun = false;
			char ATmessageString[] = "AT+CMGD=1,4\r\n";
			gsmFlags.CTS = false;
			result = write(fd, ATmessageString, strlen(ATmessageString));
			// start response timer
			ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
			clearBuffer();
			gsmFlags.responseExpected = true;
			gsmFlags.CMGD = 1;
			// if it was called by a routinely delete all, make sure we don't repeat
			if (routinelyDeleteAll) routinelyDeleteAll = false;
		}
	}
	return result;
}

void serialPort::resetModemFlags(void){
	// this clears all flags
	gsmFlags.CTS = true;
	gsmFlags.readMsg = false;
	gsmFlags.responseExpected = false;
	gsmFlags.echoOff = false;
	gsmFlags.verboseON = false;
	gsmFlags.TextModeON = false;
	gsmFlags.autoAnswerOff = false;
	gsmFlags.SIMpinOK = false;
	gsmFlags.registeredOK = false;
	gsmFlags.modemBeingRung = false;
	gsmFlags.callerIDon = false;
	gsmFlags.extendedReportsOff = false;
	gsmFlags.CNMIisOn = false;
	gsmFlags.capturedCallerID = false;
	gsmFlags.storedSMSdeleted = false;
    gsmFlags.CMGDx = false;
	strcpy(gsmFlags.callerID, "NO CALLER");
	gsmFlags.signalStrengthLvL = 0;
	gsmFlags.sendText = 0;
	gsmFlags.ATE0 = 0;
	gsmFlags.ATV1 = 0;
	gsmFlags.CMGF1 = 0;
	gsmFlags.ATS0 = 0;
	gsmFlags.CPIN = 0;
	gsmFlags.CSQ = 0;
	gsmFlags.CREG = 0;
	gsmFlags.CLIP = 0;
	gsmFlags.CNMI = 0;
	gsmFlags.CRC0 = 0;
	gsmFlags.CMGD = 0;
	gsmFlags.responseType = 0;
	gsmFlags.smsSendFailed = false;
	gsmFlags.hangUpRequested = false;
	clearBuffer();
	gotAtext = false;
	gsmFlags.deferReadSMS = false;
	readingSMS = false;
	gsmFlags.stackIsBusy = false;
	gsmFlags.allowDeleteAll = false;
	gsmFlags.sendingAtext = false;
	allowCR = false;
	groupText = false;
	singleText = false;
	getNextFromPBook = false;
    lockoutDueToPause = false;
	groupScanPointer = 0; // this scans across group string sending to particular groups
	groupIndexPosition = 0;
	isAlive = false;
}

void serialPort::clearSMSflags(void)
{
	// just clear some flags so we can resend text
	gsmFlags.CTS = true;
	gsmFlags.readMsg = false;
	gsmFlags.responseExpected = false;
	gsmFlags.sendText = 0;
	gsmFlags.responseType = 0;
	gsmFlags.smsSendFailed = false;
	gsmFlags.sendingAtext = false;
	smsSendProgress = 0;
	clearBuffer();
}

int serialPort::errorCheckSMS(char *number, char *body)
{
	int result=0;
	if (!strlen(number)) result |= NO_NUMBER;
	if (!strlen(body)) result |= NO_BODY;
	if (strlen(number) < MIN_NUMBER_DIGITS) result |= TOO_FEW_DIGITS;
	// now make sure they are numbers!
	bool validNumber = true; // will be cleared if all are OK
	if ((strlen(number) == MIN_NUMBER_DIGITS)&(number[0] = '+')) {
		// we got 0 at start delet and add +353
		for (int i = 1; i < MIN_NUMBER_DIGITS; i++){
			if ((number[i] < 48) | (number[i]>57)) validNumber = false;
		}
	}else validNumber = false;
	if (!validNumber) result |= ILLEGAL_NUMBER;
	return result;
}

int serialPort::readText()
{
	int result = 0;
    // read message number from top of stack
	if (gsmFlags.CTS){
            // first part of sms
            readingSMS = false; // will go true when we start getting the text from the modem
            gsmFlags.CTS = false;
            gsmFlags.deferReadSMS = false;
            char ATmessageString[] = "AT+CMGR=";
			char tempId[3];
            strcat(ATmessageString,messageId[readSMSstackOutIndex]);
            strcat(ATmessageString,"\r\n");
            result = write(fd, ATmessageString, strlen(ATmessageString));
            ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
            gsmFlags.responseExpected = true;
            numberOfCR = 0;
            gsmFlags.allowDeleteAll = true; // next time the stack is empty send delete all command
	}
}

int serialPort::deleteSMS()
{
    int result = 0;
    //send command to delete message from top of stack
    if (gsmFlags.CTS){
        gsmFlags.CTS = false;
        char ATmessageString[] = "AT+CMGD=";
        strcat(ATmessageString,deleteSMSstack[deleteSMSoutStackIndex]);
        strcat(ATmessageString, ",0\r\n");
        result = write(fd, ATmessageString, strlen(ATmessageString));
        ATresponseTimer->start(AT_RESPONSE_TIMEOUT);
        gsmFlags.responseExpected = true;
        gsmFlags.CMGDx = true;
        // now adjust the stack
        if (deleteSMSoutStackIndex>=deleteSMSstackIndex){
            deleteSMSoutStackIndex = -1;
            deleteSMSstackIndex = -1;
            //delete Memory
            routinelyDeleteAll = true;
        }
    }
}

int serialPort::popReadSMSstack()
{

    if (readSMSstackOutIndex>=readSMSinStackIndex){
        // everything has been read
        readSMSinStackIndex = -1;
        readSMSstackOutIndex = -1;
    }
    return readSMSstackOutIndex;
}

int serialPort::deleteAll()
{
	//wipe the stack, kill all pending messages
	for (int i = 0; i < (smsSendingStackIndex+1); i++)
	{
		smsStackElement[i].number[0] = '\0';
		smsStackElement[i].number[1] = '\0';
		smsStackElement[i].body[0] = '\0';
		smsStackElement[i].body[1] = '\0';
	}
	// delete messages left in group
	
	groupIndexPosition = 0;
	groupScanPointer = 0;
	groupText = false;
	lockoutDueToPause = false; 
	delayAfterSendTimer->stop();
	getNextFromPBook = false;

	// just clear some flags so we can resend text
	gsmFlags.CTS = true;
	gsmFlags.readMsg = false;
	gsmFlags.responseExpected = false;
	gsmFlags.sendText = 0;
	gsmFlags.responseType = 0;
	gsmFlags.smsSendFailed = false;
	gsmFlags.sendingAtext = false;
	smsSendProgress = 0;
	clearBuffer();
	popSMSstack();
	gsmFlags.CTS = true;
	gsmFlags.responseExpected = false;
	smsToGoStackIndex = -1;
	smsSendingStackIndex = -1;
	return 0;
}

void serialPort::writeToEngLog(std::string logThisPlease)
{	// lets try open using traditional approach
	logFile = fopen(ENG_LOG, "a+");
	fprintf(logFile, "----------------------------------------------------------\n");
	// now lets get date
	struct tm * timeinfo;
	time_t rawtime;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	fprintf(logFile, "Current local time and date: %s\n", asctime(timeinfo));
	fprintf(logFile, "%s\n", logThisPlease.c_str());
	fclose(logFile);
}
