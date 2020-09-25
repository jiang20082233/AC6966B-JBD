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
// #include "audio_reverb.h"
#include "ui/style_led7.h"
#include "ui/ui_api.h"
#include "mic_effect.h"
#include "application/audio_eq.h"
#include "audio_config.h"
#include "asm/spi.h"

#include "log.h"
#include "app_task.h"
#include "classic/tws_api.h"


#include "user_pa.h"
#include "user_ad.h"
#include "user_led.h"
#include "user_rgb_dev.h"
#include "user_rgb.h"
#include "user_mic.h"
#include "user_dev_check.h"

// #include "asm/adc_api.h"
// #include "system/includes.h"
// #include "user_dev_check.h"
// #include "audio_config.h"

// #include "board_ac6966b/user_ac6966b_cfg.h"
// #include "board_ac6965e/user_ac6965e_cfg.h"

#define user_print printf


#define USER_EQ_TERBLE_BIT  0
#define USER_EQ_BASS_BIT    1
#define USER_MIC_VOL_BIT    2
#define USER_REVER_BOL_BIT  3

#define USER_EQ_BASS_AD_MAX    (420)//430
#define USER_EQ_BASS_AD_MIN    (50)//62
#define USER_EQ_BASS_GAIN_MIN  (-6)//最小增益
#define USER_EQ_BASS_GAIN_MAX  (6)//最大增益

#define USER_EQ_TERBLE_AD_MAX    (950)//962
#define USER_EQ_TERBLE_AD_MIN    (590)//599
#define USER_EQ_TERBLE_GAIN_MIN  (-6)//最小增益
#define USER_EQ_TERBLE_GAIN_MAX  (6)//最大增益

#define USER_EQ_MIC_AD_MAX    (950)//962
#define USER_EQ_MIC_AD_MIN    (590)//599
#define USER_EQ_MIC_GAIN_MIN  (0)//最小增益
#define USER_EQ_MIC_GAIN_MAX  (10)//最大增益

#define USER_EQ_REV_AD_MAX    (420)//430
#define USER_EQ_REV_AD_MIN    (50)//62
#define USER_EQ_REV_GAIN_MIN  (100)//最小增益
#define USER_EQ_REV_GAIN_MAX  (200)//最大增益

typedef struct user_power_io_info {
    u8 pro;
    u8  ch;
    u32 vol;
}USER_POWER_INFO;

extern void mic_effect_set_echo_delay(u32 delay);
extern u32 mic_effect_get_echo_delay(void);
extern void audio_mic_set_gain(u8 gain);
extern const EQ_CFG_SEG *eq_type_tab[EQ_MODE_MAX];
extern struct eq_seg_info eq_tab_custom[];
extern int audio_dac_energy_get(void);

#if USER_EQ_FILE_ADD_EQ_TABLE
extern struct eq_seg_info user_eq_tab_custom[];
#endif

void user_fun_io_init(void);
void user_fun_init(void);
void user_power_off(void);

void user_fm_vol_set(bool cmd);
u16 user_fun_get_vbat(void);
void user_music_set_file_number(int number);
void user_eq_mode_sw(void);
u8 user_record_status(u8 cmd);
void user_vbat_check_init(void);
bool user_adkey_mult_irkey(u8 key_type);
void user_power_low_dow_sys_vol(void);
#endif