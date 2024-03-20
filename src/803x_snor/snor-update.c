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
#include <errno.h>
#include <tcc_snor_updater_dev.h>
#include "update-log.h"
#include "snor-update.h"
#include "update-engine-def.h"
	
#define SNOR_DEVICE_NAME  "/dev/tcc_snor_updater"

#define SNOR_BL1_ID  2
#define SNOR_MICOM_HEADER_ID  8
#define SNOR_MICOM_BINARY_ID  10

#define SNOR_ROM_INFO_OFFSET	0x100
#define SNOR_SIGNATURE          0x524F4E53 // 'SNOR'
#define SNOR_SECTION_MAX_COUNT  31

enum SNOR_SECTION_ID {
    SNOR_BOOT_HEADER_ID = 1,
    SNOR_BL1_BINARY_0_ID,
    SNOR_BL1_BINARY_1_ID,
    SNOR_CM4_BINARY_0_ID,
    SNOR_CM4_BINARY_1_ID,   /* 5 */
    SNOR_UPDATE_BINARY_0_ID,
    SNOR_UPDATE_BINARY_1_ID,
    SNOR_MICOM_HEADER_0_ID,
    SNOR_MICOM_HEADER_1_ID,
    SNOR_MICOM_BINARY_0_ID, /* 10 */
    SNOR_MICOM_BINARY_1_ID,
};

typedef struct snor_section_info {
    uint32_t section_id;
    uint32_t offset;
    uint32_t section_size;
    uint32_t image_size;
} snor_section_info_t;

typedef struct snor_rom_info {
    uint32_t        signature; //= SNOR_SIGNATURE;
    uint32_t        rom_size;
    uint32_t        rom_crc;
    uint32_t        section_info_cnt;
    snor_section_info_t section_info[31];
} snor_rom_info_t; /* total 512byte */


static int32_t update_snor_section(snor_rom_info_t snor_info, int32_t dev_fd, FILE *rom_fd, uint32_t index);

static int32_t tcc_read_file(FILE *fp, void *buf, long offset, size_t size)
{
	int ret;
	size_t r_size;

	DEBUG_UPDATE_PRINTF("offset(%ld), size(%ld)\n",offset, size);
	ret = fseek(fp, offset, SEEK_SET);
	if(ret) {
		ERROR_UPDATE_PRINTF("failed to seek file, err %d\n", ret);
		return ret;
	}

	r_size = fread(buf, 1, size, fp);
	if(r_size != size) {
		ERROR_UPDATE_PRINTF("failed to read file, size %zu, error(%d)\n", r_size, errno);
		return -1;
	}

	return 0;	
}

int32_t tcc_get_snor_section_image(FILE *fp, uint32_t id, void *buffer)
{
	int32_t i, ret = -1;
	snor_rom_info_t snor_info;

	ret = tcc_read_file(fp, &snor_info, SNOR_ROM_INFO_OFFSET, sizeof(snor_rom_info_t));
	if (ret)
		return ret;

	if (snor_info.signature != SNOR_SIGNATURE)
	{
		ERROR_UPDATE_PRINTF("signature mismatch : 0x%08x<->0x%08x\n",snor_info.signature ,SNOR_SIGNATURE);
		return -1;
	}

	for (i = 0; i < snor_info.section_info_cnt; i++)
	{
		if (snor_info.section_info[i].section_id == id)
			break;
	}

	ret = tcc_read_file(fp, buffer, snor_info.section_info[i].offset, snor_info.section_info[i].image_size);

	return ret;
}

int32_t tcc_get_snor_rom_info(FILE *fp, snor_rom_info_t *snor_info)
{
	int32_t ret = -1;

	ret = tcc_read_file(fp, snor_info, SNOR_ROM_INFO_OFFSET, sizeof(snor_rom_info_t));
	if (ret)
		return ret;

	if (snor_info->signature != SNOR_SIGNATURE)
	{
		ERROR_UPDATE_PRINTF("signature mismatch : 0x%08x<->0x%08x\n",
			snor_info->signature ,SNOR_SIGNATURE);
		return -1;
	}

	return ret;
}

