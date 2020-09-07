#include "key_event_deal.h"
#include "key_driver.h"
#include "app_config.h"
#include "board_config.h"
#include "app_task.h"

#ifdef CONFIG_BOARD_AC6965E
/***********************************************************
 *				bt 模式的 iokey table
 ***********************************************************/
#if TCFG_APP_BT_EN
const u16 bt_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        /*SHORT*/ KEY_MUSIC_PP,	
        /*LONG*/  KEY_LED_IO_CTL/*KEY_CALL_HANG_UP*/,
        /*HOLD*/  KEY_NULL,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_CALL_LAST_NO,
        /*TRIBLE*/KEY_NULL
    },
    [1] = {
        /*SHORT*/ KEY_MUSIC_NEXT,	
        /*LONG*/  KEY_VOL_UP,			
        /*HOLD*/  KEY_VOL_UP,	
        /*UP*/	  KEY_NULL,	
        /*DOUBLE*/KEY_NULL,			
        /*TRIBLE*/KEY_NULL
    },
    [2] = {
        /*SHORT*/ KEY_MUSIC_PREV,	
        /*LONG*/  KEY_VOL_DOWN,			
        /*HOLD*/  KEY_VOL_DOWN,	
        /*UP*/	  KEY_NULL,	
        /*DOUBLE*/KEY_NULL,			
        /*TRIBLE*/KEY_NULL
    },
    [3] = {
        /*SHORT*/ KEY_CHANGE_MODE, 
        /*LONG*/  KEY_USER_TWS,			
        /*HOLD*/  KEY_NULL,	
        /*UP*/	  KEY_NULL,	
        /*DOUBLE*/KEY_NULL,			
        /*TRIBLE*/KEY_NULL
    },
    [4] = {
        /*SHORT*/ KEY_NULL,		
        /*LONG*/  KEY_NULL,			
        /*HOLD*/  KEY_NULL,	
        /*UP*/	  KEY_NULL,	
        /*DOUBLE*/KEY_NULL,			
        /*TRIBLE*/KEY_NULL
    },
    [5] = {
        /*SHORT*/ KEY_NULL,		
        /*LONG*/  KEY_NULL,			
        /*HOLD*/  KEY_NULL,	
        /*UP*/	  KEY_NULL,	
        /*DOUBLE*/KEY_NULL,			
        /*TRIBLE*/KEY_NULL
    },
};
#endif

/***********************************************************
 *				fm 模式的 iokey table
 ***********************************************************/
#if TCFG_APP_FM_EN
const u16 fm_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        /*SHORT*/ KEY_FM_SCAN_ALL/*KEY_MUSIC_PP*/,
        /*LONG*/  KEY_LED_IO_CTL,//KEY_POWEROFF,
        /*HOLD*/  KEY_NULL,//KEY_POWEROFF_HOLD,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,//KEY_CALL_LAST_NO,
        /*TRIBLE*/KEY_NULL
    },
    [1] = {
        /*SHORT*/ KEY_FM_NEXT_STATION,
        /*LONG*/  KEY_VOL_UP,
        /*HOLD*/  KEY_VOL_UP,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL
    },
    [2] = {
        /*SHORT*/ KEY_FM_PREV_STATION,
        /*LONG*/  KEY_VOL_DOWN,
        /*HOLD*/  KEY_VOL_DOWN,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL
    },
    [3] = {
        /*SHORT*/ KEY_CHANGE_MODE,
        /*LONG*/  KEY_NULL,
        /*HOLD*/  KEY_NULL,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL
    },
    [4] = {
        /*SHORT*/ KEY_FM_NEXT_FREQ,
        /*LONG*/  KEY_FM_SCAN_ALL_DOWN,
        /*HOLD*/  KEY_NULL,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL
    },
    [5] = {
        /*SHORT*/ KEY_FM_PREV_FREQ,
        /*LONG*/  KEY_FM_SCAN_ALL_UP,
        /*HOLD*/  KEY_NULL,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL
    },
};
#endif

/***********************************************************
 *				linein 模式的 iokey table
 ***********************************************************/
