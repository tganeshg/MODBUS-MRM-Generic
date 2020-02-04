/*********************************************************************************************
* Copyright (c) 2017 All Right Reserved
*
* Author	  :	Ganesh
* File		  :	main.c
* Summary	  :	MRM / Pmon
* Note  	  : 
***********************************************************************************************/

/*** Include ***/
#include "general.h"

/*** Local Macros ***/
#define METER_COM_CONFIG_FILE 	"/usr/mrm/config/meterComConfig.conf"
#define FTPC_CONFIG_FILE 		"/usr/mrm/config/ftpcConfig.conf"
#define MQTTC_CONFIG_FILE 		"/usr/mrm/config/mqttcConfig.conf"
#define	CONFIG_INPUT_FILENAME	"/usr/mrm/config/mrmConfig.json"
#define	METER_POLL_INFO_PATH	"/usr/mrm/config"
#define	METER_CONFIG_STRUCT		0x01
#define	FTPC_CONFIG_STRUCT		0x02
#define	MQTTC_CONFIG_STRUCT		0x03

/*** Externs ***/

/*** Globals ***/

/*** Structure Variables ***/
PMON_CONFIG			pmonConfig;
METER_COM_CONFIG	meterComConfig;
FTPC_CONFIG			ftpcConfig;
MQTTC_CONFIG		mqttcConfig;


/* 
Note: 	Live data is transferring through HTTP POST by using lua, so no need create conf(binary) file.
		In lua, config can read directly fron "/usr/mrm/config/mrmConfig.json"
 */
 
