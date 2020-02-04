/*********************************************************************************************
* Copyright (c) 2017 All Right Reserved
*
* Author	  :	Ganesh
* File		  :	main.c
* Summary	  :	MRM / Pmon
* Note  	  : If need to add new PROCs , touch "fillProcCfgFile()" function.
***********************************************************************************************/

/*** Include ***/
#include "general.h"

/*** Local Macros ***/

/*** Externs ***/

/*** Globals ***/
unsigned char 		poweronFlag=1;
int 				numProcs ,tempNumProcs;
pid_t 				parent_pid;
int					killAll;
char				G_stopWdtTgle;
int					restart;

pthread_t 			cmsAlarmThrd_hdlr;

static int 			fdSout;
static fpos_t 		pos;
/*** Structure Variables ***/
PMON_CONFIG			pmonConfig;
METER_COM_CONFIG	meterComConfig;
FTPC_CONFIG			ftpcConfig;

PROC_INFO 			procInfo[MAX_PROCESSES];

void switchStdout(const char *newStream);
void revertStdout(void);
/*** Functions ***/
/**
 *  Function    : writeRebootInfo
 *  Params [in] : "reason" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int writeRebootInfo(char reason)
{
	static char fnName[]="writeRebootInfo()";
	int fd=0,ret=0,len=0,flags=0;;	
	char buffer[SIZE_128];
	char dateStr[SIZE_64];
	struct stat filePt;
	struct tm	*curTime,*faultTime;
	time_t t;
			
	time(&t);
	curTime = localtime(&t);
			
	memset(dateStr,0,sizeof(dateStr));
	sprintf(dateStr,"%02d-%02d-%d %02d:%02d:%02d",curTime->tm_mday,curTime->tm_mon+1,curTime->tm_year+1900,
														curTime->tm_hour,curTime->tm_min,curTime->tm_sec);

	if ( (fd=stat(RBT_INFO_PATH,&filePt)) == 0)
	{
		len = filePt.st_size;
		dbgLog(INFORM,"%s: Rbtt info file size:%d\n",fnName,len);
	}
	else
	{
		dbgLog(INFORM,"%s: Error while getting stat of Rbt fact info file.Err:%d",fnName,errno);
		len = -1;
	}
	
	if(len < MAX_RBT_INFO_SIZE)	
		flags = (O_APPEND|O_CREAT|O_RDWR);
	else
	{
		memset(buffer,0,sizeof(buffer));
		sprintf(buffer,"tail -10 %s > %s_bkp",RBT_INFO_PATH,RBT_INFO_PATH);
		system(buffer);
		
		memset(buffer,0,sizeof(buffer));
		sprintf(buffer,"rm  %s",RBT_INFO_PATH);
		system(buffer); 
		
		memset(buffer,0,sizeof(buffer));
		sprintf(buffer,"mv %s_bkp %s",RBT_INFO_PATH,RBT_INFO_PATH);
		system(buffer); 
		
		flags = (O_APPEND|O_CREAT|O_RDWR);
		//flags = (O_WRONLY|O_CREAT|O_RDWR);
	}
	if ( (fd=open(RBT_INFO_PATH,flags)) < 0)
	{
		dbgLog(REPORT,"%s: Rbt info info file open  Error. Errno:%d Err:%s\n",fnName,errno,strerror(errno));
		return RET_FAILURE;
	}
	else
	{
		memset(buffer,0,SIZE_128);
		if(reason == RBT_PING_FAIL)
			sprintf(buffer,"\n%s: All Ping Failed.TGLE WDT PIN STOPPED. RBT THRGH WDT \n",dateStr);
		else if(reason ==RBT_DEF_RST_TO)
			sprintf(buffer,"\n%s: TGLE WDT PIN STOPPED for Configured Hrs. RBT THRGH WDT \n",dateStr);
		else if(reason ==RBT_FROM_WDT)
			sprintf(buffer,"\n%s: TGLE WDT PIN STOPPED by WDT. \n",dateStr);
		else if(reason ==SIGNAL_INTR)
			sprintf(buffer,"\n%s: SIgnal Interrupt Received \n",dateStr);
		else
			sprintf(buffer,"\n%s: DEF:TGLE WDT PIN STOPPED RBT THRGH WDT \n",dateStr);
		
			
		len = strlen(buffer);
		ret=write(fd,(char *)buffer,len);
		if( ret != len)
		{
			dbgLog(REPORT,"%s: boot fact Info Write Failed. Size Mismatch.wrote:%d. Expected:%d.Errno:%d Err:%s\n",
																					fnName,ret,len,errno,strerror(errno));
			close(fd);
			return RET_FAILURE;
		}
		dbgLog(INFORM,"%s: Boot fact info write Success.Wrote:%d. Expected:%d.\n",fnName,ret,len);
		close(fd);
	}
	
	return RET_OK;
}

/**
 *  Function    : disableWdt
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void disableWdt(void)
{
	G_stopWdtTgle = 1;
	writeRebootInfo(RBT_LPC_HANG);
}

/**
 *  Function    : doWdtTgle
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int doWdtTgle(void)
{
	static int status=0;
	char buffer[64];

	if(G_stopWdtTgle == 1)
	{
		dbgLog(INFORM,"TGLE WDT PIN Stopped. Some Failure.");
		return RET_OK;
	}

	memset(buffer,0,sizeof(buffer));
	if(status == 0)
	{
		sprintf(buffer,"/bin/toggleGpio %d %d %d",WDT_PORT,WDT_PIN,status);
		system(buffer); 
		dbgLog(INFORM,"WDT Made Low");
		status =1;
	}
	else
	{
		sprintf(buffer,"/bin/toggleGpio %d %d %d",WDT_PORT,WDT_PIN,status);
		system(buffer); 
		dbgLog(INFORM,"WDT Made High");
		status =0;
	}
	return RET_OK;
}

/**
 *  Function    : cmsAlarmThrd
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int cmsAlarmThrd(void)
{
	while(1)
	{
		if ( restart != 1 )
		{
			//pulseWdt(); //not implemented
		}
		sleep(2);
	}
	return RET_OK;
}

/**
 *  Function    : main
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int main(void)
{
	int i=0;
	static char *fn = "main()";
	dbgLog(INFORM,"BGM - MRM FW : %s\n",VERSION_NUMBER);
	// writeVersionNumber();
	if ( readConfigFile() < 0 )
	{
		dbgLog(REPORT,"%s:%d:%s: Reading config file failed\n",__FILE__, __LINE__, fn);
		exit(-1);
	}
	sleep(3);
	//setEthIps(); // Not implemented
	//If need, create udp debug socket
	//return 0;
	
	parent_pid = getpid();
	//set signal handler for parent
	signal(SIGTERM,sigHandler);
	signal(SIGINT,sigHandler);
	signal(SIGHUP,sigHandler);
	signal(SIGSEGV,sigHandler);
	
	dbgLog(INFORM,"After setting signal handlers for parent pid %d\n",parent_pid);
	
	//set signal handler for childs
	signal(SIGCHLD, waitForChild);
	dbgLog(INFORM,"After setting signal handler for child \n");

	fillProcCfgFile();
	for(i=0;i<numProcs;i++)
		printf("proc no %d:%s\n",i,procInfo[i].procName);
	
	invokeProcs();
	dbgLog(INFORM,"After invoking child processes,Entering while loop\n");

#ifdef WDT_ADD
	while(1)
	{
		dbgLog(INFORM,"*******calling pthread_create*******\n");
		dbgLog(INFORM,"calling pthread_create - cmsAlarmThrd\n");
		
		sleep(2);
		if( (pthread_create(&cmsAlarmThrd_hdlr,NULL,(void *)(&cmsAlarmThrd),0)) == 0)  
		{
			dbgLog(INFORM,"CMS Alarm  Thread Created \n");
			break;
		}
		else
		{
			dbgLog(REPORT,"Error while creating CMS Alarm Thread");
			sleep(1);
			continue;
		}
	}
#endif

	while ( 1 ) 
	{	
		sleep (2);
	}
	return RET_OK;
}

/**
 *  Function    : writeVersionNumber
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int writeVersionNumber(void)
{
	static char fnName[]="writeVersionNumber()";
	int fd=0,ret=0,len=0;	
	char buffer[SIZE_128];
	
	system("mkdir -p /usr/cms/config");
	
	if ( (fd=open(VERSION_PATH,O_WRONLY|O_CREAT)) < 0)
	{
		dbgLog(REPORT,"%s: version  Error. Errno:%d Err:%s\n",fnName,errno,strerror(errno));
		return RET_FAILURE;
	}
	else
	{
		memset(buffer,0,SIZE_128);
		sprintf(buffer,"%s",VERSION_NUMBER);
		len = strlen(buffer);
		ret=write(fd,(char *)buffer,len);
		if( ret != len)
		{
			dbgLog(REPORT,"%s: version Write Failed. Size Mismatch.wrote:%d. Expected:%d.Errno:%d Err:%s\n",fnName,ret,len,errno,strerror(errno));
			close(fd);
			return RET_FAILURE;
		}
		dbgLog(INFORM,"%s: Version write Success.Wrote:%d. Expected:%d.\n",fnName,ret,len);
		close(fd);
	}
	return RET_OK;
}

/**
 *  Function    : fillProcCfgFile
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int fillProcCfgFile(void)
{
	int i=0;
	int k=0;

	memset(	&procInfo[k],0,sizeof(PROC_INFO));
	memcpy(&procInfo[k].procName,METER_COM_PROC,sizeof(METER_COM_PROC));
	// sprintf(procInfo[k].cmdLineArg1,"1");
	// sprintf(procInfo[k].cmdLineArg2,"/dev/tts/1");
	k++;

	if(pmonConfig.comDataTransfer.dataTransferEnable == ENABLE)
	{
		if(pmonConfig.comDataTransfer.selectedDataTransType == FTP)
		{
			memset(	&procInfo[k],0,sizeof(PROC_INFO));
			memcpy(&procInfo[k].procName,FTPC_PROC,sizeof(FTPC_PROC));
			// sprintf(procInfo[k].cmdLineArg1,"1");
			// sprintf(procInfo[k].cmdLineArg2,"/dev/tts/1");
			k++;
		}
		else if(pmonConfig.comDataTransfer.selectedDataTransType == MQTT)
		{
			//Mqtt not developed , so using ftp for both
			memset(	&procInfo[k],0,sizeof(PROC_INFO));
			memcpy(&procInfo[k].procName,FTPC_PROC,sizeof(FTPC_PROC));
			// sprintf(procInfo[k].cmdLineArg1,"1");
			// sprintf(procInfo[k].cmdLineArg2,"/dev/tts/1");
			k++;
		}
	}
	else
	{
		dbgLog(INFORM,"procCfg():Data file transfer is disabled\n");
	}

	#if 0
	if(pmonConfig.comDataTransfer.liveDataTransferEnable == ENABLE)
	{
			memset(	&procInfo[k],0,sizeof(PROC_INFO));
			memcpy(&procInfo[k].procName,LIVE_DATA_POST_PROC,sizeof(FTPC_PROC));
			// sprintf(procInfo[k].cmdLineArg1,"1");
			// sprintf(procInfo[k].cmdLineArg2,"/dev/tts/1");
			k++;
	}
	else
	{
		dbgLog(INFORM,"procCfg():Live Data file transfer is disabled\n");
	}
	#endif
	
	numProcs = k;
	dbgLog(INFORM,"procCfg():Num of procs : %d\n",numProcs);

	return RET_OK;
}

/**
 *  Function    : invokeProcs
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int invokeProcs(void)
{
	int i=0;
	int ret=0;
	static char fn[] = "invokeProcs()";
	
	dbgLog(INFORM,"%s:%d:%s:Number of procs %d\n",__FILE__, __LINE__, fn,numProcs);
	for ( i = 0; i < numProcs; i++ )
	{
		execProc(i);
		sleep(5);
	}
	dbgLog(INFORM,"*** End of invokeProc ***\n");
	return RET_OK;
}

/**
 *  Function    : execProc
 *  Params [in] : "idx" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void execProc(int idx)
{
	int ret;
	static char fn[] = "execProc()";
	time_t	tim;
	FILE  *fp;
	tim = time(NULL);
	
	dbgLog(INFORM,"%s:%d:%s:Starting child  %d proc %s \n",__FILE__, __LINE__, fn,idx,procInfo[idx].procName);
	
	ret = startChild(procInfo[idx].procName,procInfo[idx].cmdLineArg1,procInfo[idx].cmdLineArg2);
	if ( ret <= 0 )
	{
		dbgLog(FATAL, "%s:%d:%s:Failed to start child %d \n",__FILE__, __LINE__, fn,idx);
		procInfo[idx].status = 0;
	}
	else
	{
		procInfo[idx].status = 1;
		procInfo[idx].pid = ret;
		procInfo[idx].restartTime = tim;
		dbgLog(INFORM, "%s:%d:%s:Started child %d successfully , pid %d\n",__FILE__, __LINE__, fn,idx,ret);
	}
	dbgLog(INFORM, "***** End of execProc****\n");
	return;
}

/**
 *  Function    : startChild
 *  Params [in] : "procName" Parameter_Description
 *  Params [in] : "arg1"     Parameter_Description
 *  Params [in] : "arg2"     Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
int startChild(char *procName,char *arg1,char *arg2)
{
	pid_t		childPid=0;
	static char fn[]="startChild()";
	pid_t sid = 0;

	switch( (childPid=fork()) )
	{
		case -1: // fork failed - log and exit or return;
		{
			dbgLog(FATAL, "%s:%d:%s:Fork failed %s\n",__FILE__, __LINE__, fn,strerror(errno));
			exit(-1);
		}
		break;
		case 0: // 'child process'
		{
			switchStdout("/dev/null");
			if( execlp(procName, procName,arg1,arg2,NULL) == -1 )
			{
				revertStdout();
				dbgLog(FATAL, "%s:%d:%s:execlp failed %s\n",__FILE__, __LINE__, fn,strerror(errno));
				exit(-1);
			}
			revertStdout();
		}
		break;
		default: // 'parent process'
		{
			dbgLog(INFORM,"%s:%d:%s:Fork succeeded pid %d\n",__FILE__, __LINE__, fn,childPid);
			return childPid;
		}
		break;
	}
	return 0;
}

/**
 *  Function    : waitForChild
 *  Params [in] : "sig" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void waitForChild(int sig)
{
	int			status=0;
	pid_t		chPid=0;
	int			i;
	time_t		timeNow;
	int 		*val;
	static char fn[] = "waitForChild()";
	
	chPid = waitpid(-1,&status,0);
	//chPid = wait(val);
	dbgLog(INFORM, "%s:%d:%s:Received SIGCHLD: pid %d, status %d\n",__FILE__, __LINE__, fn,chPid,status);

	if ( killAll == 1 )
	{
		dbgLog(INFORM,"Killing the child process..\n");
		return;
	}
	for ( i = 0; i < numProcs; i++ )
	{
		if ( procInfo[i].status == 1 )
		{
			if ( procInfo[i].pid == chPid )
			{
				dbgLog(INFORM ,"%s:%d:%s:Child %d : %s \n",__FILE__, __LINE__, fn,i,procInfo[i].procName);
				procInfo[i].status = 0;
				//get time
				timeNow = time(NULL);
				dbgLog(INFORM, "%s:%d:%s:Num of restarts %d TimeDiff %d\n",__FILE__, __LINE__, fn,procInfo[i].numRestart,( timeNow - procInfo[i].numRestart));
				execProc(i);
				procInfo[i].restartTime = timeNow;
				procInfo[i].numRestart = 0; 
				return;
			}	
		}
	}
}

 /**
 *  Function    : sigHandler
 *  Params [in] : "sig" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Signal handler for parent - procmon
 */
