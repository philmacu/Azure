#include "notifierPanel.h"


notifierPanel::notifierPanel()
{
	// open the port
	openNonCanonicalUART();
	// load settings
	loadSettings();
	// lets initialise the slots
	heartbeatTimer = new QTimer;
	serialInTimer = new QTimer;
	serialErrorTimer = new QTimer;
	QMainWindow::connect(heartbeatTimer, SIGNAL(timeout()),
		this, SLOT(sendHeartbeat()));
	QMainWindow::connect(serialInTimer, SIGNAL(timeout()),
		this, SLOT(testForSerialIn()));
	QMainWindow::connect(serialErrorTimer, SIGNAL(timeout()),
		this, SLOT(timerSerialTimeOut()));

	heartbeatTimer->start(HEARTBEAT_RATE);
	serialInTimer->start(READ_INPUT_BUFFER_RATE);
	serialErrorTimer->setSingleShot(true);
	serialErrorTimer->stop();
	systemTimeUpdate = false;
	serialTXtokenFree = true;
	eventCount = 0;
	isAlive = false;
	inAlarm = false;
}

int notifierPanel::loadSettings()
{
	// these settings dictate what is sent in message
	//std::ifstream ifs_load;
	//std::string isInstalled, parameter, value;
	//ifs_load.open(PANEL_PARAMETERS, std::ifstream::in);
	//getline(ifs_load, isInstalled);
	isFitted = true;
	bool readError;
	includeInSMS.customText = true;
	includeInSMS.ID = true;
	includeInSMS.loop = true;
	includeInSMS.PanelDate = true;
	includeInSMS.panelText = true;
	includeInSMS.panelTime = true;
	includeInSMS.sensor = true;
	includeInSMS.sensorAddr = true;
	includeInSMS.sensorVal = true;
	includeInSMS.zone = true;
	//if (isInstalled != DEVICE_NOT_FITTED){
		//// Notifier is on the system
		//isFitted = true;
		//while (!ifs_load.eof())
		//{
			//getline(ifs_load, parameter, ':');
			//if (parameter == "END") break;
			//getline(ifs_load, value);
			//// what did we read?
			//readError = true; // will be cleared if parameter is ok
			//if (parameter == "ID" )
			//{
				//if (value == "Y") includeInSMS.ID = true;
				//readError = false;
			//}
			//if (parameter == "TIME")
			//{
				//if (value == "Y") includeInSMS.panelTime = true;
				//readError = false;
			//}
			//if (parameter == "ZONE")
			//{
				//if (value == "Y") includeInSMS.zone = true;
				//readError = false;
			//}
			//if (parameter == "LOOP")
			//{
				//if (value == "Y") includeInSMS.loop = true;
				//readError = false;
				//
			//}
			//if (parameter == "SENSOR")
			//{
				//if (value == "Y") includeInSMS.sensor = true;
				//readError = false;
			//}
			//if (parameter == "ADDR")
			//{
				//if (value == "Y") includeInSMS.sensorAddr = true;
				//readError = false;
			//}
			//if (parameter == "VALUE")
			//{
				//if (value == "Y") includeInSMS.sensorVal = true;
				//readError = false;
			//}
			//if (parameter == "TEXT")
			//{
				//if (value == "Y") includeInSMS.panelText = true;
				//readError = false;
			//}
			//// sending panel time is too confusing
			///*if (parameter == "DATE")
			//{
				//if (value == "Y") includeInSMS.PanelDate = true;
				//readError = false;
			//}*/
			//if (parameter == "LABEL")
			//{
				//if (value.length() < CUSTOM_LABEL_LEN)
				//{
					//includeInSMS.customText = true;
					//strcpy(panelEvent.custom, value.c_str());
				//}
				//readError = false;
			//}
			//if (readError) break;
		//}
		//ifs_load.close();
	//}
}

notifierPanel::~notifierPanel()
{
	// close the port
	closeNonCanonicalUART();
	delete heartbeatTimer;
	delete serialInTimer;
	delete serialErrorTimer;
	qDebug() << "Panel destrucor completed OK/n";
}

void notifierPanel::testForSerialIn()
{
	// function will modify the flags 
	int res;
	char buf[2];
	// clear the buffer
	buf[0] = '\0';

	res = read(fd, buf, 1);
	if (res > 0){
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
			break;
		case SPECIAL_CHAR:
			// notifier non printing char
			serialInBuffer[serialInindex] = ' ';
			break;
		case END_CHAR:
			// check to see if we had a start
			if ((serialDataFlags & (1 << START_CHAR_FLAG)) == true)
			{
				serialDataFlags |= (1 << END_CHAR_FLAG); //Stop = true;
				serialInBuffer[serialInindex] = '\0';
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
			break;
		}
	}
}

