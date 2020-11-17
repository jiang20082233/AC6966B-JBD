#include "music/music.h"
#include "music_player.h"
#include "system/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "app_main.h"
#include "clock_cfg.h"
#include "app_action.h"
#include "breakpoint.h"
#include "key_event_deal.h"
#include "tone_player.h"
#include "dev_multiplex_api.h"
#include "ui/ui_api.h"
#include "bt/bt.h"
#include "smartbox/smartbox.h"
#include "ui_manage.h"
#include "audio_dec.h"
#include "common/dev_status.h"


#include "user_fun_cfg.h"

/*************************************************************
   此文件函数主要是music模式按键处理和事件处理

	void app_music_task()
   music模式主函数

	static int music_sys_event_handler(struct sys_event *event)
   music模式系统事件所有处理入口

	static void music_task_close(void)
	music模式退出

**************************************************************/

#define LOG_TAG_CONST       APP_MUSIC
#define LOG_TAG             "[APP_MUSIC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_APP_MUSIC_EN
#undef log_i
#define log_i printf
///模式参数结构体
struct __music_task_parm {
    u8 type;
    int val;
};

///music模式控制结构体
struct __music {
    struct __music_task_parm task_parm;
    u16 file_err_counter;//错误文件统计
    u8 file_play_direct;//0:下一曲， 1：上一曲
    u8 scandisk_break;//扫描设备打断标志

};
struct __music music_hdl;
#define __this (&music_hdl)

static struct __breakpoint *breakpoint = NULL;

static void music_player_play_start(void);

//*----------------------------------------------------------------------------*/
/**@brief    music 解码成功回调
   @param    priv:私有参数， parm:暂时未用
   @return
   @note	 此处可以做一些用户操作， 如断点保存， 显示， 获取播放信息等
*/
/*----------------------------------------------------------------------------*/
static void music_player_play_success(void *priv, int parm)
{
    char *logo = music_player_get_dev_cur();
    log_i("\n\n----------------music_player_play_success----------------------\n");
    log_i("cur dev = %s\n", logo);
    log_i("total dev = %d\n", dev_manager_get_total(1));
    log_i("cur filenum = %d\n", music_player_get_file_cur());
    log_i("totol filenum = %d\n", music_player_get_file_total());
    log_i("totol time = %d\n", music_player_get_dec_total_time());
    log_i("\n");

    ///save breakpoint, 只保存文件信息
    if (music_player_get_playing_breakpoint(breakpoint, 0) == true) {
        breakpoint_vm_write(breakpoint, logo);
    }
    ///show ui
    UI_SHOW_MENU(MENU_FILENUM, 1000, music_player_get_file_cur(), NULL);
    ///smartbox info update
    SMARTBOX_UPDATE(MUSIC_FUNCTION_MASK,
                    BIT(MUSIC_INFO_ATTR_STATUS) | BIT(MUSIC_INFO_ATTR_FILE_NAME) | BIT(MUSIC_INFO_ATTR_FILE_PLAY_MODE));

}
//*----------------------------------------------------------------------------*/
/**@brief    music 解码结束回调处理
   @param
   @return
   @note	此处统一将错误通过消息的方式发出， 在key msg中统一响应
*/
/*----------------------------------------------------------------------------*/
static void music_player_play_end(void *priv, int parm)
{
    log_i("music_player_play_end\n");
    ///这里推出消息， 目的是在music主流程switch case统一入口
    app_task_put_key_msg(KEY_MUSIC_PLAYER_END, parm);
}
//*----------------------------------------------------------------------------*/
/**@brief    music 播放器扫盘打断接口
   @param
   @return  1:打断当前扫描，0:正常扫描
   @note	设备扫描耗时长的情况下， 此接口提供打断机制
*/
/*----------------------------------------------------------------------------*/
static int music_player_scandisk_break(void)
{
    ///注意：
    ///需要break fsn的事件， 请在这里拦截,
    ///需要结合MUSIC_PLAYER_ERR_FSCAN错误，做相应的处理
    int msg[32] = {0};
    struct sys_event *event = NULL;
    char *logo = NULL;
    char *evt_logo = NULL;
    app_task_get_msg(msg, ARRAY_SIZE(msg), 0);
    switch (msg[0]) {
    case APP_MSG_SYS_EVENT:
        event = (struct sys_event *)(&msg[1]);
        switch (event->type) {
        case SYS_DEVICE_EVENT:
            switch ((u32)event->arg) {
            case DRIVER_EVENT_FROM_SD0:
            case DRIVER_EVENT_FROM_SD1:
            case DRIVER_EVENT_FROM_SD2:
                evt_logo = (char *)event->u.dev.value;
            case DEVICE_EVENT_FROM_OTG:
                if ((u32)event->arg == DEVICE_EVENT_FROM_OTG) {
                    evt_logo = (char *)"udisk0";
                }
                ///设备上下线底层推出的设备逻辑盘符是跟跟音乐设备一致的（音乐/录音设备, 详细看接口注释）
                int str_len   = 0;
                logo = music_player_get_phy_dev(&str_len);
                ///响应设备插拔打断
                if (event->u.dev.event == DEVICE_EVENT_OUT) {
                    log_i("__func__ = %s logo=%s evt_logo=%s %d\n", __FUNCTION__, logo, evt_logo, str_len);
                    if (logo && (0 == memcmp(logo, evt_logo, str_len))) {
                        ///相同的设备才响应
                        __this->scandisk_break = 1;
                    }
                } else {
                    ///响应新设备上线
                    __this->scandisk_break = 1;
                }
                if (__this->scandisk_break == 0) {
                    log_i("__func__ = %s DEVICE_EVENT_OUT TODO\n", __FUNCTION__);
                    dev_status_event_filter(event);
                    log_i("__func__ = %s DEVICE_EVENT_OUT OK\n", __FUNCTION__);
                }
                break;
            }
            break;
        case SYS_BT_EVENT:
            if (bt_background_event_handler_filter(event)) {
                __this->scandisk_break = 1;
            }
            break;
        case SYS_KEY_EVENT:
            switch (event->u.key.event) {
            case KEY_CHANGE_MODE:
                ///响应切换模式事件
                __this->scandisk_break = 1;
                break;
            }
            break;
        }
        break;
    }
    if (__this->scandisk_break) {
        ///查询到需要打断的事件， 返回1， 并且重新推送一次该事件,跑主循环处理流程
        sys_event_notify(event);
        printf("scandisk_break!!!!!!\n");
        return 1;
    } else {
        return 0;
    }
}

