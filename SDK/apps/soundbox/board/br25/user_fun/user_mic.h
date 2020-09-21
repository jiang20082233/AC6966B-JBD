#ifndef _USER_MIC_H_
#define _USER_MIC_H_
#include "user_dev_check.h"

extern USER_DEV_CHECK user_mic_check;
bool user_get_mic_status(void);
u8 user_mic_check_en(u8 cmd);
void user_mic_check_init(void);
#endif