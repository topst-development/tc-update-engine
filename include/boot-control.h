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

#ifndef BOOT_CONTROL_H
#define BOOT_CONTROL_H

struct boot_control_module
{
	int32_t isInit;
	char misc[PATH_MAX];
	int32_t current_slot;
	int32_t update_slot;
	int32_t nb_slot;
};

uint32_t slot_suffix_to_index(const char* suffix);
const char *slot_index_to_suffix(uint32_t index);
int32_t bootControlInit(const char *misc, struct boot_control_module * bootctrl_module);
int32_t markBootSuccessful(struct boot_control_module * bootctrl_module);
int32_t setActiveBootSlot(struct boot_control_module * bootctrl_module, int32_t slot);
int32_t setSlotAsUnbootable(struct boot_control_module * bootctrl_module, int32_t slot);
int32_t isSlotBootable(struct boot_control_module * bootctrl_module, int32_t slot);
int32_t isSlotMarkedSuccessful(struct boot_control_module * bootctrl_module, int32_t slot);


#endif