static const struct __player_cb music_player_callback = {
    .start 		= music_player_play_success,
    .end   		= music_player_play_end,
    .fsn_break  = music_player_scandisk_break,
};
//*----------------------------------------------------------------------------*/
/**@brief    music 模式提示音播放结束处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void  music_tone_play_end_callback(void *priv, int flag)
{
    u32 index = (u32)priv;

    if (APP_MUSIC_TASK != app_get_curr_task()) {
        log_error("tone callback task out \n");
        return;
    }

    switch (index) {
    case IDEX_TONE_MUSIC:
        ///提示音播放结束， 启动播放器播放
        music_player_play_start();
        break;
    default:
        break;
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    music 模式解码错误处理
   @param    err:错误码，详细错误码描述请看MUSIC_PLAYER错误码表枚举
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void music_player_err_deal(int err)
{
    u16 msg = KEY_NULL;
    char *logo = NULL;
    if (err != MUSIC_PLAYER_ERR_NULL && err != MUSIC_PLAYER_ERR_DECODE_FAIL) {
        __this->file_err_counter = 0;///清除错误文件累计
    }

    if (err != MUSIC_PLAYER_ERR_NULL && err != MUSIC_PLAYER_SUCC) {
        log_e("music player err = %d\n", err);
    }

    switch (err) {
    case MUSIC_PLAYER_SUCC:
        __this->file_err_counter = 0;
        break;
    case MUSIC_PLAYER_ERR_NULL:
        break;
    case MUSIC_PLAYER_ERR_POINT:
    case MUSIC_PLAYER_ERR_NO_RAM:
        msg = KEY_MUSIC_PLAYER_QUIT;//退出音乐模式
        break;
    case MUSIC_PLAYER_ERR_DECODE_FAIL:
        if (__this->file_err_counter >= music_player_get_file_total()) {
            __this->file_err_counter = 0;
            dev_manager_set_valid_by_logo(music_player_get_dev_cur(), 0);///将设备设置为无效设备
            if (dev_manager_get_total(1) == 0) {//参数为1 ：获取所有有效设备  参数0：获取所有设备
                msg = KEY_MUSIC_PLAYER_QUIT;//没有设备了，退出音乐模式
            } else {
                msg = KEY_MUSIC_CHANGE_DEV;///所有文件都是错误的， 切换到下一个设备
            }
        } else {
            __this->file_err_counter ++;
            if (__this->file_play_direct == 0) {
                msg = KEY_MUSIC_NEXT;//播放下一曲
            } else {
                msg = KEY_MUSIC_PREV;//播放上一曲
            }
        }
        break;
    case MUSIC_PLAYER_ERR_DEV_NOFOUND:
        log_e("MUSIC_PLAYER_ERR_DEV_NOFOUND \n");
        if (dev_manager_get_total(1) == 0) {//参数为1 ：获取所有有效设备  参数0：获取所有设备
            msg = KEY_MUSIC_PLAYER_QUIT;///没有设备在线， 退出音乐模式
        } else {
            msg = KEY_MUSIC_PLAYER_START;///没有找到指定设备， 播放之前的活动设备
        }
        break;

    case MUSIC_PLAYER_ERR_FSCAN:
        ///需要结合music_player_scandisk_break中处理的标志位处理
        if (__this->scandisk_break) {
            __this->scandisk_break = 0;
            ///此处不做任何处理， 打断的事件已经重发， 由重发事件执行后续处理
            break;
        }
    case MUSIC_PLAYER_ERR_DEV_READ:
    case MUSIC_PLAYER_ERR_DEV_OFFLINE:
        log_e("MUSIC_PLAYER_ERR_DEV_OFFLINE \n");
        logo = music_player_get_dev_cur();
        if (dev_manager_online_check_by_logo(logo, 1)) {
            ///如果错误失败在线， 并且是播放过程中产生的，先记录下断点
            if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
                breakpoint_vm_write(breakpoint, logo);
            }
            if (err == MUSIC_PLAYER_ERR_FSCAN) {
                dev_manager_set_valid_by_logo(logo, 0);///将设备设置为无效设备
            } else {
                //针对读错误， 因为时间推到应用层有延时导致下一个模式判断不正常， 此处需要将设备卸载
                dev_manager_unmount(logo);
            }
        }
        if (dev_manager_get_total(1) == 0) {
            #if USER_MUSIC_TO_BT
                puts(">>> user goto bt mode\n");
                if(APP_MUSIC_TASK == app_get_curr_task()){
                    msg = USER_MSG_TO_BT_MODE;
                }
                
                break;
            #endif
            msg = KEY_MUSIC_PLAYER_QUIT;///没有设备在线， 退出音乐模式
        } else {
            msg = KEY_MUSIC_CHANGE_DEV;///切换设备
        }
        break;
    case MUSIC_PLAYER_ERR_FILE_NOFOUND:
        ///查找文件有扫盘的可能，也需要结合music_player_scandisk_break中处理的标志位处理
        if (__this->scandisk_break) {
            __this->scandisk_break = 0;
            ///此处不做任何处理， 打断的事件已经重发， 由重发事件执行后续处理
            break;
        }
    case MUSIC_PLAYER_ERR_PARM:
        logo = music_player_get_dev_cur();
        if (dev_manager_online_check_by_logo(logo, 1)) {
            if (music_player_get_file_total()) {
                msg = KEY_MUSIC_PLAYER_PLAY_FIRST;///有文件,播放第一个文件
                break;
            }
        }

        if (dev_manager_get_total(1) == 0) {
            msg = KEY_MUSIC_PLAYER_QUIT;//没有设备了，退出音乐模式
        } else {
            msg = KEY_MUSIC_CHANGE_DEV;
        }
        break;
    }
    if (msg != KEY_NULL) {
        app_task_put_key_msg(msg, 0);
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    music 按键消息入口
   @param    无
   @return   1、消息已经处理，common不再处理  0、消息发送到common处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int music_key_event_opr(struct sys_event *event)
{
    int ret = true;
    int err = MUSIC_PLAYER_ERR_NULL;
    u8 vol;
    int mode ;
    char *logo = NULL;

    int msg[2];
    msg[0] = event->u.key.event;
    msg[1] = event->u.key.value;//
    static int msg_demo = 0;

    log_i("music task msg = %d\n", msg[0]);
    
    user_message_filtering(msg[0]);

    switch (msg[0]) {
    case USER_KEY_MUSIC_PLAYER_START:
        logo = dev_manager_get_logo(dev_manager_find_active(1));
        if (music_player_get_play_status() == FILE_DEC_STATUS_PLAY) {
            if (music_player_get_dev_cur() && logo) {
                ///播放的设备跟当前活动的设备是同一个设备，不处理
                if (0 == strcmp(logo, music_player_get_dev_cur())) {
                    puts("the same dev 00!!\n");
                    //break;
                }
            }
        }
        if (true == breakpoint_vm_read(breakpoint, logo)) {
            puts(">>> play ccccccc 33\n");
            err = music_player_play_by_breakpoint(logo, breakpoint);
        } else {
            puts(">>> play ccccccc 44\n");
            err = music_player_play_first_file(logo);
        }    
        break;
    case KEY_MUSIC_PLAYER_START:
        log_i("KEY_MUSIC_PLAYER_START !!\n");
        ///断点播放活动设备
        logo = dev_manager_get_logo(dev_manager_find_active(1));
        if (music_player_get_play_status() == FILE_DEC_STATUS_PLAY) {
            if (music_player_get_dev_cur() && logo) {
                ///播放的设备跟当前活动的设备是同一个设备，不处理
                if (0 == strcmp(logo, music_player_get_dev_cur())) {
                    log_w("the same dev!!\n");
                    break;
                }
            }
        }
        if (true == breakpoint_vm_read(breakpoint, logo)) {
            err = music_player_play_by_breakpoint(logo, breakpoint);
        } else {
            err = music_player_play_first_file(logo);
        }
        break;
    ///播放器退出处理
    case KEY_MUSIC_PLAYER_QUIT:
        log_i("KEY_MUSIC_PLAYER_QUIT !!\n");
        app_task_switch_next();
        break;
    ///结束消息处理
    case KEY_MUSIC_PLAYER_END:
        log_i("KEY_MUSIC_PLAYER_END\n");
        err = music_player_end_deal(msg[1]);
        break;
    ///播放执行类消息
    case  KEY_MUSIC_PP:
        log_i("KEY_MUSIC_PP\n");
        err = music_player_pp();
        if (music_player_get_play_status() == FILE_DEC_STATUS_PLAY) {
            ui_update_status(STATUS_MUSIC_PLAY);
        } else {
            ui_update_status(STATUS_MUSIC_PAUSE);
        }
        break;
    case KEY_MUSIC_PLAYER_AUTO_NEXT:
        log_i("KEY_MUSIC_PLAYER_AUTO_NEXT\n");
        err = music_player_play_auto_next();
        break;
    case KEY_MUSIC_PLAYER_PLAY_FIRST:
        log_i("KEY_MUSIC_PLAYER_PLAY_FIRST\n");
        err = music_player_play_first_file(NULL);
        break;
    case  KEY_MUSIC_PREV:
        log_i("KEY_MUSIC_PREV\n");
        __this->file_play_direct = 1;
        err = music_player_play_prev();
        break;
    case  KEY_MUSIC_NEXT:
        log_i("KEY_MUSIC_NEXT\n");
        __this->file_play_direct = 0;
        err = music_player_play_next();
        break;
    case KEY_MUSIC_PLAYE_PREV_FOLDER:
        log_i("KEY_MUSIC_PLAYE_PREV_FOLDER\n");
        err = music_player_play_folder_prev();
        break;
    case KEY_MUSIC_PLAYE_NEXT_FOLDER:
        log_i("KEY_MUSIC_PLAYE_NEXT_FOLDER\n");
        err = music_player_play_folder_next();
        printf(">>>>>>>>>> cur folder number %d\n",music_player_get_dir_cur());
        UI_SHOW_MENU(MENU_MUSIC_FOLDER, 1000, music_player_get_dir_cur(), NULL);
        
        break;
    case  KEY_MUSIC_CHANGE_DEV:
        log_i("KEY_MUSIC_CHANGE_DEV\n");
        ///通过music_player_get_dev_next(),可以预判下一个设备是什么， 可以自行实现设备提示音等操作
        if (music_player_get_dev_next() == NULL) { ///找不到下一个设备，不响应设备切换
            break;
        }
        ///切换设备前先保存一下上一个设备的断点信息,包括文件和解码信息
        if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
            breakpoint_vm_write(breakpoint, music_player_get_dev_cur());
        }
        if (true == breakpoint_vm_read(breakpoint, music_player_get_dev_next())) {
            err = music_player_play_devcie_next(breakpoint);
        } else {
            err = music_player_play_devcie_next(NULL);
        }
        break;
    case KEY_MUSIC_PLAYE_REC_FOLDER_SWITCH:
        log_i("KEY_MUSIC_PLAYE_REC_FOLDER_SWITCH\n");
#if (TCFG_RECORD_FOLDER_DEV_ENABLE)
        ///尝试保存断点
        if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
            breakpoint_vm_write(breakpoint, music_player_get_dev_cur());
        }
        if (true == breakpoint_vm_read(breakpoint, music_player_get_cur_music_dev())) {
            err = music_player_play_record_folder(NULL, breakpoint);
        } else {
            err = music_player_play_record_folder(NULL, NULL);
        }
#endif//TCFG_RECORD_FOLDER_DEV_ENABLE
        break;
    case KEY_MUSIC_PLAYE_BY_DEV_FILENUM:
        log_i("KEY_MUSIC_PLAYE_BY_DEV_FILENUM\n");
        err = music_player_play_by_number((char *)"udisk", 1);
        break;
    case KEY_MUSIC_PLAYE_BY_DEV_SCLUST:
        log_i("KEY_MUSIC_PLAYE_BY_DEV_SCLUST\n");
        logo = dev_manager_get_logo(dev_manager_find_active(1));
        err = music_player_play_by_sclust(logo, msg[1]);
        break;
    case KEY_MUSIC_PLAYE_BY_DEV_PATH:
        log_i("KEY_MUSIC_PLAYE_BY_DEV_PATH\n");
        err = music_player_play_by_path((char *)"udisk", "/sin.wav");///this is a demo
        break;

    ///非播放执行类消息
    case KEY_MUSIC_FF:
        log_i("KEY_MUSIC_FF\n");
        music_player_ff(3);
        break;
    case KEY_MUSIC_FR:
        log_i("KEY_MUSIC_FR\n");
        music_player_fr(3);
        break;
    case KEY_MUSIC_CHANGE_REPEAT:
        log_i("KEY_MUSIC_CHANGE_REPEAT\n");
        mode = music_player_change_repeat_mode();
        if (mode > 0) {
            UI_SHOW_MENU(MENU_MUSIC_REPEATMODE, 1000, mode, NULL);
        }
        break;
    case KEY_MUSIC_DELETE_FILE:
        log_i("KEY_MUSIC_DELETE_FILE\n");
        err = music_player_delete_playing_file();
        break;
    #ifdef USER_USB_OR_SD    
    case KEY_CHANGE_MODE:
        puts("music USER_USB_OR_SD\n");
        if(dev_manager_get_total(1)>1){
            puts("music dev total\n");
            if (music_player_get_dev_cur() && USER_USB_OR_SD) {
                ///播放的设备与优先级高设备是同一个设备
                puts("music USER_USB_OR_SD\n");
                if (0 == strcmp(USER_USB_OR_SD, music_player_get_dev_cur())) {
                    puts("music USER_USB_OR_SD 0\n");
                    app_task_put_key_msg(KEY_MUSIC_CHANGE_DEV, 0);
                    break;
                }
            }
        }
        ret = false;
        break;
    #endif
    case USER_MSG_TO_BT_MODE:
        puts(">>>> user go to bt mode\n");
        // extern int user_app_goto_bt(void);
        user_app_goto_bt();
        break;
    case KEY_IR_NUM_0:
    case KEY_IR_NUM_1:
    case KEY_IR_NUM_2:
    case KEY_IR_NUM_3:
    case KEY_IR_NUM_4:
    case KEY_IR_NUM_5:
    case KEY_IR_NUM_6:
    case KEY_IR_NUM_7:
    case KEY_IR_NUM_8:
    case KEY_IR_NUM_9:
        printf("ir key num %d\n",msg[0]-KEY_IR_NUM_0);
        user_music_set_file_number(msg[0]-KEY_IR_NUM_0);
        break;        
    default:
        ret = false;
        break;
    }


    ///错误处理
    music_player_err_deal(err);
    ///smartbox info update
    SMARTBOX_UPDATE(MUSIC_FUNCTION_MASK,
                    BIT(MUSIC_INFO_ATTR_STATUS) | BIT(MUSIC_INFO_ATTR_FILE_PLAY_MODE));

    return ret;
}


//*----------------------------------------------------------------------------*/
/**@brief    music 设备事件响应接口
   @param    无
   @return   1、当前消息已经处理，comomon不再做处理 0、common统一处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int music_sys_event_handler(struct sys_event *event)
{
    int err = 0;
    char *logo = NULL;
    char *evt_logo = NULL;
    switch (event->type) {
    case SYS_KEY_EVENT:
        return music_key_event_opr(event);
    case SYS_DEVICE_EVENT:
        switch ((u32)event->arg) {
        case DRIVER_EVENT_FROM_SD0:
        case DRIVER_EVENT_FROM_SD1:
        case DRIVER_EVENT_FROM_SD2:
            evt_logo = (char *)event->u.dev.value;
        case DEVICE_EVENT_FROM_OTG:
            if ((u32)event->arg == DEVICE_EVENT_FROM_OTG) {
                evt_logo = (char *)"udisk0";
            }
            int str_len   = 0;
            logo = music_player_get_phy_dev(&str_len);
            log_i("evt_logo =%s, logo = %s len =%d\n", evt_logo, logo, str_len);
            if (event->u.dev.event == DEVICE_EVENT_OUT) {
                if (logo == NULL) {
                    break;
                }
                if (logo && (0 == memcmp(logo, evt_logo, str_len))) {
                    ///相同的设备才响应
                    if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
                        breakpoint_vm_write(breakpoint, logo);
                    }
                    ///停止解码,防止设备掉线后还继续使用
                    music_player_stop(1);
                    ///重新选择活动设备播放
                    app_task_put_key_msg(KEY_MUSIC_PLAYER_START, 0);//卸载了设备再执行
                    log_i("KEY_MUSIC_PLAYER_START AFTER UMOUNT\n");
                }
            } else {
                ///新设备上线, 先记录当前设备断点， 然后播放活动设备
                if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
                    breakpoint_vm_write(breakpoint, logo);
                }
                ///停止解码，播放新活动设备
                music_player_stop(1);
                app_task_put_key_msg(KEY_MUSIC_PLAYER_START, 0);
                log_i("KEY_MUSIC_PLAYER_START AFTER MOUNT\n");
                //先挂载了设备再执行
            }
            break;
        case DEVICE_EVENT_FROM_TONE:
            printf(">>>>>>>> music tone play eve\n");
            break;
        default://switch((u32)event->arg)
            break;
        }
        break;//SYS_DEVICE_EVENT
    default://switch (event->type)
        break;
    }

    return false;
}

//*----------------------------------------------------------------------------*/
/**@brief    music 模式切换前参数设置
   @param    type:播放方式,暂时支持：
  				  MUSIC_TASK_START_BY_NORMAL：首次播放按照正常断点播放
				  MUSIC_TASK_START_BY_SCLUST：首次播放按照簇号播放
			 val:播放参数
   @return
   @note	 首次播放执行参考music_player_play_start接口
*/
/*----------------------------------------------------------------------------*/
void music_task_set_parm(u8 type, int val)
{
    __this->task_parm.type = type;
    __this->task_parm.val = val;
}
//*----------------------------------------------------------------------------*/
/**@brief    music 模式首次播放
   @param    无
   @return
   @note	 切换到音乐模式前可以通过接口music_task_set_parm设置参数,
   			 进入音乐模式后会按照对应参数播放
*/
/*----------------------------------------------------------------------------*/
static void music_player_play_start(void)
{
    switch (__this->task_parm.type) {
    case MUSIC_TASK_START_BY_NORMAL:
        log_i("MUSIC_TASK_START_BY_NORMAL\n");
        app_task_put_key_msg(KEY_MUSIC_PLAYER_START, 0);
        break;
    case MUSIC_TASK_START_BY_SCLUST:
        log_i("MUSIC_TASK_START_BY_SCLUST\n");
        app_task_put_key_msg(KEY_MUSIC_PLAYE_BY_DEV_SCLUST, __this->task_parm.val);
        break;
    default:
        log_i("other MUSIC_TASK_START!!!\n");
        break;
    }
    ///提示音播放失败，直接推送KEY_MUSIC_PLAYER_START启动播放
}
void user_music_player_play_start(void){
    music_player_play_start();
}
//*----------------------------------------------------------------------------*/
/**@brief    music 模式初始化处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void music_task_start()
{
    printf("music task start!!!!!\n");
    ui_update_status(STATUS_MUSIC_MODE);

    ///显示初始化
    UI_SHOW_WINDOW(ID_WINDOW_MUSIC);
    UI_SHOW_MENU(MENU_WAIT, 0, 0, NULL);
    ///时钟初始化
    clock_idle(MUSIC_IDLE_CLOCK);
    ///按键使能
    sys_key_event_enable();
    ///播放器初始化
    struct __player_parm parm = {0};
    parm.cb = (struct __player_cb *)&music_player_callback;
    music_player_creat(NULL, &parm);
    ///获取断点句柄， 后面所有断点读/写都需要用到
    breakpoint = breakpoint_handle_creat();
    ///初始化一些参数
    __this->file_err_counter = 0;
    __this->file_play_direct = 0;
    __this->scandisk_break = 0;
}
//*----------------------------------------------------------------------------*/
/**@brief    music 模式退出处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void music_task_close()
{
    UI_HIDE_CURR_WINDOW();
    tone_play_stop_by_path(tone_table[IDEX_TONE_MUSIC]);//停止播放提示音
    char *logo = music_player_get_dev_cur();
    if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
        breakpoint_vm_write(breakpoint, logo);
    }
    breakpoint_handle_destroy(&breakpoint);
    music_player_destroy();
    memset(__this, 0, sizeof(struct __music));
}
//*----------------------------------------------------------------------------*/
/**@brief    music 在线检测  切换模式判断使用
   @param    无
   @return   1 设备在线 0 设备不在线
   @note
*/
/*----------------------------------------------------------------------------*/
int music_app_check(void)
{
    if (dev_manager_get_total(1)) {
        return true;
    }
    return false;
}

