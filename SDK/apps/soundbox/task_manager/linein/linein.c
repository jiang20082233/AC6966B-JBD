
/*************************************************************
   此文件函数主要是linein模式按键处理和事件处理

	void app_linein_task()
   linein模式主函数

	static int linein_sys_event_handler(struct sys_event *event)
   linein模式系统事件所有处理入口

	static void linein_task_close(void)
	linein模式退出

**************************************************************/


#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"

#include "app_config.h"
#include "app_task.h"

#include "media/includes.h"
#include "tone_player.h"
#include "audio_dec_linein.h"

#include "asm/audio_linein.h"
#include "asm/pwm_led.h"
#include "asm/charge.h"

#include "app_charge.h"
#include "app_main.h"
#include "app_online_cfg.h"
#include "app_power_manage.h"

#include "app_chargestore.h"
#include "gSensor/gSensor_manage.h"
#include "ui_manage.h"
#include "vm.h"

#include "linein/linein_dev.h"
#include "linein/linein.h"

#include "key_event_deal.h"
#include "user_cfg.h"
#include "ui/ui_api.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "clock_cfg.h"

#include "bt.h"
#include "bt_tws.h"

#include "user_fun_cfg.h"
extern void linein_vol_set(void);

#if TCFG_APP_LINEIN_EN

#define LOG_TAG_CONST       APP_LINEIN
#define LOG_TAG             "[APP_LINEIN]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


static u8 linein_last_onoff = (u8) - 1;
static u8 linein_bt_back_flag = 0;


///*----------------------------------------------------------------------------*/
/**@brief   当前出于非linein 模式时候linein的在common消息处理
   @param    无
   @return   1:需要切换回linein 模式 0：无
   @note
*/
/*----------------------------------------------------------------------------*/
int linein_device_event_handler(struct sys_event *event)
{
    if ((u32)event->arg == DEVICE_EVENT_FROM_LINEIN) {
        if (event->u.dev.event == DEVICE_EVENT_IN) {
            log_info("linein online \n");
            return true;
        } else if (event->u.dev.event == DEVICE_EVENT_OUT) {
            log_info("linein offline \n");
        }
    }
    return false;
}


static int linein_key_event_filter_before()
{
#if TCFG_UI_ENABLE
#if (TCFG_APP_FM_EMITTER_EN == ENABLE_THIS_MOUDLE)
    if (ui_get_app_menu(GRT_CUR_MENU) == MENU_FM_SET_FRE) {
        return false;
    }
#endif
#endif
    return TRUE;
}


//*----------------------------------------------------------------------------*/
/**@brief    linein 按键消息入口
   @param    无
   @return   1、消息已经处理，不需要发送到common  0、消息发送到common处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int linein_key_msg_deal(struct sys_event *event)
{
    int ret = true;
    int err = 0;
    u8 vol;
    int key_event = event->u.key.event;
    int key_value = event->u.key.value;//

    if (linein_key_event_filter_before() == false) {
        return false;
    }

    log_info("key_event:%d \n", key_event);
#if (SMART_BOX_EN)
    extern bool smartbox_key_event_filter_before(int key_event);
    if (smartbox_key_event_filter_before(key_event)) {
        return true;
    }
#endif

    switch (key_event) {
    case KEY_LINEIN_START:
        linein_start();
        linein_last_onoff = 1;
        UI_REFLASH_WINDOW(true);//刷新主页并且支持打断显示
        break;
    case KEY_TEST_DEMO_0:
        log_info("KEY_TEST_DEMO_0 = %d \n", key_value);
        app_task_put_key_msg(KEY_TEST_DEMO_1, 5678); //test demo
        break;
    case KEY_TEST_DEMO_1:
        log_info("KEY_TEST_DEMO_1 = %d \n", key_value); //test demo
        break;

    case  KEY_MUSIC_PP:
        log_info("KEY_MUSIC_PP\n");
        /* app_task_put_key_msg(KEY_TEST_DEMO_0,1234);  //test demo// */
        linein_last_onoff = linein_volume_pp();
        linein_last_onoff ? ui_update_status(STATUS_LINEIN_PLAY)\
        : ui_update_status(STATUS_LINEIN_PAUSE);
        user_pa_ex_linein(!linein_last_onoff);
        break;
    case  KEY_VOL_UP:
        log_info(" KEY_VOL_UP\n");
        linein_key_vol_up();
        break;

    case  KEY_VOL_DOWN:
        log_info(" KEY_VOL_DOWN\n");
        linein_key_vol_down();
        break;

    default:
        ret = false;
        break;
    }