void sigHandler(int sig)
{
	static char fn[] = "sigHandler()";
	dbgLog(INFORM ,"Signal %d\n",sig);
	if ( sig == SIGALRM )
	{
		printf("IN SIGALARM\n");
		alarm(60);
		return;
	}
	dbgLog(INFORM , "%s:%d:%s:Received signal %d. Kill all the processes\n",__FILE__, __LINE__, fn,sig);
	killAllProc();
	writeRebootInfo(SIGNAL_INTR);
	exit(0);
}

/**
 *  Function    : killAllProc
 *  Return [out]: Return_Description
 *  Details     : kills all the processes if pmon dies
 */
void killAllProc(void)
{
	int i;
	static char fn[] = "killAllProc()";
	killAll = 1;
	
	for ( i = 0; i < numProcs; i++ )
	{
		if ( procInfo[i].status )
		{
			dbgLog(INFORM, "%s:%d:%s:Killing process %s pid %d\n",__FILE__, __LINE__, fn,procInfo[i].procName,procInfo[i].pid);
			kill(procInfo[i].pid,SIGKILL);
			gotoSleep(0,250000);
		}
	}
}

/**
 *  Function    : restartApp
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void restartApp(void)
{
	static char fn[] = "restartApp()";
	int waitCnt = 0;
	restart = 1;
		
	//store number of processess
	tempNumProcs = numProcs;
	killAllProc();
	gotoSleep(5, 0);

	//blindly wait for max of 5 seconds if the processes are not killed
	//observation - some are in zombie state after this while loop
	while (( tempNumProcs != 0 ) && ( waitCnt < 15 ))
	{
		gotoSleep(1,0);
		waitCnt++;
	}

	dbgLog(INFORM,"%s:%d:%s After killing all the processes.\n",__FILE__,__LINE__,fn);
	numProcs = 0;
	killAll = 0;
	restartProcs();
	dbgLog(INFORM,"End of restartApp");
	restart = 0;
	
	return;
}

/**
 *  Function    : killPppd
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void killPppd(void) //not used
{
	FILE *fp;
	char line[32];
	char command[32];
	
	memset(command,0,32);
	memset(line,0,32);
	
	dbgLog(INFORM,"In killPpd");
	
	sprintf(command,"pidof pppd");
	fp = popen(command,"r");
	if ( fp == NULL)
	{
		dbgLog(INFORM,"PIDof cmd failed\n");
		return;
	}
	if ( fgets(line,sizeof(line),fp) == NULL )
	{
		dbgLog(INFORM,"NO resp of pidof\n");
		pclose(fp);
		return;
	}
	pclose(fp);
	dbgLog(INFORM,"PID LINE %d\n",line);

	memset(command,0,sizeof(command));
	sprintf(command,"kill -9 %s",line);
	system(command);

	dbgLog(INFORM,"After system command to kill pppd");
	return;
}

/**
 *  Function    : restartProcs
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void restartProcs(void)
{
	//killPppd();

	//read the config dir 
	readConfigFile();
	fillProcCfgFile();
	
	dbgLog(INFORM,"After parsing through config dir\n");

	//invoke the processes
	invokeProcs();
	dbgLog(INFORM,"After invoking child processes after restart\n");

}

/**
 *  Function    : gotoSleep
 *  Params [in] : "secs"  Parameter_Description
 *  Params [in] : "usecs" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void gotoSleep(int secs, int usecs)
{
	int 				retVal=0;
	struct 	timeval		sleepTime;

	static char 		fn[]="gotoSleep()";
	
	sleepTime.tv_sec  = secs;
	sleepTime.tv_usec = usecs;

	// NOTE: ********** V. V. IMPORTANT
    //       only select on Linux implements 
    //       time-out such that if 'select' returns
    //       interrupted, the 'timeout' arg contains
    //       the remaining time left. When porting
    //       to another arch, make sure we take care
    //       of this.
	while(1)
	{
		if( (retVal=select(0, 0, 0, 0, &sleepTime)) == -1)
		{
			if(errno == EINTR)
				continue;
			else
			{
				dbgLog(REPORT, "%s:%d:%s: select failed - %s\n",__FILE__, __LINE__, fn, strerror(errno)); 
				break;
			}
		}
		else
			break;
	}
	return;
}

/**
 *  Function    : switchStdout
 *  Params [in] : "newStream" Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void switchStdout(const char *newStream)
{
  fflush(stdout);
  fgetpos(stdout, &pos);
  fdSout = dup(fileno(stdout));
  freopen(newStream, "w", stdout);
}

/**
 *  Function    : revertStdout
 *  Return [out]: Return_Description
 *  Details     : Details
 */
void revertStdout(void)
{
  fflush(stdout);
  dup2(fdSout, fileno(stdout));
  close(fdSout);
  clearerr(stdout);
  fsetpos(stdout, &pos);
}

/* EOF */
