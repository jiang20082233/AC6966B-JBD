#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_style.h"
#include "app_task.h"
#include "system/timer.h"
#include "key_event_deal.h"
#include "audio_config.h"
#include "jiffies.h"
#include "app_power_manage.h"
#include "asm/charge.h"
#include "../lyrics_api.h"
#ifndef CONFIG_MEDIA_NEW_ENABLE
#include "application/audio_eq_drc_apply.h"
#else
#include "audio_eq.h"
#endif


#if TCFG_SPI_LCD_ENABLE

#define STYLE_NAME  JL

extern int ui_hide_main(int id);
extern int ui_show_main(int id);
extern void key_ui_takeover(u8 on);
extern void power_off_deal(struct sys_event *event, u8 step);

extern bool file_dec_is_play(void);
extern bool file_dec_is_pause(void);
int music_play_get_cur_time(void);
int music_play_get_total_time(void);
int music_play_get_file_number(void);
int music_play_get_file_total_file(void);
FILE *music_play_get_cur_file(void);
char *music_play_get_cur_dev(void);


typedef struct _MUSIC_UI_VAR {
    u8 start: 1;
    u8 menu: 1;
    u8 file_name[128];
    FS_DISP_INFO fs_info;
    int timer;
    int total_time;
    int file_total;
    int cur_num;
} MUSIC_UI_VAR;


static MUSIC_UI_VAR *__this = NULL;

static int music_start(const char *type, u32 arg)
{
    int file_num;
    char *dev;

    struct unumber num;
    struct utime time_r;

    if (!__this) {
        return 0;
    }


    if (arg) {
        ui_hide(MUSIC_FILE);//不显示歌名
        ui_show(MUSIC_LYRICS);//显示歌词
    } else {
        ui_hide(MUSIC_LYRICS);//不显示歌词
        ui_show(MUSIC_FILE);//显示歌名
    }

    file_num = music_play_get_file_number();
    if (file_num != __this->cur_num) {
        num.numbs =  1;
        num.number[0] = file_num;;
        __this->cur_num = file_num;
        ui_number_update_by_id(MUSIC_CUR_NUM, &num);
    }

    file_num = music_play_get_file_total_file();
    if (file_num != __this->file_total) {
        num.numbs =  1;
        num.number[0] = file_num;
        __this->file_total = file_num;
        ui_number_update_by_id(MUSIC_TOTAL_NUM, &num);
    }


    dev = music_play_get_cur_dev();
    if (dev) {
        if (!strcmp(dev, "udisk0")) {
            ui_pic_show_image_by_id(MUSIC_DEV, 1); //显示udik
        } else {
            ui_pic_show_image_by_id(MUSIC_DEV, 2); //显示sd
        }
    }

    FILE *file =  music_play_get_cur_file();
    if (file) {
        LONG_FILE_NAME *file_name;
        LONG_FILE_NAME *dir_name;
        memset(&__this->fs_info, 0, sizeof(FS_DISP_INFO));
        fget_disp_info(file, &__this->fs_info);
        file_name = &__this->fs_info.file_name;
        dir_name = &__this->fs_info.file_name;
        extern void file_comm_change_display_name(char *tpath, LONG_FILE_NAME * disp_file_name, LONG_FILE_NAME * disp_dir_name);
        file_comm_change_display_name(__this->fs_info.tpath, file_name, dir_name);//接口有缺陷
        put_buf(file_name->lfn, file_name->lfn_cnt);


        if (file_name->lfn_cnt > 11) { //获取长文件名有效

            /* printf("%d %d\n",file_name->lfn_cnt,__LINE__); */
            /* put_buf(file_name->lfn,file_name->lfn_cnt); */
            memcpy(__this->file_name, file_name->lfn, file_name->lfn_cnt);
            ui_text_set_textw_by_id(MUSIC_FILE, __this->file_name, file_name->lfn_cnt, FONT_ENDIAN_SMALL, FONT_DEFAULT | FONT_SHOW_SCROLL);
        } else { //显示短文件名

            fget_name(file, __this->file_name, 128);
            /* printf("%d %d\n",strlen(__this->file_name),__LINE__); */
            /* memcpy(__this->file_name,__this->fs_info.tpath,strlen(__this->fs_info.tpath)); */
            ui_text_set_text_by_id(MUSIC_FILE, __this->file_name, strlen(__this->file_name), FONT_DEFAULT);
        }
    }


    return 0;
}

static const struct uimsg_handl ui_msg_handler[] = {
    { "music_start",        music_start     }, /* */
    { NULL, NULL},      /* 必须以此结尾！ */
};




static void music_check_add(void *ptr);

/************************************************************
                         音乐模式主页窗口控件
 ************************************************************/


