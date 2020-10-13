#ifndef _USER_AC6969D_CFG_H_
#define _USER_AC6969D_CFG_H_

#ifdef CONFIG_BOARD_AC6969D_DEMO

#define USER_VBAT_CHECK_EN 0//外部电路检测
#define USER_LED_EN 0//io led
#define USER_LED_POR    IO_PORTB_07//LED IO
#define USER_RGB_EN 0//RGB pb7
#define USER_RGB_NUMBER 30//rgb 颗数
#define USER_RGB_LOOP_MODE  USER_RGB_LOOP_MODE_1//循环模式
#define USER_EQ_LIVE_UPDATE         0//EQ 旋钮实时更新

#define USER_SDK_BUG_3  ((TCFG_LINEIN_LR_CH == AUDIO_LIN_DACL_CH)||TCFG_LINEIN_LR_CH == AUDIO_LIN_DACR_CH)//linein 模式下 采用dac L进 dac R出的方式，播放提示音之后没声音
#define USER_SDK_BUG_4  1//linein 模式下 普通通道进 dac出，播放提示音 有参杂linein声音

#define USER_PA_MODE    USER_PA_PIN_2//功放类型  单线、双线
#define USER_PA_ABD_MUTE_PORT NO_CONFIG_PORT//单线功放
#define USER_PA_MUTE_PORT   IO_PORTA_01//双线功放 mute引脚
#define USER_PA_ABD_PORT    IO_PORTB_05//双线功放 adb引脚
#define USER_PA_ABD_MODE    USER_PA_AB_L//ab 类 为 高、低
#define USER_PA_MUTE_MODE   USER_PA_MUTE_H//mute 为 高、低

#define USER_SD_POWER_SWITCH_EN 0//SD pg 引脚为sd供电
#define USER_SD_POWER_IO 没有选NO_CONFIG_PORT//与sd pg引脚绑定的引脚 没有选NO_CONFIG_PORT


#define USER_AUXR_IN_DACL_OUT   0//linein 右输入 dac只有左输出



#endif
#endif