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


#ifndef __TCC_SNOR_UPDATER_DEV_H__
#define __TCC_SNOR_UPDATER_DEV_H__

#include <asm/ioctl.h>

#define TCC_UPDATE_MAGIC 'I'

#define IOCTL_UPDATE_START		_IO(TCC_UPDATE_MAGIC ,1)
#define IOCTL_UPDATE_DONE		_IO(TCC_UPDATE_MAGIC ,2)
#define IOCTL_FW_UPDATE			_IO(TCC_UPDATE_MAGIC ,3)

typedef struct _tcc_snor_update_param
{
	unsigned int start_address;
	unsigned int partition_size;
 	unsigned char *image;
	unsigned int image_size;
}__attribute__((packed))tcc_snor_update_param;

#endif /* __TCC_SNOR_UPDATER_DEV_H__ */
