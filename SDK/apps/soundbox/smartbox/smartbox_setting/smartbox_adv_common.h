#ifndef __SMARTBOX_SETTING_COMMON_H__
#define __SMARTBOX_SETTING_COMMON_H__

#include "typedef.h"
#include "event.h"

struct t_s_info {
    u8 edr_name[32];
    u8 key_setting[12];
    u8 led_status[14];
    u8 mic_mode;
    u8 work_mode;
};

extern struct t_s_info _s_info;

#endif
