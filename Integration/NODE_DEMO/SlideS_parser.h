#include <SD.h>
#include <OSCMessage.h>
#include <CRC32.h>

#define BUFFER_SIZE 200
#define FILL_SIZE 25

#define PSTI_30_UTC

void GPSToFiles(char *, int *, int, HardwareSerial &);
bool verify(char *);
void getFieldContents(char *, char *, uint8_t);
void sendMessage(OSCMessage *, HardwareSerial &);
int stringRank(char);
void oscMsg_to_string(char *osc_string, OSCMessage *msg);

int sent = 0;
CRC32 crc;
// fill the OSCMessage in the order UTC, Lat, Lon, Alt, Mode, Age, Ratio
// nmea string must be PSTI030
void fillGPSMsgFormat(char *, OSCMessage &);

bool processGPS(char *filename, int fromNode, HardwareSerial &serialPort)
{
	int bestStringPrev = -1; // quality indicator

	char buffer[BUFFER_SIZE + 1];
	char nextRead;

	SD.begin(chipSelect);
	File fileIn = SD.open(filename, FILE_READ);

	if (!fileIn)
	{
		LOOM_DEBUG_Print("SD could not open file properly: ");
		Serial.println(filename);
		return 0;
	}

	while (fileIn.available())
	{
		int i = 0;
		memset((char *)buffer, '\0', BUFFER_SIZE + 1);
		buffer[i++] = fileIn.read();
		while (fileIn.peek() != '$' && fileIn.available())
		{
			nextRead = fileIn.read();
			buffer[i++] = nextRead;
			if (i >= BUFFER_SIZE || nextRead == -1)
			{
				break;
			}
		}
		if (i < BUFFER_SIZE)
		{
			GPSToFiles(buffer, &bestStringPrev, fromNode, serialPort);
		}
	}
	fileIn.close();
	if (!SD.remove(filename))
	{
		Serial.print("could not remove");
		Serial.println(filename);
	}
}

// Send the GPS String to the proper files if formatted properly
void GPSToFiles(char *nmeaString, int *bestStringPrev, int fromNode, HardwareSerial &serialPort)
{
	// first segment verifies the string and sends it to the all valid measurements file
	char field[FILL_SIZE + 1];
	int len;
	memset(field, '\0', FILL_SIZE + 1);
	File temp;
	// Check that nmea string is in correct format

	if (!verify(nmeaString))
		return;

	if (nmeaString[0] == '\0')
	{
		Serial.println("Null found at start of nmea");
		return;
	}

	temp = SD.open("ALL.TXT", FILE_WRITE);
	if (!temp)
	{
		Serial.println("Could not open tempfile");
	}
	len = temp.print(nmeaString);
	temp.close();

	getFieldContents(nmeaString, field, 0);
	if (field[0] == '\0')
	{
		Serial.print("No field contents");
		return;
	}

	if (!strcmp(field, "PSTI"))
	{
		Serial.println(field);
		getFieldContents(nmeaString, field, 1);
		if (!strcmp(field, "030"))
		{
			Serial.println(field);
			getFieldContents(nmeaString, field, 13);
			int quality = stringRank(field[0]);
			Serial.print("quality: ");
			Serial.println(field);
			if (quality < *bestStringPrev)
				return;
			else if (quality >= *bestStringPrev)
			{
				*bestStringPrev = quality;
				Serial.print("sending packet: ");
				Serial.println(sent++);
				OSCMessage msg("/GPS"); // address
				msg.add((int32_t)fromNode);
				fillGPSMsgFormat(nmeaString, msg);
				Serial.println(nmeaString);
				print_message(&msg, 1);
				sendMessage(&msg, serialPort);
				delay(50);
			}
		}
	}

	memset(field, '\0', FILL_SIZE + 1);
}

