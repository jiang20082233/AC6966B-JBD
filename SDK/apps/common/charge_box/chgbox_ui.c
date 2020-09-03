#include "typedef.h"
#include "asm/pwm_led.h"
#include "system/includes.h"
#include "chgbox_ctrl.h"
#include "chargeIc_manage.h"
#include "chgbox_ui.h"


#if(TCFG_CHARGE_BOX_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHGBOXUI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


//关于仓ui的说明，分为三个部分
//1.ui状态层
//2.ui中间层
//3.ui驱动层
//状态层主要就是外部把仓的状态传进来，中间层是一个过渡，如不想用本驱动，可以自己更换中间层
//或者只使用本驱动层作其他使用

/////////////////////////////////////////////////////////////////////////////////////////////
//ui状态层
typedef struct _chgbox_ui_var_ {
    int ui_timer;
    u8  ui_power_on; //上电标志
} _chgbox_ui_var;

static _chgbox_ui_var chgbox_ui_var;
#define __this  (&chgbox_ui_var)

/*------------------------------------------------------------------------------------*/
/**@brief    UI超时函数
   @param    priv:ui状态
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_update_timeout(void *priv)
{
    u8 ledmode = (u8)priv;
    __this->ui_timer = 0;
    chgbox_led_set_mode(ledmode);
}

/*------------------------------------------------------------------------------------*/
/**@brief    UI超时设置
   @param    priv:传到func的参数
             func:超时后的回调函数
             msec:N毫秒后调用func
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
u16 chgbox_ui_timeout_add(int priv, void (*func)(void *priv), u32 msec)
{
    if (__this->ui_timer) {
        sys_timer_del(__this->ui_timer);
        __this->ui_timer = 0;
    }
    if (func != NULL) {
        __this->ui_timer = sys_timeout_add((void *)priv, func, msec);
    }
    return __this->ui_timer;
}

/*------------------------------------------------------------------------------------*/
/**@brief    设置ui上电标志位
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_set_power_on(u8 flag)
{
    __this->ui_power_on = flag;
}

/*------------------------------------------------------------------------------------*/
/**@brief    获取ui上电标志位
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
u8 chgbox_get_ui_power_on(void)
{
    return __this->ui_power_on;
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓电量态ui更新
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_update_local_power(void)
{
    //配对状态,不显示舱电量UI
    if (sys_info.pair_status) {
        return;
    }
    if (sys_info.status[USB_DET] == STATUS_ONLINE) {
        chgbox_ui_timeout_add(0, NULL, 0);
        if (sys_info.localfull) {
            chgbox_led_set_mode(CHGBOX_LED_GREEN_ON);//充满后绿灯常亮
        } else {
            chgbox_led_set_mode(CHGBOX_LED_RED_SLOW_FLASH);//充电中灯慢闪
        }
    } else {
        if (sys_info.lowpower_flag) {
            chgbox_led_set_mode(CHGBOX_LED_GREEN_FAST_FLASH); //快闪4秒
            chgbox_ui_timeout_add(CHGBOX_LED_GREEN_OFF, chgbox_ui_update_timeout, 4000);
        } else {
            chgbox_led_set_mode(CHGBOX_LED_GREEN_ON);
            chgbox_ui_timeout_add(CHGBOX_LED_GREEN_OFF, chgbox_ui_update_timeout, 8000);
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓公共态ui更新
   @param    status:UI状态
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_updata_default_status(u8 status)
{
    switch (status) {
    case CHGBOX_UI_ALL_OFF:
        chgbox_ui_timeout_add(0, NULL, 0);
        chgbox_led_set_mode(CHGBOX_LED_ALL_OFF);
        break;
    case CHGBOX_UI_ALL_ON:
        chgbox_ui_timeout_add(0, NULL, 0);
        chgbox_led_set_mode(CHGBOX_LED_ALL_ON);
        break;
    case CHGBOX_UI_POWER:
        chgbox_ui_update_local_power();
        break;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓合盖充电ui更新
   @param    status:UI状态
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_updata_charge_status(u8 status)
{
    switch (status) {
    case CHGBOX_UI_USB_IN:
    case CHGBOX_UI_KEY_CLICK:
    case CHGBOX_UI_LOCAL_FULL:
        chgbox_ui_update_local_power();
        break;
    case CHGBOX_UI_USB_OUT:
        chgbox_ui_timeout_add(0, NULL, 0);
        chgbox_led_set_mode(CHGBOX_LED_ALL_OFF);
        break;
    case CHGBOX_UI_CLOSE_LID:
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            chgbox_ui_update_local_power();
        } else {
            chgbox_ui_timeout_add(0, NULL, 0);
            chgbox_led_set_mode(CHGBOX_LED_GREEN_OFF);
        }
        break;
    case CHGBOX_UI_EAR_FULL:
        break;
    default:
        chgbox_ui_updata_default_status(status);
        break;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓开盖通信ui更新
   @param    status:UI状态
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_updata_comm_status(u8 status)
{
    switch (status) {
    case CHGBOX_UI_USB_IN:
    case CHGBOX_UI_LOCAL_FULL:
    case CHGBOX_UI_KEY_CLICK:
    case CHGBOX_UI_OPEN_LID:
        chgbox_ui_update_local_power();
        break;
    case CHGBOX_UI_USB_OUT:
        if (!sys_info.pair_status) {
            chgbox_ui_timeout_add(0, NULL, 0);
            chgbox_led_set_mode(CHGBOX_LED_RED_OFF);
        }
        break;
    case CHGBOX_UI_EAR_L_IN:
    case CHGBOX_UI_EAR_R_IN:
    case CHGBOX_UI_EAR_L_OUT:
    case CHGBOX_UI_EAR_R_OUT:
        if (!sys_info.pair_status) {
            if (sys_info.status[USB_DET] == STATUS_ONLINE) {
                if (sys_info.localfull) {
                    chgbox_led_set_mode(CHGBOX_LED_RED_OFF);
                    chgbox_ui_timeout_add(CHGBOX_LED_RED_ON, chgbox_ui_update_timeout, 500);
                }
            } else {
                chgbox_led_set_mode(CHGBOX_LED_GREEN_ON);
                chgbox_ui_timeout_add(CHGBOX_LED_GREEN_OFF, chgbox_ui_update_timeout, 500);
            }
        }
        break;
    case CHGBOX_UI_KEY_LONG:
        chgbox_ui_timeout_add(0, NULL, 0);
        chgbox_led_set_mode(CHGBOX_LED_BLUE_ON);
        break;
    case CHGBOX_UI_PAIR_START:
        chgbox_ui_timeout_add(0, NULL, 0);
        chgbox_led_set_mode(CHGBOX_LED_BLUE_FAST_FLASH);
        break;
    case CHGBOX_UI_PAIR_SUCC:
        if (sys_info.status[USB_DET] == STATUS_OFFLINE) {
            chgbox_ui_timeout_add(CHGBOX_LED_BLUE_OFF, chgbox_ui_update_timeout, 500);
        } else {
            if (!sys_info.localfull) {
                chgbox_ui_timeout_add(CHGBOX_LED_RED_SLOW_FLASH, chgbox_ui_update_timeout, 500);
            } else {
                chgbox_ui_timeout_add(0, NULL, 0);
                chgbox_led_set_mode(CHGBOX_LED_BLUE_ON);
            }
        }
        break;
    case CHGBOX_UI_PAIR_STOP:
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            chgbox_ui_update_local_power();
        } else {
            chgbox_ui_timeout_add(0, NULL, 0);
            chgbox_led_set_mode(CHGBOX_LED_BLUE_OFF);
        }
        break;
    default:
        chgbox_ui_updata_default_status(status);
        break;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓低电量ui更新
   @param    status:UI状态
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_updata_lowpower_status(u8 status)
{
    switch (status) {
    case CHGBOX_UI_LOCAL_FULL:
    case CHGBOX_UI_LOWPOWER:
    case CHGBOX_UI_KEY_CLICK:
    case CHGBOX_UI_OPEN_LID:
    case CHGBOX_UI_CLOSE_LID:
    case CHGBOX_UI_USB_OUT:
    case CHGBOX_UI_USB_IN:
        chgbox_ui_update_local_power();
        break;
    default:
        chgbox_ui_updata_default_status(status);
        break;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓UI更新仓状态
   @param    mode:  充电仓当前的UI模式（与充电仓的三个模式对应）
             status:充电仓当前状态
   @return
   @note     各个模式根据状态控制ui变化
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_update_status(u8 mode, u8 status)
{
    switch (mode) {
    case UI_MODE_CHARGE:
        chgbox_ui_updata_charge_status(status);
        break;
    case UI_MODE_COMM:
        chgbox_ui_updata_comm_status(status);
        break;
    case UI_MODE_LOWPOWER:
        chgbox_ui_updata_lowpower_status(status);
        break;
    }
    chgbox_ui_set_power_on(0);
}



/*------------------------------------------------------------------------------------*/
/**@brief    充电仓UI初始化
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_manage_init(void)
{
    chgbox_led_init();
}


/////////////////////////////////////////////////////////////////////////////////////////////
//ui中间层

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓设置呼吸灯模式
   @param    mode: 灯模式
   @return   无
   @note     设置充电仓灯的状态,不同的亮暗或闪烁可以调配
*/
/*------------------------------------------------------------------------------------*/
void chgbox_led_set_mode(u8 mode)
{
    u8 i;
    log_info("CHG_LED_mode:%d\n", mode);
    switch (mode) {
    case CHGBOX_LED_RED_ON://红灯
        chgbox_set_led_stu(CHG_LED_RED, 1, 0);
        break;
    case CHGBOX_LED_RED_OFF://红灯
        chgbox_set_led_stu(CHG_LED_RED, 0, 0);
        break;
    case CHGBOX_LED_GREEN_ON:
        chgbox_set_led_stu(CHG_LED_GREEN, 1, 1);
        break;
    case CHGBOX_LED_GREEN_OFF:
        chgbox_set_led_stu(CHG_LED_GREEN, 0, 1);
        break;
    case CHGBOX_LED_BLUE_ON:
        chgbox_set_led_stu(CHG_LED_BLUE, 1, 0);
        break;
    case CHGBOX_LED_BLUE_OFF:
        chgbox_set_led_stu(CHG_LED_BLUE, 0, 0);
        break;
    case CHGBOX_LED_RED_SLOW_FLASH:
        chgbox_set_led_bre(CHG_LED_RED, LED_FLASH_SLOW);
        break;
    case CHGBOX_LED_RED_FLAST_FLASH:
        chgbox_set_led_bre(CHG_LED_RED, LED_FLASH_FAST);
        break;
    case CHGBOX_LED_GREEN_FAST_FLASH:
        chgbox_set_led_bre(CHG_LED_GREEN, LED_FLASH_FAST);
        break;
    case CHGBOX_LED_BLUE_FAST_FLASH:
        chgbox_set_led_bre(CHG_LED_BLUE, LED_FLASH_FAST);
        break;
    case CHGBOX_LED_ALL_OFF:
        chgbox_set_led_all_off();
        break;
    case CHGBOX_LED_ALL_ON:
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
//ui驱动层

//注意时间尺度,即是mc_clk初始化出来的时间
#define UP_TIMES_DEFAULT         50
#define DOWN_TIMES_DEFAULT       50
#define SP_FLICKER_CNT_DEFAULT   30 //特殊闪烁时长计数

CHG_SOFT_PWM_LED chgbox_led[CHG_LED_MAX];


/*------------------------------------------------------------------------------------*/
/**@brief    设置灯为常亮、常暗模式
   @param    led_type:  灯序号
             on_off:    1-->常亮 0-->常暗
             sp_flicker:是否闪烁标志 1-->在亮--亮 暗--暗过程中取反一下
   @return   无
   @note     把灯设置为 常亮/常暗 模式(默认同时设置其他灯为常暗)
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_stu(u8 led_type, u8 on_off, u8 sp_flicker)
{
    //只开关一个灯,其他关掉
    u8 i;
    for (i = 0; i < CHG_LED_MAX; i++) {
        chgbox_led[i].busy = 1;

        if (led_type == i) {
            if (on_off) {
                chgbox_led[i].mode = GHGBOX_LED_MODE_ON; //常亮
                //要根据当前占空比，把cnt计算好，避免亮度突变
                chgbox_led[i].step_cnt = chgbox_led[i].up_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
                chgbox_led[i].up_times = UP_TIMES_DEFAULT;
            } else {
                chgbox_led[i].mode = GHGBOX_LED_MODE_OFF; //常暗
                //要根据当前占空比，把cnt计算好
                chgbox_led[i].step_cnt = chgbox_led[i].down_times - chgbox_led[i].down_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
                chgbox_led[i].down_times = DOWN_TIMES_DEFAULT;
            }
            //设置特殊闪烁
            chgbox_led[i].sp_flicker = sp_flicker;
            chgbox_led[i].sp_flicker_cnt = SP_FLICKER_CNT_DEFAULT;
        } else {
            chgbox_led[i].sp_flicker = 0;
            chgbox_led[i].mode = GHGBOX_LED_MODE_OFF; //其他灯常暗
            chgbox_led[i].step_cnt = chgbox_led[i].down_times - chgbox_led[i].down_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
            chgbox_led[i].down_times = 20;
        }
        chgbox_led[i].bre_times = 0;
        chgbox_led[i].busy = 0;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    设置灯为呼吸模式
   @param    led_type:  灯序号
             speed_mode:闪烁参数选择，这里默认为快、慢两种
   @return   无
   @note     把灯设置为呼吸模式(默认同时其他灯为常暗)
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_bre(u8 led_type, u8 speed_mode)
{
    //只开关一个灯,其他关掉
    u8 i;
    for (i = 0; i < CHG_LED_MAX; i++) {
        chgbox_led[i].busy = 1;

        if (led_type == i) {
            chgbox_led[i].mode = GHGBOX_LED_MODE_BRE; //呼吸
            chgbox_led[i].bre_times = 0xffff;         //循环
            if (speed_mode == LED_FLASH_FAST) {
                chgbox_led[i].up_times = 50;
                chgbox_led[i].light_times = 50;
                chgbox_led[i].down_times = 50;
                chgbox_led[i].dark_times = 10;
            } else if (speed_mode == LED_FLASH_SLOW) {
                chgbox_led[i].up_times = 300;
                chgbox_led[i].light_times = 200;
                chgbox_led[i].down_times = 300;
                chgbox_led[i].dark_times = 100;
            }
            //要根据当前占空比，把cnt计算好,
            chgbox_led[i].step_cnt = chgbox_led[i].up_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
        } else {
            chgbox_led[i].mode = GHGBOX_LED_MODE_OFF; //常暗
            chgbox_led[i].down_times = 20;
            chgbox_led[i].step_cnt = chgbox_led[i].down_times - chgbox_led[i].down_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
            chgbox_led[i].bre_times = 0;
        }
        chgbox_led[i].busy = 0;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    呼吸灯全暗
   @param    无
   @return   无
   @note     把所有的灯设置为常暗模式
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_all_off(void)
{
    u8 i;
    for (i = 0; i < CHG_LED_MAX; i++) {
        chgbox_led[i].busy = 1;

        chgbox_led[i].mode = GHGBOX_LED_MODE_OFF; //常暗
        chgbox_led[i].step_cnt = chgbox_led[i].down_times - chgbox_led[i].down_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
        chgbox_led[i].down_times = DOWN_TIMES_DEFAULT;
        chgbox_led[i].bre_times = 0;

        chgbox_led[i].busy = 0;
    }
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//硬件、软pwm驱动部分(时钟、占空比控制等)
//蓝灯初始化
void chg_red_led_init()
{
    gpio_set_die(CHG_RED_LED_IO, 1);
    gpio_set_pull_down(CHG_RED_LED_IO, 0);
    gpio_set_pull_up(CHG_RED_LED_IO, 0);
    gpio_direction_output(CHG_RED_LED_IO, 0);
    //初始化为暗态
    gpio_direction_output(CHG_RED_LED_IO, 0);
}

//蓝灯高低电平控制
//on_off:1:输出高平电  0:输出低电平
SEC(.chargebox_code)//频繁调用的放ram里
void chg_set_red_led(u8 on_off)
{
    u8 io_num;
    io_num = CHG_RED_LED_IO % 16;
    if (on_off) {
#if(CHG_RED_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT |= BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT |= BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT |= BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTD_7)
        JL_PORTD->OUT |= BIT(io_num);
#endif
    } else {
#if(CHG_RED_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT &= ~BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT &= ~BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT &= ~BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTD_7)
        JL_PORTD->OUT &= ~BIT(io_num);
#endif
    }
}

//绿灯初始化
void chg_green_led_init()
{
    gpio_set_die(CHG_GREEN_LED_IO, 1);
    gpio_set_pull_down(CHG_GREEN_LED_IO, 0);
    gpio_set_pull_up(CHG_GREEN_LED_IO, 0);
    gpio_direction_output(CHG_GREEN_LED_IO, 0);
    //初始化为暗态
    gpio_direction_output(CHG_GREEN_LED_IO, 0);
}

//绿灯高低电平控制
//on_off:1:输出高平电  0:输出低电平
SEC(.chargebox_code)//频繁调用的放ram里
void chg_set_green_led(u8 on_off)
{
    u8 io_num;
    io_num = CHG_GREEN_LED_IO % 16;
    if (on_off) {
#if(CHG_GREEN_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT |= BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT |= BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT |= BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTD_7)
        JL_PORTD->OUT |= BIT(io_num);
#endif
    } else {
#if(CHG_GREEN_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT &= ~BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT &= ~BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT &= ~BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTD_7)
        JL_PORTD->OUT &= ~BIT(io_num);
#endif
    }
}

//蓝灯初始化
void chg_blue_led_init()
{
    gpio_set_die(CHG_BLUE_LED_IO, 1);
    gpio_set_pull_down(CHG_BLUE_LED_IO, 0);
    gpio_set_pull_up(CHG_BLUE_LED_IO, 0);
    gpio_direction_output(CHG_BLUE_LED_IO, 0);
    //初始化为暗态
    gpio_direction_output(CHG_BLUE_LED_IO, 0);
}

//蓝灯高低电平控制
//on_off:1:输出高平电  0:输出低电平
SEC(.chargebox_code)
void chg_set_blue_led(u8 on_off)
{
    u8 io_num;
    io_num = CHG_BLUE_LED_IO % 16;
    if (on_off) {
#if(CHG_BLUE_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT |= BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT |= BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT |= BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTD_7)
        JL_PORTD->OUT |= BIT(io_num);
#endif
    } else {
#if(CHG_BLUE_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT &= ~BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT &= ~BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT &= ~BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTD_7)
        JL_PORTD->OUT &= ~BIT(io_num);
#endif
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    呼吸灯占空比控制
   @param    i:灯序号
   @return   无
   @note     根据常亮、常暗、呼吸等模式控制占空比
*/
/*------------------------------------------------------------------------------------*/
SEC(.chargebox_code)
void soft_pwm_led_ctrl(u8 i)
{
    switch (chgbox_led[i].mode) {
    case GHGBOX_LED_MODE_ON: //常亮
        if (chgbox_led[i].sp_flicker) { //暗一下(包括了渐暗+暗 两个过程)
            if (chgbox_led[i].cur_duty > 0) {
                chgbox_led[i].step_cnt++;
                if (chgbox_led[i].step_cnt >= chgbox_led[i].down_times) {
                    chgbox_led[i].step_cnt = 0;
                    chgbox_led[i].cur_duty  = 0;
                } else {
                    chgbox_led[i].cur_duty = (chgbox_led[i].down_times - chgbox_led[i].step_cnt) * chgbox_led[i].max_duty / chgbox_led[i].down_times;
                }
            } else {
                if (chgbox_led[i].sp_flicker_cnt) { //持续暗的时间
                    chgbox_led[i].sp_flicker_cnt--;
                    if (chgbox_led[i].sp_flicker_cnt == 0) {
                        chgbox_led[i].sp_flicker = 0; //结束暗一下流程
                    }
                }
            }
        } else if (chgbox_led[i].cur_duty < chgbox_led[i].max_duty) {
            chgbox_led[i].step_cnt++;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].up_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].cur_duty  = chgbox_led[i].max_duty;
            } else {
                //这里为了避免灯光突变，根据改变前的亮度来计算了cnt
                chgbox_led[i].cur_duty = chgbox_led[i].step_cnt * chgbox_led[i].max_duty / chgbox_led[i].up_times;
            }
        }
        break;
    case GHGBOX_LED_MODE_OFF://常暗
        if (chgbox_led[i].sp_flicker) { //亮一下
            if (chgbox_led[i].cur_duty < chgbox_led[i].max_duty) {
                chgbox_led[i].step_cnt++;
                if (chgbox_led[i].step_cnt >= chgbox_led[i].up_times) {
                    chgbox_led[i].step_cnt = 0;
                    chgbox_led[i].cur_duty  = chgbox_led[i].max_duty;
                } else {
                    //这里为了避免灯光突变，根据改变前的亮度来计算了cnt
                    chgbox_led[i].cur_duty = chgbox_led[i].step_cnt * chgbox_led[i].max_duty / chgbox_led[i].up_times;
                }
            } else {
                if (chgbox_led[i].sp_flicker_cnt) { //持续亮的时间
                    chgbox_led[i].sp_flicker_cnt--;
                    if (chgbox_led[i].sp_flicker_cnt == 0) {
                        chgbox_led[i].sp_flicker = 0; //结束亮一下流程
                    }
                }
            }
        } else if (chgbox_led[i].cur_duty > 0) {
            chgbox_led[i].step_cnt++;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].down_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].cur_duty  = 0;
            } else {
                //这里为了避免灯光突变，根据改变前的亮度来计算了cnt
                chgbox_led[i].cur_duty = (chgbox_led[i].down_times - chgbox_led[i].step_cnt) * chgbox_led[i].max_duty / chgbox_led[i].down_times;
            }
        }
        break;
    case GHGBOX_LED_MODE_BRE://呼吸灯模式
        if (chgbox_led[i].bre_times == 0) {
            break;
        }

        if (chgbox_led[i].step == SOFT_LED_STEP_UP) {
            chgbox_led[i].step_cnt++;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].up_times) { //当前段结束
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].step++; //进入下一个步骤
            } else {
                chgbox_led[i].cur_duty = chgbox_led[i].step_cnt * chgbox_led[i].max_duty / chgbox_led[i].up_times;
            }
        } else if (chgbox_led[i].step == SOFT_LED_STEP_LIGHT) {
            chgbox_led[i].step_cnt++;
            chgbox_led[i].cur_duty = chgbox_led[i].max_duty;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].light_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].step++;
            }
        } else if (chgbox_led[i].step == SOFT_LED_STEP_DOWN) {
            chgbox_led[i].step_cnt++;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].down_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].step++;
            } else {
                chgbox_led[i].cur_duty = (chgbox_led[i].down_times - chgbox_led[i].step_cnt) * chgbox_led[i].max_duty / chgbox_led[i].down_times;
            }
        } else if (chgbox_led[i].step == SOFT_LED_STEP_DARK) {
            chgbox_led[i].step_cnt++;
            chgbox_led[i].cur_duty = 0;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].dark_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].step = 0;    //重新开始下一次呼吸
                if (chgbox_led[i].bre_times != 0xffff) { //非循环
                    chgbox_led[i].bre_times--; //呼吸次数递减
                }
            }
        }
        break;
    }
}


