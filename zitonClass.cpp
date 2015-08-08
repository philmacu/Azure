#include "zitonClass.h"


zitonClass::zitonClass()
{
	// open the port
	openNonCanonicalUART();
	// load settings
	loadSettings();
	// lets initialise the slots
	heartbeatTimer = new QTimer;
	serialInTimer = new QTimer;
	serialErrorTimer = new QTimer;
	serialInTimeout = new QTimer;
	secondBlkTimeout = new QTimer;
	linkFail = new QTimer;
	//QMainWindow::connect(heartbeatTimer, SIGNAL(timeout()),
	//	this, SLOT(sendHeartbeat()));
	QMainWindow::connect(serialInTimer, SIGNAL(timeout()),
		this, SLOT(testForSerialIn()));
	QMainWindow::connect(serialErrorTimer, SIGNAL(timeout()),
		this, SLOT(timerSerialTimeOut()));
	QMainWindow::connect(serialInTimeout, SIGNAL(timeout()),
		this, SLOT(inputTimedOut()));
	QMainWindow::connect(secondBlkTimeout, SIGNAL(timeout()),
		this, SLOT(secondBlockMissing(void)));
	QMainWindow::connect(linkFail, SIGNAL(timeout()),
		this, SLOT(panelLinkFail(void)));

	heartbeatTimer->start(HEARTBEAT_RATE);
	serialInTimer->start(READ_INPUT_BUFFER_RATE);
	serialErrorTimer->setSingleShot(true);
	serialErrorTimer->stop();
	serialInTimeout->setSingleShot(true);
	serialInTimeout->stop();
	secondBlkTimeout->setSingleShot(true);
	secondBlkTimeout->stop();
	linkFail->setSingleShot(true);
	linkFail->start(LINK_FAIL);
	systemTimeUpdate = false;
	serialTXtokenFree = true;
	eventCount = 0;
	serialInindex = 0;
	isAlive = false;
	inAlarm = false;
}

int zitonClass::loadSettings()
{
	// these settings dictate what is sent in message
	//std::ifstream ifs_load;
	//std::string isInstalled, parameter, value;
	//ifs_load.open(PANEL_PARAMETERS, std::ifstream::in);
	//getline(ifs_load, isInstalled);
	isFitted = true;
	bool readError;
	includeInSMS.customText = true;
	panelEvent.custom="MacU";
	panelEvent.blocksFilled = 0;
	includeInSMS.ID = true;
	includeInSMS.loop = true;
	includeInSMS.PanelDate = true;
	includeInSMS.panelText = true;
	includeInSMS.panelTime = true;
	includeInSMS.sensor = true;
	includeInSMS.sensorAddr = true;
	includeInSMS.sensorVal = true;
	includeInSMS.zone = true;
}

zitonClass::~zitonClass()
{
	// close the port
	closeNonCanonicalUART();
	delete heartbeatTimer;
	delete serialInTimer;
	delete serialErrorTimer;
	qDebug() << "Panel destrucor completed OK/n";
}

void zitonClass::testForSerialIn()
{
	// function will modify the flags 
	int res;
	char buf[2];
	// clear the buffer
	buf[0] = '\0';
	m_readSerial = true;
	int timeout = 0;
	// start the wd if not running
	if (!serialInTimeout->isActive())
		serialInTimeout->start(SERIAL_IN_ERROR);
	while (m_readSerial)
	{ 
		res = read(fd, buf, 1);
		if (res > 0)
		{
			// have serial ---
			linkFail->start(LINK_FAIL); // reset the clock, connection in mainW
			switch (buf[0])
			{
			case '\0':
				// someone sent a null!
				break;
			case START_CHAR:
				// new line, reset
				serialInindex = 0;
				serialInBuffer[serialInindex] = buf[0];
				serialInindex++;
				serialDataFlags |= (1 << START_CHAR_FLAG);	//Start = true;
				serialDataFlags &= ~(1 << END_CHAR_FLAG); 	//Stop = false;

				// stop the free running serial timer and go into blocking
				serialInTimer->stop();
				break;
			case SPECIAL_CHAR:
				// notifier non printing char
				serialInBuffer[serialInindex] = ' ';
				break;
			case END_CHAR:
				m_readSerial = false; //remove the block
				// check to see if we had a start
				if ((serialDataFlags & (1 << START_CHAR_FLAG)) == true)
				{
					serialDataFlags |= (1 << END_CHAR_FLAG); //Stop = true;
					serialInBuffer[serialInindex] = '\0';
					// reset read rate
					serialInTimer->stop();
					serialInTimer->start(READ_INPUT_BUFFER_RATE);
					// remove WD
					serialInTimeout->stop();
					// we got end of a string so we can clear the TX lockout
					serialTXtokenFree = true;
					// reset the serial TX flag timer
					serialErrorTimer->stop();
					// call the parser
					parseReceivedData();
				}
				break;
			default:
				serialInBuffer[serialInindex] = buf[0];
				serialInindex++;
				// include an overrun
				if (serialInindex >> 400)
					emit timerSerialTimeOut();
				break;
			}
		}
		else // if not stared rxing then ignore
			if (!(serialDataFlags & (1 << START_CHAR_FLAG)))
			{
				m_readSerial = false;
				serialInTimeout->stop();
			}
		timeout++; 
		if (timeout > 1000000)
		{
			m_readSerial = false;
			emit timerSerialTimeOut();
			qDebug() << "Missed end of serial input";
			timeout = 0;
		}
	}
}

