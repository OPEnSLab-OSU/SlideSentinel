#include <CRC32.h>
#define DEBUG 1
#define MAX_LENGTH 150

CRC32 crc;

/*****************************************************
 * Function: 
 * Description: 5 for state
*****************************************************/
uint32_t functionHandler(char cmd[])
{
    uint32_t value = strtoul(cmd, NULL, 10);
    return value;
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
int stringRank(char indicator)
{
    switch (indicator)
    {
    case ('X'): //for the unintialized best string
        return -2;
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

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void print_Msg(char msg[])
{
    char buf[MAX_LENGTH];
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

/*****************************************************
 * Function: 
 * Description: 5 for state
*****************************************************/
char* getValueAt(char src[], int pos)
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

/*****************************************************
 * Function: 
 * Description: 5 for state
*****************************************************/
bool verifyMsg(char current[], uint8_t numFields)
{
    int count = 0;
    char buffer[MAX_LENGTH];
    char newMsg[MAX_LENGTH];
    char *chk;
    memset(buffer, '\0', sizeof(buffer));
    memset(newMsg, '\0', sizeof(newMsg));

    chk = getValueAt(current, numFields);
    if (chk == NULL)
        return false;

    strcpy(buffer, chk);

    uint32_t checksum = functionHandler(buffer);
    memset(buffer, '\0', sizeof(buffer));

#if DEBUG
    Serial.println();
    Serial.println("Message to verify: ");
    print_Msg(current);
    Serial.print("CHECKSUM: ");
    Serial.println(checksum);
#endif

    for (int i = 0; i < strlen(current); i++)
    {
        if (current[i] == ',')
        {
            count++;
            if (count == numFields)
                break;
        }
        newMsg[i] = current[i];
        crc.update(current[i]);
    }
    uint32_t test_checksum = crc.finalize();

#if DEBUG
    Serial.print("Calculated checksum: ");
    Serial.println(test_checksum);
#endif
    crc.reset();

    //packet intact
    if (test_checksum == checksum)
    {
#if DEBUG
        Serial.println("Checksums match, dropping checksum");
#endif
        memset(current, '\0', sizeof(current));
        strcpy(current, newMsg);
        return true;
    }

#if DEBUG
    Serial.println("Checksums don't match, tossing string");
#endif
    return false;
}

/*****************************************************
 * Function: 
 * Description:  
*****************************************************/
void compareNMEA(char current[], char best[])
{

#if DEBUG
    Serial.println();
    Serial.println("COMPARING: ");
    print_Msg(current);
    Serial.println("---- AND ----");
    print_Msg(best);
    Serial.println();
#endif

    char buf[MAX_LENGTH];
    char *ptr;
    memset(buf, '\0', sizeof(buf));
    strcpy(buf, getValueAt(current, 8));
    char modeCur = buf[0];
    memset(buf, '\0', sizeof(buf));

    ptr = getValueAt(best, 8);
    //check if this is the first packet sent this round and best is uninitialized
    if (ptr == NULL)
        buf[0] = 'X';
    else
        strcpy(buf, ptr);
    char modeBest = buf[0];
    memset(buf, '\0', sizeof(buf));

#if DEBUG
    Serial.print("Value of current mode: ");
    Serial.println(modeCur);
    Serial.print("Value of best mode: ");
    Serial.println(modeBest);
#endif

    if (stringRank(modeCur) > stringRank(modeBest))
    {
        memset(best, '\0', sizeof(best)); //delete the old best message
        strcpy(best, current);            //copy over the new best message

#if DEBUG
        Serial.println("New best position: ");
        print_Msg(best);
        Serial.println('\n');
#endif
    }
    else if (stringRank(modeCur) == stringRank(modeBest))
    {
        //check the RTK ratio
        if ((float)atof(getValueAt(current, 10)) > (float)atof(getValueAt(best, 10)))
        {
            memset(best, '\0', sizeof(best)); //delete the old best message
            strcpy(best, current);            //copy over the new best message
#if DEBUG
            Serial.println("Current string has a higher RTK ratio: ");
            print_Msg(best);
            Serial.println('\n');
#endif
        }
    }
}