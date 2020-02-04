#ifndef  __GENERAL_H__
#define  __GENERAL_H__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/errno.h>
#include <pthread.h>
#include <sys/file.h>

#include "nxjson.h"
#include "deviceConfig.h"

// Size constants
#define 	SIZE_8				8
#define 	SIZE_16				16
#define 	SIZE_32				32
#define 	SIZE_64				64
#define 	SIZE_128			128
#define 	SIZE_256			256
#define 	SIZE_512			512
#define 	SIZE_1024			1024
#define 	SIZE_2048			2048
#define 	SIZE_4096			4096

#define 	RET_OK				0
#define		RET_FAILURE			-1

#define		INFORM				0
#define		WARNING				1
#define		SEVERE				2
#define		FATAL				3
#define		REPORT				4

#define		VERSION_NUMBER		"MRM Ver 1.1.0 Dated 07July17"
#define		WDT_PORT			2
#define 	WDT_PIN				14

#define		VERSION_PATH		"/usr/mrm/config/vinfo"
#define		RBT_INFO_PATH       "/usr/mrm/config/rbtinfo"
#define 	MAX_RBT_INFO_SIZE	50000

#define 	METER_COM_PROC		"/usr/mrm/bin/modbus"
#define 	FTPC_PROC			"/usr/mrm/bin/ftpcl"
#define 	LIVE_DATA_POST_PROC	"/usr/mrm/bin/httpcl" //not developed

#define 	MAX_PROCESSES		16

#define 	RBT_PING_FAIL		0x0
#define 	RBT_DEF_RST_TO		0x1
#define		RBT_FROM_WDT		0x3	
#define	 	SIGNAL_INTR			0x4
#define		RBT_LPC_HANG		0x5
/** contains the process information **/
#pragma pack(1)
typedef	struct {
	int		status;						//status of the process, success or failure
	pid_t 	pid;						//pid of the process
	int		numRestart;					//number of times the process is restarted
	long	restartTime;				//time when the process is started ( restarted )
	char 	procName[SIZE_128];			//process name
	char 	procCfgFile[SIZE_128];		//cfg file used by the process
	char	cmdLineArg1[SIZE_128];
	char	cmdLineArg2[SIZE_128];
}PROC_INFO;

//function Prototypes
int dbgLog(int mode,const char *format, ...);
void dbgStr(int mode, char *str1);
int writeDbgLog(char *data);
int	checkForOffset(void);
int writeOffsetInFile(void);
void formatDbgLogMsg(char *msg, int len);

int writeRebootInfo(char reason);
void disableWdt(void);
int doWdtTgle(void);

int writeVersionNumber(void);
int fillProcCfgFile(void);
int invokeProcs(void);
void execProc(int idx);
int startChild(char *procName,char *arg1,char *arg2);
void waitForChild(int sig);
void sigHandler(int sig);
void killAllProc(void);
void restartApp(void);
void killPppd(void); //not used
void stopApp(void);
void gotoSleep(int secs, int usecs);
void restartProcs(void);

int getPmonConfig(const nx_json *rootVal);
int getFtpcConfig(const nx_json *rootVal);
int getMqttcConfig(const nx_json *rootVal);
int getMeterComConfig(const nx_json *rootVal);
int getMeterPollDetails(const nx_json *rootVal);
int readConfigFile(void);
int storeConfigFile(char *fileName,unsigned char structType);

#endif // __GENERAL_H__

/* EOF */
