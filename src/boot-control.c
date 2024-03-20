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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <linux/limits.h>
#include "update-engine-def.h"
#include "update-log.h"
#include "bootloader-message.h"
#include "boot-control.h"

size_t bootLoaderControlOffset = offsetof(struct bootloader_message_ab, slot_suffix);

const char * slotSuffixes[MAX_NUM_SLOTS] = { "a", "b", "c", "d" };

static uint32_t calc_crc32_le(const uint8_t* buf, size_t size);
static int32_t loadBootControl(const char *misc, struct bootloader_control* bootctrl);
static int32_t upateBootControl(const char *misc, struct bootloader_control* bootctrl);
static void printBootControlInfo(struct bootloader_control *bootctrl);

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

uint32_t slot_suffix_to_index(const char* suffix)
{
    uint32_t ret = 0;
	uint32_t slot;
	for (slot = 0; slot < (uint32_t)MAX_NUM_SLOTS; slot++) 
	{
		if (strncmp(slotSuffixes[slot], suffix, 1)==0)
		{
			ret = slot;
			break;
		}
	}

	return ret;
}

const char *slot_index_to_suffix(uint32_t index)
{
	const char * suffix = NULL;
	if(index < (uint32_t)MAX_NUM_SLOTS)
	{
		suffix = slotSuffixes[index];
	}
	return suffix;
}

static uint32_t calc_crc32_le(const uint8_t* buf, size_t size)
{
    static uint32_t crc_table[256];
    uint32_t ret = UINT32_MAX;

    // Compute the CRC-32 table only once.
    if (crc_table[1] == 0) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (uint32_t j = 0; j < 8; ++j) {
                uint32_t mask = -(crc & 1);
                crc = (crc >> 1) ^ (0xedb88320 & mask);
            }
            crc_table[i] = crc;
        }
    }

    for (size_t i = 0; i < size; ++i) {
        ret = (ret >> 8) ^ crc_table[(ret ^ buf[i]) & 0xff];
    }

    return ~ret;
}


static int32_t loadBootControl(const char *misc, struct bootloader_control* bootctrl)
{
	int ret = 0;

	DEBUG_UPDATE_PRINTF("In\n");

	if((misc != NULL)&&(bootctrl != NULL))
	{
		int fd;
		DEBUG_UPDATE_PRINTF("misc : %s\n", misc);
		fd = open(misc, O_RDONLY);
		if(fd < 0)
		{
			ERROR_UPDATE_PRINTF("Can't open %s device : (%s)\n", misc, strerror(errno));
			ret = -errno;
		}
		else
		{
			if(lseek(fd,(off_t)bootLoaderControlOffset, SEEK_SET) != (off_t)bootLoaderControlOffset)
			{
				ERROR_UPDATE_PRINTF("Can't seek to offset %d : (%s)\n", (int32_t)bootLoaderControlOffset, strerror(errno));
				ret = -errno;
			}
			else
			{
				ssize_t nread;
				char * buf = (char *)bootctrl;
				size_t request_size = sizeof(struct bootloader_control);
				nread = read(fd, buf, request_size);
				if(nread == (ssize_t)request_size)
				{
					if(bootctrl->magic != (uint32_t)BOOT_CTRL_MAGIC)
					{
						ERROR_UPDATE_PRINTF("Mismatch magic code. (0x%08x):(0x%08x)\n", bootctrl->magic, BOOT_CTRL_MAGIC);
						ret = -1;
					}
					else
					{
						printBootControlInfo(bootctrl);
						DEBUG_UPDATE_PRINTF("load boot control success.\n");
					}
				}
				else
				{
					ERROR_UPDATE_PRINTF("Can't read boot ctrl message (%d) : (%s)\n", (int32_t)request_size, strerror(errno));
					ret = -errno;
				}
			}
			close(fd);
		}
	}
	else
	{
		ret = -EINVAL;
	}

	return ret;
}

static int32_t upateBootControl(const char *misc, struct bootloader_control* bootctrl)
{
	int ret;
	DEBUG_UPDATE_PRINTF("In\n");
	
	if((misc != NULL)&&(bootctrl != NULL))
	{
		int32_t fd;

		bootctrl->crc32_le =  calc_crc32_le((const uint8_t *)bootctrl, offsetof(struct bootloader_control, crc32_le));
		fd = open(misc, O_RDWR);
		if(fd < 0)
		{
			ERROR_UPDATE_PRINTF("Can't open %s device : (%s)\n", misc, strerror(errno));
			ret = -errno;
		}
		else
		{
			if(lseek(fd, (off_t)bootLoaderControlOffset, SEEK_SET) != (off_t)bootLoaderControlOffset)
			{
				ERROR_UPDATE_PRINTF("Can't seek to offset %d : (%s)\n", (int32_t)bootLoaderControlOffset, strerror(errno));
				ret = -errno;
			}
			else
			{
				ssize_t nwrite;
				char * buf = (char *)bootctrl;
				size_t request_size = sizeof(struct bootloader_control);

				nwrite = write(fd, buf, request_size);
				if(nwrite == (ssize_t)request_size)
				{
					//printBootControlInfo(bootctrl);
					ret = 0;
					DEBUG_UPDATE_PRINTF("update boot control success.\n");
				}
				else
				{
					ERROR_UPDATE_PRINTF("Can't write boot ctrl message (%d) : (%s)\n", (int32_t)request_size, strerror(errno));
					ret = -errno;
				}
			}
			close(fd);
		}

	}
	else
	{
		ret = - EINVAL;
	}

	return ret;
}

