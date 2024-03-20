/****************************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 *
This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited 
to re-distribution in source or binary form is strictly prohibited.
This source code is provided “AS IS” and nothing contained in this source code 
shall constitute any express or implied warranty of any kind, including without limitation, 
any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, 
copyright or other third party intellectual property right. 
No warranty is made, express or implied, regarding the information’s accuracy, 
completeness, or performance. 
In no event shall Telechips be liable for any claim, damages or other liability arising from, 
out of or in connection with this source code or the use in the source code. 
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement 
between Telechips and Company.
*
****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <tcc_snor_updater_dev.h>
#include "update-engine-def.h"
#include "update-log.h"
#include "ab-updater.h"
#include "snor-update.h"
#include "update-engine.h"

int32_t g_update_debug = UPDATE_ERROR_LEVEL;

static int32_t updateMainCoreImageCheck(const char * sourcePath);
static int32_t updateSubCoreImageCheck(const char * sourcePath);
static int32_t updateSnorImageCheck(const char * sourcePath);

static void * UpdateThreadFunction(void *arg);

void setDebugLogLevel(int32_t logLevel)
{
	g_update_debug = logLevel;
}

int32_t initUpdateEngine(int32_t myCore)
{
	int32_t ret;
	ret = bootSuccess(myCore);
	return ret;
}

static int32_t updateMainCoreImageCheck(const char * sourcePath)
{
	int32_t ret;
	ret = sourceImageCheck(MAIN_CORE, sourcePath);
	DEBUG_UPDATE_PRINTF("Check Main core image : %d\n", ret);

	return ret;
}

static int32_t updateSubCoreImageCheck(const char * sourcePath)
{
	int32_t ret;
	ret = sourceImageCheck(SUB_CORE, sourcePath);
	DEBUG_UPDATE_PRINTF("Check Sub core image : %d\n", ret);

	return ret;
}

static int32_t updateSnorImageCheck(const char * sourcePath)
{
	int32_t ret = -1;
	char image_path[PATH_MAX];
	if(sourcePath != NULL)
	{
		/* 1.open snor image */
		if((strnlen(sourcePath,PATH_MAX)+strnlen(SNOR_IMAGE,PATH_MAX)) <= (size_t)PATH_MAX)
		{
			memset(image_path, 0x00, PATH_MAX);
			sprintf(image_path, "%s/%s", (const char *)sourcePath, (const char *)SNOR_IMAGE);
			ret = access(image_path, R_OK);
			DEBUG_UPDATE_PRINTF("Check Snor image : %d\n", ret);
		}
	}
	return ret;
}

int32_t updateFWImageFileCheck(int32_t core_ID, const char * sourcePath)
{
	int ret = -1;
	if(sourcePath != NULL)
	{
		if(core_ID == MAIN_CORE)
		{
			ret = updateMainCoreImageCheck(sourcePath);
		}
		else if(core_ID == SUB_CORE)
		{
			ret = updateSubCoreImageCheck(sourcePath);
		}
		else if(core_ID == MICOM_CORE)
		{
			ret = updateSnorImageCheck(sourcePath);
		}
		else
		{
			ret = -1;
			ERROR_UPDATE_PRINTF("Unknown Core ID(%d)\n", ret);
		}
	}
	return ret;
}


int32_t updateFWImage(int32_t core_ID, const char * sourcePath)
{
	int32_t ret = -1;
	/* Check source image */
	if(sourcePath != NULL)
	{
		DEBUG_UPDATE_PRINTF("Core(%d), src path(%s)\n", core_ID, sourcePath);
		if(core_ID == MAIN_CORE)
		{
			ret = updateCorefwImage(MAIN_CORE, sourcePath);
		}
		else if(core_ID == SUB_CORE)
		{
			ret = updateCorefwImage(SUB_CORE, sourcePath);
		}
		else if(core_ID == MICOM_CORE)
		{
			ret = updateSnorImage(sourcePath);
		}
		else
		{
			ret = -1;
			ERROR_UPDATE_PRINTF("Unknown Core ID(%d)\n", ret);
		}

	}

	return ret;
}




