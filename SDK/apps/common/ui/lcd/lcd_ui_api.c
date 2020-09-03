#include "includes.h"
#include "ui/ui_api.h"
#include "ui/ui.h"
#include "typedef.h"

#if (TCFG_UI_ENABLE &&TCFG_SPI_LCD_ENABLE )

#define CURR_WINDOW_MAIN (0)

#define UI_NO_ARG (-1)
#define UI_TASK_NAME "ui"

enum {
    UI_MSG_OTHER,
    UI_MSG_KEY,
    UI_MSG_TOUCH,
    UI_MSG_SHOW,
    UI_MSG_HIDE,
    UI_MSG_EXIT,
};


struct ui_server_env {
    u8 init: 1;
    u8 key_lock : 1;
    OS_SEM start_sem;
};

static struct ui_server_env __ui_display = {0};

int key_is_ui_takeover()
{
    return __ui_display.key_lock;
}

void key_ui_takeover(u8 on)
{
    __ui_display.key_lock = !!on;
}

static int post_ui_msg(int *msg, u8 len)
{
    int err = 0;
    int count = 0;
    if (!__ui_display.init) {
        return -1;
    }
__try:
        err =  os_taskq_post_type(UI_TASK_NAME, msg[0], len - 1, &msg[1]);

    if (cpu_in_irq() || cpu_irq_disabled()) {
        return err;
    }

    if (err) {

        if (!strcmp(os_current_task(), UI_TASK_NAME)) {
            return err;
        }

        if (count > 20) {
            return -1;
        }
        count++;
        os_time_dly(1);
        goto __try;
    }
    return err;
}


//=================================================================================//
// @brief: 显示主页 应用于非ui线程显示主页使用
//有针对ui线程进行处理，允许用于ui线程等同ui_show使用
//=================================================================================//
int ui_show_main(int id)
{
    static u32 mem_id[8] = {0};
    OS_SEM sem;// = zalloc(sizeof(OS_SEM));
    int msg[8];
    int i;

    if (id <= 0) {
        if (mem_id[7 + id]) {
            id = mem_id[7 + id];
        } else {
            printf("[ui_show_main] id %d is invalid.\n", id);
            return 0;
        }
    } else {
        if (mem_id[7] != id) {
            for (i = 1; i <= 7; i++) {
                mem_id[i - 1] = mem_id[i];
            }
            mem_id[7] = id;
        }
    }

    printf("[ui_show_main] id = 0x%x\n", id);

    if (!strcmp(os_current_task(), UI_TASK_NAME)) {
        ui_show(id);
        return 0;
    }
    os_sem_create(&sem, 0);
    msg[0] = UI_MSG_SHOW;
    msg[1] = id;
    msg[2] = (int)&sem;
    if (!post_ui_msg(msg, 3)) {
        os_sem_pend(&sem, 0);
    }
    return 0;
}


//=================================================================================//
// @brief: 关闭主页 应用于非ui线程关闭使用
//有针对ui线程进行处理，允许用于ui线程等同ui_hide使用
//=================================================================================//
int ui_hide_main(int id)
{
    OS_SEM sem;// = zalloc(sizeof(OS_SEM));
    int msg[8];
    if (!strcmp(os_current_task(), UI_TASK_NAME)) {
        if (CURR_WINDOW_MAIN == id) {
            id = ui_get_current_window_id();
        }
        ui_hide(id);
        return 0;
    }
    os_sem_create(&sem, 0);
    msg[0] = UI_MSG_HIDE;
    msg[1] = id;
    msg[2] = (int)&sem;
    if (!post_ui_msg(msg, 3)) {
        os_sem_pend(&sem, 0);
    }
    return 0;
}

//=================================================================================//
// @brief: 关闭当前主页 灵活使用自动判断当前主页进行关闭
//用户可以不用关心当前打开的主页,特别适用于一个任务使用了多个主页的场景
//=================================================================================//
int ui_hide_curr_main()
{
    return ui_hide_main(CURR_WINDOW_MAIN);
}



//=================================================================================//
// @brief: 应用往ui发送消息，ui主页需要注册回调函数关闭当前主页
// //消息发送方法demo： UI_MSG_POST("test1:a=%4,test2:bcd=%4,test3:efgh=%4,test4:hijkl=%4", 1,2,3,4);
// 往test1、test2、test3、test4发送了字符为a、bcd、efgh、hijkl，附带变量为1、2、3、4
// 每次可以只发一个消息，也可以不带数据例如:UI_MSG_POST("test1")
//=================================================================================//
int ui_server_msg_post(const char *msg, ...)
{
    int ret = 0;
    int argv[9];
    argv[0] = UI_MSG_OTHER;
    argv[1] = (int)msg;
    va_list argptr;
    va_start(argptr, msg);
    for (int i = 2; i < 7; i++) {
        argv[i] = va_arg(argptr, int);
    }
    va_end(argptr);
    return post_ui_msg(argv, 9);
}

//=================================================================================//
// @brief: 应用往ui发送key消息，由ui控件分配
//=================================================================================//
int ui_key_msg_post(int key)
{
    u8 count = 0;
    int msg[8];
    msg[0] = UI_MSG_KEY;
    msg[1] = key;
    return post_ui_msg(msg, 2);
}

