/*******************************************************************************
* File : main.c
* Summary : Modbus Protocol
*
*
* Author : Ganesh
*******************************************************************************/
//Include
#include "general.h"
#include "modbus.h"
//Local macro
#define	METER_POLL_INFO_PATH	"/usr/mrm/config"

int 			serFd;
int				socFdArray[MAX_METERS];
char			tmpBuf[SIZE_512];
unsigned char 	cnt[8];
unsigned char	readBuff[SIZE_2048];
unsigned int	parameterIndex,sLen;
char			liveParamNemes[MAX_PARAMS][SIZE_256];
unsigned char	procResponceFailedFlag,soc1ConnectedFlag,soc2ConnectedFlag;

FILE 					*fpt=NULL;
//Externs
extern unsigned short	numOfReqParams;
extern char 			paramNameList[];
extern int				sockFd;

extern MODBUS_CONFIG	modConfig;
extern MODBUS_DATA		modbusDataValues[];

//Structure variables
time_t				ltm;
time_t 				mytime;
struct tm 			tstamp;
METER_COM_CONFIG	meterComConfig;

/**
 *  Function    : parseParams
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void parseParams(void)
{
	char *cpt1=NULL;
	int i=0;
	
	memset((void*)&modConfig, 0, sizeof(MODBUS_CONFIG));
	i=0;
	cpt1 = tmpBuf;
	memset(cnt,0,8);
	while( *cpt1 != ',')
	{
		cnt[i++] = *cpt1;	
		cpt1++;
	}
	modConfig.nodeAddr = (unsigned char)atoi(cnt);
	
	cpt1++;
	i=0;
	memset(cnt,0,8);
	while( *cpt1 != ',')
	{
		cnt[i++] = *cpt1;	
		cpt1++;
	}
	modConfig.funcCode = (unsigned char)atoi(cnt);

	cpt1++;
	i=0;
	memset(cnt,0,8);
	while( *cpt1 != ',')
	{
		cnt[i++] = *cpt1;	
		cpt1++;
	}
	modConfig.startAddr = (unsigned short)atoi(cnt);
	
	cpt1++;
	i=0;
	memset(cnt,0,8);
	while( *cpt1 != ',')
	{
		cnt[i++] = *cpt1;	
		cpt1++;
	}
	modConfig.noOfRegis = (unsigned short)atoi(cnt);
	
	cpt1++;
	i=0;
	memset(cnt,0,8);
	while( *cpt1 != ',')
	{
		cnt[i++] = *cpt1;	
		cpt1++;
	}
	modConfig.byteOrder = (unsigned char)atoi(cnt);
	
	cpt1++;
	i=0;
	memset(cnt,0,8);
	while( *cpt1 != ',')
	{
		cnt[i++] = *cpt1;	
		cpt1++;
	}
	modConfig.wordOrder = (unsigned char)atoi(cnt);

	cpt1++;
	i=0;
	memset(cnt,0,8);
	while( *cpt1 != ',')
	{
		cnt[i++] = *cpt1;	
		cpt1++;
	}
	modConfig.regSize = (unsigned char)atoi(cnt);
	
	cpt1++;
	i=0;
	memset(cnt,0,8);
	//while( (*cpt1 != '\r') && (*cpt1 != '\0') && (*cpt1 != '\n') )
	while( *cpt1 != ',')
	{
		cnt[i++] = *cpt1;	
		cpt1++;
	}
	modConfig.typeOfValue = (unsigned char)atoi(cnt);

	return;
}

/**
 *  Function : createParamNameList
 *  return [out] : Return_Description
 *  details : Details
 */
void createParamNameList(void)
{
	char *token=NULL;
	char tBuff[SIZE_128];
	const char tStr[2] = ",";
	
	numOfReqParams=0;
	rewind(fpt);
	memset(liveParamNemes,0,(MAX_PARAMS * SIZE_256));
	memset(paramNameList,0,PARAM_NAME_LIST_LEN);
	strcat(paramNameList,"Time,");
	memset(tmpBuf,0,sizeof(tmpBuf));
	while( fgets(tmpBuf,sizeof(tmpBuf),fpt) != NULL )
	{
		fflush(fpt);
		if(strchr(tmpBuf,',') == NULL)
			continue;
		token = strtok(tmpBuf,tStr);
		while( token != NULL ) 
		{
			memset(tBuff,0,sizeof(tBuff));
			strcpy(tBuff,token);
			token = strtok(NULL, tStr);
		}
		
		tBuff[strlen(tBuff)-1] = '\0';
		strcpy(liveParamNemes[numOfReqParams],tBuff);
		strcat(paramNameList,tBuff);
		strcat(paramNameList,",");
		memset(tmpBuf,0,sizeof(tmpBuf));
		numOfReqParams++;
	}
	paramNameList[strlen(paramNameList)-1] = '\0';
	
	printf("paramNameList %d : %s\n\n",numOfReqParams,paramNameList);
	return;
}

