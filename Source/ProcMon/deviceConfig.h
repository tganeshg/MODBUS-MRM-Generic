#ifndef  __DEVICE_CONFIG__
#define  __DEVICE_CONFIG__

#define 	SIZE_8				8
#define 	SIZE_16				16
#define 	SIZE_32				32
#define 	SIZE_64				64
#define 	SIZE_128			128
#define 	SIZE_256			256
#define 	SIZE_512			512
#define 	SIZE_1024			1024

//Macros
#define		DEVICE_CFG_FILE		"/usr/mrm/config/config.cfg"

//enum
typedef enum
{
	NONE = 0,
	MODBUS = 1,
	DLMS_LS = 2,
	PACT = 3,
	IEC_1107 = 4
}SUPPORTING_PROTOCOLS;

typedef enum
{
	RTU = 1,
	TCP_IP = 2
}SEL_MTR_COM_TYPE;

typedef enum
{
	DISABLE = 0,
	ENABLE = 1
}ENABLE_DISABLE;

typedef enum
{
	FTP = 1,
	MQTT = 2,
	HTTP = 3
}COM_PROTOCOL;

//Structures
#pragma pack(1)
/*** Structures belongs to Pmon process ***/
typedef struct
{
	char	ipAddr[SIZE_32];
	char	netmask[SIZE_32];
	char	gateway[SIZE_32];
}ETH_CONFIG;

typedef struct
{
	unsigned char	enable;
	char			serverIpAddr[SIZE_32];
	unsigned int 	port;
}DEBUG_CONFIG;

typedef struct
{
	unsigned char	enable;
	char			serverIpAddr[SIZE_32];
	unsigned int 	port;
	unsigned int 	synInterval;
}NTP_CONFIG;

typedef struct
{
	ENABLE_DISABLE			dataTransferEnable; // 0 Disable , 1 Enable
	COM_PROTOCOL			selectedDataTransType; // 1 - FTP or 2 - MQTT
	ENABLE_DISABLE			liveDataTransferEnable; // 0 Disable , 1 Enable (HTTP POST)
	COM_PROTOCOL			selectedLiveDataTransType; // 3 - HTTP
}COM_DATA_TRANSFER;

typedef struct
{
	SUPPORTING_PROTOCOLS 	selectedProtocol;
	ETH_CONFIG				ethConfig;
	DEBUG_CONFIG			debugConfig;
	NTP_CONFIG				ntpConfig;
	COM_DATA_TRANSFER		comDataTransfer;
}PMON_CONFIG;

/*** Structures belongs to meter communication process ***/
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

/*** Structures belongs ro mqttc process ***/
typedef struct
{
	unsigned int hour;
	unsigned int min;
}MQTT_FILE_TRANS_SCH;

typedef struct
{
	unsigned char 		transType; //1 periodic, 2 schedule
	unsigned char 		periodicInterval; // fixed - every 1hr/2hr/3hr/6hr/12hr
	MQTT_FILE_TRANS_SCH fileTransSch; //if schedule use this
}MQTT_FILE_TRANS_TYPE;

typedef struct
{
	char 					host[SIZE_32];
	unsigned int			port;
	char					userName[SIZE_128];
	char					password[SIZE_128];
	MQTT_FILE_TRANS_TYPE 	fileTransType;
}MQTTC_CONFIG;
#endif

/* EOF */