/*------------------------------------------------------------------------------------*/
/**@brief    mc_clk中断回调
   @param    无
   @return   无
   @note     用于循环所有的呼吸灯，包括亮暗控制,占空比设置等
*/
/*------------------------------------------------------------------------------------*/
SEC(.chargebox_code)
___interrupt
void soft_pwm_led_isr(void)
{
    JL_MCPWM->TMR0_CON |= BIT(10); //清pending
    u8 i;
    for (i = 0; i < CHG_LED_MAX; i++) { //循环所有的灯
        if (!chgbox_led[i].busy) {
            if (chgbox_led[i].p_cnt < chgbox_led[i].cur_duty) { //占空比
                chgbox_led[i].led_on_off(1); //亮
            } else {
                chgbox_led[i].led_on_off(0);
            }
            chgbox_led[i].p_cnt++;
            if (chgbox_led[i].p_cnt >= SOFT_MC_PWM_MAX) { //完成一个PWM周期
                chgbox_led[i].p_cnt = 0;
                soft_pwm_led_ctrl(i);//占空比控制
            }
        }
    }
}

static const u32 timer_div_mc[] = {
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768
};
#define MC_MAX_TIME_CNT            0x7fff
#define MC_MIN_TIME_CNT            0x10
/*------------------------------------------------------------------------------------*/
/**@brief    mc_clk的初始化
   @param    无
   @return   无
   @note     初始clk,注册中断,MC_TIMER_UNIT_US 为起中断的时间,单位us
*/
/*------------------------------------------------------------------------------------*/
void mc_clk_init(void)
{
    u32 prd_cnt;
    u8 index;

    JL_MCPWM->TMR0_CON = BIT(10); //清pending,清其他bit
    JL_MCPWM->MCPWM_CON0 = 0;

    for (index = 0; index < (sizeof(timer_div_mc) / sizeof(timer_div_mc[0])); index++) {
        prd_cnt = MC_TIMER_UNIT_US * (clk_get("lsb") / 1000000) / timer_div_mc[index];
        if (prd_cnt > MC_MIN_TIME_CNT && prd_cnt < MC_MAX_TIME_CNT) {
            break;
        }
    }

    JL_MCPWM->TMR0_CNT = 0;
    JL_MCPWM->TMR0_PR = prd_cnt;
    JL_MCPWM->TMR0_CON |= index << 3; //分频系数

    request_irq(IRQ_MCTMRX_IDX, 3, soft_pwm_led_isr, 0);

    JL_MCPWM->TMR0_CON |= BIT(8);  //允许定时溢出中断
    JL_MCPWM->TMR0_CON |= BIT(0);  //递增模式

    JL_MCPWM->MCPWM_CON0 |= BIT(8); //只开mc timer 0
    /* log_info("prd_cnt:%d,index:%d,t0:%x,MCP:%x\n",prd_cnt,index,JL_MCPWM->TMR0_CON,JL_MCPWM->MCPWM_CON0); */
    /* log_info("lsb:%d\n",clk_get("lsb")); */
}