static int music_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    static int bt_status_timer = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***music_mode_onchange***\n");
        key_ui_takeover(1);
        if (!__this) {
            __this = zalloc(sizeof(MUSIC_UI_VAR));
        }
        ui_register_msg_handler(ID_WINDOW_MUSIC, ui_msg_handler);
        /*
         * 注册APP消息响应
         */
        if (!__this->timer) {
            __this->timer = sys_timer_add(NULL, music_check_add, 500);
        }
        break;
    case ON_CHANGE_RELEASE:
        if (__this->timer) {
            sys_timer_del(__this->timer);
        }
        if (__this) {
            free(__this);
            __this = NULL;
        }
        break;
    default:
        return false;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(ID_WINDOW_MUSIC)
.onchange = music_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};




/* extern void test_browser(); */
/************************************************************
                         音乐模式布局控件按键事件
 ************************************************************/
static int music_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("music_layout_onkey %d\n", e->value);
    switch (e->value) {
    case KEY_MENU:
        ui_show(MUSIC_MENU_LAYOUT);
        break;
    case KEY_OK:
        /* test_browser(); */
        music_play_file_pp();
        break;
    case KEY_UP:
        music_play_file_prev();
        break;
    case KEY_DOWN:
        music_play_file_next();
        break;
    case KEY_VOLUME_INC:
    /* app_audio_volume_up(1); */
    /* break; */
    case KEY_VOLUME_DEC:
        /* app_audio_volume_down(1); */
        ui_show(MUSIC_VOL_LAYOUT);
        break;
    case KEY_MODE:
        ui_hide_main(ID_WINDOW_MUSIC);
        ui_show_main(ID_WINDOW_MAIN);
        break;
    case KEY_POWER_START:
    case KEY_POWER:
        power_off_deal(NULL, e->value - KEY_POWER_START);
        break;
    default:
        return false;
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(MUSIC_LAYOUT)
.onchange = NULL,
 .onkey = music_layout_onkey,
  .ontouch = NULL,
};

#if 0
static int music_file_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        /* ui_text_set_text_attrs(text, NULL, 0, FONT_ENCODE_UTF8, 0, FONT_DEFAULT); */
        break;
    case ON_CHANGE_SHOW_PROBE:
        return TRUE;
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(MUSIC_FILE)
.onchange = music_file_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
#endif

/************************************************************
                         音乐模式的定时器
 ************************************************************/
static void music_check_add(void *ptr)
{
    int sencond;
    struct utime time_r;

    if (!__this) {
        return;
    }



    if (true == file_dec_is_play()) {
        if (!__this->start) {
            ui_highlight_element_by_id(MUSIC_START);
            sencond = music_play_get_total_time();
            time_r.hour = sencond / 60 / 60;
            time_r.min = sencond / 60 % 60;
            time_r.sec = sencond % 60;
            ui_time_update_by_id(MUSIC_TOTAL_TIME, &time_r);
            __this->total_time = sencond;
            __this->start = 1;
        }
        sencond = music_play_get_total_time();
        if (sencond != __this->total_time) {
            time_r.hour = sencond / 60 / 60;
            time_r.min = sencond / 60 % 60;
            time_r.sec = sencond % 60;
            ui_time_update_by_id(MUSIC_TOTAL_TIME, &time_r);
            __this->total_time = sencond;
        }
        sencond = music_play_get_cur_time();
        printf("sec = %d \n", sencond);
        time_r.hour = sencond / 60 / 60;
        time_r.min = sencond / 60 % 60;
        time_r.sec = sencond % 60;
        ui_time_update_by_id(MUSIC_CUR_TIME, &time_r);
#if (TCFG_LRC_LYRICS_ENABLE)
        lrc_show_api(MUSIC_LYRICS, music_play_get_cur_time(), 0);
#endif

    } else if (file_dec_is_pause()) {
        if (__this->start) {
            ui_no_highlight_element_by_id(MUSIC_START);
            __this->start = 0;
        }
    } else {
        if (__this->start) {
            ui_no_highlight_element_by_id(MUSIC_START);
            __this->start = 0;
        }
    }

}



/************************************************************
                         音乐模式的一些控件初始化事件
 ************************************************************/