/*** Functions ***/
/**
 *  Function    : loadFile
 *  Params [in] : "filepath" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
static char* loadFile(const char* filepath) 
{
	int			fd=0;
	char		*text=NULL;	
	struct stat st;
	
	// Check if file is present
	if (stat(filepath, &st)==-1) 
	{
		printf("can't find file %s\n", filepath);
		return NULL;
	}
	
	// Open the file
	fd = open(filepath, O_RDONLY);
	if (fd == -1) 
	{
		printf("can't open file - %s", filepath);
		return NULL;
	}
	  
	// Read file
	text = malloc(st.st_size+1); // this is not going to be freed
	if (st.st_size!=read(fd, text, st.st_size)) 
	{
		printf("can't read file - %s", filepath);
		close(fd);
		return NULL;
	}
	
	close(fd);
	
	// Add null char
	text[st.st_size]='\0';
	
	return text;
}

/**
 *  Function    : getPmonConfig
 *  Params [in] : "rootVal" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int getPmonConfig(const nx_json *rootVal)
{
	const char	*enablePtr=NULL,*ipAddr=NULL, *newIpAddr=NULL, *gw=NULL, *subnetMask=NULL,
	            *dbgLogIpAddrPtr=NULL;
	const   nx_json *json_trav=NULL,*json_root=NULL;

	
	enablePtr = nx_json_get(rootVal, "SelectedProtoCol")->text_value;
	if(enablePtr != NULL)
	{
		printf("getPmonConfig() : Selected Protocol = %s\n", enablePtr);
		if(strcmp(enablePtr,"MODBUS") == 0)
			pmonConfig.selectedProtocol = MODBUS;
		else
			pmonConfig.selectedProtocol = NONE;
	}
	else
	{
		printf("getPmonConfig(): improper config file format - 3\n");
		exit(-1);
	}
	
	json_root = nx_json_get(rootVal, "GenConfig");
	if(json_root != NULL)
	{
		json_trav = nx_json_get(json_root, "LANConfig");
		if(json_trav != NULL)
		{
			ipAddr = nx_json_get(json_trav, "LanIP")->text_value;
			if(ipAddr != NULL)
			{
				printf("getPmonConfig(): Current IP = %s\n", ipAddr);
				strcpy(pmonConfig.ethConfig.ipAddr,ipAddr);
			}
			else
			{
				printf("getPmonConfig(): getting ipAddr failed\n");
				return RET_FAILURE;
			}
			
			subnetMask = nx_json_get(json_trav, "SubnetMask")->text_value;
			if(subnetMask != NULL)
			{
				printf("getPmonConfig(): SubnetMask = %s\n", subnetMask);
				strcpy(pmonConfig.ethConfig.netmask,subnetMask);
			}
			else
			{
				printf("getPmonConfig(): getting SubnetMask failed\n");
				return RET_FAILURE;
			}
			
			gw = nx_json_get(json_trav, "Gateway")->text_value;
			if(gw != NULL)
			{
				printf("getPmonConfig(): Gateway = %s\n", gw);
				strcpy(pmonConfig.ethConfig.gateway,gw);
			}
			else
			{
				printf("getPmonConfig(): getting Gateway failed\n");
				return RET_FAILURE;
			}
		}
		else
		{
			printf("getPmonConfig(): improper config file format - 1\n");
			exit(-1);
		}
	
		json_trav = nx_json_get(json_root, "DebugConfig");
		if(json_trav != NULL)
		{
			enablePtr = nx_json_get(json_trav, "Enable")->text_value;
			if(enablePtr != NULL)
			{
				printf("getPmonConfig(): Enable debug = %s\n", enablePtr);
				if(strcmp(enablePtr,"Yes") == 0)
					pmonConfig.debugConfig.enable = 1;
				else
					pmonConfig.debugConfig.enable = 0;
			}
			else
			{
				printf("getPmonConfig(): getting Enable debug failed\n");
				return RET_FAILURE;
			}
			
			dbgLogIpAddrPtr = nx_json_get(json_trav, "DebugLogIP")->text_value;
			if(dbgLogIpAddrPtr != NULL)
			{
				printf("getPmonConfig(): dbgLogIpAddr = %s\n", dbgLogIpAddrPtr);
				strcpy(pmonConfig.debugConfig.serverIpAddr,dbgLogIpAddrPtr);
			}
			else
			{
				printf("getPmonConfig(): getting dbgLogIpAddr failed\n");
				return RET_FAILURE;
			}
			
			dbgLogIpAddrPtr = nx_json_get(json_trav, "DebugPort")->text_value;
			if(dbgLogIpAddrPtr != NULL)
			{
				printf("getPmonConfig(): dbgLog Port = %s\n", dbgLogIpAddrPtr);
				pmonConfig.debugConfig.port = (unsigned int)atoi(dbgLogIpAddrPtr);
			}
			else
			{
				printf("getPmonConfig(): getting dbgLog Port failed\n");
				return RET_FAILURE;
			}
		}
		else
		{
			printf("getPmonConfig(): improper config file format - 2\n");
			exit(-1);
		}
	
		json_trav = nx_json_get(json_root, "NTPConfig");
		if(json_trav != NULL)
		{
			enablePtr = nx_json_get(json_trav, "Enable")->text_value;
			if(enablePtr != NULL)
			{
				printf("getPmonConfig(): Enable ntp = %s\n", enablePtr);
				if(strcmp(enablePtr,"Yes") == 0)
					pmonConfig.ntpConfig.enable = 1;
				else
					pmonConfig.ntpConfig.enable = 0;
			}
			else
			{
				printf("getPmonConfig(): getting Enable ntp failed\n");
				return RET_FAILURE;
			}
			
			dbgLogIpAddrPtr = nx_json_get(json_trav, "ServerAddr")->text_value;
			if(dbgLogIpAddrPtr != NULL)
			{
				printf("getPmonConfig(): ntpServerAddr = %s\n", dbgLogIpAddrPtr);
				strcpy(pmonConfig.ntpConfig.serverIpAddr,dbgLogIpAddrPtr);
			}
			else
			{
				printf("getPmonConfig(): getting ntpServerAddr failed\n");
				return RET_FAILURE;
			}
			
			dbgLogIpAddrPtr = nx_json_get(json_trav, "Port")->text_value;
			if(dbgLogIpAddrPtr != NULL)
			{
				printf("getPmonConfig(): NTP Port = %s\n", dbgLogIpAddrPtr);
				pmonConfig.ntpConfig.port = (unsigned int)atoi(dbgLogIpAddrPtr);
			}
			else
			{
				printf("getPmonConfig(): getting NTP Port failed\n");
				return RET_FAILURE;
			}
			
			dbgLogIpAddrPtr = nx_json_get(json_trav, "Interval")->text_value;
			if(dbgLogIpAddrPtr != NULL)
			{
				printf("getPmonConfig(): NTP Interval = %s\n", dbgLogIpAddrPtr);
				pmonConfig.ntpConfig.synInterval = (unsigned int)atoi(dbgLogIpAddrPtr);
			}
			else
			{
				printf("getPmonConfig(): getting NTP Interval failed\n");
				return RET_FAILURE;
			}
		}
		else
		{
			printf("getPmonConfig(): improper config file format - 2\n");
			exit(-1);
		}
	}
	else
	{
		printf("getPmonConfig(): improper config file format - 3\n");
		exit(-1);
	}
	
	json_root = nx_json_get(rootVal, "ComDataTransferType");
	if(json_root != NULL)
	{
		enablePtr = nx_json_get(json_root, "Enable")->text_value;
		if(enablePtr != NULL)
		{
			printf("getPmonConfig() : Enable = %s\n", enablePtr);
			if(strcmp(enablePtr,"Yes") == 0)
				pmonConfig.comDataTransfer.dataTransferEnable = ENABLE;
			else
				pmonConfig.comDataTransfer.dataTransferEnable = DISABLE;
		}
		else
		{
			printf("getPmonConfig(): improper config file format - 13\n");
			pmonConfig.comDataTransfer.dataTransferEnable = DISABLE;
			exit(-1);
		}

		enablePtr = nx_json_get(json_root, "SelectedDataComType")->text_value;
		if(enablePtr != NULL)
		{
			printf("getPmonConfig() : SelectedDataComType = %s\n", enablePtr);
			if(strcmp(enablePtr,"FTP") == 0)
				pmonConfig.comDataTransfer.selectedDataTransType = FTP;
			else
				pmonConfig.comDataTransfer.selectedDataTransType = MQTT;
		}
		else
		{
			printf("getPmonConfig(): improper config file format - 23\n");
			pmonConfig.comDataTransfer.selectedDataTransType = FTP;
			exit(-1);
		}
	}
	else
	{
		printf("getPmonConfig(): improper config file format - 03\n");
		exit(-1);
	}
	
	json_root = nx_json_get(rootVal, "LiveComDataTransferType");
	if(json_root != NULL)
	{
		enablePtr = nx_json_get(json_root, "Enable")->text_value;
		if(enablePtr != NULL)
		{
			printf("getPmonConfig() : Enable = %s\n", enablePtr);
			if(strcmp(enablePtr,"Yes") == 0)
				pmonConfig.comDataTransfer.liveDataTransferEnable = ENABLE;
			else
				pmonConfig.comDataTransfer.liveDataTransferEnable = DISABLE;
		}
		else
		{
			printf("getPmonConfig(): improper config file format - 113\n");
			pmonConfig.comDataTransfer.liveDataTransferEnable = DISABLE;
			exit(-1);
		}

		enablePtr = nx_json_get(json_root, "SelectedLiveDataComType")->text_value;
		if(enablePtr != NULL)
		{
			printf("getPmonConfig() : SelectedLiveDataComType = %s\n", enablePtr);
			if(strcmp(enablePtr,"HTTP") == 0)
				pmonConfig.comDataTransfer.selectedLiveDataTransType = HTTP;
			else
				pmonConfig.comDataTransfer.selectedLiveDataTransType = HTTP;
		}
		else
		{
			printf("getPmonConfig(): improper config file format - 213\n");
			pmonConfig.comDataTransfer.selectedLiveDataTransType = HTTP;
			exit(-1);
		}
	}
	else
	{
		printf("getPmonConfig(): improper config file format - 013\n");
		exit(-1);
	}
	return RET_OK;
}

/**
 *  Function    : getFtpcConfig
 *  Params [in] : "rootVal" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int getFtpcConfig(const nx_json *rootVal)
{
	const char	*cpPtr=NULL;
	const   nx_json *json_trav=NULL,*json_root=NULL;

	json_root = nx_json_get(rootVal, "ComDataTransferType");
	if(json_root != NULL)
	{
		json_trav = nx_json_get(json_root, "FtpcConfig");
		if(json_trav != NULL)
		{
			cpPtr = nx_json_get(json_trav, "Host")->text_value;
			if(cpPtr != NULL)
			{
				printf("getFtpcConfig(): Host IP = %s\n", cpPtr);
				strcpy(ftpcConfig.host,cpPtr);
			}
			else
			{
				printf("getFtpcConfig(): getting Host ipAddr failed\n");
				return RET_FAILURE;
			}
			cpPtr = nx_json_get(json_trav, "UserName")->text_value;
			if(cpPtr != NULL)
			{
				printf("getFtpcConfig(): UserName = %s\n", cpPtr);
				strcpy(ftpcConfig.userName,cpPtr);
			}
			else
			{
				printf("getFtpcConfig(): getting UserName failed\n");
				return RET_FAILURE;
			}
			cpPtr = nx_json_get(json_trav, "PassWord")->text_value;
			if(cpPtr != NULL)
			{
				printf("getFtpcConfig(): PassWord = %s\n", cpPtr);
				strcpy(ftpcConfig.password,cpPtr);
			}
			else
			{
				printf("getFtpcConfig(): getting PassWord failed\n");
				return RET_FAILURE;
			}
		}
		else
		{
			printf("getFtpcConfig(): improper config file format - 4\n");
			exit(-1);
		}
		
		json_trav = nx_json_get(json_root, "TransferingType");
		if(json_trav != NULL)
		{
			cpPtr = nx_json_get(json_trav, "FileTransType")->text_value;
			if(cpPtr != NULL)
			{
				printf("getFtpcConfig(): FileTransType = %s\n", cpPtr);
				if(strcmp(cpPtr,"Periodic") == 0)
					ftpcConfig.fileTransType.transType = 1;
				else
					ftpcConfig.fileTransType.transType = 2;
			}
			else
			{
				printf("getFtpcConfig(): getting FileTransType failed\n");
				return RET_FAILURE;
			}
			if( ftpcConfig.fileTransType.transType == 1 )
			{
				cpPtr = nx_json_get(json_trav, "PeriodicInt")->text_value;
				if(cpPtr != NULL)
				{
					printf("getFtpcConfig(): PeriodicInt = %s\n", cpPtr);
					ftpcConfig.fileTransType.periodicInterval = (unsigned char)atoi(cpPtr); //in min
				}
				else
				{
					printf("getFtpcConfig(): getting PeriodicInt failed\n");
					return RET_FAILURE;
				}
			}
			else //sch part
			{
				cpPtr = nx_json_get(json_trav, "SchHH")->text_value;
				if(cpPtr != NULL)
				{
					printf("getFtpcConfig(): SchHH = %s\n", cpPtr);
					ftpcConfig.fileTransType.fileTransSch.hour = (unsigned char)atoi(cpPtr); //in hr
				}
				else
				{
					printf("getFtpcConfig(): getting SchHH failed\n");
					return RET_FAILURE;
				}
				cpPtr = nx_json_get(json_trav, "SchMM")->text_value;
				if(cpPtr != NULL)
				{
					printf("getFtpcConfig(): SchMM = %s\n", cpPtr);
					ftpcConfig.fileTransType.fileTransSch.min = (unsigned char)atoi(cpPtr); //in min
				}
				else
				{
					printf("getFtpcConfig(): getting SchMM failed\n");
					return RET_FAILURE;
				}
			}
			//create config file in "/usr/mrm/config/ftpcConfig.conf"
			storeConfigFile(FTPC_CONFIG_FILE,FTPC_CONFIG_STRUCT);
		}
		else
		{
			printf("getFtpcConfig(): improper config file format - 441\n");
			exit(-1);
		}
	}
	else
	{
		printf("getFtpcConfig(): improper config file format - 44\n");
		exit(-1);
	}
	return RET_OK;
}

/**
 *  Function    : getMqttcConfig
 *  Params [in] : "rootVal" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int getMqttcConfig(const nx_json *rootVal)
{
	const char	*cpPtr=NULL;
	const   nx_json *json_trav=NULL,*json_root=NULL;

	json_root = nx_json_get(rootVal, "ComDataTransferType");
	if(json_root != NULL)
	{
		json_trav = nx_json_get(json_root, "MqttcConfig");
		if(json_trav != NULL)
		{
			cpPtr = nx_json_get(json_trav, "Host")->text_value;
			if(cpPtr != NULL)
			{
				printf("getMqttcConfig(): Host IP = %s\n", cpPtr);
				strcpy(mqttcConfig.host,cpPtr);
			}
			else
			{
				printf("getMqttcConfig(): getting Host ipAddr failed\n");
				return RET_FAILURE;
			}
			cpPtr = nx_json_get(json_trav, "UserName")->text_value;
			if(cpPtr != NULL)
			{
				printf("getMqttcConfig(): UserName = %s\n", cpPtr);
				strcpy(mqttcConfig.userName,cpPtr);
			}
			else
			{
				printf("getMqttcConfig(): getting UserName failed\n");
				return RET_FAILURE;
			}
			cpPtr = nx_json_get(json_trav, "PassWord")->text_value;
			if(cpPtr != NULL)
			{
				printf("getMqttcConfig(): PassWord = %s\n", cpPtr);
				strcpy(mqttcConfig.password,cpPtr);
			}
			else
			{
				printf("getMqttcConfig(): getting PassWord failed\n");
				return RET_FAILURE;
			}
		}
		else
		{
			printf("getMqttcConfig(): improper config file format - 54\n");
			exit(-1);
		}
		
		json_trav = nx_json_get(json_root, "TransferingType");
		if(json_trav != NULL)
		{
			cpPtr = nx_json_get(json_trav, "FileTransType")->text_value;
			if(cpPtr != NULL)
			{
				printf("getMqttcConfig(): FileTransType = %s\n", cpPtr);
				if(strcmp(cpPtr,"Periodic") == 0)
					mqttcConfig.fileTransType.transType = 1;
				else
					mqttcConfig.fileTransType.transType = 2;
			}
			else
			{
				printf("getMqttcConfig(): getting FileTransType failed\n");
				return RET_FAILURE;
			}
			if( mqttcConfig.fileTransType.transType == 1 )
			{
				cpPtr = nx_json_get(json_trav, "PeriodicInt")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMqttcConfig(): PeriodicInt = %s\n", cpPtr);
					mqttcConfig.fileTransType.periodicInterval = (unsigned char)atoi(cpPtr); //in min
				}
				else
				{
					printf("getMqttcConfig(): getting PeriodicInt failed\n");
					return RET_FAILURE;
				}
			}
			else //sch part
			{
				cpPtr = nx_json_get(json_trav, "SchHH")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMqttcConfig(): SchHH = %s\n", cpPtr);
					mqttcConfig.fileTransType.fileTransSch.hour = (unsigned char)atoi(cpPtr); //in hr
				}
				else
				{
					printf("getMqttcConfig(): getting SchHH failed\n");
					return RET_FAILURE;
				}
				cpPtr = nx_json_get(json_trav, "SchMM")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMqttcConfig(): SchMM = %s\n", cpPtr);
					mqttcConfig.fileTransType.fileTransSch.min = (unsigned char)atoi(cpPtr); //in min
				}
				else
				{
					printf("getMqttcConfig(): getting SchMM failed\n");
					return RET_FAILURE;
				}
			}
			//create config file in "/usr/mrm/config/mqttcConfig.conf"
			storeConfigFile(MQTTC_CONFIG_FILE,MQTTC_CONFIG_STRUCT);
		}
		else
		{
			printf("getMqttcConfig(): improper config file format - 5441\n");
			exit(-1);
		}
	}
	else
	{
		printf("getFtpcConfig(): improper config file format - 44\n");
		exit(-1);
	}
	return RET_OK;
}

/**
 *  Function    : getMeterComConfig
 *  Params [in] : "rootVal" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int getMeterComConfig(const nx_json *rootVal)
{
	const char	*cpPtr=NULL;
	const   nx_json *json_trav_1=NULL,*json_root=NULL,*json_trav_2=NULL;
	
	json_root = nx_json_get(rootVal, "MeterComConfig");
	if(json_root != NULL)
	{
		json_trav_1 = nx_json_get(json_root, "SlaveComType");
		if(json_trav_1 != NULL)
		{
			json_trav_2 = nx_json_get(json_trav_1, "SerialConfig");
			if(json_trav_2 != NULL)
			{
				cpPtr = nx_json_get(json_trav_2, "Baudrate")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): Baudrate = %s\n", cpPtr);
					meterComConfig.meterComType.serialPortConfig.baudRate = (int)atoi(cpPtr);
				}
				else
				{
					printf("getMeterComConfig(): getting Baudrate failed\n");
					meterComConfig.meterComType.serialPortConfig.baudRate =  9600;
					return RET_FAILURE;
				}
				cpPtr = nx_json_get(json_trav_2, "Databits")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): Databits = %s\n", cpPtr);
					meterComConfig.meterComType.serialPortConfig.databits = (unsigned char)atoi(cpPtr);
				}
				else
				{
					printf("getMeterComConfig(): getting Databits failed\n");
					meterComConfig.meterComType.serialPortConfig.databits =  8;
					return RET_FAILURE;
				}
				cpPtr = nx_json_get(json_trav_2, "Stopbits")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): Stopbits = %s\n", cpPtr);
					meterComConfig.meterComType.serialPortConfig.stopbits = (unsigned char)atoi(cpPtr);
				}
				else
				{
					printf("getMeterComConfig(): getting Stopbits failed\n");
					meterComConfig.meterComType.serialPortConfig.stopbits =  1;
					return RET_FAILURE;
				}
				cpPtr = nx_json_get(json_trav_2, "Parity")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): Parity = %s\n", cpPtr);
					meterComConfig.meterComType.serialPortConfig.parity = (char)cpPtr[0];
				}
				else
				{
					printf("getMeterComConfig(): getting Parity failed\n");
					meterComConfig.meterComType.serialPortConfig.parity =  'N';
					return RET_FAILURE;
				}
				cpPtr = nx_json_get(json_trav_2, "SerialDvice")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): SerialDvice = %s\n", cpPtr);
					strcpy(meterComConfig.meterComType.serialPortConfig.serialFile,cpPtr);
				}
				else
				{
					printf("getMeterComConfig(): getting SerialDvice failed\n");
					strcpy(meterComConfig.meterComType.serialPortConfig.serialFile,"ttyUSB0");
					return RET_FAILURE;
				}
				cpPtr = nx_json_get(json_trav_2, "Handshake")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): Handshake = %s\n", cpPtr);
					meterComConfig.meterComType.serialPortConfig.handshake = (char)cpPtr[0];
				}
				else
				{
					printf("getMeterComConfig(): getting Handshake failed\n");
					meterComConfig.meterComType.serialPortConfig.handshake =  'N';
					return RET_FAILURE;
				}
			}
			else
			{
				printf("getMeterComConfig(): improper config file format - 1\n");
				exit(-1);
			}
			
			json_trav_2 = nx_json_get(json_trav_1, "TcpIpConfig");
			if(json_trav_2 != NULL)
			{
				cpPtr = nx_json_get(json_trav_2, "SlaveIpM1")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): SlaveIpM1 = %s\n", cpPtr);
					strcpy(meterComConfig.meterComType.tcpIpConfigSlave1.ipAddr,cpPtr);
				}
				else
				{
					printf("getMeterComConfig(): getting SlaveIpM1 failed\n");
					strcpy(meterComConfig.meterComType.tcpIpConfigSlave1.ipAddr,"0.0.0.0");
					return RET_FAILURE;
				}
				cpPtr = nx_json_get(json_trav_2, "SlavePortM1")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): SlavePortM1 = %s\n", cpPtr);
					meterComConfig.meterComType.tcpIpConfigSlave1.port = (int)atoi(cpPtr);
				}
				else
				{
					printf("getMeterComConfig(): getting SlavePortM1 failed\n");
					meterComConfig.meterComType.tcpIpConfigSlave1.port =  502;
					return RET_FAILURE;
				}
				cpPtr = nx_json_get(json_trav_2, "SlaveIpM2")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): SlaveIpM2 = %s\n", cpPtr);
					strcpy(meterComConfig.meterComType.tcpIpConfigSlave2.ipAddr,cpPtr);
				}
				else
				{
					printf("getMeterComConfig(): getting SlaveIpM2 failed\n");
					strcpy(meterComConfig.meterComType.tcpIpConfigSlave2.ipAddr,"0.0.0.0");
					return RET_FAILURE;
				}
				cpPtr = nx_json_get(json_trav_2, "SlavePortM2")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): SlavePortM2 = %s\n", cpPtr);
					meterComConfig.meterComType.tcpIpConfigSlave2.port = (int)atoi(cpPtr);
				}
				else
				{
					printf("getMeterComConfig(): getting SlavePortM2 failed\n");
					meterComConfig.meterComType.tcpIpConfigSlave2.port =  502;
					return RET_FAILURE;
				}
			}
			else
			{
				printf("getMeterComConfig(): improper config file format - 01\n");
				exit(-1);
			}
			
			cpPtr = nx_json_get(json_trav_1, "SelectedMtrComType")->text_value;
			if(cpPtr != NULL)
			{
				printf("getMeterComConfig(): SelectedMtrComType = %s\n", cpPtr);
				if(strcmp(cpPtr,"RTU") == 0)
					meterComConfig.meterComType.selectedMtrcomType = RTU;
				else
					meterComConfig.meterComType.selectedMtrcomType = TCP_IP;
			}
			else
			{
				printf("getMeterComConfig(): getting SelectedMtrComType failed\n");
				meterComConfig.meterComType.selectedMtrcomType = RTU;
				return RET_FAILURE;
			}
		}
		else
		{
			printf("getMeterComConfig(): improper config file format - 11\n");
			exit(-1);
		}
		
		json_trav_1 = nx_json_get(json_root, "LoggingType");
		if(json_trav_1 != NULL)
		{
			cpPtr = nx_json_get(json_trav_1, "LogType")->text_value;
			if(cpPtr != NULL)
			{
				printf("getMeterComConfig(): LogType = %s\n", cpPtr);
				if(strcmp(cpPtr,"Periodic") == 0)
					meterComConfig.loggingType.logType = 1;
				else
					meterComConfig.loggingType.logType = 2;
			}
			else
			{
				printf("getMeterComConfig(): getting LogType failed\n");
				return RET_FAILURE;
			}
			
			if(meterComConfig.loggingType.logType == 1)
			{
				cpPtr = nx_json_get(json_trav_1, "PeriodicInt")->text_value;
				if(cpPtr != NULL)
				{
					printf("getMeterComConfig(): PeriodicInt = %s\n", cpPtr);
					meterComConfig.loggingType.periodicInterval = (unsigned char)atoi(cpPtr);
				}
				else
				{
					printf("getMeterComConfig(): getting PeriodicInt failed\n");
					return RET_FAILURE;
				}
			}
			else
			{
				//sch type yet to be decide
			}
		}
		else
		{
			printf("getMeterComConfig(): improper config file format - 3\n");
			exit(-1);
		}
		cpPtr = nx_json_get(json_root, "EnableLogData")->text_value;
		if(cpPtr != NULL)
		{
			printf("getMeterComConfig(): EnableLogData = %s\n", cpPtr);
			if(strcmp(cpPtr,"Yes") == 0)
				meterComConfig.enableLoging = 1;
			else
				meterComConfig.enableLoging = 0;
		}
		else
		{
			printf("getMeterComConfig(): getting EnableLogData failed\n");
			return RET_FAILURE;
		}
		//create config file in "/usr/mrm/config/meterComConfig.conf"
		storeConfigFile(METER_COM_CONFIG_FILE,METER_CONFIG_STRUCT);
	}
	else
	{
		printf("getMeterComConfig(): improper config file format - 2\n");
		exit(-1);
	}
	return RET_OK;
}

/**
 *  Function    : getMeterPollDetails
 *  Params [in] : "rootVal" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int getMeterPollDetails(const nx_json *rootVal)
{
	int				idx=0,i=0;
	char 			fName[SIZE_64];
	const char 		*genPtr=NULL;
	const nx_json 	*inputDetailsPtr=NULL, *arr_trav=NULL,*json_trav=NULL,*json_root=NULL;
	FILE 			*fp;

	json_root = nx_json_get(rootVal, "MeterPollInform");
	if(json_root != NULL)
	{
		genPtr = nx_json_get(json_root, "ConnectedMeters")->text_value;
		if(genPtr != NULL)
		{
			printf("getMeterPollDetails(): ConnectedMeters = %s\n", genPtr);
			meterComConfig.numOfMtsConnected = (unsigned int)atoi(genPtr);
			//create config file in "/usr/mrm/config/meterComConfig.conf"
			storeConfigFile(METER_COM_CONFIG_FILE,METER_CONFIG_STRUCT);
		}
		else
		{
			printf("getMeterPollDetails(): getting ConnectedMeters failed\n");
			return RET_FAILURE;
		}
		
		for(i=1;i<=meterComConfig.numOfMtsConnected;i++)
		{
			memset(fName,0,sizeof(fName));
			sprintf(fName,"%s/%d_Config.list",METER_POLL_INFO_PATH,i);
			fp = fopen(fName,"w");
			if( fp == NULL )
			{
				printf("Failed to create file %s\n",strerror(errno));
				return RET_FAILURE;
			}
			memset(fName,0,sizeof(fName));
			sprintf(fName,"Meter_%d",i);
			inputDetailsPtr = nx_json_get(json_root,fName);
			if(inputDetailsPtr != NULL)
			{
				printf("inputDetailsPtr->length = %d\n", inputDetailsPtr->length);
				for(idx=0; idx<inputDetailsPtr->length; idx++)
				{
					arr_trav = nx_json_item(inputDetailsPtr, idx);
					if(arr_trav != NULL)
					{
						genPtr = nx_json_get(arr_trav, "MtrAddr")->text_value;
						if(genPtr != NULL)
						{
							printf("getMeterPollDetails(): [%d] = %s\n", idx, genPtr);
							fprintf(fp,"%s,",genPtr);
						}
						else
						{
							printf("getMeterPollDetails(): getting failed idx = %d \n", idx);
							fclose(fp);
							return -1;
						}
						
						genPtr = nx_json_get(arr_trav, "FunCode")->text_value;
						if(genPtr != NULL)
						{
							printf("getMeterPollDetails(): [%d] = %s\n", idx, genPtr);
							fprintf(fp,"%s,",genPtr);
						}
						else
						{
							printf("getMeterPollDetails(): getting failed idx = %d \n", idx);
							fclose(fp);
							return -1;
						}
						
						genPtr = nx_json_get(arr_trav, "StartAddr")->text_value;
						if(genPtr != NULL)
						{
							printf("getMeterPollDetails(): [%d] = %s\n", idx, genPtr);
							fprintf(fp,"%s,",genPtr);
						}
						else
						{
							printf("getMeterPollDetails(): getting failed idx = %d \n", idx);
							fclose(fp);
							return -1;
						}
						
						genPtr = nx_json_get(arr_trav, "NoOfReg")->text_value;
						if(genPtr != NULL)
						{
							printf("getMeterPollDetails(): [%d] = %s\n", idx, genPtr);
							fprintf(fp,"%s,",genPtr);
						}
						else
						{
							printf("getMeterPollDetails(): getting failed idx = %d \n", idx);
							fclose(fp);
							return -1;
						}
						
						genPtr = nx_json_get(arr_trav, "ByteOrder")->text_value;
						if(genPtr != NULL)
						{
							printf("getMeterPollDetails(): [%d] = %s\n", idx, genPtr);
							if(strcmp(genPtr,"highLowByte") == 0)
								fprintf(fp,"1,");
							else if(strcmp(genPtr,"lowHighByte") == 0)
								fprintf(fp,"0,");
							else
								fprintf(fp,"1,");
						}
						else
						{
							printf("getMeterPollDetails(): getting failed idx = %d \n", idx);
							fclose(fp);
							return -1;
						}	
						
						genPtr = nx_json_get(arr_trav, "WordOrder")->text_value;
						if(genPtr != NULL)
						{
							printf("getMeterPollDetails(): [%d] = %s\n", idx, genPtr);
							if(strcmp(genPtr,"highLowWord") == 0)
								fprintf(fp,"1,");
							else if(strcmp(genPtr,"lowHighWord") == 0)
								fprintf(fp,"0,");
							else
								fprintf(fp,"1,");
						}
						else
						{
							printf("getMeterPollDetails(): getting failed idx = %d \n", idx);
							fclose(fp);
							return -1;
						}
						
						genPtr = nx_json_get(arr_trav, "RegisterSize")->text_value;
						if(genPtr != NULL)
						{
							printf("getMeterPollDetails():[%d] = %s\n", idx, genPtr);
							if(strcmp(genPtr,"bit16") == 0)
								fprintf(fp,"0,");
							else if(strcmp(genPtr,"bit32") == 0)
								fprintf(fp,"1,");
							else
								fprintf(fp,"0,");
						}
						else
						{
							printf("getMeterPollDetails(): getting failed idx = %d \n", idx);
							fclose(fp);
							return -1;
						}
						
						genPtr = nx_json_get(arr_trav, "DataTypeOfValue")->text_value;
						if(genPtr != NULL)
						{
							printf("getMeterPollDetails(): [%d] = %s\n", idx, genPtr);
							if(strcmp(genPtr,"float32Bit") == 0)
								fprintf(fp,"1,");
							else if(strcmp(genPtr,"uint32Bit") == 0)
								fprintf(fp,"2,");
							else if(strcmp(genPtr,"int32Bit") == 0)
								fprintf(fp,"3,");
							else if(strcmp(genPtr,"uint16Bit") == 0)
								fprintf(fp,"4,");
							else if(strcmp(genPtr,"int16Bit") == 0)
								fprintf(fp,"5,");
							else
								fprintf(fp,"1,");
						}
						else
						{
							printf("getMeterPollDetails(): getting failed idx = %d \n", idx);
							fclose(fp);
							return -1;
						}
						
						genPtr = nx_json_get(arr_trav, "ParamName")->text_value;
						if(genPtr != NULL)
						{
							printf("getMeterPollDetails(): [%d] = %s\n", idx, genPtr);
							fprintf(fp,"%s\n",genPtr);
						}
						else
						{
							printf("getMeterPollDetails(): getting failed idx = %d \n", idx);
							fclose(fp);
							return -1;
						}
					}
					else
					{
						printf("getMeterPollDetails(): improper config file format - 1\n");
						fclose(fp);
						exit(-1);
					}
				}
			}
			else
			{
				printf("getMeterPollDetails(): improper config file format - 2\n");
				fclose(fp);
				exit(-1);
			}
			fclose(fp);
		}
	}
	
	return RET_OK;
}

/**
 *  Function    : readConfigFile
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int readConfigFile(void)
{
	char	inputFile[SIZE_128];
	const   nx_json *root_value=NULL;

	memset(&pmonConfig,0,sizeof(PMON_CONFIG));
	memset(&meterComConfig,0,sizeof(METER_COM_CONFIG));
	memset(&ftpcConfig,0,sizeof(FTPC_CONFIG));
	memset(&mqttcConfig,0,sizeof(MQTTC_CONFIG));
	strcpy(inputFile, CONFIG_INPUT_FILENAME);
	char *input = loadFile(inputFile);
	
	if(input != NULL)
	{
		root_value = nx_json_parse(input, 0);
		if (root_value != NULL) 
		{
			getPmonConfig(root_value);
			getMeterComConfig(root_value);
			getMeterPollDetails(root_value);
			getFtpcConfig(root_value);
			getMqttcConfig(root_value);
		}
		else
			printf("parsing failed - improper config file format\n");
		
		nx_json_free(root_value);
	}

	return RET_OK;
	
	//read json
	//fill config struct belongs to Pmon 
		/*
			Ip address for lan
			Debug Ip and port (UDP)
			Ntp server ip and port etc..
			Which protocol selected, like Modbus , dlms or pact..etc
			Note : Make a struct with above params and fill
		*/
	//fill config struct belongs to ftpc and create config file in "/usr/mrm/config"
		/*
			transfer type sch or periodic
			if sch , get hr:mn
			if periodic, get interval
			hosr ip , username and password
			Note : Make a struct with above params and fill
		*/
	//create config file for meter reading process in "/usr/mrm/config"
		/*
			Depends upon selected protocol create config file.
			Ref: for modbus, check 'Config.list'
		*/
}

