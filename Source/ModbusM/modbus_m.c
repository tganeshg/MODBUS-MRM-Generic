/*******************************************************************************
* File : modbus.c
* Summary : Modbus Protocol
*
*
* Author : Ganesh
*******************************************************************************/
//Include
#include "general.h"
#include "modbus.h"

//Local macro
#define	FOR_TEMP	0
//Globals
unsigned char			modQueryBuff[SIZE_32];
unsigned short 			tmpCrc;
unsigned short			numOfReqParams,tcpTransId,tcpLength=6;
//Externs
extern unsigned char	readBuff[];
extern char				liveParamNemes[][SIZE_256];

extern METER_COM_CONFIG	meterComConfig;

//Structure variables
MODBUS_CONFIG	modConfig;
MODBUS_DATA		modbusDataValues[MAX_PARAMS];

/**
 *  Function    : initModQuery
 *  Return [out]: Return_Description
 *  Details     : It will assign the values of parameters and construct the modbus query	
 */
int initModQuery(void)
{
	unsigned char index=0,i=0;
	unsigned char *ptr=NULL;
	
	if(meterComConfig.meterComType.selectedMtrcomType == RTU)
	{
		memset(modQueryBuff,0,sizeof(modQueryBuff));
		#if FOR_TEMP
		modConfig.nodeAddr = 1;
		modConfig.funcCode = READ_DISCRETE_INPUT;
		modConfig.startAddr = 0;
		modConfig.noOfRegis = 6; // quantity
		modConfig.byteOrder = HIGH_LOW_BYTE;
		modConfig.wordOrder = HIGH_LOW_WORD;
		modConfig.regSize = BIT_16;
		modConfig.typeOfValue = INT_16BIT;
		#endif
		modQueryBuff[index++] = modConfig.nodeAddr;
		modQueryBuff[index++] = modConfig.funcCode;
		
		ptr = (unsigned char *)&modConfig.startAddr;
		modQueryBuff[index++] = ptr[1];
		modQueryBuff[index++] = ptr[0];

		ptr = (unsigned char *)&modConfig.noOfRegis;
		modQueryBuff[index++] = ptr[1];
		modQueryBuff[index++] = ptr[0];
		
		tmpCrc = crc( modQueryBuff,0,(int)index);
		ptr = (unsigned char *)&tmpCrc;
		modQueryBuff[index++] = ptr[1];
		modQueryBuff[index++] = ptr[0];

		if(sendPort(modQueryBuff,index) != RET_OK)
			return RET_FAILURE;
	}
	else if(meterComConfig.meterComType.selectedMtrcomType == TCP_IP)
	{
		memset(modQueryBuff,0,sizeof(modQueryBuff));
		ptr = (unsigned char *)&tcpTransId;
		modQueryBuff[index++] = ptr[1];
		modQueryBuff[index++] = ptr[0];
		
		modQueryBuff[index++] = 0;
		modQueryBuff[index++] = 0;
	
		ptr = (unsigned char *)&tcpLength;
		modQueryBuff[index++] = ptr[1];
		modQueryBuff[index++] = ptr[0];
		
		modQueryBuff[index++] = modConfig.nodeAddr;
		modQueryBuff[index++] = modConfig.funcCode;
		
		ptr = (unsigned char *)&modConfig.startAddr;
		modQueryBuff[index++] = ptr[1];
		modQueryBuff[index++] = ptr[0];

		ptr = (unsigned char *)&modConfig.noOfRegis;
		modQueryBuff[index++] = ptr[1];
		modQueryBuff[index++] = ptr[0];

		if(sendSocket(modQueryBuff,index) != RET_OK)
			return RET_FAILURE;

	}
	else
		assert(0); //control should not come here
	
	return RET_OK;
}

/**
 *  Function : procModResponce
 *  param [in] : paramIndx Parameter_Description
 *  return [out] : Return_Description
 *  details : Details
 */
