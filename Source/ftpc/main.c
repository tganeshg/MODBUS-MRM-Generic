/*******************************************************************************
* File : main.c
* Summary : ftp client
*
*
* Author : Ganesh
*******************************************************************************/
//Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftplib.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#pragma pack(1)

//Define Macros
#define	SUCCESS					1
#define	FAILURE					0
#define	PERIODIC_TYPE			1
#define	SCH_TYPE				2
#define	VERSION					"FTPC 1.0.1 07July17"
#define	DATAPATH				"/usr/mrm/data"
#define FTPC_CONFIG_FILE 		"/usr/mrm/config/ftpcConfig.conf"

// Size constants
#define 	SIZE_8				8
#define 	SIZE_16				16
#define 	SIZE_32				32
#define 	SIZE_64				64
#define 	SIZE_128			128
#define 	SIZE_256			256
#define 	SIZE_512			512
#define 	SIZE_1024			1024

//Structures
/*** Structures belongs ro ftpc process ***/
typedef struct
{
	unsigned int hour;
	unsigned int min;
}FTP_FILE_TRANS_SCH;

typedef struct
{
	unsigned char transType; //1 periodic, 2 schedule
	unsigned char periodicInterval; // fixed - every 1hr/2hr/3hr/6hr/12hr
	FTP_FILE_TRANS_SCH fileTransSch; //if schedule use this
}FTP_FILE_TRANS_TYPE;

typedef struct
{
	char 			host[SIZE_32];
	unsigned int	port;
	char			userName[SIZE_128];
	char			password[SIZE_128];
	FTP_FILE_TRANS_TYPE fileTransType;
}FTPC_CONFIG;

//Globals
static char 	*months[12] = {"JANUARY","FEBRUARY","MARCH","APRIL","MAY","JUNE","JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"};
static 			netbuf *conn = NULL;
char			errBuff[SIZE_512];
int 			fileSent = -1;  // -1 for first time

time_t tictoc,now,configTime;
struct tm *today;
struct tm *yestDay;
struct tm *confTime;

FTPC_CONFIG			ftpcConfig;

