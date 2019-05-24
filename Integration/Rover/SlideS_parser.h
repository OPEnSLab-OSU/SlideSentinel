#include <SD.h>
#include <CRC32.h>

#define BUFFER_SIZE 200
#define FILL_SIZE 25
#define DEBUG 1
#define PSTI_30_UTC

void isRTKTen(char *nmeaString);
bool verifyChecksum(char *nmeaString);
void getFieldContents(char *, char *, uint8_t);
int stringRank(char);
byte stringChecksum(char *s);
bool sd_save_elem_nodelim(char *file, char *data);
void readLine(char buf[], File e);

int sent = 0;
const char *BestLogs = "BEST.TXT";
const char *BestCur = "CUR.TXT";
CRC32 crc;
void printMsg(char msg[]);
void sendMessage(char msg[], HardwareSerial &);
void fillGPSMsgFormat(char *pstiThirtyString, char msg[]);
uint8_t collectTens(char *filename);
void dispatchTens(char *filename, HardwareSerial &serialPort);

void processGPS(char *filename, int fromNode, HardwareSerial &serialPort)
{
	SD.begin(chipSelect);

	//move all RTK fixes with ratio 10.0 to BEST.TXT
	if(collectTens(filename)){
		Serial.println("DONE COLLECTING"); 
		dispatchTens("CUR.TXT", serialPort);
	}
}

//move all RTK fixes in CRWKG with ratio 10.0 to BEST.TXT
uint8_t collectTens(char *filename)
{
	File fileIn = SD.open(filename, FILE_READ);
	char buffer[BUFFER_SIZE + 1];
	memset(buffer, '\0', sizeof(buffer));
	if (!fileIn)
	{
		LOOM_DEBUG_Print("SD could not open file properly: ");
		Serial.println(filename);
		return 0;
	}

	//read through all of the logged data and find the 10 values
	Serial.println("printing everything");
	while (fileIn.available())
	{
		readLine(buffer, fileIn);
#if DEBUG
		Serial.println(buffer);
#endif
		isRTKTen(buffer);
		memset(buffer, '\0', sizeof(buffer));
	}

	//Remove CRWKG
	fileIn.close();
	if (!SD.remove(filename))
	{
		Serial.print("could not remove");
		Serial.println(filename);
	}
	return 1; 
}

//send all tens found to the base station
void dispatchTens(char *filename, HardwareSerial &serialPort)
{
	File fileIn = SD.open(filename, FILE_READ);
	char buffer[BUFFER_SIZE + 1];
	char nmea[BUFFER_SIZE + 1];
	memset(buffer, '\0', sizeof(buffer));
	memset(nmea, '\0', sizeof(nmea));
	if (fileIn.size() > 0)
	{
		while (fileIn.available())
		{
			readLine(nmea, fileIn);
			strcat(buffer, "/GPS");
			strcat(buffer, ",");
			strcat(buffer, "0");
			strcat(buffer, ",");
			fillGPSMsgFormat(nmea, buffer);
			sendMessage(buffer, serialPort);
			delay(50);
			memset(buffer, '\0', sizeof(buffer));
			memset(nmea, '\0', sizeof(nmea));
		}
	}

	//Remove CUR.TXT, all of the 10's for the wake cycle
	fileIn.close();
	if (!SD.remove("CUR.TXT"))
	{
		Serial.print("could not remove");
		Serial.println(filename);
	}
}

void append(char s[], char c)
{
	int len = strlen(s);
	s[len] = c;
	s[len + 1] = '\0';
}

void readLine(char buf[], File e)
{
	char input;
	input = e.read();
	while (input != '\n') // if char not  eol
	{
		append(buf, input);
		input = e.read();
	}
}

//Checks if passed string is RTK with a fix ratio of 10.0
void isRTKTen(char *nmeaString)
{
	char field[FILL_SIZE + 1];
	char msg[200];
	memset(field, '\0', FILL_SIZE + 1);
	memset(msg, '\0', sizeof(msg));

	getFieldContents(nmeaString, field, 15);
	if (atof(field) == 10.0)
	{
		append(nmeaString, '\n');
		sd_save_elem_nodelim((char *)BestLogs, nmeaString);
		sd_save_elem_nodelim((char *)BestCur, nmeaString);
		printMsg(msg);
	}
	memset(field, '\0', FILL_SIZE + 1);
}

void printMsg(char *msg)
{
	char buf[150];
	memset(buf, '\0', sizeof(buf));
	strcpy(buf, msg);
	uint8_t count = 0;
	char *token = strtok(buf, ",");
	while (token != NULL)
	{
		Serial.print(count);
		Serial.print(": \t");
		Serial.println(token);
		token = strtok(NULL, ",");
		count++;
	}
}

//Adds a 32 bit checksum to any data sent from the rover to base station
void addChecksum(char msg[])
{
	char sum[20];
	memset(sum, '\0', sizeof(sum));
	for (uint8_t i = 0; i < strlen(msg) - 1; i++)
	{
		crc.update(msg[i]);
	}
	uint32_t checksum = crc.finalize();
	crc.reset();

#if DEBUG
	Serial.print("CHECKSUM: ");
	Serial.println(checksum);
#endif
	String check = String(checksum);
	check.toCharArray(sum, sizeof(sum));
	strcat(msg, sum);
	Serial.print("Sum: ");
	Serial.println(sum);
}