int procModResponce(unsigned int paramIndx)
{
	char fnCode=0;
	int  temp =0,ret=0,i=0,j=0;
	unsigned char *ptr=NULL;
	unsigned short locCrc=0,coilInx=0;
	unsigned char index=0;
	short int paramValue1=0;
	unsigned short int paramValue2=0;
	float paramValue3=0.0;
	unsigned long paramValue4=0;
	long int paramValue5=0;
	memset((void *)readBuff,0,SIZE_2048);
	
	if(meterComConfig.meterComType.selectedMtrcomType == RTU)
	{
		while((ret = readPort()) == 0 ) 
		{
			usleep(1000);
			temp++;
			if(temp == 100)
				break;
		}
		if(temp >= 100)
		{
			printf("Responce timeout \n");
			return RET_FAILURE;
		}
		
		//printf("Received %d\n",ret);
		//check CRC first
		tmpCrc = crc(readBuff,0,(int)(ret-2));
		ptr = (unsigned char *)&locCrc;
		ptr[1] = readBuff[ret-2];
		ptr[0] = readBuff[ret-1];
		
		//printf("%x %x\n",tmpCrc,locCrc);
		if(tmpCrc != locCrc)
		{
			printf("CRC Error \n");
			return RET_FAILURE;
		}
		//Check Node address
		index=0;
		//printf("Node addr %d\n",(unsigned char)readBuff[index]);
		index++;
	}
	else if(meterComConfig.meterComType.selectedMtrcomType == TCP_IP)
	{
		if( readSocket() != RET_OK )
		{
			printf("Invalid Responce\n");
			return RET_FAILURE;
		}
		ptr = (unsigned char *)&locCrc;
		ptr[1] = readBuff[0];
		ptr[0] = readBuff[1];
		
		//printf("id : %d %d\n",locCrc,tcpTransId);
		if(locCrc != tcpTransId)
		{
			printf("Transaction Id Mismatched \n");
			return RET_FAILURE;
		}
		else
			tcpTransId++;
		
		//Check Node address
		index=0;
		//printf("Node addr %d\n",(unsigned char)readBuff[index+6]);
		index+=7;
	}
	else
		assert(0); //control should not come here

	//Take Functionn Code
	fnCode = (unsigned char)readBuff[index++];
	if(CHECK_BIT(fnCode,8))
	{
		printf("Exception %02x \n",(unsigned char)readBuff[index]);
		return RET_FAILURE;
	}
	
	switch(fnCode)
	{
		case READ_DISCRETE_COIL:
		case READ_DISCRETE_INPUT:
		{
			temp = (int)readBuff[index++];
			printf("No#Bytes : %d\n",temp);
			coilInx = modConfig.startAddr;
			for(i=0;i<temp;i++,index++)
			{
				for(j=0;j<8;j++)
				{
					printf("Coil %d : %d\n",coilInx++,(CHECK_BIT(readBuff[index],j)?1:0));
					if((coilInx - modConfig.startAddr) >= modConfig.noOfRegis)
						return RET_OK;
				}
			}
		}
		break;
		case READ_HOLDING_REG:
		case READ_INPUT_REG:
		{
			temp = (int)readBuff[index++];
			printf("No#Bytes : %d\n",temp);
			if(modConfig.regSize == BIT_32)
			{
				if(modConfig.typeOfValue == IEEEFLOAT_32BIT)
				{
					for(i=0;i<modConfig.noOfRegis;i++)
					{
						ptr = (unsigned char *)&paramValue3;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						printf("Reg %d = %f\n",(modConfig.startAddr+i),paramValue3);
						modbusDataValues[paramIndx].storedType = IEEEFLOAT_32BIT;
						modbusDataValues[paramIndx].modData.floatValue = paramValue3;
					}
				}
				else if(modConfig.typeOfValue == UINT_32BIT)
				{
					for(i=0;i<modConfig.noOfRegis;i++)
					{
						ptr = (unsigned char *)&paramValue4;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						printf("Reg %d = %lu\n",(modConfig.startAddr+i),(unsigned long)paramValue4);
						modbusDataValues[paramIndx].storedType = UINT_32BIT;
						modbusDataValues[paramIndx].modData.unSingned32BitValue = (unsigned long int)paramValue4;
					}
				}
				else if(modConfig.typeOfValue == INT_32BIT)
				{
					for(i=0;i<modConfig.noOfRegis;i++)
					{
						ptr = (unsigned char *)&paramValue5;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						printf("Reg %d = %ld\n",(modConfig.startAddr+i),paramValue5);
						modbusDataValues[paramIndx].storedType = INT_32BIT;
						modbusDataValues[paramIndx].modData.singned32BitValue = (long int)paramValue5;
					}
				}
				else
					printf("Type of value incorrect..! 32 bit\n");
			}
			else
			{
				if(modConfig.typeOfValue == UINT_16BIT)
				{
					for(i=0;i<modConfig.noOfRegis;i++)
					{
						ptr = (unsigned char *)&paramValue2;
						if(modConfig.byteOrder == LOW_HIGH_BYTE)
						{
							ptr[0] = readBuff[index++];
							ptr[1] = readBuff[index++];
						}
						else
						{
							ptr[1] = readBuff[index++];
							ptr[0] = readBuff[index++];
						}
						printf("Reg %d = %d\n",(modConfig.startAddr+i),paramValue2);
						modbusDataValues[paramIndx].storedType = UINT_16BIT;
						modbusDataValues[paramIndx].modData.unSingned16BitValue = (unsigned short int)paramValue2;
					}
				}
				else if(modConfig.typeOfValue == INT_16BIT)
				{
					for(i=0;i<modConfig.noOfRegis;i++)
					{
						ptr = (unsigned char *)&paramValue1;
						if(modConfig.byteOrder == LOW_HIGH_BYTE)
						{
							ptr[0] = readBuff[index++];
							ptr[1] = readBuff[index++];
						}
						else
						{
							ptr[1] = readBuff[index++];
							ptr[0] = readBuff[index++];
						}
						printf("Reg %d = %d\n",(modConfig.startAddr+i),paramValue1);
						modbusDataValues[paramIndx].storedType = INT_16BIT;
						modbusDataValues[paramIndx].modData.singned16BitValue = (short int)paramValue1;
					}
				}
				else if(modConfig.typeOfValue == IEEEFLOAT_32BIT)
				{
					for(i=0;i<(modConfig.noOfRegis/2);i++)
					{
						ptr = (unsigned char *)&paramValue3;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						printf("Reg %d = %f\n",(modConfig.startAddr+(i*2)),paramValue3);
						modbusDataValues[paramIndx].storedType = IEEEFLOAT_32BIT;
						modbusDataValues[paramIndx].modData.floatValue = paramValue3;
					}
				}
				else if(modConfig.typeOfValue == UINT_32BIT)
				{
					for(i=0;i<(modConfig.noOfRegis/2);i++)
					{
						ptr = (unsigned char *)&paramValue4;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						printf("Reg %d = %lu\n",(modConfig.startAddr+(i*2)),(unsigned long)paramValue4);
						modbusDataValues[paramIndx].storedType = UINT_32BIT;
						modbusDataValues[paramIndx].modData.unSingned32BitValue = (unsigned long int)paramValue4;
					}
				}
				else if(modConfig.typeOfValue == INT_32BIT)
				{
					for(i=0;i<(modConfig.noOfRegis/2);i++)
					{
						ptr = (unsigned char *)&paramValue5;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						printf("Reg %d = %ld\n",(modConfig.startAddr+(i*2)),paramValue5);
						modbusDataValues[paramIndx].storedType = INT_32BIT;
						modbusDataValues[paramIndx].modData.singned32BitValue = (long int)paramValue5;
					}
				}
				else
					printf("Type of value incorrect..! 16 bit\n");
			}
		}
		break;
		default:
			printf("Invalid Function code \n");
		break;
	}
	return RET_OK;
}

/**
 *  Function    : crc
 *  Params [in] : "data"  Parameter_Description
 *  Params [in] : "start" Parameter_Description
 *  Params [in] : "cnt"   Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : CRC calculation
 */
unsigned short int crc( unsigned char *data, int start, int cnt)
{
	int         i=0,j=0;
	unsigned    temp,temp2,flag;

	temp=0xFFFF;

	for ( i=start; i<cnt; i++ )
	{
		temp=temp ^ data[i];
		for ( j=1; j<=8; j++ )
		{
			flag=temp & 0x0001;
			temp=temp >> 1;
			if ( flag ) 
				temp=temp ^ 0xA001;
		}
	}

	// Reverse byte order. 
	temp2=temp >> 8;
	temp=(temp << 8) | temp2;
	temp &= 0xFFFF;

	return(temp);
}

/* EOF */
