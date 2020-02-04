/*******************************************************************************
* File : uart.c
* Summary : Modbus Protocol
*
*
* Author : Ganesh
*******************************************************************************/
#include "general.h"

#define		MAX_RETRY_CNT		3
#define		MAX_CNT				5

static char		serPortMsg[SIZE_1024];
//Globals
char			deviceName[SIZE_64];
char 			DateNTime[SIZE_32];

//Externs
extern unsigned char readBuff[];
extern int			 serFd;

//Structure variables
struct termios 	portSettings;
time_t 			timer;
struct tm* 		tm_info;

//Functions
/**
 *  Function    : openPort
 *  Params [in] : "comPortNumber" Parameter_Description
 *  Params [in] : "baudRate"      Parameter_Description
 *  Params [in] : "mode"          Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int openPort(char *comPortNumber, int baudRate, const char *mode)
{
	unsigned long baudr;
	int fd;
	int status,error;
	int cbits=CS8,cpar=0,ipar=IGNPAR,bstop=0;
	
	sprintf(deviceName,"/dev/%s",comPortNumber);
#if 0
	if((comPortNumber>37)||(comPortNumber<0))
	{
		printf("Invalid comport number\n");
		return RET_FAILURE;
	}
	else
	{
		memset(deviceName,0,SIZE_64);
		#ifdef USBFORMAT
		sprintf(deviceName,"/dev/ttyUSB%d",comPortNumber);
		#else
		sprintf(deviceName,"/dev/ttyS%d",comPortNumber);
		#endif
	}
#endif
	switch(baudRate)
	{
		case      50 : baudr = B50;
					   break;
		case      75 : baudr = B75;
					   break;
		case     110 : baudr = B110;
					   break;
		case     134 : baudr = B134;
					   break;
		case     150 : baudr = B150;
					   break;
		case     200 : baudr = B200;
					   break;
		case     300 : baudr = B300;
					   break;
		case     600 : baudr = B600;
					   break;
		case    1200 : baudr = B1200;
					   break;
		case    1800 : baudr = B1800;
					   break;
		case    2400 : baudr = B2400;
					   break;
		case    4800 : baudr = B4800;
					   break;
		case    9600 : baudr = B9600;
					   break;
		case   19200 : baudr = B19200;
					   break;
		case   38400 : baudr = B38400;
					   break;
		case   57600 : baudr = B57600;
					   break;
		case  115200 : baudr = B115200;
					   break;
		case  230400 : baudr = B230400;
					   break;
		case  460800 : baudr = B460800;
					   break;
		case  500000 : baudr = B500000;
					   break;
		case  576000 : baudr = B576000;
					   break;
		case  921600 : baudr = B921600;
					   break;
		case 1000000 : baudr = B1000000;
					   break;
		case 1152000 : baudr = B1152000;
					   break;
		default      : printf("Invalid baudRate\n");
					   return RET_FAILURE;
	}

	if(strlen(mode) != 3)
	{
		printf("Invalid mode %s\n", mode);
		return RET_FAILURE;
	}

	switch(mode[0])
	{
		case '8': cbits = CS8;
				  break;
		case '7': cbits = CS7;
				  break;
		case '6': cbits = CS6;
				  break;
		case '5': cbits = CS5;
				  break;
		default : printf("Invalid number of data-bits '%c'\n", mode[0]);
				  return RET_FAILURE;
	}

	switch(mode[1])
	{
		case 'N':
		case 'n': cpar = 0;
				  ipar = IGNPAR;
				  break;
		case 'E':
		case 'e': cpar = PARENB;
				  ipar = INPCK;
				  break;
		case 'O':
		case 'o': cpar = (PARENB | PARODD);
				  ipar = INPCK;
				  break;
		default : printf("Invalid parity '%c'\n", mode[1]);
				  return RET_FAILURE;
	}

	switch(mode[2])
	{
		case '1': bstop = 0;
				  break;
		case '2': bstop = CSTOPB;
				  break;
		default : printf("Invalid number of stop bits '%c'\n", mode[2]);
				  return RET_FAILURE;
	}

	fd = open(deviceName, O_RDWR | O_NOCTTY | O_NDELAY );
	//fd = open(deviceName, O_RDWR | O_NOCTTY);
	if(fd == RET_FAILURE)
	{
		printf("Unable to open comport - %s\n",deviceName);
		return RET_FAILURE;
	}
	else
	{
		printf("%s Port opened success\n\n",comPortNumber);
	}

	memset(&portSettings, 0, sizeof(portSettings));  /* clear the new struct */

	portSettings.c_cflag = cbits | cpar | bstop | CLOCAL | CREAD;
	portSettings.c_iflag = ipar;
	portSettings.c_oflag = 0;
	portSettings.c_lflag = 0;
	portSettings.c_cc[VMIN] = 0;      /* block untill n bytes are received */
	portSettings.c_cc[VTIME] = 0;     /* block untill a timer expires (n * 100 mSec.) */

	cfsetispeed(&portSettings, baudr);
	cfsetospeed(&portSettings, baudr);
	
	tcflush(fd, TCIFLUSH);
	error = tcsetattr(fd, TCSANOW, &portSettings);
	if(error==-1)
	{
		close(fd);
		printf("Unable to set port settings \n");
		return RET_FAILURE;
	}

	if(ioctl(fd, TIOCMGET, &status) == RET_FAILURE)
	{
		printf("Unable to get port status\n");
		return RET_FAILURE;
	}

	status |= TIOCM_DTR;    /* turn on DTR */
	status |= TIOCM_RTS;    /* turn on RTS */

	if(ioctl(fd, TIOCMSET, &status) == RET_FAILURE)
	{
		printf("Unable to set port status \n");
		return RET_FAILURE;
	}
	return fd;
}

