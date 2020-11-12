#ifndef _USER_AC6966B_CFG_H_
#define _USER_AC6966B_CFG_H_

#ifdef CONFIG_BOARD_AC6966B

#define USER_SDK_BUG_3  ((TCFG_LINEIN_LR_CH == AUDIO_LIN_DACL_CH)||TCFG_LINEIN_LR_CH == AUDIO_LIN_DACR_CH)//linein 模式下 采用dac L进 dac R出的方式，播放提示音之后没声音
#define USER_SDK_BUG_4  0//linein 模式下 普通通道进 dac出，播放提示音 有参杂linein声音

#define USER_VBAT_CHECK_EN 1//外部电路检测
#define USER_LED_EN 1//io led
#define USER_LED_POR    IO_PORTB_07//LED IO
#define USER_RGB_EN 0//RGB pb7
#define USER_RGB_NUMBER 30//rgb 颗数
#define USER_RGB_LOOP_MODE  USER_RGB_LOOP_MODE_3//循环模式
#define USER_EQ_LIVE_UPDATE         1//EQ 旋钮实时更新

// #define USER_PA_MODE    USER_PA_PIN_2
#define USER_PA_MUTE_PORT   IO_PORTA_02//单双线复用功放 mute引脚 单线时做ab、mute切换
#define USER_PA_ABD_PORT    IO_PORTA_03//单双线复用功放 adb引脚  单线时开机输出高电平 客户通用pcb需要
#define USER_PA_ABD_MODE    USER_PA_AB_L//ab 类 为 高、低
#define USER_PA_MUTE_MODE   USER_PA_MUTE_H//mute 为 高、低

#define USER_SD_POWER_SWITCH_EN 0//SD 电源开关
#define USER_SD_POWER_IO IO_PORTA_00//与sd pg引脚绑定的引脚 没有选NO_CONFIG_PORT
#define USER_3IO_CHECK_4AD_EN 1//3IO检测4路ad
#define USER_SYS_VOL_CHECK_EN 0//旋钮控制系统音量 ad检测

#define USER_CHARGE_WAKE_UP 0//充电唤醒
#endif
#endif