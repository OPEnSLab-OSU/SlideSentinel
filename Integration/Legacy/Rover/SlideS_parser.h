#include <SD.h>
#include <CRC32.h>

#define chipSelect 4
#define BUFFER_SIZE 200
#define FILL_SIZE 25
#define DEBUG 1
#define MAX_LENGTH 200
#define STREAK_LENGTH 20

bool isRTKTen(char *nmeaString);
bool verifyChecksum(char *nmeaString);
void getFieldContents(char *, char *, uint8_t);
int stringRank(char);
byte stringChecksum(char *s);
bool sd_save_elem_nodelim(char *file, char *data);
void readLine(char buf[], File e);
void printMsg(char msg[]);
void sendMessage(char msg[], HardwareSerial &);
void fillGPSMsgFormat(char *pstiThirtyString, char msg[]);
uint8_t collectTens(char *filename, char best[]);
void dispatchBest(HardwareSerial &serialPort, char best[]);
void append(char s[], char c);

const char *BestLogs = "BEST.TXT";
const char *BestCur = "CUR.TXT";
CRC32 crc;
File sdFile;
int streak;
int numTens;

void processGPS(char *filename, int fromNode, HardwareSerial &serialPort)
{
	SD.begin(chipSelect);
	char best[BUFFER_SIZE + 1]; // holds the ideal string to send, this string is selected as the algorithm iterates through tthe file
	memset(best, '\0', sizeof(best));

	//move all RTK fixes with ratio 10.0 to BEST.TXT
	if (collectTens(filename, best))
	{
		Serial.println("DONE COLLECTING");
		if(numTens >= 50){
			dispatchBest(serialPort, best);
		}
	}
}

// Move all RTK fixes in CRWKG with ratio 10.0 to BEST.TXT
// This algorithm selects the best string to chose. The selected string will be the last string in hte longest sequence of RTK 10 fixes
// This string is only actually uploaded if the occured 300 or more RTK fixes during the wake cycle
uint8_t collectTens(char *filename, char best[])
{
	int streak = 0;		// the current longest streak of RTK 10's
	int bestStreak = 0; // the previous longest streak of RTK 10's
	uint16_t lineno = 0;
	uint16_t bestlineno = 0;
	numTens = 0;				  // the number of RTK 10's that occured during the read cycle
	char buffer[BUFFER_SIZE + 1]; // holds the NMEA string on the SD card
	char buf[BUFFER_SIZE + 1];
	memset(buf, '\0', sizeof(buf));
	memset(buffer, '\0', sizeof(buffer));

	File fileIn = SD.open(filename, FILE_READ);
	if (!fileIn)
	{
		Serial.println("SD could not open file properly: ");
		Serial.println(filename);
		return 0;
	}

	//read through all of the logged data and find the 10 values, keep a running track of the optimal string to upload
	while (fileIn.available())
	{
		readLine(buffer, fileIn);
		lineno++;
#if DEBUG
		Serial.println(buffer);
#endif
		if (isRTKTen(buffer))
			streak++;
		else
			streak = 0;
		if (streak >= bestStreak)
		{ //if a streak of some value of RTK fixes of 10's is seen we select that string
			/*Serial.println("New best string found");
			bestStreak = streak;
			getFieldContents(buffer, buf, 14);
			if ((!strcmp(buf, "1.0")) || (!strcmp(buf, "2.0")) )
			{
				memset(best, '\0', sizeof(best));
				strcpy(best, buffer); //copy over the best string
			}*/
			bestStreak = streak;
			bestlineno = lineno;
		}

		memset(buffer, '\0', sizeof(buffer));
	}

	//iterate through the file again until reaching the start of the longest streak
	//go through the longest streak, any RTK 10 with age 1.0 or 2.0 should be taken
	fileIn.close();
	fileIn = SD.open(filename, FILE_READ);
	uint16_t startline = bestlineno - bestStreak + 1;
	bool ageFlag = false;
	Serial.print("best line: ");
	Serial.println(bestlineno);
	Serial.print("best streak: ");
	Serial.println(bestStreak);
	Serial.print("Start line: ");
	Serial.println(startline);
	if (!fileIn)
	{
		Serial.println("SD could not open file properly: ");
		Serial.println(filename);
		return 0;
	}
	while (fileIn.available())
	{
		readLine(buffer, fileIn);
		startline--;
		memset(buffer, '\0', sizeof(buffer));
		if (startline == 0)
		{
			for (uint16_t i = 0; i < bestStreak - 1; i++)
			{
#if DEBUG
				Serial.println(buffer);
#endif
				readLine(buffer, fileIn);
				getFieldContents(buffer, buf, 14);
				if ((!strcmp(buf, "1.0")) || (!strcmp(buf, "2.0")))
				{
					ageFlag = true;
					memset(best, '\0', sizeof(best));
					strcpy(best, buffer); //copy over the best string
					Serial.print("New best: ");
					Serial.println(best);
				}
				if (!ageFlag)
				{
					memset(best, '\0', sizeof(best));
					strcpy(best, buffer); //copy over the best string
					Serial.print("New best not a valid age: ");
					Serial.println(best);
				}
				memset(buffer, '\0', sizeof(buffer));
			}
			break;
		}
	}

	Serial.print("BEST STRING: ");
	Serial.println(best);
	Serial.print("NUMBER OF 10's: ");
	Serial.println(numTens);
	Serial.print("LONGEST STREAK: ");
	Serial.println(bestStreak);

	//Remove CRWKG
	fileIn.close();
	if (!SD.remove(filename))
	{
		Serial.print("could not remove");
		Serial.println(filename);
	}
	return 1;
}

