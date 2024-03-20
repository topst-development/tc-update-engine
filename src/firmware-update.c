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
#include <stdint.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "update-log.h"
#include "firmware-update.h"

#define SPLASH_TAG				"splash"
#define SUBCORE_SPLASH_TAG		"subcore_splash"

#define CERT_MAGIC				"CERT"
#define BOOT_MAGIC 				"ANDROID!"


#define MAX_FW_BUF_SIZE		1024*1024*10	//10MByte

int32_t update_firmware(int32_t imageType, const char *srcImage, const char *targetDev)
{
	int32_t ret = 0;
	DEBUG_UPDATE_PRINTF("In\n");

	if((srcImage != NULL)&&(targetDev != NULL))
	{
		
		int rom_fd = -1;
		int dev_fd = -1;
		unsigned char *rom_buff = NULL;
		uint32_t image_len=0;
		uint32_t malloc_size = 0, remain_size = 0;
		struct stat st;
	
		DEBUG_UPDATE_PRINTF("Update Firmware : imageType(%d), srcImage(%s), targetDev(%s)\n", imageType, srcImage, targetDev);

		/* 1. Open&Read source F/W image */
		rom_fd = open((const char *)srcImage, O_RDONLY|O_NDELAY);
		if(rom_fd < 0)
		{
			ERROR_UPDATE_PRINTF("Can't open file(%s): error(%s)\n",srcImage, strerror(errno));
			ret = -1;
		}
		else
		{
			ret = fstat(rom_fd, &st);
			if(ret ==0)
			{
				image_len = (uint32_t)st.st_size;
				
				if(image_len > MAX_FW_BUF_SIZE)
				{
					malloc_size = MAX_FW_BUF_SIZE;
				}
				else
				{
					malloc_size = image_len;
				}
				DEBUG_UPDATE_PRINTF("fw image size(%d), malloc size(%d)\n", image_len, malloc_size);
				
				rom_buff = (unsigned char *)malloc(malloc_size);
				if(rom_buff == NULL)
				{
					ERROR_UPDATE_PRINTF("ERROR: Not enough memory\n");
					ret = -1;
				}
				else
				{
					memset(rom_buff, 0x0, malloc_size);
					ret = read(rom_fd, (void *)rom_buff, (size_t)malloc_size);
					if(ret != malloc_size)
					{
						ERROR_UPDATE_PRINTF("ERROR: f/w image read fail\n");
						ret = -1;
					}
					else
					{
						ret = 0;
					}
				}
			}
			else
			{
				ERROR_UPDATE_PRINTF("Can't fstat(%d):(%s)\n", errno, strerror(errno));
			}
		}

		/* 2. check F/W image header */
#if 0
		if(ret ==0 )
		{
			switch(imageType)
			{
				case UPDATE_BOOTLOADER:
				case UPDATE_KERNEL:
				case UPDATE_SPLASH:
				case UPDATE_DTB:
				case UPDATE_ROOT_FS:
				case UPDATE_HOME:
				case UPDATE_USER:
				default:
					break;
			}
		}
#endif
		/* 3. Open target partition */
		if(ret ==0)
		{
			dev_fd = open((const char *)targetDev, O_RDWR|O_NDELAY);
			if (dev_fd < 0) {
				ERROR_UPDATE_PRINTF("Cannot open %s\n", targetDev);
				ret  = -1;
			}
		}

		/* 4. Write F/W image */
		if(ret == 0)
		{
			int32_t writed_size;
			DEBUG_UPDATE_PRINTF("fw image write start\n");
			writed_size = write(dev_fd, (void *)rom_buff, (size_t)malloc_size);
			if( writed_size == malloc_size)
			{
				DEBUG_UPDATE_PRINTF("Write %s from %s (%d bytes),remain(%d)\n", srcImage, targetDev, malloc_size, remain_size);
				ret = 0;
				uint32_t read_len = 0;
			
				remain_size = image_len - malloc_size;
			
				while(remain_size != (uint32_t)0)
				{
					if(remain_size >= malloc_size)
					{
						read_len = malloc_size;
					}
					else
					{
						read_len = remain_size;
					}
					memset(rom_buff, 0x0, malloc_size);
					ret = read(rom_fd, rom_buff, read_len);
					if(ret == (int32_t)read_len)
					{
						if(write(dev_fd, (void*)rom_buff, (size_t)read_len) == read_len)
						{
							DEBUG_UPDATE_PRINTF("Write %s from %s (%d bytes),remain(%d)\n", srcImage, targetDev, read_len, (remain_size-read_len));
							remain_size -= read_len;
							ret = 0;
						}
						else
						{
							ERROR_UPDATE_PRINTF("ERROR: F/W Write error : offset(%d)\n", (image_len - remain_size));
							ret = -1;
							break;
						}
					}
					else
					{
						ret = -1;
						ERROR_UPDATE_PRINTF("ERROR: F/W read error : offset(%d), read size(%d)\n", (image_len - remain_size),read_len);
						break;
					}
				}
			}
			else
			{
				ERROR_UPDATE_PRINTF("Write error. request size(%d) -> writed size(%d)\n", malloc_size, writed_size);
			}
		}

		/* 5. update done */
		sync();
		if(rom_buff != NULL)
		{
			free(rom_buff);
		}
		if(rom_fd > -1)
		{
			close(rom_fd);
		}
		if(dev_fd > -1)
		{
			close(dev_fd);
		}		
	}
	else
	{
		ret = -1;
	}
	return ret;
}