static int play_timer_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_time *timer = (struct ui_time *)ctr;
    int sencond = 0;
    switch (e) {
    case ON_CHANGE_INIT:

        if (timer->text.elm.id == MUSIC_CUR_TIME) {
            sencond = music_play_get_cur_time();
        } else if (timer->text.elm.id == MUSIC_TOTAL_TIME) {
            sencond = music_play_get_total_time();
        }

        timer->hour = sencond / 60 / 60;
        if (true == file_dec_is_play()) {
            timer->min = sencond / 60 % 60;
            timer->sec = sencond % 60;
        } else {
            timer->min = sencond / 60 % 60;
            timer->sec = sencond % 60;
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(MUSIC_TOTAL_TIME)
.onchange = play_timer_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(MUSIC_CUR_TIME)
.onchange = play_timer_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};







/************************************************************
                         音乐模式主菜单列表按键事件
 ************************************************************/

static int music_enter_callback(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    switch (e->value) {
    case KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
            ui_show(MUSIC_EQ_LAYOUT);
            break;
        case 1:
            /* ui_show(SYS_LANGUAGE); */
            break;
        case 2:
            ui_hide(MUSIC_MENU_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}


REGISTER_UI_EVENT_HANDLER(MUSIC_MENU_LIST)
.onchange = NULL,
 .onkey = music_enter_callback,
  .ontouch = NULL,
};





/************************************************************
                         音乐模式eq菜单列表按键事件
 ************************************************************/

static int music_eq_callback(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    switch (e->value) {
    case KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
#if TCFG_EQ_ENABLE
            eq_mode_set(sel_item);
#endif
        case 6:
            ui_hide(MUSIC_EQ_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}


REGISTER_UI_EVENT_HANDLER(MUSIC_EQ_LIST)
.onchange = NULL,
 .onkey = music_eq_callback,
  .ontouch = NULL,
};

/************************************************************
                         电池控件事件
 ************************************************************/

static void battery_timer(void *priv)
{
    int  incharge = 0;//充电标志
    int id = (int)(priv);
    static u8 percent = 0;
    static u8 percent_last = 0;
    if (get_charge_online_flag()) { //充电时候图标动态效果
        if (percent > get_vbat_percent()) {
            percent = 0;
        }
        ui_battery_set_level_by_id(id, percent, incharge); //充电标志,ui可以显示不一样的图标
        percent += 20;
    } else {

        percent = get_vbat_percent();
        if (percent != percent_last) {
            ui_battery_set_level_by_id(id, percent, incharge); //充电标志,ui可以显示不一样的图标,需要工具设置
            percent_last = percent;
        }

    }
}


static int battery_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_battery *battery = (struct ui_battery *)ctr;
    static u32 timer = 0;
    int  incharge = 0;//充电标志

    switch (e) {
    case ON_CHANGE_INIT:
        ui_battery_set_level(battery, get_vbat_percent(), incharge);
        if (!timer) {
            timer = sys_timer_add((void *)battery->elm.id, battery_timer, 1 * 1000); //传入的id就是BT_BAT
        }
        break;
    case ON_CHANGE_FIRST_SHOW:
        break;
    case ON_CHANGE_RELEASE:
        if (timer) {
            sys_timer_del(timer);
            timer = 0;
        }
        break;
    default:
        return false;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(MUSIC_BAT)
.onchange = battery_onchange,
 .ontouch = NULL,
};

/************************************************************
                         音乐模式音量布局事件
 ************************************************************/

static u16 music_timer = 0;
static void music_vol_timeout(void *p)
{
    int id = (int)(p);
    if (ui_get_disp_status_by_id(id) == TRUE) {
        ui_hide(id);
    }
    music_timer = 0;
}

static int music_vol_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    struct unumber num;
    switch (e) {
    case ON_CHANGE_INIT:
        if (!music_timer) {
            music_timer  = sys_timeout_add((void *)layout->elm.id, music_vol_timeout, 3000);
        }
        break;

    case ON_CHANGE_FIRST_SHOW://布局第一次显示，可以对布局内的一些控件进行初始化
        num.numbs =  1;
        num.number[0] = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        ui_number_update_by_id(MUSIC_VOL_NUM, &num);
        break;

    case ON_CHANGE_SHOW_POST:
        //有显示动作
        break;

    case ON_CHANGE_RELEASE:
        if (music_timer) {
            sys_timeout_del(music_timer);
            music_timer = 0;
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}


static int music_vol_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("music_vol_layout_onkey %d\n", e->value);
    struct unumber num;
    u8 vol;

    if (music_timer) {
        sys_timer_modify(music_timer, 3000);
    }

    switch (e->value) {
    case KEY_MENU:
        break;
    case KEY_DOWN:
    case KEY_VOLUME_INC:
        app_audio_volume_up(1);
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(MUSIC_VOL_NUM, &num);

        break;
    case KEY_UP:
    case KEY_VOLUME_DEC:
        app_audio_volume_down(1);
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(MUSIC_VOL_NUM, &num);

        break;
    default:
        return false;
        break;
    }
    return true;
}



REGISTER_UI_EVENT_HANDLER(MUSIC_VOL_LAYOUT)
.onchange = music_vol_layout_onchange,
 .onkey = music_vol_layout_onkey,
  .ontouch = NULL,
};



#endif