/*------------------------------------------------------------------------------------*/
/**@brief    led呼吸灯初始化
   @param    无
   @return   无
   @note     初始化每个led:渐亮、亮、渐暗、暗，最大亮度，对应IO的初始化.mc_clk的初始化，用于
             控制pwm
*/
/*------------------------------------------------------------------------------------*/
void chgbox_led_init(void)
{
    u8 i;
    for (i = 0; i < CHG_LED_MAX; i++) { //循环所有的灯
        memset(&chgbox_led[i], 0x0, sizeof(CHG_SOFT_PWM_LED));
        chgbox_led[i].up_times = UP_TIMES_DEFAULT;
        chgbox_led[i].light_times = 100;
        chgbox_led[i].down_times = DOWN_TIMES_DEFAULT;
        chgbox_led[i].dark_times = 10;
        chgbox_led[i].bre_times = 0;

        //可根据需要修改初始化，但要把初始化与亮灭注册进来
        if (i == CHG_LED_RED) {
            chgbox_led[i].led_on_off = chg_set_red_led;
            chgbox_led[i].led_init = chg_red_led_init;
            chgbox_led[i].max_duty = SOFT_MC_PWM_MAX;
            chgbox_led[i].mode = GHGBOX_LED_MODE_OFF;
        } else if (i == CHG_LED_GREEN) {
            chgbox_led[i].led_on_off = chg_set_green_led;
            chgbox_led[i].led_init = chg_green_led_init;
            chgbox_led[i].max_duty = SOFT_MC_PWM_MAX;
            chgbox_led[i].mode = GHGBOX_LED_MODE_OFF;
        } else if (i == CHG_LED_BLUE) {
            chgbox_led[i].led_on_off = chg_set_blue_led;
            chgbox_led[i].led_init = chg_blue_led_init;
            chgbox_led[i].max_duty = SOFT_MC_PWM_MAX;
            chgbox_led[i].mode = GHGBOX_LED_MODE_OFF;
        }
        chgbox_led[i].led_init();
    }
    mc_clk_init();
}

#endif
