#ifndef __SMARTBOX_EQ_SETTING_H__
#define __SMARTBOX_EQ_SETTING_H__

#include "le_rcsp_adv_module.h"

#if RCSP_ADV_EQ_SET_ENABLE

u8 app_get_eq_info(u8 *get_eq_info);
u8 app_get_eq_all_info(u8 *get_eq_info);

#endif
#endif
