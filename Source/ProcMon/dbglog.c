/*********************************************************************************************
* Copyright (c) 2017 All Right Reserved
*
* Author	  :	Ganesh
* File		  :	dbglog.c
* Summary	  :	MRM / Pmon
* Note  	  : If need to add new PROCs , touch "fillProcCfgFile()" function.
***********************************************************************************************/

/*** Include ***/
#include "general.h"

/*** Local Macros ***/
//#define		FILE_SIZE_EXCEED		4194304 //4 MB
#define		FILE_SIZE_EXCEED		1048576 //1 MB
#define		OFFSET_FILE				"/usr/mrm/log/Pmon"
#define     LOG_FILE_NAME			"/usr/mrm/log/Pmon"

/*** Externs ***/
extern unsigned char 	poweronFlag;

/*** Globals ***/
static	char	*dbgMsgs[5] = { "[Inform]", "[Warning]", "[Severe]","[Fatal]", "[Report]" };
char			msgStr[SIZE_1024],dbgBuf[SIZE_1024],filePath[SIZE_64];
unsigned long	OffSet, len;
FILE 			*dbgFPtr, *OffsetFp;
int				fd,fdOffset;
/*** Structure Variables ***/
struct stat		fileStat ;
struct stat 	OffSetFileStat ;

/*** Functions ***/

/**
 *  Function    : dbgLog
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int dbgLog(int mode,const char *format, ...)
{
	unsigned int done = 0;
	va_list arg;

	memset(dbgBuf,0,SIZE_1024);
	va_start (arg, format);
	done = vsprintf ((char *)dbgBuf, (const char *)format, arg);
	printf("%s:%s",dbgMsgs[ mode ],dbgBuf);
	dbgStr(mode,dbgBuf);
	va_end(arg);
	
	return done;
}

/**
 *  Function    : dbgStr
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void dbgStr(int mode, char *str1)
{
	time_t		timeSinceEpochInSecs=0;
	struct tm	*currTimeTm;
	char		timeStr[SIZE_64];
		
	timeSinceEpochInSecs = time(NULL);
	currTimeTm = localtime(&timeSinceEpochInSecs);
	strftime(timeStr, SIZE_64,  "%d_%b_%Y_%H_%M_%S", currTimeTm);
	
	memset(msgStr,0,SIZE_1024);
	strcpy(msgStr, timeStr);
	strcat(msgStr, ":");
	strcat(msgStr, dbgMsgs[ mode ]);
	strcat(msgStr, ":");
	strcat(msgStr, str1);
	
	writeDbgLog(msgStr);
}

/**
 *  Function    : writeDbgLog
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int writeDbgLog(char *data)
{
	int		res=0;
	static char fn[]="writeDbgLog()";

	len = strlen(data);
	if(poweronFlag)
	{
		memset(filePath,0,SIZE_64);
		sprintf(filePath,"%s/debug.log",LOG_FILE_NAME);
		res = stat(filePath, &fileStat);
		if(res < 0)
		{
			printf("%s: File not found\n", fn);
			/*Log file create */
			fd = open(filePath,O_RDWR | O_CREAT , 0777  );
			if(fd <0)
			{
				printf("open call failed for %s\n",filePath);
				return RET_FAILURE;
			}
			dbgFPtr = fdopen(fd,"w+");
		}
		else
		{
			/*Log file open */
			fd = open(filePath,O_RDWR , 0777  );
			if(fd <0)
			{
				printf("open call failed for %s\n",filePath);
				return RET_FAILURE;
			}
			
			dbgFPtr = fdopen(fd,"w+");
		}
		poweronFlag=0;
	}
	//check OffSet when its value is 0.
	if(OffSet == 0)
		checkForOffset();
	
	if( ((OffSet + len) > FILE_SIZE_EXCEED))
	{
		OffSet = 0;
		printf("File Size Exceed...Data Rolled to beginning\n");
		printf("Offset: %ld,  File Size: %ld\n",OffSet,(long int)fileStat.st_size);
	}
	
	fflush(dbgFPtr);
	fseek(dbgFPtr, OffSet, SEEK_SET);
	fprintf(dbgFPtr, "%s\n",data);
	fflush(dbgFPtr);
	OffSet  = OffSet + len;
	writeOffsetInFile();
	return RET_OK;
}

/**
 *  Function    : checkForOffset
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int	checkForOffset(void)
{
	memset(filePath,0,SIZE_64);
	sprintf(filePath,"%s/OffsetFile.txt",OFFSET_FILE);
    if(stat(filePath, &OffSetFileStat) < 0) 
	{
		fdOffset = open(filePath,O_RDWR | O_CREAT , 0777  );
		if(fdOffset < 0)
		{
			printf("Create %s failed\n",filePath);
			return RET_FAILURE;
		}
	}
	else
	{
		fdOffset = open(filePath,O_RDWR , 0777  );
		if(fdOffset <0)
		{
			printf("open call fail for %s\n",filePath);
			return RET_FAILURE;
		}
	}
	
	OffsetFp = fdopen(fdOffset,"w+");

	fseek(OffsetFp, 0, SEEK_SET);
	fscanf(OffsetFp, " %ld", &OffSet);
	
	if(OffSet >= FILE_SIZE_EXCEED)
		OffSet = 0;
		
	if (OffSet < 0 )
		OffSet = 0;
	
	return RET_OK;
}

/**
 *  Function    : writeOffsetInFile
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int writeOffsetInFile(void)
{
	fseek(OffsetFp, 0, SEEK_SET);
	fprintf(OffsetFp, "%ld", OffSet);
	fflush(OffsetFp);
	
	return RET_OK;
}

/**
 *  Function    : formatDbgLogMsg
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void formatDbgLogMsg(char *msg, int len)
{
	int 	i=0, maxIdx=0, msgIdx=0, sLen=0;
	char	tmpMsg[SIZE_64];

	memset((void*)tmpMsg,0,SIZE_64);
	while(len > 0)
	{
		if(len > SIZE_64)
			maxIdx = SIZE_64;
		else
			maxIdx = len;

		for(i=0; i<maxIdx; i++)
		{
			sLen = strlen(tmpMsg);
			sprintf( (char*)&tmpMsg[sLen],"[%02x] ", (unsigned char)msg[msgIdx++]);
			if( (i != 0) && ( ((i+1)%8) == 0 ) )
			{
				sLen = strlen(tmpMsg);
				sprintf( (char*)&tmpMsg[sLen],"\n");
				dbgLog(INFORM,tmpMsg);
				memset((void*)tmpMsg,0,SIZE_64);
			}
		}
		len = len - maxIdx;
	}

	sLen = strlen(tmpMsg);
	sprintf( (char*)&tmpMsg[sLen],"\n");
	dbgLog(INFORM,tmpMsg);
	return;
}

/* EOF */
