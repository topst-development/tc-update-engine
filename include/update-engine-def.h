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


#ifndef UPDATE_ENGINE_DEF_H
#define UPDATE_ENGINE_DEF_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef UPDATE_TCC803X
#define MAIN_BOOTLOADER_IMAGE	"bootloader.rom"
#define MAIN_KERNEL_IMAGE		"main_boot.img"
#define MAIN_ROOTFS_IMAGE		"main_rootfs.ext4"
#define MATN_DTB_IMAGE			"main.dtb"

#define SUB_KERNEL_IMAGE		"sub_boot.bin"
#define SUB_ROOTFS_IMAGE		"sub_rootfs.cpio"
#define SUB_DTB_IMAGE			"sub.dtb"
#else
#define MAIN_BOOTLOADER_IMAGE	"ca72_bl3.rom"
#define MAIN_KERNEL_IMAGE		"main_boot.img"
#define MAIN_ROOTFS_IMAGE		"main_rootfs.ext4"	
#define MATN_DTB_IMAGE			"main.dtb"

#define	SUB_BOOTLOADER_IMAGE	"ca53_bl3.rom"
#define SUB_KERNEL_IMAGE		"sub_boot.img"
#define SUB_ROOTFS_IMAGE		"sub_rootfs.ext4"
#define SUB_DTB_IMAGE			"sub.dtb"
#endif

#define SPLASH_IMAGE			"subcore_splash"

#define SNOR_IMAGE				"snor.rom"

/* Do not moidfy this. */
enum updateCoreID
{
	MAIN_CORE = 0, 
	SUB_CORE,
	MICOM_CORE,
	MAX_CORE
};


#ifdef __cplusplus
}
#endif

#endif