void zitonClass::sendHeartbeat()
{
	// called by the QTimer, if we have received an updated time then 
	// we only send HEARTBEAT's if not we send a Time update
	// request every second message
	static bool flipFlop = true;
	if (serialTXtokenFree)
	{
		isAlive = false;
		if (!systemTimeUpdate)
		{
			// every second one is a HEARTBEAT
			if (flipFlop) sendSerialMessage(HEARTBEAT);
			else sendSerialMessage(GET_PANEL_TIME);
			flipFlop = !flipFlop;
		}
		else sendSerialMessage(HEARTBEAT);
	}
}

void zitonClass::timerSerialTimeOut(){
	serialTXtokenFree = true;
	// restore read rate
	m_readSerial = false;
	serialInTimer->stop();
	serialInTimer->start(READ_INPUT_BUFFER_RATE);
}

void zitonClass::closeNonCanonicalUART(){
	// do a flush of both RX and TX sata
	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &oldtio);
}

int zitonClass::openNonCanonicalUART()
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
	char welcomeMsg[] = "Hello, 9600n81";

	int n = write(fd, welcomeMsg, strlen(welcomeMsg));
	if (n < 0)
	{
		qDebug() << "Can't open fire panel port";
	}
	else
		qDebug() << "Fire Panel Port Open";
	if (fd <0) { perror(MODEMDEVICE); return (-1); }
	result = true;


	return result;
}

int zitonClass::sendSerialMessage(int messageCode)
{
	//respond with -1 for nothing sent ie ERR, otherwise respond with code 
	int result = -1;
	char messageToSend[255];
	bool startTimer = false;
	switch (messageCode)
	{
	case NAK:
		// send a nak
		strcpy(messageToSend, ">IN\r");
		result = NAK;
		break;
	case ACK:
		// send an ack
		// should parse, this is done by adding the ascii value
		// which for this code is 296
		strcpy(messageToSend, "x00100100296x");
		messageToSend[0] = 0x06;
		messageToSend[12] = 0x04;
		result = ACK;
		break;
	case HEARTBEAT:
		// send a heartbeat, we will expect a response
		/*strcpy(messageToSend, ">IQS\r");
		serialTXtokenFree = false;
		result = HEARTBEAT;
		startTimer = true;
		break;*/
	case GET_PANEL_TIME:
		// send string that will result in panel sending back time
		strcpy(messageToSend, ">IQI0\r");
		serialTXtokenFree = false;
		result = GET_PANEL_TIME;
		startTimer = true;
		break;
	default:
		// unkonown message
		return -1;
		break;
	}
	// if we got the token it means we have a message to go
	write(fd, messageToSend, strlen(messageToSend));
	if (startTimer) serialErrorTimer->start(SERIAL_ERROR_TIEMOUT_VALUE);
	return result;
}

