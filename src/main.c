/****************************************************************************************
 *   FileName    : ipc_test.c
 *   Description : 
 ****************************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited 
to re-distribution in source or binary form is strictly prohibited.
This source code is provided ¡°AS IS¡± and nothing contained in this source code 
shall constitute any express or implied warranty of any kind, including without limitation, 
any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, 
copyright or other third party intellectual property right. 
No warranty is made, express or implied, regarding the information¡¯s accuracy, 
completeness, or performance. 
In no event shall Telechips be liable for any claim, damages or other liability arising from, 
out of or in connection with this source code or the use in the source code. 
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement 
between Telechips and Company.
*
****************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdint.h>
#include "update-engine-def.h"
#include "update-log.h"
#include "update-engine.h"

#define UPDATE_TARGET_MAIN 		(1 << 0)
#define UPDATE_TARGET_SUB 		(1 << 1)
#define UPDATE_TARGET_SNOR 		(1 << 2)


const char *update_pid_file = "/var/run/update-engine.pid";

static struct option long_opt[] = {
	{"daemon", required_argument, NULL, 'd'},
	{"current", required_argument, NULL, 'c'},
	{"fwdir", required_argument, NULL, 'f'},
	{"target", required_argument, NULL, 't'},
	{"log", required_argument, NULL, 'l'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, NULL, 0}
};

static void SignalHandler(int sig);
static void help_msg(char* argv);
static void help_msg(char* argv)
{
	(void)printf("------------------------------------------------------------\n"
           "|  Linux_IVI update-engine\n"
           "| ===========================\n"
           "| <Daemonize running>\n"
           "|    daemon on \n"
           "|    ex) --daemon=1 or -d 1 \n"
           "|    daemon off \n"
           "|    ex) --daemon=0 or -d 0 \n"
           "| <Current core>\n"
           "|    current core (main or sub) \n"
           "|    ex) --current=main -c main \n"
		   "| <fwimage dir path>\n"
		   "|    select ipc for micom device \n"
		   "|    ex) --fwdir=/run/media/sda1/update -f /run/media/sda1/update \n"
		   "| <select update Target core>\n"
		   "|    update target core bit \n"
		   "|        main : 1 <<1\n"
		   "|        sub  : 1 <<2\n"
		   "|        snor : 1 <<3\n"
		   "|    ex) $ --target=main 7 -t main 7 \n"
           "| <Debug log>\n"
           "|     set debug level \n"
           "|     ex) $ --log=<debug level> or -l <debug level> \n"
           "|     error   : 0 \n"
           "|     warning : 1 \n"
           "|     info    : 2 \n"
           "|     debug   : 3 \n"
           "|\n"
           "------------------------------------------------------------\n");
}

static void SignalHandler(int sig)
{
	;
}


static void Daemonize(void)
{
	pid_t pid = 0;
	FILE *pid_fp;

	// create child process
	pid = fork();

	// fork failed
	if (pid < 0)
	{
		ERROR_UPDATE_PRINTF("fork failed\n");
		exit(1);
	}
	// parent process
	if (pid > 0)
	{
		// exit parent process for daemonize
		exit(0);
	}
	// umask the file mode
	umask(0);

	// create pid file

	pid_fp = fopen(update_pid_file, "w+");
	if (pid_fp != NULL)
	{
		(void)fprintf(pid_fp, "%d\n", getpid());
		fclose(pid_fp);
		pid_fp = NULL;
	}
	else
	{
		ERROR_UPDATE_PRINTF("pid file open failed");
	}
	// to do: open log

	// set new session
	if (setsid() < 0)
	{
		ERROR_UPDATE_PRINTF("set new session failed\n");
		exit(1);
	}

	// change the current working directory for safety
	if (chdir("/") < 0)
	{
		ERROR_UPDATE_PRINTF("change directory failed\n");
		exit(1);
	}
}

int32_t main(int32_t argc, char **argv)
{
	int32_t ret;
	char switch_num;
	int32_t option_idx = 0;
	int32_t opt_val = 0;
	int32_t debug_level = 0;
	int32_t daemonize = 0;
	int32_t current_core = -1;
	int32_t target_core = -1;
	const char * fwimage_path = NULL;

	
	signal(SIGINT, SignalHandler);
	signal(SIGTERM, SignalHandler);
	signal(SIGABRT, SignalHandler);
	signal(SIGKILL, SignalHandler);

	if (argc < 2) {
		help_msg(NULL);
		ret = -1;
	}
	else {
		while(1)
		{
			switch_num = getopt_long(argc, argv,"d:c:f:t:l:h" , long_opt, &option_idx);
			if (switch_num == (char)-1)
			{ 
				break;
			}
			if (switch_num == (char)255)
			{ 
				break;
			}
			if(optarg != NULL)
			{
				opt_val = atoi(optarg);
			}

			switch(switch_num)
			{
				case 0:
					if (long_opt[option_idx].flag != NULL)
						break;

					INFO_UPDATE_PRINTF("option %s", long_opt[option_idx].name);
	
					if (optarg != NULL)
					{
						INFO_UPDATE_PRINTF(" with arg %s\n", optarg);
					}
					break;
				case 'd':
					if(opt_val != 0){
						daemonize = opt_val;
					}
					else{
						daemonize = 0;
					}
					INFO_UPDATE_PRINTF("Daemonize: 0x%x \n", opt_val);
					break;
				case 'c':
					if(optarg != NULL)
					{
						INFO_UPDATE_PRINTF("current core : %s \n", optarg);
						if(strncmp(optarg,"main",3)==0)
						{
							current_core = MAIN_CORE;
						}
						else if(strncmp(optarg,"sub",3)==0)
						{
							current_core = SUB_CORE;
						}
						else
						{
							current_core = -1;
						}
					}
					break;					
				case 'f':
					if(optarg != NULL)
					{
						fwimage_path = optarg;
						INFO_UPDATE_PRINTF("fw image path : %s \n", fwimage_path);
					}
					break;
				case 't':
					target_core = opt_val;
					INFO_UPDATE_PRINTF("target core : %d \n", target_core);
					break;
				case 'l':
					if(opt_val){
						debug_level = opt_val;
						setDebugLogLevel(debug_level);
					}
					else{
						debug_level= 0;
					}
					INFO_UPDATE_PRINTF("debug log : 0x%x \n", debug_level);
					break;
				case 'h':
					help_msg(NULL);
					break;
				default :
					INFO_UPDATE_PRINTF("Wrong options %d \n",switch_num);
					break;
			}
		}		
	}

	if (daemonize == 1)
	{
		Daemonize();
	}


	if((current_core < (int32_t)0)&&(current_core >= (int32_t)MAX_CORE))
	{
		ERROR_UPDATE_PRINTF("Must Set current Core!!\n");
	}
	else
	{
		ret = initUpdateEngine(current_core);
		if(ret == 0)
		{
			if((target_core > -1)&&(fwimage_path != NULL))
			{
				if((target_core & (int32_t)UPDATE_TARGET_MAIN) == (int32_t)UPDATE_TARGET_MAIN)
				{
					INFO_UPDATE_PRINTF("Update MAIN Core\n");
					ret = updateFWImage(MAIN_CORE, (const char *)fwimage_path);
				}
				if((target_core & (int32_t)UPDATE_TARGET_SUB) == (int32_t)UPDATE_TARGET_SUB)
				{
					INFO_UPDATE_PRINTF("Update Sub Core\n");
					ret = updateFWImage(SUB_CORE, (const char *)fwimage_path);

				}
				if((target_core & (int32_t)UPDATE_TARGET_SNOR) == (int32_t)UPDATE_TARGET_SNOR)
				{
					INFO_UPDATE_PRINTF("Update Snor Core\n");
					ret = updateFWImage(MICOM_CORE, (const char *)fwimage_path);

				}
			}
		}
	}

	if (access(update_pid_file, F_OK) == 0)
	{
		if (unlink(update_pid_file) != 0)
		{
			ERROR_UPDATE_PRINTF("delete pid file failed\n");
		}
	}

	return ret;
}



