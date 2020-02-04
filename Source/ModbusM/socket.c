/*******************************************************************************
* File : socket.c
* Summary : Modbus Protocol
*
*
* Author : Ganesh
*******************************************************************************/
#include "general.h"

#define		MAX_RETRY_CNT		3
#define		MAX_CNT				3

//Globals
static char		socketMsg[SIZE_1024];
int 			sockFd;
//Externs
extern unsigned char 	readBuff[];

//Structure variables
struct sockaddr_in 		server;

//Functions

/**
 *  Function    : socConnect
 *  Params [in] : "ipAddr" Parameter_Description
 *  Params [in] : "port"   Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int socConnect(char *ipAddr,unsigned int port)
{
	int sFd = 0;
	
	if(sFd != 0)
		close(sFd);
	//Create socket
    sFd = socket(AF_INET , SOCK_STREAM , 0);
    if(sFd < 0)
    {
        printf("socConnect(): Could not create socket..!\n");
		return RET_FAILURE;
    }
    printf("socConnect(): Socket created\n");
     
    server.sin_addr.s_addr = inet_addr(ipAddr);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
 
    //Connect to remote server
    if (connect(sFd , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        printf("socConnect(): Connect failed. Error..!\n");
        return RET_FAILURE;
    }

    printf("socConnect(): Socket Connected\n");
	return sFd;
}

/**
 *  Function    : readSocket
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int readSocket(void)
{
	int totalBytesRead=0, unreadBytesPresent=0, bytesRead=0;
	int ret=0,j=0;
	unsigned char *trav = NULL;
	int waitCnt;
	
	trav = readBuff;
	waitCnt = 0;
	
	while ( waitCnt < MAX_CNT )
	{
		if(ioctl(sockFd,FIONREAD,&unreadBytesPresent) == -1)
		//if(ioctl(sockFd,TIOCINQ,&unreadBytesPresent) == -1)
		{
			printf("IOCTL failed - %s\n",strerror(errno)); 
			usleep(100000);
			waitCnt++;
			continue;
		}

		if(unreadBytesPresent > 0)
		{
			break;
		}
		else if(unreadBytesPresent == 0)
		{
			usleep(100000);
			waitCnt++;
		}
	}

	if ( waitCnt == MAX_CNT )
	{
		printf("No data to read in Serial Port\n"); 
		return RET_FAILURE;
	}
	
	while(1)
	{
		BACKREAD:
		memset(socketMsg, 0, SIZE_1024);
		if((bytesRead = recv(sockFd , (void*)socketMsg , SIZE_1024 , 0)) < 0)
		{
			printf( "Read failed - %s\n",strerror(errno)); 
			return RET_FAILURE;
		}

		// Copy the message into 'msg' and increment 'trav'
		memcpy((void *)trav, socketMsg, bytesRead);
		trav += bytesRead;
		totalBytesRead += bytesRead;

		if(ioctl(sockFd,FIONREAD,&unreadBytesPresent) == -1)
		//if(ioctl(sockFd,TIOCINQ,&unreadBytesPresent) == -1) //for cygwin only
		{
			printf("ioctl_1 failed - %s\n", strerror(errno)); 
			return RET_FAILURE;
		}

		if(unreadBytesPresent > 0)
		{
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
				if(ioctl(sockFd,FIONREAD,&unreadBytesPresent) == -1)
				//if(ioctl(sockFd,TIOCINQ,&unreadBytesPresent) == -1) //for cygwin only
				{
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
	return RET_OK;
}

/**
 *  Function    : sendSocket
 *  Params [in] : "sendBuff" Parameter_Description
 *  Params [in] : "nofBytes" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int sendSocket(char *sendBuff,unsigned int nofBytes)
{
	int i=0;
	
#if DEBUG_LOG
	printf("-> ");
	for(i=0;i<nofBytes;i++)
	{
		printf("[%02x] ",(unsigned char)sendBuff[i]);
	}
	printf("\n");
#endif
	//Send some data
	if( send(sockFd , sendBuff , nofBytes , 0) < 0)
	{
		printf("sendSocket(): Send failed\n");
		return RET_FAILURE;
	}
	return RET_OK;
}

/**
 *  Function    : closeSocket
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void closeSocket(void)
{
	if(sockFd != 0)
		close(sockFd);
	return;
}
/* EOF */
