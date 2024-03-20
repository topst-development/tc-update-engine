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
#include <errno.h>
#include <linux/limits.h>
#include "update-engine-def.h"
#include "update-log.h"
#include "boot-control.h"
#include "firmware-update.h"
#include "ab-updater.h"

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define main_misc_dev	"/dev/disk/by-partlabel/misc"
#define sub_misc_dev	"/dev/disk/by-partlabel/subcore_misc"

struct imageTargetInfo
{
	const char *imageName;
	const char *targetName;
	int32_t		imageType;
	int32_t		canIgnore;
};

#ifdef UPDATE_TCC803X
static struct imageTargetInfo mainCoreImageList[] = {
	{MAIN_BOOTLOADER_IMAGE, "/dev/disk/by-partlabel/bl3_ca53",		UPDATE_BOOTLOADER, 	0},
	{MAIN_KERNEL_IMAGE, 	"/dev/disk/by-partlabel/boot", 			UPDATE_KERNEL, 		0},
	{MAIN_ROOTFS_IMAGE,		"/dev/disk/by-partlabel/system", 		UPDATE_ROOT_FS, 	0},
	{MATN_DTB_IMAGE,		"/dev/disk/by-partlabel/dtb", 			UPDATE_DTB, 		0},
	{NULL, NULL, -1, 0},
};

static struct imageTargetInfo subCoreImageList[] = {
	{SUB_KERNEL_IMAGE,		"/dev/disk/by-partlabel/a7s_boot", 		UPDATE_KERNEL, 		0},
	{SUB_ROOTFS_IMAGE,		"/dev/disk/by-partlabel/a7s_root", 		UPDATE_ROOT_FS, 	0},
	{SUB_DTB_IMAGE, 		"/dev/disk/by-partlabel/a7s_dtb",		UPDATE_DTB, 		0},
	{NULL, NULL, -1, 0},
};
#else
static struct imageTargetInfo mainCoreImageList[] = { 
	{MAIN_BOOTLOADER_IMAGE, "/dev/disk/by-partlabel/bl3_ca72",		UPDATE_BOOTLOADER, 	0},
	{MAIN_KERNEL_IMAGE, 	"/dev/disk/by-partlabel/boot", 			UPDATE_KERNEL, 		0},
	{MAIN_ROOTFS_IMAGE,		"/dev/disk/by-partlabel/system", 		UPDATE_ROOT_FS, 	0},
	{MATN_DTB_IMAGE,		"/dev/disk/by-partlabel/dtb", 			UPDATE_DTB, 		0},
	{NULL, NULL, -1, 0},
};

static struct imageTargetInfo subCoreImageList[] = { 
	{SUB_BOOTLOADER_IMAGE,	"/dev/disk/by-partlabel/bl3_ca53", 			UPDATE_BOOTLOADER, 	0},
	{SUB_KERNEL_IMAGE,		"/dev/disk/by-partlabel/subcore_boot", 		UPDATE_KERNEL, 		0},
	{SUB_ROOTFS_IMAGE,		"/dev/disk/by-partlabel/subcore_root", 		UPDATE_ROOT_FS, 	0},
	{SUB_DTB_IMAGE, 		"/dev/disk/by-partlabel/subcore_dtb",		UPDATE_DTB, 		0},
	{SPLASH_IMAGE,			"/dev/disk/by-partlabel/subcore_splash",	UPDATE_SPLASH, 		1},
	{NULL, NULL, -1, 0},
};
#endif
struct coreUpdateInfo
{
	const char *misc_dev;
	struct imageTargetInfo *imageList;	
};

#ifdef UPDATE_TCC803X
static struct coreUpdateInfo gCoreUpdateInfo[2] = {
								{main_misc_dev, mainCoreImageList},	/* Main */
								{main_misc_dev, subCoreImageList}	/* sub */
};
#else
static struct coreUpdateInfo gCoreUpdateInfo[2] = {
								{main_misc_dev, mainCoreImageList},	/* Main */
								{sub_misc_dev, subCoreImageList}	/* sub */
};

#endif


