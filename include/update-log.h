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

#ifndef UPDATE_LOG_H
#define UPDATE_LOG_H

extern int32_t g_update_debug;

#define UPDATE_ERROR_LEVEL 	(0U)
#define UPDATE_WARN_LEVEL 		(1U)
#define UPDATE_INFO_LEVEL 		(2U)
#define UPDATE_DEBUG_LEVEL 	(3U)

#define ERROR_UPDATE_PRINTF(format, arg...) \
		fprintf(stderr, "[ERROR][UpdateEn][%s:%d] : "format"", __FUNCTION__, __LINE__, ##arg); \

#define WARN_UPDATE_PRINTF(format, arg...) \
	if (g_update_debug > (int32_t)UPDATE_ERROR_LEVEL) \
	{ \
		fprintf(stderr, "[WARN][UpdateEn][%s:%d] : "format"", __FUNCTION__, __LINE__, ##arg); \
	}

#define INFO_UPDATE_PRINTF(format, arg...) \
	if (g_update_debug > (int32_t)UPDATE_WARN_LEVEL) \
	{ \
		fprintf(stderr, "[INFO][UpdateEn][%s:%d] : "format"", __FUNCTION__, __LINE__, ##arg); \
	}

#define DEBUG_UPDATE_PRINTF(format, arg...) \
	if (g_update_debug > (int32_t)UPDATE_INFO_LEVEL) \
	{ \
		fprintf(stderr, "[DEBUG][UpdateEn][%s:%d] : "format"", __FUNCTION__, __LINE__, ##arg); \
	}


#endif
