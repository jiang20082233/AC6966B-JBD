#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_style.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_power_manage.h"
#include "asm/charge.h"

#if TCFG_SPI_LCD_ENABLE

#define STYLE_NAME  JL
REGISTER_UI_STYLE(STYLE_NAME)

extern int ui_hide_main(int id);
extern int ui_show_main(int id);
extern void key_ui_takeover(u8 on);
extern void power_off_deal(struct sys_event *event, u8 step);


static const char *task_switch[4] = {
    APP_NAME_BT,
    APP_NAME_MUSIC,
    APP_NAME_RTC,
    /* APP_NAME_PC,							 */
};

static int task_switch_check(const char *name)
{
    return true;
}


static int main_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    static int id = 0;

    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***main_mode_onchange***\n");
        key_ui_takeover(1);
        /* ui_register_msg_handler(ID_WINDOW_VIDEO_REC, rec_msg_handler); */
        /*
         * 注册APP消息响应
         */
        break;
    case ON_CHANGE_RELEASE:
        /*
         * 要隐藏一下系统菜单页面，防止在系统菜单插入USB进入USB页面
         */
        break;
    default:
        return false;
    }
    return false;
}


REGISTER_UI_EVENT_HANDLER(ID_WINDOW_MAIN)
.onchange = main_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};







static int menu_control_onkey(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int sel_item = 0;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    switch (e->value) {
    case KEY_OK:
        puts("KEY_OK\n");
        sel_item = ui_grid_cur_item(grid);
        printf("sel_item = %d \n", sel_item);
        struct application *cur;

        cur = get_current_app();

        if (sel_item == 0) {
            if (cur) {
                if (!strcmp(cur->name, APP_NAME_BT)) {
                    ui_hide_main(ID_WINDOW_MAIN);
                    ui_show_main(ID_WINDOW_BT);
                    break;
                } else {
                    ui_hide_main(ID_WINDOW_MAIN);
                }
            }

            if (task_switch_check(APP_NAME_BT)) {
                app_task_switch(APP_NAME_BT, ACTION_APP_MAIN, NULL);
            }
            break;
        }

        if (sel_item == 1) {
            if (cur) {
                if (!strcmp(cur->name, APP_NAME_MUSIC)) {
                    ui_hide_main(ID_WINDOW_MAIN);
                    ui_show_main(ID_WINDOW_MUSIC);
                    break;
                } else {
                    ui_hide_main(ID_WINDOW_MAIN);
                }

            }

            if (task_switch_check(APP_NAME_MUSIC)) {
                app_task_switch(APP_NAME_MUSIC, ACTION_APP_MAIN, NULL);
            }
            break;
        }

        if (sel_item == 2) {
            if (cur) {
                if (!strcmp(cur->name, APP_NAME_RTC)) {
                    ui_hide_main(ID_WINDOW_MAIN);
                    ui_show_main(ID_WINDOW_CLOCK);
                    break;
                } else {
                    ui_hide_main(ID_WINDOW_MAIN);
                }
            }

            if (task_switch_check(APP_NAME_RTC)) {
                app_task_switch(APP_NAME_RTC, ACTION_APP_MAIN, NULL);
            }
            break;
        }



        if (sel_item == 3) {

            ui_hide_main(ID_WINDOW_MAIN);
            ui_show_main(ID_WINDOW_SYS);
            break;
        }
        if (sel_item < sizeof(task_switch) / sizeof(task_switch[0])) {
            if (task_switch_check(task_switch[sel_item])) {
                app_task_switch(task_switch[sel_item], ACTION_APP_MAIN, NULL);
            } else {

            }

        }

        break;
    case KEY_MENU:

        break;
    case KEY_DOWN:
    case KEY_UP:
        return FALSE;
    /*
     * 向后分发消息
     */

    case KEY_POWER_START:
    case KEY_POWER:
        power_off_deal(NULL, e->value - KEY_POWER_START);
        break;
    default:
        /* ui_hide(BT_CTRL_LAYOUT); */
        break;
    }

    return true;
    /*
     * 不向后分发消息
     */
}

static int menu_control_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static int sel_item_last = -1;
    int sel_item = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_SHOW_PROBE:
        /* break; */
        /* case ON_CHANGE_HIGHLIGHT: */
        /* puts("ON_CHANGE_HIGHLIGHT \n"); */
        sel_item = ui_grid_cur_item(grid);
        if (sel_item_last != sel_item) {
            printf("sel_item = %d \n", sel_item);
            /* ui_text_show_index_by_id(MENU_TEXT,sel_item); */
            sel_item_last = sel_item;
        }
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(MENU_MAIN_LIST)
.onkey = menu_control_onkey,
 .onchange = menu_control_onchange,
  .ontouch = NULL,
};



static int MENU_MAIN_LIST_common_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_SHOW_POST:
        if ((pic->elm.id == MENU_LIST_BT_PIC) && pic->elm.highlight) {
            ui_invert_element_by_id(MENU_LIST_BT);
            ui_text_show_index_by_id(MENU_TEXT, 0);
        } else if ((pic->elm.id == MENU_LIST_MUSIC_PIC) && pic->elm.highlight) {
            ui_invert_element_by_id(MENU_LIST_MUSIC);
            ui_text_show_index_by_id(MENU_TEXT, 1);
        } else if ((pic->elm.id == MENU_LIST_CLOCK_PIC) && pic->elm.highlight) {
            ui_invert_element_by_id(MENU_LIST_CLOCK);
            ui_text_show_index_by_id(MENU_TEXT, 2);
        } else if ((pic->elm.id == MENU_LIST_SET_PIC) && pic->elm.highlight) {
            ui_invert_element_by_id(MENU_LIST_SET);
            ui_text_show_index_by_id(MENU_TEXT, 3);
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(MENU_LIST_BT_PIC)
.onchange = MENU_MAIN_LIST_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(MENU_LIST_MUSIC_PIC)
.onchange = MENU_MAIN_LIST_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(MENU_LIST_CLOCK_PIC)
.onchange = MENU_MAIN_LIST_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(MENU_LIST_SET_PIC)
.onchange = MENU_MAIN_LIST_common_onchange,
};


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
REGISTER_UI_EVENT_HANDLER(MENU_BAT)
.onchange = battery_onchange,
 .ontouch = NULL,
};



#endif