/**
 *  Function    : sendPort
 *  Params [in] : "sendBuff" Parameter_Description
 *  Params [in] : "nofBytes" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int sendPort(char *sendBuff,unsigned int nofBytes)
{
	int n=0,i=0;
	
#if DEBUG_LOG
	printf("-> ");
	for(i=0;i<nofBytes;i++)
	{
		printf("[%02x] ",(unsigned char)sendBuff[i]);
	}
	printf("\n");
#endif
	n = write(serFd, sendBuff, nofBytes);
	if (n < 0)
	{
		printf("write() of bytes failed!\n");
		return RET_FAILURE;
	}
	//usleep(200000);
	usleep(5000);
	return RET_OK;
}

/**
 *  Function    : readPort
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int readPort(void)
{
	unsigned short int fCnt =0;
	int i=0,n=0,j=0;
	//char ch =0;
	
	int				totalBytesRead=0, unreadBytesPresent=0, bytesRead=0;
	char			ch=0;
	unsigned char	*trav = NULL;
	int				waitCnt;
	static char		*fn="readPort()";

	trav = readBuff;
	waitCnt = 0;

	//check initially if bytes are there to read
	while ( waitCnt < MAX_CNT )
	{
		// Check if there are more chars to read
		if(ioctl(serFd,FIONREAD,&unreadBytesPresent) == -1)
		//if(ioctl(serFd,TIOCINQ,&unreadBytesPresent) == -1)
		{
			// Do some logging, set state, re-initialise timers and return
			printf("IOCTL failed - %s\n",strerror(errno)); 
			usleep(100000);
			waitCnt++;
			continue;
		}

		if(unreadBytesPresent > 0)
		{
			// Check if reached the maximum of the array
			// in which case, possible the sensor is
			// spewing junk characters
			break;
		}
		else if(unreadBytesPresent == 0)
		{
			// There are no further bytes present, 
			// lets sleep for sometime and then try
			// again - numCharTimeout is in microsecs
			usleep(100000);
			waitCnt++;
		}
	}

	if ( waitCnt == MAX_CNT )
	{
		// Do some logging, set state, re-initialise timers and return
		printf("No data to read in Serial Port\n"); 
		return RET_FAILURE;
	}
	
	while(1)
	{
		BACKREAD:
		memset(serPortMsg, 0, SIZE_1024);
		if((bytesRead = read(serFd,serPortMsg,SIZE_1024)) == -1)
		{
			// Do some logging, set state, re-initialise timers and return
			printf( "Read failed - %s\n",strerror(errno)); 
			return RET_FAILURE;
		}

		// Copy the message into 'msg' and increment 'trav'
		memcpy((void *)trav, serPortMsg, bytesRead);
		trav += bytesRead;
		totalBytesRead += bytesRead;

		// Check if there are more chars to read
		if(ioctl(serFd,FIONREAD,&unreadBytesPresent) == -1)
		//if(ioctl(serFd,TIOCINQ,&unreadBytesPresent) == -1) //for cygwin only
		{
			// Do some logging, set state, re-initialise timers and return
			printf("ioctl_1 failed - %s\n", strerror(errno)); 
			return RET_FAILURE;
		}

		if(unreadBytesPresent > 0)
		{
			// Check if reached the maximum of the array
			// in which case, possible the sensor is
			// spewing junk characters
			if(totalBytesRead == SIZE_1024)
			{
				printf("Serial read Buffer overflow\n"); 
				break;
			}
			else 
				continue;
		}
		else if(unreadBytesPresent == 0)
		{
			waitCnt=0;
			while ( waitCnt < MAX_RETRY_CNT )
			{
				// Check if there are more chars to read
				if(ioctl(serFd,FIONREAD,&unreadBytesPresent) == -1)
				//if(ioctl(serFd,TIOCINQ,&unreadBytesPresent) == -1) //for cygwin only
				{
					// Do some logging, set state, re-initialise timers and return
					printf("IOCTL failed - %s\n",strerror(errno)); 
					usleep(100000);
					waitCnt++;
					continue;
				}

				if(unreadBytesPresent > 0)
				{
					waitCnt=0;
					goto BACKREAD;
				}
				else if(unreadBytesPresent == 0)
				{
					usleep(100000);
					waitCnt++;
				}
			}
			if ( waitCnt == MAX_RETRY_CNT )
				break;
		}
	}
	//printf("No #of bytes read %d\n",totalBytesRead); 

#if DEBUG_LOG
	if(totalBytesRead != 0)
	{
		printf("\n<- ");
		for(j=0;j<totalBytesRead;j++)
		{
			printf("[%02x] ",(unsigned char)readBuff[j]);
			/* if( ((j%8) == 0 )&& (j != 0) )  
				printf("\n"); */
		}
		printf("\n");
	}
#endif
	return totalBytesRead;
}

/**
 *  Function    : closePort
 *  Params [in] : "fd" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void closePort(int fd)
{
	if(fd != 0)
		close(fd);
}

/**
 *  Function    : PrintDateTime
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void PrintDateTime(void)
{
	time(&timer);
	tm_info = localtime(&timer);
	strftime(DateNTime, 19, "%d_%m_%Y_%H_%M_%S", tm_info);
	//puts(DateNTime);
	return;
}

/* EOF */
