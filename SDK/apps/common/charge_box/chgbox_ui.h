#ifndef _CHGBOX_UI_H_
#define _CHGBOX_UI_H_

#include "typedef.h"
//关于仓ui的说明，分为三个部分
//1.ui状态层
//2.ui中间层
//3.ui驱动层
//状态层主要就是外部把仓的状态传进来，中间层是一个过渡，如不想用本驱动，可以自己更换中间层
//或者只使用本驱动层作其他使用
/////////////////////////////////////////////////////////////////////////////////////////////
//ui状态层
typedef enum {
    CHGBOX_UI_NULL = 0,

    CHGBOX_UI_ALL_OFF,
    CHGBOX_UI_ALL_ON,

    CHGBOX_UI_POWER,
    CHGBOX_UI_EAR_FULL,
    CHGBOX_UI_LOCAL_FULL,
    CHGBOX_UI_LOWPOWER,

    CHGBOX_UI_EAR_L_IN,
    CHGBOX_UI_EAR_R_IN,
    CHGBOX_UI_EAR_L_OUT,
    CHGBOX_UI_EAR_R_OUT,

    CHGBOX_UI_KEY_CLICK,
    CHGBOX_UI_KEY_LONG,
    CHGBOX_UI_PAIR_START,
    CHGBOX_UI_PAIR_SUCC,
    CHGBOX_UI_PAIR_STOP,

    CHGBOX_UI_OPEN_LID,
    CHGBOX_UI_CLOSE_LID,

    CHGBOX_UI_USB_IN,
    CHGBOX_UI_USB_OUT,
} UI_STATUS;

enum {
    UI_MODE_CHARGE,
    UI_MODE_COMM,
    UI_MODE_LOWPOWER,
};

void  chgbox_ui_manage_init(void);
void chgbox_ui_update_status(u8 mode, u8 status);
void chgbox_ui_set_power_on(u8 flag);
u8 chgbox_get_ui_power_on(void);



/////////////////////////////////////////////////////////////////////////////////////////////
//ui中间层

//点灯模式
enum {
    CHGBOX_LED_RED_OFF,
    CHGBOX_LED_RED_ON,
    CHGBOX_LED_RED_SLOW_FLASH,
    CHGBOX_LED_RED_FLAST_FLASH,

    CHGBOX_LED_GREEN_OFF,
    CHGBOX_LED_GREEN_ON,
    CHGBOX_LED_GREEN_FAST_FLASH,

    CHGBOX_LED_BLUE_ON,
    CHGBOX_LED_BLUE_OFF,
    CHGBOX_LED_BLUE_FAST_FLASH,

    CHGBOX_LED_ALL_OFF,
    CHGBOX_LED_ALL_ON,
};
void chgbox_led_set_mode(u8 mode);


/////////////////////////////////////////////////////////////////////////////////////////////
//ui驱动层

//定义n个灯
enum {
    CHG_LED_RED,
    CHG_LED_GREEN,
    CHG_LED_BLUE,
    CHG_LED_MAX,
};

///IO设置
#define CHG_RED_LED_IO      IO_PORTC_02

#define CHG_GREEN_LED_IO    IO_PORTC_03

#define CHG_BLUE_LED_IO     IO_PORTC_04

//常亮、常暗、呼吸
enum {
    GHGBOX_LED_MODE_ON,
    GHGBOX_LED_MODE_OFF,
    GHGBOX_LED_MODE_BRE,
};


#define MC_TIMER_UNIT_US           30  //多少us起一次中断
#define SOFT_MC_PWM_MAX            128  //pwm周期(==MC_TIMER_UNIT_US * SOFT_MC_PWM_MAX 单位us)

//呼吸灯的步骤 渐亮--亮--渐暗--暗
enum {
    SOFT_LED_STEP_UP = 0,
    SOFT_LED_STEP_LIGHT,
    SOFT_LED_STEP_DOWN,
    SOFT_LED_STEP_DARK,
};

//闪烁快慢
enum {
    LED_FLASH_FAST,
    LED_FLASH_SLOW,

};

typedef struct _CHG_SOFT_PWM_LED {
    //初始化，亮暗接口接口
    void (*led_init)(void);
    void (*led_on_off)(u8 on_off);

    u16 bre_times;    //呼吸次数,0xffff为循环

    u16 up_times;     //渐亮次数
    u16 light_times;  //亮次数
    u16 down_times;   //渐暗次数
    u16 dark_times;   //暗次数

    u16 step_cnt;
    u8  step;         //步骤

    u8  p_cnt;        //占空比计数
    u8  cur_duty;     //当前占空比
    u8  max_duty;     //最大占空比，控制最大亮度

    u8  busy;         //忙标志，更换参数时作保护用

    u8  sp_flicker;    //特殊闪烁标志（用于处理亮-->亮，暗-->暗变化时灯光无反应的现象）
    u16 sp_flicker_cnt;//特殊闪烁维持时间

    u8  mode;         //亮灯模式
} CHG_SOFT_PWM_LED;

//led驱动初始化
void chgbox_led_init(void);
void chgbox_set_led_stu(u8 led_type, u8 on_off, u8 sp_flicker);
void chgbox_set_led_bre(u8 led_type, u8 speed_mode);
void chgbox_set_led_all_off(void);

#endif    //_APP_CHARGEBOX_H_