//Checks if passed string is RTK with a fix ratio of 10.0
bool isRTKTen(char *nmeaString)
{
	char field[FILL_SIZE + 1];
	char msg[200];
	memset(field, '\0', FILL_SIZE + 1);
	memset(msg, '\0', sizeof(msg));

	getFieldContents(nmeaString, field, 15);
	if (atof(field) == 10.0)
	{
		numTens++;
		append(nmeaString, '\n');
		//sd_save_elem_nodelim((char *)BestLogs, nmeaString);
		//sd_save_elem_nodelim((char *)BestCur, nmeaString);
		return true;
	}
	return false;
}

//send all tens found to the base station
void dispatchBest(HardwareSerial &serialPort, char nmea[])
{
	char buffer[BUFFER_SIZE + 1];
	memset(buffer, '\0', sizeof(buffer));
	strcat(buffer, "/GPS");
	strcat(buffer, ",");
	strcat(buffer, "0");
	strcat(buffer, ",");
	fillGPSMsgFormat(nmea, buffer);

	//send 10 copies of the message
	for (uint8_t i = 0; i < 50; i++)
	{
		printMsg(buffer);
		Serial.println();
		sendMessage(buffer, serialPort);
		delay(50);
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

	getFieldContents(pstiThirtyString, toFill, 7);			//altitude change to numTens in cycle
	strcat(msg, toFill);
	strcat(msg, ",");

	memset(toFill, '\0', strlen(toFill));
	snprintf(toFill, sizeof(toFill), "%d", numTens);
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

//takes a RAW NMEA string with <CR><LF> and verifies it
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
		Serial.println("Invalid string");
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
	Serial.println(msg);
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
		sdFile.print(data);
	else
		return false;
	sdFile.close();
	return true;
}

char *getValueAt(char src[], int pos)
{
	char buf[MAX_LENGTH];
	memset(buf, '\0', sizeof(buf));
	strcpy(buf, src);
	uint16_t count = 0;
	char *token = strtok(buf, ",");
	while (token != NULL)
	{
		if (count == pos)
		{
			return token;
		}
		count++;
		token = strtok(NULL, ",");
	}
	Serial.println("Value does not exist.");
	return NULL;
}

/*
		if (numTens >= 400)
			dispatchBest(serialPort, best, '1'); // ~95% chance
		else if (numTens >= 300)
			dispatchBest(serialPort, best, '2'); // ~80% chance
		else if (numTens >= 200)
			dispatchBest(serialPort, best, '3'); // ~50% chance
		else if (numTens >= 100)
			dispatchBest(serialPort, best, '4'); // ~30% chance
		else if (numTens >= 20)
			dispatchBest(serialPort, best, '5'); // ~10% chance
			*/