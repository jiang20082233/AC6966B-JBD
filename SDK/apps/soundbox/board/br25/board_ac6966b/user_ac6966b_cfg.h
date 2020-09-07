#ifndef _USER_AC6966B_CFG_H_
#define _USER_AC6966B_CFG_H_

#ifdef CONFIG_BOARD_AC6966B

#define USER_VBAT_CHECK_EN 1//外部电路检测
#define USER_PA_MODE    USER_PA_PIN_1

#define USER_PA_ABD_MUTE_PORT IO_PORTA_02//单线功放
#define USER_PA_MUTE_PORT   IO_PORTB_06//双线功放 mute引脚
#define USER_PA_ABD_PORT    IO_PORTB_04//双线功放 adb引脚
#define USER_PA_ABD_MODE    USER_PA_AB_L//ab 类 为 高、低
#define USER_PA_MUTE_MODE   USER_PA_MUTE_H//mute 为 高、低

#define USER_SD_POWER_SWITCH_EN 0//SD 电源开关


#endif
#endif