void notifierPanel::sendHeartbeat()
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

void notifierPanel::timerSerialTimeOut(){
	serialTXtokenFree = true;
}

void notifierPanel::closeNonCanonicalUART(){
	// do a flush of both RX and TX sata
	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &oldtio);
}

int notifierPanel::openNonCanonicalUART()
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

int notifierPanel::sendSerialMessage(int messageCode)
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
		strcpy(messageToSend, ">IACK\r");
		result = ACK;
		break;
	case HEARTBEAT:
		// send a heartbeat, we will expect a response
		strcpy(messageToSend, ">IQS\r");
		serialTXtokenFree = false;
		result = HEARTBEAT;
		startTimer = true;
		break;
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

void notifierPanel::parseReceivedData()
{
	isAlive = true;
	// lets see what sort of a message we got
	serialDataFlags = 0;	// only because we have two flags at present!
	serialInindex = 0;
	int whatWasThat = 0; // if >0 we will send a NAK, but only do so a fixed amount of times
	int sendresult, msgType = 0;;
	bool logIt = false, gotNAK;
	char DTGstring[100];
	FILE *logFile;
	int parseLength = strlen(serialInBuffer);
	if (strncmp(serialInBuffer, ">IACK", 5) == 0)
	{
		//cout << "That was an ACK\n";
		serialTXtokenFree = true;
		gotNAK = false;
		return;
	}
	if (strncmp(serialInBuffer, ">IN", 3) == 0)
	{
		//cout << "That was an NAK\n";
		serialTXtokenFree = true;
		gotNAK = true;
		return;
	}
	// what type of message is it
	switch (serialInBuffer[MSG_TYPE_OFFSET])
	{
	case EVENT_MSG:
		// test for fire message
		if (strncmp((serialInBuffer + EVENT_OFFSET), "001", 3) == 0){
			strcpy(panelEvent.event, "Fire");
			logIt = true;
			// send type of email to send
			//emailType = ALARM_MAIL;
			// transfer info into a structure
			;
			for (size_t i = 0; i < parseLength; i++)
			{
				if (i >= PANEL_OFFSET & i<EVENT_OFFSET) panelEvent.panel[i - PANEL_OFFSET] = serialInBuffer[i];
				if (i >= EVENT_TIME_OFFSET & i<LOOP_OFFSET) panelEvent.time[i - EVENT_TIME_OFFSET] = serialInBuffer[i];
				if (i >= LOOP_OFFSET & i<ZONE_OFFSET) panelEvent.loop[i - LOOP_OFFSET] = serialInBuffer[i];
				if (i >= ZONE_OFFSET & i<SENSOR_OFFSET) panelEvent.zone[i - ZONE_OFFSET] = serialInBuffer[i];
				if (i >= SENSOR_OFFSET & i < SENS_ADDR_OFFSET) panelEvent.sensor[i - SENSOR_OFFSET] = serialInBuffer[i];
				if (i >= SENS_ADDR_OFFSET & i< ANALOGUE_VAL_OFFSET) panelEvent.sensorAddr[i - SENS_ADDR_OFFSET] = serialInBuffer[i];
				if (i >= ANALOGUE_VAL_OFFSET & i < OFFSET_END) panelEvent.analogVal[i - ANALOGUE_VAL_OFFSET] = serialInBuffer[i];
				if (i >= TEXT_START & i < (parseLength - 3)) panelEvent.text[i - TEXT_START] = serialInBuffer[i];
			}
			// add in NULLS
			SMStext[0] = '\0'; // clear our sms string
			panelEvent.panel[2] = '\0';
			panelEvent.time[6] = '\0';
			panelEvent.loop[2] = '\0';
			panelEvent.zone[5] = '\0';
			panelEvent.sensor[1] = '\0';
			panelEvent.sensorAddr[2] = '\0';
			panelEvent.analogVal[3] = '\0';
			panelEvent.text[((parseLength - 3) - TEXT_START)] = '\0';
			panelEvent.date[10] = '\0';

			// now build message to go depending on config settings
			if (includeInSMS.customText) strcat(SMStext, panelEvent.custom);
			if (includeInSMS.ID) {
				strcat(SMStext, " PANEL:");
				strcat(SMStext, panelEvent.panel);
			}
			if (includeInSMS.loop) {
				strcat(SMStext, " Loop:");
				strcat(SMStext, panelEvent.loop);
			}
			if (includeInSMS.zone) {
				strcat(SMStext, " Zone:");
				strcat(SMStext, panelEvent.zone);
			}
			if (includeInSMS.sensor) {
				strcat(SMStext, " Sensor:");
				strcat(SMStext, panelEvent.sensor);
			}
			if (includeInSMS.sensorAddr) {
				strcat(SMStext, " Addr:");
				strcat(SMStext, panelEvent.sensorAddr);
			}
			if (includeInSMS.sensorVal) {
				strcat(SMStext, " Value:");
				strcat(SMStext, panelEvent.analogVal);
			}
			if (includeInSMS.PanelDate) {
				strcat(SMStext, " ");
				strcat(SMStext, panelEvent.date);
			}
			if (includeInSMS.panelTime) {
				strcat(SMStext, " ");
				strcat(SMStext, panelEvent.time);
			}
			if (includeInSMS.panelText) {
				strcat(SMStext, " ");
				strcat(SMStext, panelEvent.text);
			}
			// lets try to emit a signal,
			/*
			This will only be allowed if we havent exceeded the number of events that 
			we are allowing before a reset, for the panels, the count is reset by either
			a panel reset or the master reset, the deleteAll() routine
			a 0 allows infinite events from the panel, <0 will switch off messaging
			*/

			SMStext[0] = ' ';
			// all serial passsed, it is up to the scenario to decide if
			// it is transmitted
			eventCount++;
			emit callFirePanelEvent(QString::fromStdString(SMStext),eventCount);
			inAlarm = true; // used for graphics only
		}
		// reset
		if (strncmp((serialInBuffer + EVENT_OFFSET), "129", 3) == 0)
		{
			strcpy(panelEvent.event, "Reset");
			// reset the event count
			eventCount = 0;
			logIt = true;
			inAlarm = false;
			//emailType = RESET_MAIL;
			for (size_t i = 0; i < parseLength; i++)
			{
				if (i >= PANEL_OFFSET & i<EVENT_OFFSET) panelEvent.panel[i - PANEL_OFFSET] = serialInBuffer[i];
				if (i >= EVENT_TIME_OFFSET & i<LOOP_OFFSET) panelEvent.time[i - EVENT_TIME_OFFSET] = serialInBuffer[i];
			}
		}
		serialTXtokenFree = true;
		break;
	case INFO_MSG:
		// eg time date from panel

		strcpy(DTGstring, "sudo date -s 'yyyy-mm-dd hh:mm:ss'>>debugLog.txt");
		// replace with received data
		for (size_t i = SYSTEM_DATE_OFFSET; i < SYSTEM_DATE_END; i++)
		{
			DTGstring[i] = serialInBuffer[i + 14];
		}
		sendresult = system(DTGstring);
		// 256 is a fail, 0 seems to be all oK
		if (sendresult == 0)
		{
			systemTimeUpdate = true;
			qDebug() << "Updated system time";
		}
		serialTXtokenFree = true;
		break;
	case STATUS_MSG:
		serialTXtokenFree = true;
		break;
	default:
		// unknown, request a retry, unless this is a retry!
		if (whatWasThat == 0) whatWasThat = SEND_NAK_TIMES;
		break;
	}

	// lets ack or nak the message we got
	if (whatWasThat == 0)	sendresult = sendSerialMessage(ACK);
	else{
		sendresult = sendSerialMessage(NAK);
		whatWasThat--;
	}
	// do we log stuff?
	if (logIt == true)
	{
		logIt = false;
		/*
		char DTGstring[100];
		strcpy(DTGstring, "sudo date>>debugLog.txt");
		system(DTGstring);
		*/
		// lets try open using traditional approach
		logFile = fopen(PANEL_EVENT_LOG, "a+");
		fprintf(logFile, "----------------------------------------------------------\n");
		// now lets get date
		struct tm * timeinfo;
		time_t rawtime;
		time(&rawtime);
		timeinfo = localtime(&rawtime);

		fprintf(logFile, "Current local time and date: %s\n", asctime(timeinfo));
		fprintf(logFile, "Message Received: %s\n", panelEvent.event);
		fprintf(logFile, "%s\n", panelEvent.text);
		fprintf(logFile, "Panel: %s Zone: %s Event Time: %s\n", panelEvent.panel, panelEvent.zone, panelEvent.time);
		fprintf(logFile, "Sensor: %s Sensor Address: %s Analogue Value: %s\n\n", panelEvent.sensor, panelEvent.sensorAddr, panelEvent.analogVal);
		fprintf(logFile, "Raw Message: %s\n", serialInBuffer);
		fclose(logFile);
		//sendMail(emailType);
	}

}

int notifierPanel::deleteAll()
{
	// simulates a reset
	eventCount = 0;
}