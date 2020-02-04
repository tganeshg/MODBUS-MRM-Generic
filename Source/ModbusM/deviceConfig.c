/*******************************************************************************
* File : deviceConfig.c
* Summary : Modbus Protocol
*
*
* Author : Ganesh
*******************************************************************************/
//Include
/*** Include ***/
#include "general.h"
#include "modbus.h"

/*** Local Macros ***/
#define METER_COM_CONFIG_FILE 	"/usr/mrm/config/meterComConfig.conf"

/*** Externs ***/
extern METER_COM_CONFIG	meterComConfig;

/*** Globals ***/

/*** Structure Variables ***/

/*** Functions ***/

int readConfigFile(void)
{
	int 			cfgFd;
	int 			numBytes;
	static char 	fn[] = "readConfigFile()";
	int				i=0;
	FILE 			*dbgfp;
	
	memset(&meterComConfig,0,sizeof(METER_COM_CONFIG));
	printf("In read config file %d\n",sizeof(METER_COM_CONFIG));

	if ( ( dbgfp = fopen(METER_COM_CONFIG_FILE,"r" )) == NULL )
	{
		printf("%s:%d:%s: Unable to open config file. error = %s Exiting..\n",__FILE__, __LINE__, fn, strerror(errno));
		return RET_FAILURE;

	}
	if ( ( numBytes = fread(&meterComConfig,sizeof(meterComConfig),1,dbgfp)) < 1 )
	{
		printf("%s:%d:%s: Failed to read cfg file error = %s Exiting.. Size of struct %d bytes read %d ",
										__FILE__, __LINE__, fn, strerror(errno),sizeof(meterComConfig),numBytes);
		fclose(dbgfp);
		return RET_FAILURE;
	}
	fclose(dbgfp);

	printf("Baudrate : %d\n",meterComConfig.meterComType.serialPortConfig.baudRate);
	printf("databits : %d\n",meterComConfig.meterComType.serialPortConfig.databits);
	printf("parity : %c\n",meterComConfig.meterComType.serialPortConfig.parity);
	printf("stopbits : %d\n",meterComConfig.meterComType.serialPortConfig.stopbits);
	printf("Connected Meters : %d\n",meterComConfig.numOfMtsConnected);
	printf("Slave Ip 1 : %s\n",meterComConfig.meterComType.tcpIpConfigSlave1.ipAddr);
	printf("Slave Port 1 : %d\n",meterComConfig.meterComType.tcpIpConfigSlave1.port);
	printf("Slave Ip 2 : %s\n",meterComConfig.meterComType.tcpIpConfigSlave2.ipAddr);
	printf("Slave Port 2 : %d\n",meterComConfig.meterComType.tcpIpConfigSlave2.port);
	return RET_OK;	
}

/* EOF */