//*----------------------------------------------------------------------------*/
/**@brief    music 主任务
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void app_music_task()
{
    int res;
    int msg[32];
    music_task_start();

    int user_dev_tone_number = IDEX_TONE_MUSIC;
    
    #ifdef USER_USB_OR_SD
    printf(">>>>>> music dev onlie nubmer %d\n",dev_manager_get_total(1));
    if(dev_manager_get_total(1)>1){
        if((USER_USB_OR_SD == USER_DEV_USB) || (USER_USB_OR_SD == USER_DEV_SD0) || (USER_USB_OR_SD == USER_DEV_SD1)){
            // if(USER_USB_OR_SD == USER_DEV_USB){
            //     user_dev_tone_number = IDEX_TONE_USB;
            // }else{
            //     user_dev_tone_number = IDEX_TONE_SD;
            // }
            dev_manager_set_active_by_logo(/*dev_logo[0]*/USER_USB_OR_SD);
        }        
    }
    #endif

    int err =  tone_play_with_callback_by_name(tone_table[user_dev_tone_number], 1, music_tone_play_end_callback, (void *)IDEX_TONE_MUSIC);
    if (err) {
        music_player_play_start();
    }


    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);
        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (music_sys_event_handler((struct sys_event *)(&msg[1])) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));
            }
            break;
        default:
            break;
        }
        if (app_task_exitting()) {
            music_task_close();
            return;
        }
    }
}

#else

void app_music_task()
{

}

#endif



