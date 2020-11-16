#ifndef _USER_PA_H_
#define _USER_PA_H_
// #include "cpu.h"
// #include "board_config.h"


typedef struct USER_PA_CTL_IO {
    u32 port_mute;//双io控制 pa开关io口
    u32 port_abd;//双io控制 pa类型io口
    u8 port_io_init_ok;//初始化ok

    //如果是单线功放 下面两个设置不起作用
    bool port_mute_io_status;//高低mute
    bool port_ab_io_status;//高低ab类

    u8  pa_mute_status;//pa mute 状态
    u8  pa_class_status;//pa ab d 状态
    bool pa_power_off;//关机状态

    //所需外部状态
    volatile bool pa_mic_online;//mic
    volatile bool pa_linein_mute;//linien
    volatile bool pa_manual_mute;//手动 
    volatile u8 pa_sys_automute;//自动
}PA_CTL_IO;

typedef struct pa_internal{
    PA_CTL_IO *pa_io;
    u8 pa_mode;//存放双线单线功放标志
    //最终调用控制功放
    void (*mute)(void *pa,u8 cmd);
    void (*abd)(void *pa,u8 cmd);
    
    int (*init)(void *pa);//初始化io口
    void (*io_strl)(void *pa,u8 cmd);//内部 pa 控制总入口        
    void (*service)(void *pa);//服务函数
    u16 service_id;

    //双io控制功放
    void (*mute_2pin)(void *pa,u8 cmd);
    void (*abd_2pin)(void *pa,u8 cmd);
    //单io控制功放
    void (*abd_and_mute)(void *pa,u8 cmd);
}PA_IN_STRL;


enum {
    PA_INIT,
    PA_POWER_OFF,
    PA_CLASS_AB,
    PA_CLASS_D,
    PA_MUTE,
    PA_UMUTE,
};
enum {
    PA_MODE_1,//双线功放
    PA_MODE_2,//单线电压功放
    PA_MODE_3,//单线脉冲功放
    PA_MODE_MAX,//
};

/**********************模块内部使用**************************/
void user_pa_in_abd_and_mute(void *pa,u8 cmd);//单io 功放
void user_pa_in_mute(void *pa,u8 cmd);//双io功放 mute
void user_pa_in_abd(void *pa,u8 cmd);//双io功放 abd
void user_pa_in_strl(void *pa,u8 cmd);//模块功放控制接口 
int  user_pa_in_pin_init(void *pa);//初始化
void user_pa_in_service(void *pa);//自动控制功放

/**********************注册函数外部调用**************************/
void user_pa_ex_io_init(void);//功放io初始化
void user_pa_ex_init(void);
void user_pa_ex_strl(u8 cmd);//功放控制api
void user_pa_ex_automute(u8 cmd);
void user_pa_ex_mic(u8 cmd);
void user_pa_ex_linein(u8 cmd);
bool user_pa_ex_manual(u8 cmd);
void user_pa_ex_del(void);
#endif