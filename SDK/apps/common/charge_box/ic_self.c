#include "chargeIc_manage.h"
#include "chgbox_ctrl.h"
#include "ic_self.h"
#include "app_config.h"
#include "app_main.h"
#include "chgbox_box.h"


#if(TCFG_CHARGE_BOX_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[IC_SELF]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


//进入退出lowpower时的电压,加点压差，避免频繁切换
#define LOWER_POWER_ENTER_VOLT  3300  //进入低电电压
#define LOWER_POWER_EXIT_VOLT  3320  //退出低电电压

//USB上下线次数
#define  USB_DET_OFFLINE_TIME  10
#define  USB_DET_ONLINE_TIME   10

//电量检测
#define STAT_LOW_POWER_MAX    50       //进入与退出低电次数

//满电检测
#define STAT_FULL_POWER_MAX   200      //n次满电后发消息

//上电没电电压（马上关机）
#define POWER_ON_SHUTDOWN_VOLT   3100


//ldo升压 检测电压
#define LDO_SUCC_VOLT     4000 //
#define LDO_SUCC_TIMES    5
#define LDO_OFF_TIMES     5



static volatile u16 cur_bat_volt = 0;//当前充电盒子的电量

#define BAT_AVE_COUNTS    10 //检测n次
#define BAT_CUT_COUNTS     2  //首尾各去n个后计算均值
static u16 battery_value_tab[BAT_AVE_COUNTS];