///GPS,i0,s115954.000,s0000.0000000,s00000.0000000,s0.000,sN,s0.0,s0.0
// fill the OSCMessage in the order UTC, Lat, N/S, Lon, E/W, Alt, Mode, Age, Ratio
// nmea string must be PSTI030
void fillGPSMsgFormat(char *pstiThirtyString, OSCMessage &msg)
{
	char sum[20];
	memset(sum, '\0', sizeof(sum));
	char buffer[120];
	memset(buffer, '\0', sizeof(buffer));
	char toFill[FILL_SIZE + 1];
	memset(toFill, '\0', FILL_SIZE + 1);
	getFieldContents(pstiThirtyString, toFill, 2);
	msg.add((const char *)toFill);
	getFieldContents(pstiThirtyString, toFill, 4);
	msg.add((const char *)toFill);
	getFieldContents(pstiThirtyString, toFill, 5);
	msg.add((const char *)toFill);
	getFieldContents(pstiThirtyString, toFill, 6);
	msg.add((const char *)toFill);
	getFieldContents(pstiThirtyString, toFill, 7);
	msg.add((const char *)toFill);
	getFieldContents(pstiThirtyString, toFill, 8);
	msg.add((const char *)toFill);
	getFieldContents(pstiThirtyString, toFill, 13);
	msg.add((const char *)toFill);
	getFieldContents(pstiThirtyString, toFill, 14);
	msg.add((const char *)toFill);
	getFieldContents(pstiThirtyString, toFill, 15);
	msg.add((const char *)toFill);

	//convert the OSC message to a string, produce the checksum and append to message
	Serial.println("Converting the message to a string...");
	oscMsg_to_string(buffer, &msg);
	for (int i = 0; i < strlen(buffer); i++)
	{
		crc.update(buffer[i]);
	}
	uint32_t checksum = crc.finalize();
	crc.reset();
	
#if DEBUG
	Serial.print("CHECKSUM: ");
	Serial.println(checksum);
#endif

	String check = String(checksum);
	check.toCharArray(sum, sizeof(sum));
	msg.add((const char *)sum);
}



bool verify(char *nmeaString)
{
	int length = strlen(nmeaString);
	if (length < 2)
	{
		return false;
	}
	if (nmeaString[length - 1] != '\n' || nmeaString[length - 2] != '\r')
	{
		Serial.print((int)nmeaString[length - 2]);
		Serial.print(",");
		Serial.println((int)nmeaString[length - 1]);
		return false;
		memset(nmeaString, '\0', BUFFER_SIZE + 1);
	}
	return true;
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

void sendMessage(OSCMessage *output, HardwareSerial &serialPort)
{
	serialPort.write(byte(2)); // start of text
	output->send(serialPort);
	serialPort.write(byte(4)); // End of transmission
}

void oscMsg_to_string(char *osc_string, OSCMessage *msg)
{
  char data_type;
  data_value value;
  int addr_len = 40;
  char addr_buf[addr_len];

  memset(osc_string, '\0', sizeof(osc_string));
  memset(addr_buf, '\0', addr_len);
  msg->getAddress(addr_buf, 0);
  strcat(osc_string, addr_buf);

  for (int j = 0; j < msg->size(); j++)
  {
    data_type = msg->getType(j);
    switch (data_type)
    {
    case 'f':
      value.f = msg->getFloat(j);
      snprintf(addr_buf, addr_len, ",f%lu", value.u);
      strcat(osc_string, addr_buf);
      break;

    case 'i':
      value.i = msg->getInt(j);
      snprintf(addr_buf, addr_len, ",i%lu", value.u);
      strcat(osc_string, addr_buf);
      break;

    case 's':
      char data_buf[40];
      msg->getString(j, data_buf, sizeof(data_buf));
      snprintf(addr_buf, addr_len, ",s%s", data_buf);
      strcat(osc_string, addr_buf);
      break;

    default:
      if (data_type != '\0')
      {
        LOOM_DEBUG_Println("Invalid message arg type");
      }
      break;
    }
  }
  if (msg != NULL)
    strcat(osc_string, " ");
}