#if TCFG_APP_LINEIN_EN
const u16 linein_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
    /*SHORT*/ KEY_MUSIC_PP,
    /*LONG*/  KEY_LED_IO_CTL/*KEY_POWEROFF*/,
    /*HOLD*/  KEY_NULL/*KEY_POWEROFF_HOLD*/,
    /*UP*/	  KEY_NULL,
    /*DOUBLE*/KEY_NULL,
    /*TRIBLE*/KEY_NULL
    },
    [1] = {
        /*SHORT*/ KEY_NULL,
        /*LONG*/  KEY_VOL_UP,
        /*HOLD*/  KEY_VOL_UP,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL,
    },
    [2] = {
        /*SHORT*/ KEY_NULL,
        /*LONG*/  KEY_VOL_DOWN,
        /*HOLD*/  KEY_VOL_DOWN,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL
    },
    [3] = {
        /*SHORT*/ KEY_CHANGE_MODE,
        /*LONG*/  KEY_NULL,
        /*HOLD*/  KEY_NULL,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL
    },
    [4] = {
        /*SHORT*/ KEY_NULL,
        /*LONG*/  KEY_NULL,
        /*HOLD*/  KEY_NULL,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL
    },
    [5] = {
        /*SHORT*/ KEY_NULL,
        /*LONG*/  KEY_NULL,
        /*HOLD*/  KEY_NULL,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL
    },
};
#endif

/***********************************************************
 *				music 模式的 iokey table
 ***********************************************************/
#if TCFG_APP_MUSIC_EN
const u16 music_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        /*SHORT*/ KEY_MUSIC_PP,
        /*LONG*/  KEY_LED_IO_CTL,
        /*HOLD*/  KEY_NULL,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL,
    },
    [1] = {
        /*SHORT*/ KEY_MUSIC_NEXT,
        /*LONG*/  KEY_VOL_UP,
        /*HOLD*/  KEY_VOL_UP,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL,
    },
    [2] = {
        /*SHORT*/ KEY_MUSIC_PREV,
        /*LONG*/  KEY_VOL_DOWN,
        /*HOLD*/  KEY_VOL_DOWN,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL,
    },
    [3] = {
        /*SHORT*/ KEY_CHANGE_MODE,
        /*LONG*/  KEY_NULL,
        /*HOLD*/  KEY_NULL,
        /*UP*/	  KEY_NULL,
        /*DOUBLE*/KEY_NULL,
        /*TRIBLE*/KEY_NULL,
    },
    [4] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [5] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
};
#endif

/***********************************************************
 *				pc 模式的 iokey table
 ***********************************************************/
#if TCFG_APP_PC_EN
const u16 pc_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_MUSIC_PP,			KEY_POWEROFF,			KEY_POWEROFF_HOLD,	KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [1] = {
        KEY_VOL_UP,				KEY_VOL_UP,				KEY_VOL_UP,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [2] = {
        KEY_VOL_DOWN,			KEY_VOL_DOWN,			KEY_VOL_DOWN,		KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [3] = {
        KEY_CHANGE_MODE,		KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [4] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [5] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
};
#endif

/***********************************************************
 *				record 模式的 iokey table
 ***********************************************************/
#if TCFG_APP_RECORD_EN
const u16 record_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_MUSIC_PP,			KEY_POWEROFF,			KEY_POWEROFF_HOLD,	KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [1] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [2] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [3] = {
        KEY_CHANGE_MODE,		KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [4] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [5] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
};
#endif

/***********************************************************
 *				rtc 模式的 iokey table
 ***********************************************************/
#if TCFG_APP_RTC_EN
const u16 rtc_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_MUSIC_PP,			KEY_POWEROFF,			KEY_POWEROFF_HOLD,	KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [1] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [2] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [3] = {
        KEY_CHANGE_MODE,		KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [4] = {
        KEY_RTC_SW_POS,			KEY_RTC_SW,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [5] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
};
#endif

/***********************************************************
 *				spdif 模式的 iokey table
 ***********************************************************/
#if TCFG_APP_SPDIF_EN
const u16 spdif_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_MUSIC_PP,			KEY_POWEROFF,			KEY_POWEROFF_HOLD,	KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [1] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_SPDIF_SW_SOURCE, KEY_NULL
    },
    [2] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_SPDIF_SW_SOURCE, KEY_NULL
    },
    [3] = {
        KEY_CHANGE_MODE,		KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [4] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [5] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
};
#endif

/***********************************************************
 *				idle 模式的 iokey table
 ***********************************************************/
const u16 idle_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        // KEY_NULL,			    KEY_POWER_ON,			KEY_POWER_ON_HOLD,	KEY_NULL,	KEY_NULL,			KEY_NULL
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,           KEY_NULL
    },
    [1] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,           KEY_NULL
    },
    [2] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,           KEY_NULL
    },
    [3] = {
        KEY_NULL,		        KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [4] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [5] = {
        KEY_NULL,				KEY_NULL,				KEY_NULL,			KEY_NULL,	KEY_NULL,			KEY_NULL
    },
};
#endif