/**
 *  Function    : createAdirAndPutFile
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int createAdirAndPutFile(void)
{
	char fileName[SIZE_64];
	char monthBuff[SIZE_64];
	char yearBuff[SIZE_8];
	char todayDt[SIZE_16];
	char yestDt[SIZE_16];
	int ret = 0;
	
	DIR *dp;
	struct dirent *p;

	//change directory
	if (!FtpChdir((const char *)"/",conn))
	{
		fprintf(stderr,"*.Unable to change directory %s\n",FtpLastResponse(conn));
		return -1;
	}
	//check the directory , if not create a directory 
    time(&tictoc); /* Get the current time */
    today = localtime(&tictoc); /* Read today structure */
	memset(monthBuff,0,sizeof(monthBuff));
	memset(yearBuff,0,sizeof(yearBuff));
    sprintf(yearBuff,"%d",today->tm_year+1900);
    sprintf(monthBuff,"%s",months[today->tm_mon]);

	ret = FtpMkdir((const char *)yearBuff,conn);
	if( (ret == SUCCESS ) || ( (ret == FAILURE) && (strstr(FtpLastResponse(conn),"file already exists") != NULL) ) )
	{
		//change directory
		if (!FtpChdir((const char *)yearBuff,conn))
		{
			fprintf(stderr,"1.Unable to change directory %s\n",FtpLastResponse(conn));
			return -1;
		}
		else
		{
			ret = FtpMkdir((const char *)monthBuff,conn);
			if( (ret == SUCCESS ) || ( (ret == FAILURE) && (strstr(FtpLastResponse(conn),"file already exists") != NULL) ) )
			{
				//change directory
				if (!FtpChdir((const char *)monthBuff,conn))
				{
					fprintf(stderr,"2.Unable to change directory %s\n",FtpLastResponse(conn));
					return -1;
				}
				else
				{
					memset(todayDt,0,sizeof(todayDt));
					sprintf(todayDt,"%d_%d_%d",today->tm_mday,today->tm_mon+1,today->tm_year+1900);
					//check Today's file in data directory
					if( (dp=opendir(DATAPATH)) == NULL)
					{
						fprintf(stderr,"1.Directory Open Error %s\n",strerror(errno));
						return -1;
					}

					while((p=readdir(dp))!=NULL)
					{
						if( strcmp(p->d_name,".") && strcmp(p->d_name,"..") )
						{
							if(strstr(p->d_name,todayDt) != NULL)
							{
								//put file
								memset(fileName,0,sizeof(fileName));
								sprintf(fileName,"%s/%s",DATAPATH,p->d_name);
								if(!FtpPut(fileName, p->d_name, FTPLIB_ASCII ,conn))
								{
									fprintf(stderr,"file put failure\n%s",FtpLastResponse(conn));
								}
								else
								{
									printf("Ftp file put success..%s\n",fileName);
									if(ftpcConfig.fileTransType.transType == SCH_TYPE)
										fileSent = today->tm_mday;
									else if(ftpcConfig.fileTransType.transType == PERIODIC_TYPE)
										fileSent = today->tm_min;
								}
							}
						}
					}
				}
			}
			else
			{
				fprintf(stderr,"%s - Directory Create failure\n",monthBuff);
				return -1;
			}
		}
	}
	else
	{
		fprintf(stderr,"%s - Directory Create failure\n",yearBuff);
		return -1;
	}

	//change directory
	if (!FtpChdir((const char *)"/",conn))
	{
		fprintf(stderr,"#.Unable to change directory %s\n",FtpLastResponse(conn));
		return -1;
	}
	//check the directory , if not create a directory 
	now = time(NULL);
	now = now - (24*60*60);
	yestDay = localtime(&now);
	memset(monthBuff,0,sizeof(monthBuff));
	memset(yearBuff,0,sizeof(yearBuff));
    sprintf(yearBuff,"%d",yestDay->tm_year+1900);
    sprintf(monthBuff,"%s",months[yestDay->tm_mon]);

	ret = FtpMkdir((const char *)yearBuff,conn);
	if( (ret == SUCCESS ) || ( (ret == FAILURE) && (strstr(FtpLastResponse(conn),"file already exists") != NULL) ) )
	{
		//change directory
		if (!FtpChdir((const char *)yearBuff,conn))
		{
			fprintf(stderr,"1.Unable to change directory %s\n",FtpLastResponse(conn));
			return -1;
		}
		else
		{
			ret = FtpMkdir((const char *)monthBuff,conn);
			if( (ret == SUCCESS ) || ( (ret == FAILURE) && (strstr(FtpLastResponse(conn),"file already exists") != NULL) ) )
			{
				//change directory
				if (!FtpChdir((const char *)monthBuff,conn))
				{
					fprintf(stderr,"2.Unable to change directory %s\n",FtpLastResponse(conn));
					return -1;
				}
				else
				{
					memset(yestDt,0,sizeof(yestDt));
					sprintf(yestDt,"%d_%d_%d",yestDay->tm_mday,yestDay->tm_mon+1,yestDay->tm_year+1900);
					//check yesterday's file in data directory
					if( (dp=opendir(DATAPATH)) == NULL)
					{
						fprintf(stderr,"1.Directory Open Error %s\n",strerror(errno));
						return -1;
					}

					while((p=readdir(dp))!=NULL)
					{
						if( strcmp(p->d_name,".") && strcmp(p->d_name,"..") )
						{
							if(strstr(p->d_name,yestDt) != NULL)
							{
								//put file
								memset(fileName,0,sizeof(fileName));
								sprintf(fileName,"%s/%s",DATAPATH,p->d_name);
								if(!FtpPut(fileName, p->d_name, FTPLIB_ASCII ,conn))
								{
									fprintf(stderr,"file put failure\n%s",FtpLastResponse(conn));
								}
								else
									printf("Ftp file put success..%s\n",fileName);
							}
						}
					}
				}
			}
			else
			{
				fprintf(stderr,"%s - Directory Create failure\n",monthBuff);
				return -1;
			}
		}
	}
	else
	{
		fprintf(stderr,"%s - Directory Create failure\n",yearBuff);
		return -1;
	}
	
	return 0;
}

/**
 *  Function : readConfigFile
 *  return [out] : Return_Description
 *  details : Details
 */
