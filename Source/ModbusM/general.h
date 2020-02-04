#ifndef _GENERAL_H_
#define _GENERAL_H_

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <termios.h>
#include <pthread.h>
//#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/vfs.h>
#include <assert.h>

#include "enum.h"

/*
*Macros
*/
#define		DEBUG_LOG			1
#define		TRUE				1

#define		MAX_METERS			2

#define		DATA_FILE_PATH			"/usr/mrm/data"

#define		PARAM_NAME_LIST_LEN	8192  //Maximum 64 params with string lenth is 128 characters.

//check bit 
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define IS_LEAP_YEAR(Y)    ( ((Y)>0) && !((Y)%4) && ( ((Y)%100) || !((Y)%400) ) )
/*
*Structure
*/
/* Config Structure */
#pragma pack(1)
// Serial configuration
typedef struct
{
    int   			baudRate;     	// Baudrate of serial port
    unsigned char   databits;      	// Num of databits
    unsigned char   stopbits;      	// Num of stop bits
    char   			parity ;       	// Parity - ODD/EVEN/NONE
    unsigned char   handshake;     	// Handshake - SW/HW/NONE
    unsigned char   infMode;		// RS232 /485 //not used
	char			serialFile[SIZE_32]; // this will be "ttyUSB0" or "tts/x" , this should append with '/dev/'
}SER_PORT_CFG;

typedef struct
{
	char			ipAddr[SIZE_32];
	unsigned int	port;
}TCP_IP_CFG;

typedef struct
{
	unsigned int hour;
	unsigned int min;
}LOG_SCH;

typedef struct
{
	unsigned char 	logType; //1 periodic, 2 schedule
	unsigned char 	periodicInterval; // modbus: 1min to 60min , others yet to be decide 
	LOG_SCH 		logSch; //if schedule use this
}LOGGING_TYPE;

typedef struct
{
	SEL_MTR_COM_TYPE	selectedMtrcomType; // RTU(1) or TCP_IP(2)
	SER_PORT_CFG		serialPortConfig;
	TCP_IP_CFG			tcpIpConfigSlave1;
	TCP_IP_CFG			tcpIpConfigSlave2;
}METER_COM_TYPE;

typedef struct
{
	unsigned char	enableLoging;  // 0 - no need to log , 1 - Enable log -> to csv file
	unsigned int	numOfMtsConnected;
	LOGGING_TYPE	loggingType;
	METER_COM_TYPE	meterComType;
}METER_COM_CONFIG;

/*
*Function declarations
*/
int readLine(int fpt,char *buffer,size_t bufSize);
int readConfigFile(void);
//uart.c
int openPort(char *comPortNumber, int baudRate, const char *mode);
int sendPort(char *sendBuff,unsigned int nofBytes);
void closePort(int fd);
unsigned short crc16(char *data_p, unsigned short length);
int readPort(void);

void createParamNameList(void);
int writeIntoFile(char *fName);
void updateFile(int mIdx,TYPE_OF_FILE typeOfFile);
void constructValueString(void);
void writeLiveJsonFile(char *fileName);

//socket.c
int socConnect(char *ipAddr,unsigned int port);
int readSocket(void);
int sendSocket(char *sendBuff,unsigned int nofBytes);
void closeSocket(void);

#endif

/* EOF */
