/*******************************************************************************
* File : fileHandle.c
* Summary : Modbus Protocol
*
*
* Author : Ganesh
*******************************************************************************/
//Include
#include "general.h"
#include "modbus.h"
//Local macro
#define DATA_LIVE_FILE_PATH "/usr/mrm/live"
//Globals
char					fileName[SIZE_64];
char					fileNamePath[SIZE_128];
char					paramNameList[PARAM_NAME_LIST_LEN];
int 					lastUpdatedMtrId = -1;
char 					paramValueStringBuf[SIZE_2048];

FILE 					*fileDp;
//Externs
extern unsigned int		parameterIndex;
extern char				liveParamNemes[][SIZE_256];

extern MODBUS_CONFIG	modConfig;
extern MODBUS_DATA		modbusDataValues[];
extern METER_COM_CONFIG	meterComConfig;

extern time_t			ltm;
extern time_t 			mytime;
extern struct tm 		tstamp;
//Structure variables


/**
 *  Function    : updateFile
 *  Params [in] : "typeOfFile" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void updateFile(int mIdx,TYPE_OF_FILE typeOfFile)
{
	struct stat sb;
	struct tm *timeinfo;
	switch(typeOfFile)
	{
		case FILE_1:
		{
			mytime = time(&ltm);
			localtime_r(&ltm,&tstamp);
			printf("Write FILE_1 %d\n",tstamp.tm_min);
			if( (tstamp.tm_min % meterComConfig.loggingType.periodicInterval) == 0 )
			{
				if(lastUpdatedMtrId != modConfig.nodeAddr)
				{
					lastUpdatedMtrId = modConfig.nodeAddr;
					memset(fileName,0,sizeof(fileName));
					sprintf(fileName,"%d_%d_%d_%d.csv",modConfig.nodeAddr,tstamp.tm_mday,tstamp.tm_mon+1,tstamp.tm_year+1900);
					//Check file update status of current minute
					memset(fileNamePath,0,sizeof(fileNamePath));
					sprintf(fileNamePath,"%s/%s",DATA_FILE_PATH,fileName);
					if (stat(fileNamePath, &sb) < 0) 
					{
						perror("updateFile() : stat");
						writeIntoFile(fileNamePath);
						break;
					}
					else
					{
						timeinfo = localtime ( &sb.st_mtime );
						if(timeinfo->tm_min != tstamp.tm_min)
							writeIntoFile(fileNamePath);
						else
							printf("This file modified for this minute ..!\n");
					}
				}
			}
			else
			{
				lastUpdatedMtrId = -1;
			}
		}
		break;
		case FILE_2:
		{
			printf("Write FILE_2\n");
			memset(fileName,0,sizeof(fileName));
			sprintf(fileName,"%s/%d_liveFromMeter.json",DATA_LIVE_FILE_PATH,mIdx);
			writeLiveJsonFile(fileName);
		}
		break;
		default:
			assert(0);
		break;
	}
	return;
}

/**
 *  Function    : writeIntoFile
 *  Params [in] : "fName" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int writeIntoFile(char *fName)
{
	struct stat sb;
	
	if(stat(fName, &sb) == -1) 
	{
		printf("File Not Present.. Create new file..!\n");
		fileDp=fopen(fName,"a");
		if(fileDp==NULL)
		{
			printf("File is not created - %s\n",fName);
			return RET_FAILURE;
		}
		else
		{
			memset(paramValueStringBuf,0,sizeof(paramValueStringBuf));
			constructValueString();
			fprintf(fileDp,"%s\n",paramNameList); 
			fprintf(fileDp,"%s\n",paramValueStringBuf);
		}
	}
	else
	{
		printf("File is Present..!\n");
		fileDp=fopen(fName,"a");
		if(fileDp==NULL)
		{
			printf("File is not created - %s\n",fName);
			return RET_FAILURE;
		}
		else
		{
			memset(paramValueStringBuf,0,sizeof(paramValueStringBuf));
			constructValueString();
			fprintf(fileDp,"%s\n",paramValueStringBuf);
		}
	}
	fflush(fileDp);
	fclose(fileDp);
	printf("File update success...!!\n");
	return RET_OK;
}

/**
 *  Function : constructValueString
 *  return [out] : Return_Description
 *  details : Details
 */
