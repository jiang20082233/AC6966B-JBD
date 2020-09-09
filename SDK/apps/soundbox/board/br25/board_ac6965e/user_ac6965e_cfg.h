#ifndef _USER_AC6965E_CFG_H_
#define _USER_AC6965E_CFG_H_

#ifdef CONFIG_BOARD_AC6965E

#define USER_VBAT_CHECK_EN 0//外部电路检测

#define USER_PA_MODE    USER_PA_PIN_2//功放类型  单线、双线
#define USER_PA_ABD_MUTE_PORT NO_CONFIG_PORT//单线功放
#define USER_PA_MUTE_PORT   IO_PORTB_06//双线功放 mute引脚
#define USER_PA_ABD_PORT    IO_PORTB_04//双线功放 adb引脚
#define USER_PA_ABD_MODE    USER_PA_AB_L//ab 类 为 高、低
#define USER_PA_MUTE_MODE   USER_PA_MUTE_H//mute 为 高、低

#define USER_SD_POWER_SWITCH_EN 1//SD pg 引脚为sd供电
#define USER_SD_POWER_IO IO_PORTA_00//与sd pg引脚绑定的引脚 没有选NO_CONFIG_PORT


#define USER_AUXR_IN_DACL_OUT   1//linein 右输入 dac只有左输出

#endif
#endif