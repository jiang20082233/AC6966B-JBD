#ifndef _USER_DEV_CHECK_H_
#define _USER_DEV_CHECK_H_
// #include "system/includes.h"
// #include "user_fun_config.h"
// #include "app_config.h"

// 设备
typedef enum __DET_STATUS {
    DET_HOLD = 0,
    DET_OFF,
    DET_ON,
} DET_STATUS;

typedef struct ex_dev_opr {
    const u8 * dev_name;// 设备名称
    u8 port;            //IO
    u8 init;            //当前IO 是否初始化
    u8 up : 1;          //上拉
    u8 down : 1;        //下拉
    u8 ad_channel;      //AD 通道
    u16 ad_vol;         //判断的AD值

    bool multiplex_sd;
    u8 enable;          //是否使能
    u8 cnt;
    u8 stu;
    u8 online;          //是否在线
    u8 active;
    u8 step;
    int msg;            //上下线发送的消息
    const int scan_time;// 扫描的时间
    int timer;

    void (*dev_callback_fun)(void *priv);
}USER_DEV_CHECK;

char ex_dev_detect_init(void *arg);



// static struct ex_dev_opr spk_dev = {
//     //设置
//     .dev_name = "spk",
//     .enable = JL_SPK_EN,
//     .port = JL_SPK_PORT,            //检测IO
//     .up = 1,                        // 检测IO上拉使能
//     .down = 0,                      // 检测IO下拉使能
//     .ad_channel = NO_CONFIG_PORT,   // 检测IO是否使用AD检测
//     .ad_vol = 0,                    // AD检测时的阀值

//     .scan_time = 50,
//     .msg =USER_MSG_SYS_SPK_STATUS, // 发送的消息

//     //状态
//     .step = 0,
//     .stu  = 0,//1
//     .online = false,
// };
#endif