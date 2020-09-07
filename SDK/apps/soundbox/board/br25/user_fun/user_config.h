#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#define USER_DEV_USB	"udisk0"
#define USER_DEV_SD0	"sd0"
#define USER_DEV_SD1	"sd1"

#define USER_PA_PIN_1 1//单io功放
#define USER_PA_PIN_2 2//双io功放

#define USER_PA_MUTE_H    1//高mute
#define USER_PA_MUTE_L    0//低mute

#define USER_PA_AB_H    1//高ab类
#define USER_PA_AB_L    0//低ab类

#define USER_PA_EN  1//功放总开关
#define USER_LED_EN 1//io led

#define USER_BT_VBAT_DISPLAY    1//手机电量显示 但不要通话功能  需要打开电量显示宏BT_SUPPORT_DISPLAY_BAT 与通话协议宏USER_SUPPORT_PROFILE_HFP
#define USER_FM_MODE_SYS_VOL    20//进FM 大于此音量设置为此音量 退出时恢复

#define USER_FIRST_BOOT_VOL     (SYS_MAX_VOL*2/3)//first boot 第一次上电开机音量
#define USER_POWER_ON_VOL       USER_FIRST_BOOT_VOL//每次开机音量 0或者注释 为系统默认
#define USER_TONE_PLAY_MODE     1//1:打断方式播放提示音 0:嵌入方式播放提示音
#define USER_MUSIC_TO_BT        1//music 下只有一个设备时 拔出设备 跳到bt模式
#define USER_USB_OR_SD	        USER_DEV_USB//进music模式优先设备
#define USER_POWER_ON_INIT      1//等开机提示音播放完之后再切到上线设备


#define USER_ADKEY_MAPPING_EN   1//AD KEY 按鍵值映射
#endif