#if (SMART_BOX_EN)
    extern void smartbot_linein_msg_deal(int msg, u8 ret);
    smartbot_linein_msg_deal(key_event, ret);
#endif

    return ret;
}


//*----------------------------------------------------------------------------*/
/**@brief    音乐播放结束回调函数
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void  line_tone_play_end_callback(void *priv, int flag)
{
    u32 index = (u32)priv;

    if (APP_LINEIN_TASK != app_get_curr_task()) {
        log_error("tone callback task out \n");
        return;
    }

    switch (index) {
    case IDEX_TONE_LINEIN:
        ///提示音播放结束， 启动播放器播放
        app_task_put_key_msg(KEY_LINEIN_START, 0);
        linein_vol_set();
        break;
    default:
        break;
    }
}


//*----------------------------------------------------------------------------*/
/**@brief    linein 入口
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void linein_app_init(void)
{
    UI_SHOW_WINDOW(ID_WINDOW_LINEIN);//打开ui主页
    UI_SHOW_MENU(MENU_AUX, 0, 0, NULL);
    sys_key_event_enable();//开启按键
    ui_update_status(STATUS_LINEIN_MODE);
    clock_idle(LINEIN_IDLE_CLOCK);

}

//*----------------------------------------------------------------------------*/
/**@brief    linein 退出
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

static void linein_task_close(void)
{
    UI_HIDE_CURR_WINDOW();
    linein_stop();
    /* tone_play_stop(); */
    tone_play_stop_by_path(tone_table[IDEX_TONE_LINEIN]);
    user_pa_ex_linein(0);
}


//*----------------------------------------------------------------------------*/
/**@brief    linein 模式活跃状态 所有消息入口
   @param    无
   @return   1、当前消息已经处理，不需要发送comomon 0、当前消息不是linein处理的，发送到common统一处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int linein_sys_event_handler(struct sys_event *event)
{
    int ret = TRUE;
    printf(">>>>>>>>>>> lienin type %d arg %d %d\n",event->type,(u32)event->arg,AUDIO_DEC_EVENT_END);
    switch (event->type) {
    case SYS_KEY_EVENT:
        return linein_key_msg_deal(event);
        break;
    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_LINEIN) {
            if (event->u.dev.event == DEVICE_EVENT_IN) {
                log_info("linein online \n");
            } else if (event->u.dev.event == DEVICE_EVENT_OUT) {
                log_info("linein offline \n");
                app_task_switch_next();
            }
            return true;
        }else if(DEVICE_EVENT_FROM_TONE == (u32)event->arg){
                printf(">>>>>>>> linein tone play eve\n");
        }
        return false;
        break;
    default:
        return false;
    }
    return false;
}


//*----------------------------------------------------------------------------*/
/**@brief    linein 在线检测  切换模式判断使用
   @param    无
   @return   1 linein设备在线 0 设备不在线
   @note
*/
/*----------------------------------------------------------------------------*/
int linein_app_check(void)
{
    if (linein_is_online()) {
        return true;
    }
    return false;
}

//*----------------------------------------------------------------------------*/
/**@brief    linein 主任务
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void app_linein_task()
{
    int res;
    int err = 0;
    int msg[32];

#if TCFG_APP_BT_EN
    linein_bt_back_flag = get_bt_back_flag();//从蓝牙后台返回标志
    set_bt_back_flag(0);
#else
    linein_bt_back_flag = 0;
#endif
    log_info("linein_bt_back_flag == %d linein_last_onoff = %d\n", \
             linein_bt_back_flag, linein_last_onoff);

    linein_app_init();//初始化时钟和开启ui

    // void audio_linein_ch_combine(u8 LR_2_L, u8 LR_2_R);
    #if (defined(USER_AUXR_IN_DACL_OUT) && USER_AUXR_IN_DACL_OUT)
    audio_linein_ch_combine(1,0);
    #endif

    // audio_linein_ch_combine(1,1);
    err = tone_play_with_callback_by_name(tone_table[IDEX_TONE_LINEIN], 1,
                                          line_tone_play_end_callback, (void *)IDEX_TONE_LINEIN);
    if (err) { //
        ///提示音播放失败，直接推送KEY_MUSIC_PLAYER_START启动播放
        app_task_put_key_msg(KEY_LINEIN_START, 0);
        linein_vol_set();
    }

    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (linein_sys_event_handler((struct sys_event *)(&msg[1])) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));    //由common统一处理
            }
            break;
        default:
            break;
        }

        if (app_task_exitting()) {
            linein_task_close();
            return;
        }
    }
}

#else

int linein_app_check(void)
{
    return false;
}

void app_linein_task()
{

}

#endif


