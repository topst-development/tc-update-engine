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
#include <string.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <linux/limits.h>
#include <tcc_snor_updater_dev.h>
#include "update-log.h"
#include "snor-update.h"
#include "update-engine-def.h"

#define SNOR_DEVICE_NAME  "/dev/tcc_snor_updater"

#define SNOR_ROM_INFO_OFFSET	0x00000000
#define SNOR_ROM_ID             0x524F4E53 // 'SNOR'
#define SNOR_SECTION_MAX_COUNT  30

enum SNOR_SECTION_ID {
    SNOR_SFMC_INIT_HEADER_ID = 0,
    SNOR_MASTER_CERTI_ID,
    SNOR_HSM_BINARY_ID,
    SNOR_BL1_BINARY_ID,
    SNOR_MICOM_SUB_BINARY_ID,
    SNOR_UPDATE_FLAG_ID,
    SNOR_MICOM_BINARY_ID,
    SNOR_SECTION_ID_END = 29,
};

typedef struct snor_section_info {
    uint32_t offset;
    uint32_t section_size;
    uint32_t data_size;
    uint32_t reserved;
} snor_section_info_t;

typedef struct snor_rom_info {
    snor_section_info_t section_info[SNOR_SECTION_MAX_COUNT];
    uint32_t        reserved[4];
    uint32_t        debug_enable;
    uint32_t        debug_port;
    uint32_t        rom_id; //= SNOR_ROM_ID;
    uint32_t        crc;
} snor_rom_info_t; /* total 512byte */

static int32_t tcc_read_file(FILE *fp, void *buf, long offset, size_t size);
static int32_t tcc_get_snor_rom_info(FILE *fp, snor_rom_info_t *snor_info);
static int32_t writeSnorImage(snor_rom_info_t snor_info, int32_t dev_fd, FILE *rom_fd, uint32_t index);

static int32_t tcc_read_file(FILE *fp, void *buf, long offset, size_t size)
{
	int32_t ret;
	size_t r_size;

	ret = fseek(fp, offset, SEEK_SET);
	if(ret != 0) {
		ERROR_UPDATE_PRINTF("failed to seek file, err %d\n", ret);
	}
	else
	{
		r_size = fread(buf, 1, size, fp);
		if(r_size != size) {
			ERROR_UPDATE_PRINTF("failed to read file, size %zu\n", r_size);
			ret = -1;
		}
	}

	return ret;
}

static int32_t tcc_get_snor_rom_info(FILE *fp, snor_rom_info_t *snor_info)
{
	int32_t ret = -1;

	ret = tcc_read_file(fp, snor_info, SNOR_ROM_INFO_OFFSET, sizeof(snor_rom_info_t));
	if(ret == 0)
	{
		if(snor_info->rom_id != (uint32_t)SNOR_ROM_ID)
		{
			ERROR_UPDATE_PRINTF("snor id mismatch. read id(0x%08x), snor id(0x%08x)\n", snor_info->rom_id,SNOR_ROM_ID);
			ret = -1;
		}
	}

	return ret;
}