int storeConfigFile(char *fileName,unsigned char structType) 
{
	int			fd=0, ret=0;
	
	switch(structType)
	{
		case METER_CONFIG_STRUCT:
		{
			// Open the file
			fd = open(fileName, (O_RDWR|O_CREAT), S_IRWXU);
			if (fd == -1) 
			{
				printf("can't open file - %s\n", fileName);
				perror("File open failed");
				return -1;
			}
			  
			// Write config into file
			ret = write(fd, (char *)&meterComConfig, sizeof(METER_COM_CONFIG));
			printf("ret = %d\n", ret);
			if(ret < 0) 
			{
				printf("can't write file - %s\n", fileName);
				perror("Write failed\n");
				close(fd);
				return -1;
			}
		}
		break;		
		case FTPC_CONFIG_STRUCT:
		{
			// Open the file
			fd = open(fileName, (O_RDWR|O_CREAT), S_IRWXU);
			if (fd == -1) 
			{
				printf("can't open file - %s\n", fileName);
				perror("File open failed");
				return -1;
			}
			  
			// Write config into file
			ret = write(fd, (char *)&ftpcConfig, sizeof(FTPC_CONFIG));
			printf("ret = %d\n", ret);
			if(ret < 0) 
			{
				printf("can't write file - %s\n", fileName);
				perror("Write failed\n");
				close(fd);
				return -1;
			}
		}
		break;
		case MQTTC_CONFIG_STRUCT:
		{
			// Open the file
			fd = open(fileName, (O_RDWR|O_CREAT), S_IRWXU);
			if (fd == -1) 
			{
				printf("can't open file - %s\n", fileName);
				perror("File open failed");
				return -1;
			}
			  
			// Write config into file
			ret = write(fd, (char *)&ftpcConfig, sizeof(MQTTC_CONFIG));
			printf("ret = %d\n", ret);
			if(ret < 0) 
			{
				printf("can't write file - %s\n", fileName);
				perror("Write failed\n");
				close(fd);
				return -1;
			}
		}
		break;
		default:
		break;
	}
	
	close(fd);	
	return RET_OK;
}

/* EOF */