int32_t bootControlInit(const char *misc, struct boot_control_module * bootctrl_module)
{
	int32_t ret = 0;

	DEBUG_UPDATE_PRINTF("In\n");

	if((misc != NULL)&&(bootctrl_module != NULL))
	{
		struct bootloader_control bootctrl;
		ret = loadBootControl(misc, &bootctrl);
		if(ret == 0)
		{
			uint32_t crc32;

			crc32 = calc_crc32_le((const uint8_t *)&bootctrl, offsetof(struct bootloader_control, crc32_le));

			if(bootctrl.crc32_le == crc32)
			{
				bootctrl_module->isInit = true;
				bootctrl_module->nb_slot = (int32_t)bootctrl.nb_slot;
				memset(bootctrl_module->misc, 0x00, PATH_MAX);
				memcpy(bootctrl_module->misc, misc, strnlen(misc, PATH_MAX));
				bootctrl_module->current_slot = (int32_t)slot_suffix_to_index(bootctrl.slot_suffix);
				DEBUG_UPDATE_PRINTF("Boot control init done. Slot(%d)\n",bootctrl_module->current_slot);
			}
			else
			{
				ERROR_UPDATE_PRINTF("CRC mismatch : bootctrl(0x%08x) vs calc(0x%08x)\n",bootctrl.crc32_le, crc32);
				ret = -1;
			}
		}
	}
	else
	{
		ret = -1;
	}

	return ret;
}


int32_t markBootSuccessful(struct boot_control_module * bootctrl_module)
{
	int32_t ret;

	DEBUG_UPDATE_PRINTF("In\n");

	if((bootctrl_module != NULL)&&(bootctrl_module->isInit == true))
	{
		struct bootloader_control bootctrl;
		ret = loadBootControl(bootctrl_module->misc, &bootctrl);
		if(ret == 0)
		{
			if((bootctrl.slot_info[bootctrl_module->current_slot].successful_boot != (uint8_t)1)||
				(bootctrl.slot_info[bootctrl_module->current_slot].tries_remaining != (uint8_t)1))
			{
				bootctrl.slot_info[bootctrl_module->current_slot].successful_boot = (uint8_t)1;
				bootctrl.slot_info[bootctrl_module->current_slot].tries_remaining = (uint8_t)1;

				DEBUG_UPDATE_PRINTF("Slot(%d) : Set sucessuful_boot(%d), trires_remaining(%d)\n",
					bootctrl_module->current_slot,
					bootctrl.slot_info[bootctrl_module->current_slot].successful_boot,
					bootctrl.slot_info[bootctrl_module->current_slot].tries_remaining);

				ret = upateBootControl(bootctrl_module->misc, &bootctrl);
			}
			if(ret ==0 )
			{
				DEBUG_UPDATE_PRINTF("markBootSuccessful is success\n");
			}
		}
	}
	else
	{
		ret = -1;
	}

	if(ret != 0)
	{
		ERROR_UPDATE_PRINTF("markBootSuccessful is fail\n");
	}

	return ret;
}

int32_t setActiveBootSlot(struct boot_control_module * bootctrl_module, int32_t slot)
{
	int32_t ret;

	DEBUG_UPDATE_PRINTF("In. slot(%d)\n", slot);

	if((bootctrl_module != NULL)&&(bootctrl_module->isInit == true))
	{
		struct bootloader_control bootctrl;
		ret = loadBootControl(bootctrl_module->misc, &bootctrl);
		if(ret == 0)
		{
			int32_t i;
			for(i=0; i< (int32_t)bootctrl.nb_slot; i++)
			{
				if((i!= slot)&&(bootctrl.slot_info[i].priority >= (uint8_t)ACTIVE_PRIORITY))
				{
					bootctrl.slot_info[i].priority = ACTIVE_PRIORITY-1;
				}
			}

			bootctrl.slot_info[slot].priority = (uint8_t)ACTIVE_PRIORITY;
			bootctrl.slot_info[slot].tries_remaining = (uint8_t)ACTIVE_TIRES;

			if((slot != bootctrl_module->current_slot)&&(slot < MAX_NUM_SLOTS))
			{
				bootctrl.slot_info[slot].verity_corrupted = 0;
			}

			ret = upateBootControl(bootctrl_module->misc, &bootctrl);
			if(ret ==0 )
			{
				DEBUG_UPDATE_PRINTF("setActiveBootSlot is success\n");
			}
		}
	}
	else
	{
		ret = -1;
	}

	if(ret != 0)
	{
		ERROR_UPDATE_PRINTF("setActiveBootSlot is fail\n");
	}

	return ret;
}