static int32_t update_snor_section(snor_rom_info_t snor_info, int32_t dev_fd, FILE *rom_fd, uint32_t index)
{
	tcc_snor_update_param	fwInfo;
	int32_t i;
	int32_t ret;
	unsigned char *buffer;

	for (i = 0; i < snor_info.section_info_cnt; i++)
	{
		if (snor_info.section_info[i].section_id == index)
			break;
	}
	
	buffer = malloc(snor_info.section_info[i].section_size);
	memset(buffer, 0xFF, snor_info.section_info[i].section_size);
	
	ret = tcc_get_snor_section_image(rom_fd, snor_info.section_info[i].section_id, buffer);
	/* Result Output */
	DEBUG_UPDATE_PRINTF("Image partition offset=0x%X\n", snor_info.section_info[i].offset);
	DEBUG_UPDATE_PRINTF("Image partition size=0x%X\n", snor_info.section_info[i].section_size);
	DEBUG_UPDATE_PRINTF("Stored image size=0x%X(%d bytes)\n", snor_info.section_info[i].image_size, snor_info.section_info[i].image_size);
	
	fwInfo.start_address = snor_info.section_info[i].offset;
	fwInfo.partition_size = snor_info.section_info[i].section_size;
	fwInfo.image = buffer;
	fwInfo.image_size = snor_info.section_info[i].image_size;
	
	sleep(1);
	ret = ioctl(dev_fd, IOCTL_FW_UPDATE, &fwInfo);
	free(buffer);
	
	return ret;

}

int32_t updateSnorImage(const char * sourcePath)
{
	int32_t ret = -1;
	int32_t dev_fd;
	FILE *rom_fd = NULL;
	char image_path[PATH_MAX];

	INFO_UPDATE_PRINTF("snor_rom_name %s\n", sourcePath);

	DEBUG_UPDATE_PRINTF("In. source PATH : %s\n", sourcePath);
	
	/* 1.open snor image */
	if((strnlen(sourcePath,PATH_MAX)+strnlen(SNOR_IMAGE,PATH_MAX)) <= (size_t)PATH_MAX)
	{
		memset(image_path, 0x00, PATH_MAX);
		sprintf(image_path, "%s/%s", (const char *)sourcePath, (const char *)SNOR_IMAGE);
		DEBUG_UPDATE_PRINTF("Open snor image : %s\n", image_path);
		rom_fd = fopen(image_path, "rb");
		if(rom_fd == NULL)
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

	if(rom_fd != NULL)
	{
		dev_fd = open(SNOR_DEVICE_NAME, O_RDWR);
		if(dev_fd != -1)
		{
			snor_rom_info_t snor_info;

			ret = ioctl(dev_fd, IOCTL_UPDATE_START, NULL);
			INFO_UPDATE_PRINTF("update start : (%d)\n", ret);

			if(ret ==0)
			{
				INFO_UPDATE_PRINTF("Get SNOR Rom file infomation\n");
				ret = tcc_get_snor_rom_info(rom_fd, &snor_info);
			}

			if(ret == 0)
			{
				INFO_UPDATE_PRINTF("Update micom binrary start\n");
				ret = update_snor_section(snor_info, dev_fd, rom_fd, SNOR_MICOM_BINARY_ID);
				INFO_UPDATE_PRINTF("Update micom binrary end (%d)\n", ret);
			}

			if(ret == 0)
			{
				INFO_UPDATE_PRINTF("Update micom header start\n");
				ret = update_snor_section(snor_info, dev_fd, rom_fd, SNOR_MICOM_HEADER_ID);
				INFO_UPDATE_PRINTF("Update micom header end (%d)\n", ret);
			}

			if(ret == 0)
			{
				INFO_UPDATE_PRINTF("Update BL1 binrary start\n");
				ret = update_snor_section(snor_info, dev_fd, rom_fd, SNOR_BL1_ID);
				INFO_UPDATE_PRINTF("Update BL1 binrary end (%d)\n", ret);
			}

			if(ret == 0)
			{
				ret = ioctl(dev_fd, IOCTL_UPDATE_DONE, NULL);
				INFO_UPDATE_PRINTF("Snor update success");
			}
			else
			{
				ERROR_UPDATE_PRINTF("Snor upate fail : (%d)\n", ret);
			}
			close(dev_fd);
		}
		else
		{
			ERROR_UPDATE_PRINTF("open error : (%s)", SNOR_DEVICE_NAME);
		}
		fclose(rom_fd);
	}

	return ret;
}


