#include "HyteraInterfaceClass.h"


HyteraInterfaceClass::HyteraInterfaceClass()
{
	qDebug() << "Hytera object created OK";
	openNonCanonicalUART();
	readSerial = new QTimer;
	readSerial->stop();
	connect(readSerial, SIGNAL(timeout()), this, SLOT(getSerial()));
	readSerial->start(SLOW_READ_RATE);
}


HyteraInterfaceClass::~HyteraInterfaceClass()
{
	closeNonCanonicalUART();
	qDebug() << "Hytera Object destroyed OK";
}

int HyteraInterfaceClass::sendHYTmessage(QString radioGrpCode, QString body)
{
	m_textSentOK = false;
	m_killNow = false;
	if (radioGrpCode[0] != 'P' & radioGrpCode[0] != 'G')
	{
		pleaseLogThis("Unknown group or private designator :" + radioGrpCode[0] + " must be P or G");
		qDebug() << "group/Private designator error, what is: " << radioGrpCode[0];
		m_killNow = true;
		return 0;
	}
	bool privateHYT = false;
	// group or private messae?
	if (radioGrpCode[0] == 'P') privateHYT = true;
	else
	{
		// send group
		// dump group designator
		radioGrpCode = radioGrpCode.mid(1);
		int sendState = sendGroupMessage(radioGrpCode + " " + body + '\n');
	}
	// all went ok
	m_textSentOK = true;
	return 1;
}

bool HyteraInterfaceClass::isTextCycleFinished(void)
{
	// used by task to do a timeout
	return m_textSentOK;
}

void HyteraInterfaceClass::killText(void)
{
	// request to kill the transmission
	m_killNow = true;
	// reset flags etc to allow new message
}

void HyteraInterfaceClass::closeNonCanonicalUART()
{
	// do a flush of both RX and TX sata
	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &oldtio);
}

int HyteraInterfaceClass::openNonCanonicalUART()
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
	char welcomeMsg[] = "HYT zon 1\n";
	hytFlags.sentZON = true;
	clearStateFlags();
	m_CTS = false;
	int n = write(fd, welcomeMsg, strlen(welcomeMsg));
	if (n < 0)
	{
		qDebug() << "Can't open fire panel port";
	}
	else
		qDebug() << "Hyters Interface Open";
	if (fd <0) { perror(MODEMDEVICE); return (-1); }
	result = true;


	return result;
}

int HyteraInterfaceClass::sendSerial(QString s)
{

}

int HyteraInterfaceClass::sendPrivateMessage(QString s)
{

}

int HyteraInterfaceClass::sendGroupMessage(QString s)
{
	if (m_CTS)
	{
		clearStateFlags();
		m_CTS = false;
		QString serial = "HYT grp " + s;
		int n = write(fd, serial.toUtf8().data(), serial.size());
		if (n < 0)
		{
			qDebug() << "Send in HYT error!!!!";
			m_CTS = true;
			pleaseLogThis("Undefined Problem sending HYT data");
			return 0;
		}
		else
		{
			hytFlags.sentGRP = true;
		}
	}
	else
		return 0;
	return 1;
}

void HyteraInterfaceClass::getSerial(void)
{
	// this is a timed event... if not expecting anythin slow
	// down the duty cycle

	int res;
	char buf[2];
	// clear the buffer
	buf[0] = '\0';
	res = read(fd, buf, 1);
	if (res > 0)
	{
		// speed up read rate
		readSerial->stop();
		readSerial->start(FAST_READ_RATE);
		// add char to buffer unless we have an overrun
		if (serialInindex < SERIAL_IN_BUFFER)
			serialInBuffer[serialInindex++] = buf[0]; // add to buffer and increment
		else
			clearBuffer(); // overrun
		// did we get an end
		if (buf[0] == END_CHAR)
			parseSerial();
	}
}

void HyteraInterfaceClass::parseSerial(void)
{
	// we got a <LF> in,
	hytFlags.gotResponse = true;
	// what type of string is it?
	if (serialInBuffer[0] == ACK_CHAR)
	{
		qDebug() << "Got an ACK from Hagen";
		hytFlags.gotACK = true;
		m_CTS = true;
	}
	else if (serialInBuffer[0] == NAK_CHAR)
	{
		hytFlags.gotNAK = true;
	}
	else if (serialInBuffer[0] == TX_FAIL)
	{
		hytFlags.gotTXfail = true;
	}
	else if (serialInBuffer[0] == TX_OK)
	{
		hytFlags.gotTXOK = true;
	}

	// clear buffer
	clearBuffer();
}

void HyteraInterfaceClass::clearBuffer(void)
{
	serialInindex = 0;
	serialInBuffer[serialInindex] = '\0';
	// slow down the check serial rate
	readSerial->stop();
	readSerial->start(SLOW_READ_RATE);
}

void HyteraInterfaceClass::clearStateFlags(void)
{
	// should be called before sending to H
	hytFlags.gotACK = false;
	hytFlags.gotNAK = false;
	hytFlags.gotTXfail = false;
	hytFlags.gotTXOK = false;
	hytFlags.gotResponse = false;
	hytFlags.sentPRV = false;
	hytFlags.sentGRP = false;
	hytFlags.sentZON = false;
	hytFlags.sentCHN = false;
}