int readConfigFile(void)
{
	int 			cfgFd;
	int 			numBytes;
	static char 	fn[] = "readConfigFile()";
	int				i=0;
	FILE 			*dbgfp;
	
	memset(&ftpcConfig,0,sizeof(FTPC_CONFIG));
	printf("In read config file\n");

	if ( ( dbgfp = fopen(FTPC_CONFIG_FILE,"r" )) == NULL )
	{
		printf("%s:%d:%s: Unable to open config file. error = %s Exiting..\n",__FILE__, __LINE__, fn, strerror(errno));
		return -1;

	}
	if ( ( numBytes = fread(&ftpcConfig,sizeof(FTPC_CONFIG),1,dbgfp)) < 1 )
	{
		printf("%s:%d:%s: Failed to read cfg file error = %s Exiting.. Size of struct %d bytes read %d ",
										__FILE__, __LINE__, fn, strerror(errno),sizeof(FTPC_CONFIG),numBytes);
		fclose(dbgfp);
		return -1;
	}
	fclose(dbgfp);
	
	printf("UserName = %s\n", ftpcConfig.userName);
	printf("PassWord = %s\n", ftpcConfig.password);
	printf("FileTransType = %d\n", ftpcConfig.fileTransType.transType);
	printf("PeriodicInt = %d in min\n", ftpcConfig.fileTransType.periodicInterval);
	
	return 0;	
}
/**
 *  Function    : main
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int main(void)
{
	printf("\n<<<  BGM Ftp Client for file transfer  %s >>>\n",VERSION);
	
	//Get Configuration Parameters
	if( readConfigFile() != 0 )
	{
		printf("Configuration Read Error..!\n");
		return -1;
	}
	
	while( SUCCESS )
	{
		if(ftpcConfig.fileTransType.transType == SCH_TYPE)
		{
			time(&configTime); /* Get the current time */
			confTime = localtime(&configTime); /* Read today structure */
			if( (fileSent == -1) || 
			    ( (fileSent != confTime->tm_mday) && 
				  (confTime->tm_hour >= ftpcConfig.fileTransType.fileTransSch.hour) &&
				  (confTime->tm_min >= ftpcConfig.fileTransType.fileTransSch.min) ) )
			{
				printf("File Sending Date : %d\n",confTime->tm_mday);
				//Init client lib
				FtpInit();
				
				//connect to server
				if (!FtpConnect(ftpcConfig.host,&conn))
				{
					fprintf(stderr,"Unable to connect to node %s\n",ftpcConfig.host);
					return -1;
				}
				else
					printf("Ftp connect Success..\n");
				
				//Login
				if (!FtpLogin(ftpcConfig.userName,ftpcConfig.password,conn))
				{
					fprintf(stderr,"Login failure\n%s",FtpLastResponse(conn));
					return -1;
				}
				else
					printf("Ftp Login success..\n");
				
				if( createAdirAndPutFile() < 0 )
				{
					if (conn)
						FtpClose(conn);
					return -1;
				}
				
				if (conn)
					FtpClose(conn);
			}
			printf("File sent date : %d\n",fileSent);
		}
		else if(ftpcConfig.fileTransType.transType == PERIODIC_TYPE)
		{
			time(&configTime); /* Get the current time */
			confTime = localtime(&configTime); /* Read today structure */
			if( (fileSent == -1) || 
			    (((fileSent != confTime->tm_min)) && ( (confTime->tm_min % ftpcConfig.fileTransType.periodicInterval) == 0)) )
			{
				printf("File Sending Min : %d\n",confTime->tm_min);
				//Init client lib
				FtpInit();
				
				//connect to server
				if (!FtpConnect(ftpcConfig.host,&conn))
				{
					fprintf(stderr,"Unable to connect to node %s\n",ftpcConfig.host);
					return -1;
				}
				else
					printf("Ftp connect Success..\n");
				
				//Login
				if (!FtpLogin(ftpcConfig.userName,ftpcConfig.password,conn))
				{
					fprintf(stderr,"Login failure\n%s",FtpLastResponse(conn));
					return -1;
				}
				else
					printf("Ftp Login success..\n");
				
				if( createAdirAndPutFile() < 0 )
				{
					if (conn)
						FtpClose(conn);
					return -1;
				}
				
				if (conn)
					FtpClose(conn);
			}
			printf("File sent Min : %d\n",fileSent);
		}
		sleep(3);
	}
	
	return 0;
}

/* EOF */
