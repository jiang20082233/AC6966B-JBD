#ifndef __RCSP_ADV_TWS_SYNC_H__
#define __RCSP_ADV_TWS_SYNC_H__

#include "classic/tws_api.h"

#define TWS_FUNC_ID_ADV_SETTING_SYNC \
	TWS_FUNC_ID('R', 'C', 'S', 'P')
// 属于弹窗
#define TWS_FUNC_ID_TIME_STAMP_SYNC \
	TWS_FUNC_ID('R' + 'C' + 'S' + 'P', \
			    'A' + 'D' + 'V', \
				'T' + 'I' + 'M' + 'E', \
				'S' + 'T' + 'A' + 'M' + 'P')
#define TWS_FUNC_ID_ADV_RESET_SYNC \
	(((u8)('R' + 'C' + 'S' + 'P') << (3 * 8)) | \
	 ((u8)('A' + 'D' + 'V') << (2 * 8)) | \
	 ((u8)('R' + 'E' + 'S' + 'E' + 'T') << (1 * 8)) | \
	 ((u8)('S' + 'Y' + 'N' + 'C') << (0 * 8)))
#endif