int32_t bootSuccess(int32_t coreID)
{
	int32_t ret = -1;
	DEBUG_UPDATE_PRINTF("In. coreID(%d)\n", (int32_t)coreID);

	if(coreID < (int32_t)MAX_CORE)
	{
		struct coreUpdateInfo * coreInfo = &gCoreUpdateInfo[coreID];
		struct boot_control_module bootctrl_module;

		ret = bootControlInit(coreInfo->misc_dev, &bootctrl_module);
		if(ret == 0)
		{
			ret = markBootSuccessful(&bootctrl_module);
			if(ret == 0)
			{
				DEBUG_UPDATE_PRINTF("bootSuccess OK\n");
			}
		}
	}
	if(ret < 0)
	{
		ERROR_UPDATE_PRINTF("bootSuccess error\n");
	}

	return ret;
}


int32_t sourceImageCheck(int32_t coreID, const char *sourceDir)
{
	int32_t ret = -1;
	DEBUG_UPDATE_PRINTF("In\n");

	if((coreID < (int32_t)MAX_CORE)&&(sourceDir != NULL))
	{
		char imageFullPath[PATH_MAX];
		int32_t i=0;
		struct coreUpdateInfo * coreInfo = &gCoreUpdateInfo[coreID];

		DEBUG_UPDATE_PRINTF("coreID(%d), sourceDir(%s)\n", coreID,sourceDir);

		ret = 0;
		while(coreInfo->imageList[i].imageName != NULL)
		{
			if((strnlen(sourceDir,PATH_MAX)+strnlen(coreInfo->imageList[i].imageName,PATH_MAX)) <= (size_t)PATH_MAX)
			{
				memset(imageFullPath, 0x00, PATH_MAX);
				sprintf(imageFullPath, "%s/%s", sourceDir, coreInfo->imageList[i].imageName);

				ret = access(imageFullPath, R_OK);
				if(ret != 0)
				{
					if(coreInfo->imageList[i].canIgnore == 1)
					{
						WARN_UPDATE_PRINTF("Can Not find %s. skip\n", imageFullPath);
						ret = 0;
					}
					else
					{
						ERROR_UPDATE_PRINTF("Can Not find %s\n", imageFullPath);
						ret = -ENOENT;
						break;
					}
				}
				else
				{
					DEBUG_UPDATE_PRINTF("Check '%s' : OK.\n", imageFullPath);
				}
			}
			else
			{
				ERROR_UPDATE_PRINTF("PATH_MAX is %d.But current path is too long (%s/%s)",
					PATH_MAX, sourceDir, coreInfo->imageList[i].imageName);
				break;
			}
			i++;
		}
	}

	return ret;
}