void zitonClass::parseReceivedData()
{
	isAlive = true;
	// lets see what sort of a message we got
	serialDataFlags = 0;	// only because we have two flags at present!
	serialInindex = 0;
	int whatWasThat = 0; // if >0 we will send a NAK, but only do so a fixed amount of times
	int sendresult, msgType = 0;;
	bool logIt = false, gotNAK;
	sendSerialMessage(ACK);
	// lets check for an evacuate
	if ((serialInBuffer[11] == '0')&((serialInBuffer[12] == '3'))&(serialInBuffer[13] == '8'))
	{
		// single part message
		QString temp = QString::fromStdString(serialInBuffer);
		panelEvent.time = temp.mid(42, 2) + ":" + temp.mid(45, 2);
		qDebug() << " Fire Panel Parsed: " << temp;
		emit callEvacuatePanelEvent("Evacuation");
	}
	else if ((serialInBuffer[11] == '0')&((serialInBuffer[12] == '4'))&(serialInBuffer[13] == '1'))
	{
		// this is a two block piece
		if (!panelEvent.blocksFilled)
		{
			// first block
			QString temp = QString::fromStdString(serialInBuffer);
			int startPostition = temp.indexOf(0x09);
			int endPosition = temp.indexOf(0x03);
			//get time and other vars
			panelEvent.time = temp.mid(42, 2) + ":" + temp.mid(45, 2);
			panelEvent.zone = temp.mid(23, 3);
			panelEvent.addr = temp.mid(20, 3);
			panelEvent.block1text = panelEvent.custom + " " + "Panel Time ("
				+panelEvent.time +") Zone:" + panelEvent.zone +
				" Addr:" + panelEvent.addr + " " 
				+ temp.mid(startPostition, ((endPosition - startPostition)));
			panelEvent.block1text.remove(QChar(0x7f));
			panelEvent.block1text = panelEvent.block1text.simplified();

			// movee to next block, start timer, fire it off if 2nd does not arrive
			secondBlkTimeout->start(SECOND_BLK_TIMEOUT);
			panelEvent.blocksFilled = 1;
			eventCount++;
		}
		else
		{
			QString temp = QString::fromStdString(serialInBuffer);
			int startPostition = temp.indexOf(0x09);
			int endPosition = temp.indexOf(0x03);
			panelEvent.block2text = temp.mid(startPostition, ((endPosition - startPostition)));
			panelEvent.block2text.remove(QChar(0x7f));
			panelEvent.block2text = panelEvent.block2text.simplified();
			qDebug() << panelEvent.block2text;
			panelEvent.blocksFilled = 0;
			secondBlkTimeout->stop();
			// trigger the fire etc
			QString message = panelEvent.block1text + " " + panelEvent.block2text;
			emit callFirePanelEvent(message,eventCount);
		}
	}
	// reset don't forget reset event count!
	else if ((serialInBuffer[11] == '0')&((serialInBuffer[12] == '3'))&(serialInBuffer[13] == '6'))
	{
		QString temp = QString::fromStdString(serialInBuffer);
		qDebug() << " Fire Panel Parsed: " << temp;
		panelEvent.time = temp.mid(42, 2) + ":" + temp.mid(45, 2);
		panelEvent.blocksFilled = 0;
		eventCount = 0;
		emit callResetPanelEvent("Evacuation");
	}
	// silence bells
	else if ((serialInBuffer[11] == '0')&((serialInBuffer[12] == '3'))&(serialInBuffer[13] == '7'))
	{
		QString temp = QString::fromStdString(serialInBuffer);
		qDebug() << " Fire Panel Parsed: " << temp;
		panelEvent.time = temp.mid(42, 2) + ":" + temp.mid(45, 2);
		emit callSilencePanelEvent("BELLS SILENCED!");
	}
}

int zitonClass::deleteAll()
{
	// simulates a reset
	eventCount = 0;
}

void zitonClass::inputTimedOut()
{
	// corrupt data possibly
	//? send a nak???
	serialInTimeout->stop();
	m_readSerial = false;	// remove block
	serialInTimer->stop();
	serialInTimer->start(READ_INPUT_BUFFER_RATE);
	qDebug() << "Corrupt incoming string";
}

void zitonClass::secondBlockMissing()
{
	secondBlkTimeout->stop();
	QString message = panelEvent.block1text + " " + SECOND_BLK_WARNING;
	emit callFirePanelEvent(message, eventCount);
	panelEvent.blocksFilled = 0;
	qDebug() << "Missing 2nd Block";
}

void zitonClass::panelLinkFail()
{
	// emit a signal that MainW can catch
	emit serialLinkFailed(LINK_FAIL_MSG,PERIPH_FAULT_CODE);
}

QString zitonClass::getPanelAlarmTime(void)
{
	return panelEvent.time;
}