/**
 *  Function    : main
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int main(void)
{
	int i=0,bRtate=0;
	int sLength=0;
	char configParms[SIZE_8];
	char fName[SIZE_64];
		
	printf("\n*** Modbus Master Ver 1.1.0 ***\n");
	if( readConfigFile() != RET_OK )
	{
		printf("Configuration Read Error..!\n");
		return RET_FAILURE;
	}
	
	if(meterComConfig.meterComType.selectedMtrcomType == RTU)
	{
		memset(configParms,0,SIZE_8);
		sprintf(configParms,"%d%c%d",meterComConfig.meterComType.serialPortConfig.databits,meterComConfig.meterComType.serialPortConfig.parity,meterComConfig.meterComType.serialPortConfig.stopbits);
		
		if( ( serFd = openPort(meterComConfig.meterComType.serialPortConfig.serialFile,meterComConfig.meterComType.serialPortConfig.baudRate,configParms) ) < 0 )
			return RET_FAILURE;
	}
	else if(meterComConfig.meterComType.selectedMtrcomType == TCP_IP)
	{
		while(1)
		{
			if(meterComConfig.numOfMtsConnected >= 1)
			{
				if( (socFdArray[0] = socConnect(meterComConfig.meterComType.tcpIpConfigSlave1.ipAddr,meterComConfig.meterComType.tcpIpConfigSlave1.port) ) < 0)
				{
					soc1ConnectedFlag = 0;
					socFdArray[0]=0;//return RET_FAILURE;
				}
				else
					soc1ConnectedFlag = 1;
			}

			if(meterComConfig.numOfMtsConnected == MAX_METERS )
			{
				if( (socFdArray[1] = socConnect(meterComConfig.meterComType.tcpIpConfigSlave2.ipAddr,meterComConfig.meterComType.tcpIpConfigSlave2.port) ) < 0)
				{
					soc2ConnectedFlag = 0;
					socFdArray[1]=0;//return RET_FAILURE;
				}
				else
					soc2ConnectedFlag = 1;
			}
			if( (soc1ConnectedFlag) || (soc2ConnectedFlag) )
				break;
			sleep(3);
		}
	}
	else
	{
		printf("Invalid configuration for meter communication..!\n");
		return RET_FAILURE;
	}
	
	while(TRUE)
	{
		for(i=1;i<=meterComConfig.numOfMtsConnected;i++)
		{
			memset(fName,0,sizeof(fName));
			sprintf(fName,"%s/%d_Config.list",METER_POLL_INFO_PATH,i);
			fpt=fopen(fName,"r");
			if(fpt==NULL)
			{
				printf("fopen:Error %s\n",fName);   
				return RET_FAILURE;             
			}
			//construct list of parameter names
			createParamNameList();
			
			rewind(fpt);
			memset(&modbusDataValues,0,sizeof(MODBUS_DATA));
			parameterIndex=0;
			while( fgets(tmpBuf,sizeof(tmpBuf),fpt) != NULL )
			{
				fflush(fpt);
				if(strchr(tmpBuf,',') == NULL)
					continue;
				
				parseParams();
				
				if(meterComConfig.meterComType.selectedMtrcomType == TCP_IP)
				{
					if(!soc1ConnectedFlag && (meterComConfig.numOfMtsConnected >= 1) )
					{
						printf("Socket 1 Not connected..Trying to reconnect..\n");
						if( (socFdArray[0] = socConnect(meterComConfig.meterComType.tcpIpConfigSlave1.ipAddr,meterComConfig.meterComType.tcpIpConfigSlave1.port) ) < 0)
						{
							soc1ConnectedFlag = 0;
							socFdArray[0]=0;//return RET_FAILURE;
						}
						else
							soc1ConnectedFlag = 1;
					}
					else if(!soc2ConnectedFlag && (meterComConfig.numOfMtsConnected == MAX_METERS) )
					{
						printf("Socket 2 Not connected..Trying to reconnect..\n");
						if( (socFdArray[1] = socConnect(meterComConfig.meterComType.tcpIpConfigSlave2.ipAddr,meterComConfig.meterComType.tcpIpConfigSlave2.port) ) < 0)
						{
							soc2ConnectedFlag = 0;
							socFdArray[1]=0;//return RET_FAILURE;
						}
						else
							soc2ConnectedFlag = 1;
					}
					sockFd = socFdArray[i-1];
				}

				if( ( initModQuery() != RET_OK) || ( procModResponce(parameterIndex++) != RET_OK ) )
				{
					printf("Proc Responce Failed\n");
					procResponceFailedFlag = 1;
				}
				else
					procResponceFailedFlag = 0;
			}
			fclose(fpt);
			
			if(!procResponceFailedFlag)
			{
				updateFile(0,FILE_1); //FILE_1 is a csv file which will update every 15 min
				updateFile(i,FILE_2); //FILE_2 is live data , this ia json file
			}
		}
		if(meterComConfig.numOfMtsConnected < 1)
			sleep(10); //for safty
		mytime = time(&ltm);
		localtime_r(&ltm,&tstamp);
		printf("Time : %d-%d-%d %d:%d:%d\n",tstamp.tm_mday,tstamp.tm_mon+1,tstamp.tm_year+1900,tstamp.tm_hour,tstamp.tm_min,tstamp.tm_sec );
		 //break;// remove
	}
	
	if(meterComConfig.meterComType.selectedMtrcomType == RTU)
		closePort(serFd);
	else if(meterComConfig.meterComType.selectedMtrcomType == TCP_IP)
		closeSocket();
	
	return RET_OK;
}

/* EOF */