int32_t updateCorefwImage(int32_t coreID, const char *sourceDir)
{
	int32_t ret = -1;
	DEBUG_UPDATE_PRINTF("In\n");

	if((coreID < (int32_t)MAX_CORE)&&(sourceDir != NULL))
	{
		struct coreUpdateInfo * coreInfo = &gCoreUpdateInfo[coreID];
		struct boot_control_module bootctrl_module;
		struct imageTargetInfo *coreImageList = gCoreUpdateInfo[coreID].imageList;

		DEBUG_UPDATE_PRINTF("coreID(%d), sourceDir(%s)\n", coreID,sourceDir);

		bootctrl_module.isInit = false;

		/* Check source image */
		ret = sourceImageCheck(coreID, sourceDir);
		if(ret == 0)
		{
			/* 1. Init boot control module */
			ret = bootControlInit(coreInfo->misc_dev, &bootctrl_module);
			if(ret < 0)
			{
				ERROR_UPDATE_PRINTF("Boot control init error(%d)\n", ret);
			}

			/* 2. check mark BootSucessful flag */			
			if(ret == 0)
			{
				int32_t isBootSuccess;
				isBootSuccess = isSlotMarkedSuccessful(&bootctrl_module, bootctrl_module.current_slot);
				if(isBootSuccess == 1)
				{
					DEBUG_UPDATE_PRINTF("bootSuccess is 1\n");
				}
				else if(isBootSuccess == 0)
				{
					ERROR_UPDATE_PRINTF("bootSuccess is 0. Can't update this core(%d)\n", coreID);
					ret = -1;
				}
				else
				{
					ret = -1;
				}
			}

			/* 3. select update slot*/
			if(ret == 0)
			{
				int32_t slot_index;

				bootctrl_module.update_slot = -1;

				for(slot_index = 0; slot_index < bootctrl_module.nb_slot; slot_index++)
				{
					if(slot_index != bootctrl_module.current_slot)
					{
						bootctrl_module.update_slot =  slot_index;
					}
				}

				if((bootctrl_module.update_slot < 0)&&(bootctrl_module.update_slot > bootctrl_module.nb_slot))
				{
					ERROR_UPDATE_PRINTF("update slot(%d) is not enable\n", bootctrl_module.update_slot);
					ret = -1;
				}
				else
				{
					DEBUG_UPDATE_PRINTF("update slot (%d)\n", bootctrl_module.update_slot);
				}
			}

			/* 4. setSlotAsUnbootable */
			if(ret == 0)
			{
				ret = setSlotAsUnbootable(&bootctrl_module, bootctrl_module.update_slot);
				if(ret < 0)
				{
					ERROR_UPDATE_PRINTF("setSlotAsUnbootable is error(%d)\n", ret)
				}
			}

			/* 5. update image */
			if((ret == 0)&&(coreImageList != NULL))
			{
				int32_t imageIndex = 0;
				char image_path[PATH_MAX], target_path[PATH_MAX];
				while(coreImageList[imageIndex].imageName != NULL)
				{
					if(((strnlen(sourceDir,PATH_MAX)+strnlen(coreInfo->imageList[imageIndex].imageName,PATH_MAX)) <= (size_t)PATH_MAX))
					{

						memset(image_path, 0x00, PATH_MAX);
						sprintf(image_path, "%s/%s", sourceDir, coreInfo->imageList[imageIndex].imageName);

						if(coreInfo->imageList[imageIndex].imageType == UPDATE_BOOTLOADER)
						{
							memset(target_path, 0x00, PATH_MAX);
							sprintf(target_path, "%s_a", 
								coreInfo->imageList[imageIndex].targetName);

							ret = update_firmware(coreImageList[imageIndex].imageType, image_path, target_path);
							if(ret < 0)
							{
								ERROR_UPDATE_PRINTF("update_firmware error\n source(%s)->target(%s)\n",image_path, target_path);
								break;
							}

							memset(target_path, 0x00, PATH_MAX);
							sprintf(target_path, "%s_b", 
								coreInfo->imageList[imageIndex].targetName);
							ret = update_firmware(coreImageList[imageIndex].imageType, image_path, target_path);
							if(ret < 0)
							{
								ERROR_UPDATE_PRINTF("update_firmware error\n source(%s)->target(%s)\n",image_path, target_path);
								break;
							}
						}
						else
						{
							memset(image_path, 0x00, PATH_MAX);
							sprintf(image_path, "%s/%s", sourceDir, coreInfo->imageList[imageIndex].imageName);

							memset(target_path, 0x00, PATH_MAX);
							sprintf(target_path, "%s_%s",
								coreInfo->imageList[imageIndex].targetName,
								slot_index_to_suffix((uint32_t)bootctrl_module.update_slot));

							ret = update_firmware(coreImageList[imageIndex].imageType, image_path, target_path);
							if(ret < 0)
							{
								if(coreImageList[imageIndex].canIgnore == 1)
								{
									INFO_UPDATE_PRINTF("skip : source(%s)\n",image_path);
									ret = 0;
								}
								else
								{
									ERROR_UPDATE_PRINTF("update_firmware error\n source(%s)->target(%s)\n",image_path, target_path);
									break;
								}
							}
						}
					}
					else
					{
						ERROR_UPDATE_PRINTF("PATH_MAX is %d.But current path is too long (%ld)",
							PATH_MAX,
							(strnlen(sourceDir,PATH_MAX) + strnlen(coreInfo->imageList[imageIndex].imageName,PATH_MAX)));
					}
					imageIndex++;	
				}
			}

			if((bootctrl_module.isInit == true) && (ret == 0))
			{
				/* 6. setActiveBootSlot */
				ret = setActiveBootSlot(&bootctrl_module, bootctrl_module.update_slot);
				if(ret < 0)
				{
					ERROR_UPDATE_PRINTF("setSlotAsUnbootable is error(%d)\n", ret)
				}
			}

			if(ret == 0)
			{
				DEBUG_UPDATE_PRINTF("FW Update Done\n");
			}
		}
		else
		{
			ERROR_UPDATE_PRINTF("source Image Check error\n");
		}
	}

	return ret;
}

