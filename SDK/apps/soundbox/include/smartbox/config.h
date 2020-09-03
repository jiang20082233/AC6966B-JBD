#ifndef __SMARTBOX_CFG_H__
#define __SMARTBOX_CFG_H__

#include "typedef.h"
#include "app_config.h"
#include "le_common.h"
#include "spp_user.h"
#include "ble_user.h"
#include "app_task.h"
#include "btstack_3th_protocol_user.h"

#define TARGET_FEATURE_RESP_BUF_SIZE			(256)

#define DEV_ICON_ALL_DISPLAY	0

enum func_type {
    BT_FUNCTION      = 0,
    MUSIC_FUNCTION   = 1,
    RTC_FUNCTION     = 2,
    LINEIN_FUNCTION  = 3,
    FM_FUNCTION      = 4,
    LIGHT_FUNCTION   = 5,
    FMTX_FUNCTION    = 6,
};

enum mask_func {
    BT_FUNCTION_MASK      =  0,
    MUSIC_FUNCTION_MASK   =  1,
    RTC_FUNCTION_MASK     =  2,
    LINEIN_FUNCTION_MASK  =  3,
    FM_FUNCTION_MASK      =  4,
    FMTX_FUNCTION_MASK    =  6,
    FUNCTION_MASK_MAX,

    COMMON_FUNCTION    	  =  0xFF,
};

enum music_icon_display_mask {
    USB_ICON_DISPLAY = 1,
    SD0_ICON_DISPLAY = 2,
    SD1_ICON_DISPLAY = 3,
};


struct smartbox {
    u8 F_platform;  //deepbrain/turing
    u8 A_platform;	//0:Andriod  1:Ios
    u8 sdk_type;	//0:support AI 1:BT MATE
    u8 ota_type;	//0:单备份 1:双备份
    u8 auth_check;	//0:不需要握手 1:需要握手
    u8 trans_chl;   //0:ble, 1:spp
    u8 emitter_en: 4;   //0x1：使能发射模式, 0x0:不使能发射模式
    u8 emitter_sw: 4;   //0x0：普通接收模式, 0x1:发射模式
    u32 function_mask;

    u8 music_icon_mask;	// BIT0:显示方式  BIT(1):显示USB  BIT(2):显示SD0  BIT(3):显示SD1
    u8 dev_vol_sync; // 是否支持音量同步
    u8 find_dev_en;
    u8 game_mode_en;
    u8 err_code;

    u8 *smartbox_buf;

    u8 emitter_bt_state;
    u8 emitter_con_addr[6];

    u8 cur_app_mode;
    u8 cur_music_dev;

    OS_SEM		sem;
};

struct SMARTBOX_CONFIG_VAR {
    volatile u8 speech_state;
    u32 feature_mask;
    u8 device_type;
    u8 phone_platform;
    void (*start_speech)(void);
    void (*stop_speech)(void);
    u8 err_report;
    volatile u8 file_browse_lock_flag;
    u32 return_msg;
    u8 spec_mode;
    struct __rcsp_user_var *rcsp_user;
    u8 ffr_mode;
    u16 ffr_time;
    volatile u8 wait_asr_end;
};



void smartbox_config(struct smartbox *smart);
struct smartbox *smartbox_handle_get(void);
u8 get_defalut_bt_channel_sel(void);

#endif//__SMARTBOX_CFG_H__