/*------------------------------------------------------------------------------------*/
/**@brief    电池电量检测
   @param    无
   @return   无
   @note     用于检测电池的电量,作出低电判断等
*/
/*------------------------------------------------------------------------------------*/
static void ic_self_power_det()
{
    ///上电初始化时会给一个初值给电池电量
    static u8 bat_cnt = 0;
    static u16 lowpower_cnt = 0;
    u16 ad_min, bat_volt_tmp;
    u8 i, j, k;

    battery_value_tab[bat_cnt++] = adc_get_voltage(BAT_DET_AD_CH) * 3; //注意电路分压

    if (bat_cnt == BAT_AVE_COUNTS) { //n次检测后排序
        for (i = 1; i < BAT_AVE_COUNTS; i++) {
            for (j = i; j > 0; j--) {
                if (battery_value_tab[j] < battery_value_tab[j - 1]) {
                    ad_min = battery_value_tab[j];
                    battery_value_tab[j] = battery_value_tab[j - 1];
                    battery_value_tab[j - 1] = ad_min;
                }
            }
        }
        bat_volt_tmp = 0;
        for (k = BAT_CUT_COUNTS; k < (BAT_AVE_COUNTS - BAT_CUT_COUNTS); k++) {
            bat_volt_tmp = battery_value_tab[k] + bat_volt_tmp;
        }
        bat_volt_tmp = (bat_volt_tmp / (BAT_AVE_COUNTS - (BAT_CUT_COUNTS * 2))); //均值
        //更新电池电量
        cur_bat_volt = bat_volt_tmp;

        /* printf("V:%d\n",cur_bat_volt); */
        //低电与退出低电检测
        if (sys_info.lowpower_flag) {
            if (cur_bat_volt > LOWER_POWER_EXIT_VOLT) {
                lowpower_cnt++;
                if (lowpower_cnt >= STAT_LOW_POWER_MAX) { //n次后，注意时间尺度
                    sys_info.lowpower_flag = 0;
                    app_chargebox_event_to_user(CHGBOX_EVENT_EXIT_LOWPOWER);
                    log_info("Exit cbox lowpower\n");
                    lowpower_cnt = 0;
                }
            } else {
                lowpower_cnt = 0; //去抖
            }
        } else {
            if (cur_bat_volt <= LOWER_POWER_ENTER_VOLT) {
                lowpower_cnt++;
                if (lowpower_cnt >= STAT_LOW_POWER_MAX) {
                    sys_info.lowpower_flag = 1;
                    app_chargebox_event_to_user(CHGBOX_EVENT_ENTER_LOWPOWER);
                    lowpower_cnt = 0;
                    log_info("Eenter cbox lowpower\n");
                }
            } else {
                lowpower_cnt = 0; //去抖
            }
        }

        /* printf("L:%d\n",sys_info.lowpower_flag); */
        bat_cnt = 0;//清0，进行下一次统计
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    usb上下线检测
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
static void ic_self_usb_online_det(void)
{
    static u16 usb_det_cnt = 0;
    u8 io_level;
    io_level = gpio_read(USB_ONLE_DET_IO);
    if (sys_info.status[USB_DET] == STATUS_ONLINE) {
        if (((USB_DET_ONLINE_LEVEL == LOW_LEVEL) && io_level) || ((USB_DET_ONLINE_LEVEL == HIGHT_LEVEL) && (!io_level))) {
            usb_det_cnt++;
            sys_info.life_cnt = 0;//状态变化时，清除休眠计时
            if (usb_det_cnt >= USB_DET_OFFLINE_TIME) {
                usb_det_cnt = 0;
                sys_info.localfull = 0; ///充满标志清0
                sys_info.status[USB_DET] = STATUS_OFFLINE;
                app_chargebox_event_to_user(CHGBOX_EVENT_USB_OUT);
            }
        } else {
            usb_det_cnt = 0;//防抖
        }
    } else {
        if (((USB_DET_ONLINE_LEVEL == LOW_LEVEL) && (!io_level)) || ((USB_DET_ONLINE_LEVEL == HIGHT_LEVEL) && io_level)) {
            usb_det_cnt++;
            if (usb_det_cnt >= USB_DET_ONLINE_TIME) {
                usb_det_cnt = 0;
                sys_info.status[USB_DET] = STATUS_ONLINE;
                app_chargebox_event_to_user(CHGBOX_EVENT_USB_IN);
            }
        } else {
            usb_det_cnt = 0;//防抖
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    LDO升压成功  检测
   @param    无
   @return   无
   @note     用于检测ldo升压是否正常
*/
/*------------------------------------------------------------------------------------*/
static void ic_self_usb_charge_full_det(void)
{
    u8 io_level = 0;
    //如果在充电,判断是否充满
    if ((sys_info.status[USB_DET] == STATUS_ONLINE) && (!sys_info.localfull)) {
        io_level = gpio_read(CHARGE_FULL_DET_IO);
        if (((CHARGE_FULL_LEVEL == LOW_LEVEL) && (!io_level)) || ((CHARGE_FULL_LEVEL == HIGHT_LEVEL) && io_level)) {
            sys_info.check_cnt++;
            if (sys_info.check_cnt > STAT_FULL_POWER_MAX) {
                sys_info.localfull = 1; ///注意清0动作在usb拔出动作那里
                app_chargebox_event_to_user(CHGBOX_EVENT_LOCAL_FULL);
                log_info("Usb charge is Full\n");
            }
        } else {
            sys_info.check_cnt = 0;
        }
    } else {
        sys_info.check_cnt = 0;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    LDO升压成功  检测
   @param    无
   @return   无
   @note     用于检测ldo升压是否正常
*/
/*------------------------------------------------------------------------------------*/
void ldo_succ_det(void)
{
    static u16 ldo_detect_cnt = 0;
    /* u16 tmp; */
    /* tmp = adc_get_voltage(CHG_LDO_DET_AD_CH); */
    /* printf("A:%d\n",tmp); */
    if (sys_info.status[LDO_DET] == STATUS_ONLINE) {
        if (adc_get_voltage(CHG_LDO_DET_AD_CH) * 2 < LDO_SUCC_VOLT) { //注意分压
            ldo_detect_cnt++;
            sys_info.life_cnt = 0;
            if (ldo_detect_cnt >= LDO_OFF_TIMES) {
                ldo_detect_cnt = 0;
                sys_info.status[LDO_DET] = STATUS_OFFLINE;
                log_info("LDO OFF\n");
            }
        } else {
            ldo_detect_cnt = 0;
        }
    } else {
        if (adc_get_voltage(CHG_LDO_DET_AD_CH) * 2 >= LDO_SUCC_VOLT) {
            ldo_detect_cnt++;
            sys_info.life_cnt = 0;
            if (ldo_detect_cnt >= LDO_SUCC_TIMES) {
                ldo_detect_cnt = 0;
                sys_info.status[LDO_DET] = STATUS_ONLINE;
                log_info("LDO ON\n");
            }
        } else {
            ldo_detect_cnt = 0;
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电ic检测回调
   @param    无
   @return   无
   @note     usb上下线检测、满电检测、5v升压正常 检测、电池电量检测
             该函数由外面的进行回调，注意多长时间调用一次
*/
/*------------------------------------------------------------------------------------*/
static void ic_self_detect(void)
{
    ///检测usb是否在线
    if (sys_info.init_ok) {
        ic_self_usb_online_det();
    }

    ///检测usb是否充满电
    if (sys_info.init_ok) {
        ic_self_usb_charge_full_det();
    }

    ///检测耳机电量(改放到CHG_STATUS_CHARGE里做)
    /* if (sys_info.init_ok) { */
    /*     app_chargebox_ear_full_det(); */
    /* } */

    //5v升压检测
    if (sys_info.init_ok) {
        ldo_succ_det();
    }

    ///使用芯片的电压,低电与低电退出
    if (sys_info.init_ok) {
        ic_self_power_det();
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    耳机充电升压控制
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
static void ic_self_boost_ctrl(u8 en)
{
    gpio_direction_output(BOOST_CTRL_IO, en);
    log_debug("boost_ctrl:%s\n", en ? "open" : "close");
}

/*------------------------------------------------------------------------------------*/
/**@brief    耳机充电开关控制
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
static void ic_self_vol_ctrl(u8 en)
{
    if (sys_info.init_ok) {
        gpio_direction_output(VOL_CTRL_IO, en);
        log_debug("vol_ctrl:%s\n", en ? "open" : "close");
    }
}
static void ic_self_vor_ctrl(u8 en)
{
    if (sys_info.init_ok) {
        gpio_direction_output(VOR_CTRL_IO, en);
        log_debug("vor_ctrl:%s\n", en ? "open" : "close");
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    获取当前仓电池电量
   @param    无
   @return   电量
   @note
*/
/*------------------------------------------------------------------------------------*/
static u16 ic_self_get_vbat(void)
{
    if (cur_bat_volt > 4200) {
        return 4200;
    } else {
        return cur_bat_volt;
    }
}

extern void delay_2ms(int cnt);
extern u8 get_cur_total_ad_ch(void);
extern void clr_wdt(void);
/*------------------------------------------------------------------------------------*/
/**@brief    充电初始化
   @param    无
   @return   初始化是否成功
   @note     初始化电池电量检测、5v升压、升压检测、充电开关控制口、usb在线检测、满电检测
             的IO初始化。检测usb是否在线、电池的初始电量情况
*/
/*------------------------------------------------------------------------------------*/
static u8 ic_self_init(void)
{

    u16 status_init_cnt = 0;
    u16 shut_down_cnt = 0;
    u16 i = 0;
    u8 io_level;
    u16 val;
    u32 volt_sum = 0;
    u8 total_ad_ch = 0;

    sys_info.localfull = 0;
    sys_info.status[USB_DET] = STATUS_OFFLINE;

    ///电池电量检测初始化
    adc_add_sample_ch(BAT_DET_AD_CH);

    gpio_set_die(BAT_DET_IO, 0);
    gpio_set_direction(BAT_DET_IO, 1);
    gpio_set_pull_down(BAT_DET_IO, 0); //关下拉
    gpio_set_pull_up(BAT_DET_IO, 0);   //关上拉

    //使能IO输出低电平 ,5V升压初始化
    gpio_set_die(BOOST_CTRL_IO, 0);
    gpio_set_pull_down(BOOST_CTRL_IO, 0);
    gpio_set_pull_up(BOOST_CTRL_IO, 0);
    gpio_direction_output(BOOST_CTRL_IO, 0);

    ///5v升压检测初始化
    adc_add_sample_ch(CHG_LDO_DET_AD_CH);          //注意：初始化AD_KEY之前，先初始化ADC

    gpio_set_die(CHG_LDO_DET_IO, 0);
    gpio_set_direction(CHG_LDO_DET_IO, 1);
    gpio_set_pull_down(CHG_LDO_DET_IO, 0);//关下拉
    gpio_set_pull_up(CHG_LDO_DET_IO, 0);  //关上拉

    //VOL/VOR_CTRL  充电的时候要拉高
    gpio_set_die(VOL_CTRL_IO, 0);
    gpio_set_pull_down(VOL_CTRL_IO, 0);
    gpio_set_pull_up(VOL_CTRL_IO, 0);
    gpio_direction_output(VOL_CTRL_IO, 0);

    gpio_set_die(VOR_CTRL_IO, 0);
    gpio_set_pull_down(VOR_CTRL_IO, 0);
    gpio_set_pull_up(VOR_CTRL_IO, 0);
    gpio_direction_output(VOR_CTRL_IO, 0);

    ///usb在线检测IO初始化
    gpio_set_direction(USB_ONLE_DET_IO, 1);
    gpio_set_die(USB_ONLE_DET_IO, 1);
    gpio_set_pull_down(USB_ONLE_DET_IO, 0);
    gpio_set_pull_up(USB_ONLE_DET_IO, 0);

    ///满电检测IO初始化
    gpio_set_direction(CHARGE_FULL_DET_IO, 1);
    gpio_set_die(CHARGE_FULL_DET_IO, 1);
    gpio_set_pull_down(CHARGE_FULL_DET_IO, 0);
    gpio_set_pull_up(CHARGE_FULL_DET_IO, 1);

    total_ad_ch = get_cur_total_ad_ch();//ad总通道数

    sys_info.init_ok = 1;//其他充电ic需要通信确认

    if (sys_info.init_ok) {

        ///检测usb是否在线
        for (i = 0; i < 20; i++) {
            io_level = gpio_read(USB_ONLE_DET_IO);
            if (((USB_DET_ONLINE_LEVEL == LOW_LEVEL) && (!io_level)) || ((USB_DET_ONLINE_LEVEL == HIGHT_LEVEL) && io_level)) {
                status_init_cnt++;
            } else {
                break;
            }
            asm("nop");
            asm("nop");
            asm("nop");
        }
        //usb在线
        if (status_init_cnt == 20) {
            sys_info.status[USB_DET] = STATUS_ONLINE;
        }

        ///usb在线时是否充满
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            status_init_cnt = 0;
            for (i = 0; i < 20; i++) {
                io_level = gpio_read(CHARGE_FULL_DET_IO);
                if (((CHARGE_FULL_LEVEL == LOW_LEVEL) && (!io_level)) || ((CHARGE_FULL_LEVEL == HIGHT_LEVEL) && io_level)) {
                    status_init_cnt++;
                } else {
                    break;
                }
                asm("nop");
                asm("nop");
                asm("nop");
            }
            //usb 在线且满电
            if (status_init_cnt == 20) {
                sys_info.localfull = 1;
            }
        }

        //检测是否处于低电量状态
        status_init_cnt = 0;

        for (i = 0; i < 5; i++) {
            clr_wdt();
            delay_2ms(total_ad_ch + 2);//等待所有通道采完
            val = adc_get_voltage(BAT_DET_AD_CH) * 3; //注意电路分压
            volt_sum += val;
            printf("Volt:%d", val);
            if (val > LOWER_POWER_ENTER_VOLT) {         //是否大于低电电压
                status_init_cnt++;
            } else if (val < POWER_ON_SHUTDOWN_VOLT) {  //是否小于关机电压
                shut_down_cnt++;
            }
        }

        cur_bat_volt = volt_sum / 5; //电池电压初始值

        if (status_init_cnt != 5) { //低电量模式
            sys_info.lowpower_flag = 1;
        }
        if (shut_down_cnt > 2) {
            ///直接关机
            sys_info.init_ok = 0;
        }


        log_info("usb status: %d\n", sys_info.status[USB_DET]);
        log_info("localfull : %d\n", sys_info.localfull);
        log_info("lowpower_flag: %d\n", sys_info.lowpower_flag);
        log_info("sys_info.init_ok: %d\n", sys_info.init_ok);
        log_info("power_on volt: %d\n", cur_bat_volt);
    }
    return 0;
}

REGISTER_CHARGE_IC(ic_self) = {
    .logo = "IC_SELF",
    .init  = ic_self_init,
    .detect = ic_self_detect,
    .boost_ctrl = ic_self_boost_ctrl,
    .vol_ctrl = ic_self_vol_ctrl,
    .vor_ctrl = ic_self_vor_ctrl,
    .get_vbat = ic_self_get_vbat,
};

#endif