int32_t setSlotAsUnbootable(struct boot_control_module * bootctrl_module, int32_t slot)
{
	int32_t ret;

	DEBUG_UPDATE_PRINTF("In. slot(%d)\n", slot);

	if((bootctrl_module != NULL)&&(bootctrl_module->isInit == true))
	{
		struct bootloader_control bootctrl;
		ret = loadBootControl(bootctrl_module->misc, &bootctrl);
		if((ret == 0)&&(slot > -1)&&(slot < MAX_NUM_SLOTS))
		{
			bootctrl.slot_info[slot].successful_boot = 0;
			bootctrl.slot_info[slot].tries_remaining = 0;

			ret = upateBootControl(bootctrl_module->misc, &bootctrl);
			if(ret ==0 )
			{
				DEBUG_UPDATE_PRINTF("setSlotAsUnbootable is success\n");
			}
		}
	}
	else
	{
		ret = -1;
	}

	if(ret != 0)
	{
		ERROR_UPDATE_PRINTF("setSlotAsUnbootable is fail\n");
	}

	return ret;
}

int32_t isSlotBootable(struct boot_control_module * bootctrl_module, int32_t slot)
{
	int32_t ret;

	DEBUG_UPDATE_PRINTF("In. slot(%d)\n", slot);

	if((bootctrl_module != NULL)&&(bootctrl_module->isInit == true))
	{
		struct bootloader_control bootctrl;
		ret = loadBootControl(bootctrl_module->misc, &bootctrl);
		if(ret == 0)
		{
			ret = (int32_t)bootctrl.slot_info[slot].tries_remaining;
		}
	}
	else
	{
		ret = -1;
	}

	if(ret < 0)
	{
		ERROR_UPDATE_PRINTF("isSlotBootable is fail.\n");
	}

	return ret;
}

int32_t isSlotMarkedSuccessful(struct boot_control_module * bootctrl_module, int32_t slot)
{
	int32_t ret;

	DEBUG_UPDATE_PRINTF("In. slot(%d)\n", slot);

	if((bootctrl_module != NULL)&&(bootctrl_module->isInit == true))
	{
		struct bootloader_control bootctrl;
		ret = loadBootControl(bootctrl_module->misc, &bootctrl);
		if(ret == 0)
		{
			if((bootctrl.slot_info[slot].successful_boot == (uint8_t)1)&&(bootctrl.slot_info[slot].tries_remaining == (uint8_t)1))
			{
				ret = 1;
			}
			else
			{
				ret = 0;
			}
		}
	}
	else
	{
		ret = -1;
	}

	if(ret < 0)
	{
		ERROR_UPDATE_PRINTF("isSlotBootable is fail.\n");
	}

	return ret;
}

static void printBootControlInfo(struct bootloader_control *bootctrl)
{
	if(bootctrl != NULL)
	{
		int32_t i;
		DEBUG_UPDATE_PRINTF("slot_suffix : %s\n", bootctrl->slot_suffix);
		DEBUG_UPDATE_PRINTF("magic : 0x%08x\n", bootctrl->magic);
		DEBUG_UPDATE_PRINTF("version : %d\n", bootctrl->version);
		DEBUG_UPDATE_PRINTF("nb_slot : %d\n", bootctrl->nb_slot);
		DEBUG_UPDATE_PRINTF("recovery_tries_remaining : %d\n", bootctrl->recovery_tries_remaining);
		for(i=0; i< (int32_t)bootctrl->nb_slot; i++ )
		{
			DEBUG_UPDATE_PRINTF("slot[%s] : priority(%d)\n",slotSuffixes[i], bootctrl->slot_info[i].priority);
			DEBUG_UPDATE_PRINTF("slot[%s] : tries_remaining(%d)\n",slotSuffixes[i], bootctrl->slot_info[i].tries_remaining);
			DEBUG_UPDATE_PRINTF("slot[%s] : successful_boot(%d)\n",slotSuffixes[i], bootctrl->slot_info[i].successful_boot);
			DEBUG_UPDATE_PRINTF("slot[%s] : verity_corrupted(%d)\n",slotSuffixes[i], bootctrl->slot_info[i].verity_corrupted);
		}
		DEBUG_UPDATE_PRINTF("crc32_le ; 0x%08x\n",bootctrl->crc32_le);
	}
}