//=================================================================================//
// @brief: 应用往ui发送触摸消息，由ui控件分配
//=================================================================================//
int ui_touch_msg_post(struct touch_event *event)
{
    int msg[8];
    int i = 0;
    msg[0] = UI_MSG_TOUCH;
    memcpy(&msg[1], event, sizeof(struct touch_event));
    return post_ui_msg(msg, sizeof(struct touch_event) / 4 + 1);
}



const char *str_substr_iter(const char *str, char delim, int *iter)
{
    const char *substr;
    ASSERT(str != NULL);
    substr = str + *iter;
    if (*substr == '\0') {
        return NULL;
    }
    for (str = substr; *str != '\0'; str++) {
        (*iter)++;
        if (*str == delim) {
            break;
        }
    }
    return substr;
}


static int do_msg_handler(const char *msg, va_list *pargptr, int (*handler)(const char *, u32))
{
    int ret = 0;
    int width;
    int step = 0;
    u32 arg = 0x5678;
    int m[16];
    char *t = (char *)&m[3];
    va_list argptr = *pargptr;

    if (*msg == '\0') {
        handler((const char *)' ', 0);
        return 0;
    }

    while (*msg && *msg != ',') {
        switch (step) {
        case 0:
            if (*msg == ':') {
                step = 1;
            }
            break;
        case 1:
            switch (*msg) {
            case '%':
                msg++;
                if (*msg >= '0' && *msg <= '9') {
                    if (*msg == '1') {
                        arg = va_arg(argptr, int) & 0xff;
                    } else if (*msg == '2') {
                        arg = va_arg(argptr, int) & 0xffff;
                    } else if (*msg == '4') {
                        arg = va_arg(argptr, int);
                    }
                } else if (*msg == 'p') {
                    arg = va_arg(argptr, int);
                }
                m[2] = arg;
                handler((char *)&m[3], m[2]);
                t = (char *)&m[3];
                break;
            case '=':
                *t = '\0';
                break;
            case ' ':
                break;
            default:
                *t++ = *msg;
                break;
            }
            break;
        }
        msg++;
    }
    *pargptr = argptr;
    return ret;
}


int ui_message_handler(int id, const char *msg, va_list argptr)
{
    int iter = 0;
    const char *str;
    const struct uimsg_handl *handler;
    struct window *window = (struct window *)ui_core_get_element_by_id(id);
    if (!window || !window->private_data) {
        return -EINVAL;
    }
    handler = (const struct uimsg_handl *)window->private_data;
    while ((str = str_substr_iter(msg, ',', &iter)) != NULL) {
        for (; handler->msg != NULL; handler++) {
            if (!memcmp(str, handler->msg, strlen(handler->msg))) {
                do_msg_handler(str + strlen(handler->msg), &argptr, handler->handler);
                break;
            }
        }
    }

    return 0;
}




extern void sys_param_init(void);


static void ui_task(void *p)
{
    int msg[32];
    int ret;
    struct element_key_event e = {0};
    struct ui_style style;
    style.file = NULL;

    ui_framework_init(p);
    ret =  ui_set_style_file(&style);
    if (ret) {
        printf("ui task fail!\n");
        return;
    }
    sys_param_init();
    __ui_display.init = 1;
    os_sem_post(&(__ui_display.start_sem));
    struct touch_event *touch;
    struct element_touch_event t;
    while (1) {
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)); //500ms_reflash
        if (ret != OS_TASKQ) {
            continue;
        }

        switch (msg[0]) { //action
        case UI_MSG_EXIT:
            os_sem_post((OS_SEM *)msg[1]);
            os_time_dly(10000);
            break;
        case UI_MSG_OTHER:
            ui_message_handler(ui_get_current_window_id(), (const char *)msg[1], (void *)&msg[2]);
            break;
        case UI_MSG_KEY:
            e.value = msg[1];
            ui_event_onkey(&e);
            break;
        case UI_MSG_TOUCH:
            touch = (struct touch_event *)&msg[1];
            t.event = touch->event;
            t.pos.x = touch->x;
            t.pos.y = touch->y;
            /* printf("event = %d %d %d \n", t.event, t.pos.x, t.pos.y); */
            ui_event_ontouch(&t);
            break;
        case UI_MSG_SHOW:
            ui_show(msg[1]);
            os_sem_post((OS_SEM *)msg[2]);
            break;
        case UI_MSG_HIDE:
            if (CURR_WINDOW_MAIN == msg[1]) {
                ui_hide(ui_get_current_window_id());
            } else {
                ui_hide(msg[1]);
            }

            os_sem_post((OS_SEM *)msg[2]);
            break;
        default:
            break;
        }
    }
}

int lcd_ui_init(void *arg)
{
    int err = 0;
    os_sem_create(&(__ui_display.start_sem), 0);
    err = task_create(ui_task, arg, UI_TASK_NAME);
    os_sem_pend(&(__ui_display.start_sem), 0);
    return err;
}

#else

int key_is_ui_takeover()
{
    return 0;
}

void key_ui_takeover(u8 on)
{

}

int ui_key_msg_post(int key)
{
    return 0;
}


int ui_touch_msg_post(struct touch_event *event)
{
    return 0;
}
#endif