void constructValueString(void)
{
	int i=0;
	char buffer[SIZE_32];
	
	mytime = time(&ltm);
	localtime_r(&ltm,&tstamp);
	memset(paramValueStringBuf,0,sizeof(paramValueStringBuf));
	sprintf(paramValueStringBuf,"%d-%d-%d %d:%d",tstamp.tm_mday,tstamp.tm_mon+1,tstamp.tm_year+1900,tstamp.tm_hour,tstamp.tm_min);
	
	for(i=0;i<parameterIndex;i++)
	{
		strcat(paramValueStringBuf,",");
		switch(modbusDataValues[i].storedType)
		{
			case IEEEFLOAT_32BIT:
			{
				memset(buffer,0,SIZE_32);
				sprintf(buffer,"%f",modbusDataValues[i].modData.floatValue);
				strcat(paramValueStringBuf,buffer);
			}
			break;
			case UINT_32BIT:
			{
				memset(buffer,0,SIZE_32);
				sprintf(buffer,"%ld",modbusDataValues[i].modData.unSingned32BitValue);
				strcat(paramValueStringBuf,buffer);
			}
			break;
			case INT_32BIT:
			{
				memset(buffer,0,SIZE_32);
				sprintf(buffer,"%ld",modbusDataValues[i].modData.singned32BitValue);
				strcat(paramValueStringBuf,buffer);
			}
			break;
			case UINT_16BIT:
			{
				memset(buffer,0,SIZE_32);
				sprintf(buffer,"%d",modbusDataValues[i].modData.unSingned16BitValue);
				strcat(paramValueStringBuf,buffer);
			}
			break;
			case INT_16BIT:
			{
				memset(buffer,0,SIZE_32);
				sprintf(buffer,"%d",modbusDataValues[i].modData.singned16BitValue);
				strcat(paramValueStringBuf,buffer);
			}
			break;
			default:
				printf("Getting Wrong Data Type..!\n");
				//assert(0);
			break;
		}
	}
	
	return;
}

/**
 *  Function : writeLiveJsonFile
 *  param [in] : fileName Parameter_Description
 *  return [out] : Return_Description
 *  details : Details
 */
void writeLiveJsonFile(char *fileName)
{
	int i=0;
	char buffer[SIZE_64];
	FILE *fp=NULL;

	fp = fopen(fileName,"w");
	if ( fp == NULL )
	{
		printf("writeLiveJsonFile():Failed to create file %s\n",strerror(errno));
		return;
	}
	
	fprintf(fp,"{\n\t");
	for(i=0;i<parameterIndex;i++)
	{
		switch(modbusDataValues[i].storedType)
		{
			case IEEEFLOAT_32BIT:
			{
				memset(buffer,0,SIZE_64);
				sprintf(buffer,"%f",modbusDataValues[i].modData.floatValue);
			}
			break;
			case UINT_32BIT:
			{
				memset(buffer,0,SIZE_64);
				sprintf(buffer,"%ld",modbusDataValues[i].modData.unSingned32BitValue);
			}
			break;
			case INT_32BIT:
			{
				memset(buffer,0,SIZE_64);
				sprintf(buffer,"%ld",modbusDataValues[i].modData.singned32BitValue);
			}
			break;
			case UINT_16BIT:
			{
				memset(buffer,0,SIZE_64);
				sprintf(buffer,"%d",modbusDataValues[i].modData.unSingned16BitValue);
			}
			break;
			case INT_16BIT:
			{
				memset(buffer,0,SIZE_64);
				sprintf(buffer,"%d",modbusDataValues[i].modData.singned16BitValue);
			}
			break;
			default:
				printf("Getting Wrong Data Type..!\n");
				//assert(0);
			break;
		}
		fprintf(fp,"\"%s\":\"%s\",\n\t",liveParamNemes[i],buffer);
	}
	
	fseek(fp,-3,SEEK_CUR);
	fprintf(fp,"\n}\n");
	fflush(fp);
	fclose(fp);
	
	memset(buffer,0,SIZE_64);
	sprintf(buffer,"cp %s /www/live/.",fileName);
	system(buffer);
	return;
}
/* EOF */

