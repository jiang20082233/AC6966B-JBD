#ifndef _USER_FUN_CONFIG_H_
#define _USER_FUN_CONFIG_H_

#include "typedef.h"
#include "timer.h"
#include "string.h"
#include "gpio.h"
#include "cpu.h"

#include "app_task.h"

#include "board_config.h"
#include "app_config.h"
#include "app_action.h"
#include "audio_dec.h"
#include "btstack/avctp_user.h"
#include "app_main.h"

#include "media/file_decoder.h"
#include "music/music_player.h"

#include "key_event_deal.h"
#include "audio_reverb.h"


#include "log.h"


#include "user_pa.h"
#include "user_led.h"
#include "user_mic.h"
#include "user_dev_check.h"

// #include "asm/adc_api.h"
// #include "system/includes.h"
// #include "user_dev_check.h"
// #include "audio_config.h"

// #include "board_ac6966b/user_ac6966b_cfg.h"
// #include "board_ac6965e/user_ac6965e_cfg.h"

#define user_print printf

void user_fm_vol_set(bool cmd);
void user_fun_io_init(void);
void user_fun_init(void);

#endif