void fillGPSMsgFormat(char *pstiThirtyString, char msg[])
{
	char toFill[FILL_SIZE + 1];
	memset(toFill, '\0', FILL_SIZE + 1);
	getFieldContents(pstiThirtyString, toFill, 2);
	strcat(msg, toFill);
	strcat(msg, ",");
	getFieldContents(pstiThirtyString, toFill, 4);
	strcat(msg, toFill);
	strcat(msg, ",");
	getFieldContents(pstiThirtyString, toFill, 5);
	strcat(msg, toFill);
	strcat(msg, ",");
	getFieldContents(pstiThirtyString, toFill, 6);
	strcat(msg, toFill);
	strcat(msg, ",");
	getFieldContents(pstiThirtyString, toFill, 7);
	strcat(msg, toFill);
	strcat(msg, ",");
	getFieldContents(pstiThirtyString, toFill, 8);
	strcat(msg, toFill);
	strcat(msg, ",");
	getFieldContents(pstiThirtyString, toFill, 13);
	strcat(msg, toFill);
	strcat(msg, ",");
	getFieldContents(pstiThirtyString, toFill, 14);
	strcat(msg, toFill);
	strcat(msg, ",");
	getFieldContents(pstiThirtyString, toFill, 15);
	strcat(msg, toFill);
	strcat(msg, ",");
	addChecksum(msg);
}

bool verifyChecksum(char nmeaString[])
{
	static uint32_t bad_count = 0;
	static uint32_t good_count = 0;
	char *pch;
	char buffer[BUFFER_SIZE + 1];
	char chkbuf[50];
	uint8_t chksum;
	memset(chkbuf, '\0', sizeof(chkbuf));
	memset(buffer, '\0', sizeof(buffer));
	pch = strrchr(nmeaString, '*'); //take the last occurence of the '*' character
	if (pch == NULL)
	{
		Serial.println("Invald string");
		bad_count++;
		return false;
	}
	strcpy(chkbuf, pch + 1);
	memset(chkbuf + 2, '\0', sizeof(chkbuf) - 2); //chop out the ending <CR><LF> and \n

	//testing to generate the checksum
	chksum = (uint8_t)strtol(chkbuf, NULL, 16);

	//We need the $.*\* portion of the stirng
	uint16_t i = 0;
	while (nmeaString + i != pch - 1)
	{
		buffer[i] = nmeaString[i + 1];
		i++;
	}

#if DEBUG
	Serial.print("NMEA CHECKSUM: "); //CHECKSUM is ascii encoded HEX
	Serial.print(chksum);
	Serial.print(", ");
	Serial.print(chkbuf);
	Serial.println();
	Serial.print("ORIGINAL: ");
	Serial.print(nmeaString);
	Serial.print("BUFFER: ");
	Serial.println(buffer);
	Serial.print("CHECKSUM: ");
	Serial.println(stringChecksum(buffer));
	Serial.println();
	Serial.print("good count: ");
	Serial.println(good_count);
	Serial.print("bad count: ");
	Serial.println(bad_count);
#endif

	if (stringChecksum(buffer) == chksum)
	{
		good_count++;
		return true;
	}
	else
	{
		bad_count++;
		return false;
	}
}

//function for calculating the 8 bit checksum on NMEA strings
byte stringChecksum(char *s)
{
	byte c = 0;
	while (*s != '\0')
		c ^= *s++;
	return c;
}

// parse a nmea string and return contents of field corresponding to fieldNum
// index starts at 0 with string type
void getFieldContents(char *nmeaString, char *toFill, uint8_t fieldNum)
{
	memset(toFill, '\0', strlen(toFill));
	int index = 0, position = 0, fillIndex = 0;
	while (nmeaString[index] != '\0')
	{
		if (nmeaString[index++] == ',')
		{
			position++;
		}
		if (position == fieldNum)
		{
			break;
		}
	}
	while (nmeaString[index] != ',' && nmeaString[index] != '*')
	{
		if (nmeaString[index] == '\0')
		{
			memset(toFill, '\0', fillIndex + 1);
			return;
		}
		if (nmeaString[index] == ',' || nmeaString[index] == '*')
			break;
		if (fillIndex == FILL_SIZE)
		{
			memset(toFill, '\0', FILL_SIZE + 1);
			return;
		}
		toFill[fillIndex++] = nmeaString[index++];
	}
}

int stringRank(char indicator)
{
	switch (indicator)
	{
	case ('N'):
		return -1;
	case ('A'):
		return 0;
	case ('D'):
		return 1;
	case ('F'):
		return 2;
	case ('R'):
		return 3;
	default:
		return -1;
	}
}

void sendMessage(char msg[], HardwareSerial &serialPort)
{
	printMsg(msg);
	serialPort.write(byte('*')); // start of text
	for (int i = 0; i < strlen(msg); i++)
	{
		serialPort.write(msg[i]);
	}
	serialPort.write(byte('!')); // End of transmission, pad this with more 4's
	serialPort.write(byte('!'));
	serialPort.write(byte('!'));
	serialPort.write(byte('!'));
}

bool sd_save_elem_nodelim(char *file, char *data)
{
#if is_lora == 1
	digitalWrite(8, HIGH); // if using LoRa
#endif
	SD.begin(chipSelect); // It seems that SD card may become 'unsetup' sometimes, so re-setup

	sdFile = SD.open(file, FILE_WRITE);

	if (sdFile)
	{
		LOOM_DEBUG_Println4("Saving ", data, " to SD file: ", file);
		sdFile.print(data);
	}
	else
	{
		LOOM_DEBUG_Println2("Error opening: ", file);
		return false;
	}
	sdFile.close();
	return true;
}