static int32_t writeSnorImage(snor_rom_info_t snor_info, int32_t dev_fd, FILE *rom_fd, uint32_t index)
{
	tcc_snor_update_param	fwInfo;
	int32_t ret = -1;
	unsigned char *buffer;
	
	DEBUG_UPDATE_PRINTF("Snor image extraction. index:%d\n", index);

	if((snor_info.section_info[index].section_size != (uint32_t)0)&&
		(snor_info.section_info[index].data_size != (uint32_t)0))
	{
		buffer = malloc(snor_info.section_info[index].section_size);
		if(buffer != NULL)
		{
			memset(buffer, 0xFF, snor_info.section_info[index].section_size);
			ret = tcc_read_file(rom_fd, buffer, (long)snor_info.section_info[index].offset, (size_t)snor_info.section_info[index].data_size);		
			//image_crc = snorCalcCrc8(buffer, snor_info.section_info[index].data_size);

			/* Result Output */
			DEBUG_UPDATE_PRINTF("snor image partition offset=0x%X\n", snor_info.section_info[index].offset);
			DEBUG_UPDATE_PRINTF("snor image partition size=0x%X\n", snor_info.section_info[index].section_size);
			DEBUG_UPDATE_PRINTF("snor image size=0x%X(%d bytes)\n", snor_info.section_info[index].data_size, snor_info.section_info[index].data_size);
			//print_hex_dump(buffer, 16*16);

			fwInfo.start_address = snor_info.section_info[index].offset;
			fwInfo.partition_size = snor_info.section_info[index].section_size;
			fwInfo.image = buffer;
			fwInfo.image_size = snor_info.section_info[index].data_size;
			
			DEBUG_UPDATE_PRINTF("buffer : %p\n", buffer);
			sleep(1);
			ret = ioctl(dev_fd, (int32_t)IOCTL_FW_UPDATE, &fwInfo);
			DEBUG_UPDATE_PRINTF("fw update : (%d)", ret);

			free(buffer);			
		}
		else
		{
			ERROR_UPDATE_PRINTF("Allocate memory fail.\n");
		}	
	}
	else
	{
		ERROR_UPDATE_PRINTF("Image size error. section_size(%d), data_size(%d)\n",snor_info.section_info[index].section_size,snor_info.section_info[index].data_size );

	}

	return ret;

}

int32_t updateSnorImage(const char * sourcePath)
{
	int32_t ret = -1;
	int32_t dev_fd = -1;
	FILE *src_fd = NULL;
	char image_path[PATH_MAX];

	if(sourcePath != NULL)
	{
		DEBUG_UPDATE_PRINTF("In. source PATH : %s\n", sourcePath);

		/* 1.open snor image */
		if((strnlen(sourcePath,PATH_MAX)+strnlen(SNOR_IMAGE,PATH_MAX)) <= (size_t)PATH_MAX)
		{
			memset(image_path, 0x00, PATH_MAX);
			sprintf(image_path, "%s/%s", (const char *)sourcePath, (const char *)SNOR_IMAGE);
			DEBUG_UPDATE_PRINTF("Open snor image : %s\n", image_path);
			src_fd = fopen(image_path, "rb");
			if(src_fd == NULL)
			{
				ERROR_UPDATE_PRINTF("Can't open %s\n", image_path);
			}
			else
			{
				ret = 0;
			}
		}
		else
		{
			ERROR_UPDATE_PRINTF("PATH_MAX is %d.But current path is too long (%d)", 
				PATH_MAX, (int32_t)(strnlen(sourcePath,PATH_MAX) + strnlen(SNOR_IMAGE,PATH_MAX)));
		}

		/* 2.open update driver & update snor image*/
		if(ret == 0)
		{
			dev_fd = open(SNOR_DEVICE_NAME, O_RDWR);
			if(dev_fd > -1)
			{
				snor_rom_info_t snor_info;

				ret = ioctl(dev_fd, IOCTL_UPDATE_START, NULL);
				DEBUG_UPDATE_PRINTF("snor update start : (%d)\n", ret);

				if(ret ==0)
				{
					DEBUG_UPDATE_PRINTF("Get SNOR Rom file infomation\n");
					ret = tcc_get_snor_rom_info(src_fd, &snor_info);
				}
				
				if(ret ==0)
				{
					DEBUG_UPDATE_PRINTF("Update micom binrary start\n");
					ret = writeSnorImage(snor_info, dev_fd, src_fd, (uint32_t)SNOR_MICOM_BINARY_ID);
					DEBUG_UPDATE_PRINTF("Update micom binrary end (%d)\n", ret);
					ret = ioctl(dev_fd, IOCTL_UPDATE_DONE, NULL);
					DEBUG_UPDATE_PRINTF("Micom update done (%d)\n", ret);
				}
				close(dev_fd);				
			}
			else
			{
				ERROR_UPDATE_PRINTF("Can't open snor update driver(%s)\n", SNOR_DEVICE_NAME);
				ret = -1;
			}

		}

		if(src_fd != NULL)
		{
			fclose(src_fd);
		}
	}
	else
	{
		ERROR_UPDATE_PRINTF("image source dir path is NULL\n");
	}
	
	return ret;
}



