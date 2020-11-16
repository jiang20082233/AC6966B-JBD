#include "user_fun_cfg.h"
void mic_callback_fun(void *priv);
#ifdef CONFIG_BOARD_AC6965E
USER_DEV_CHECK user_mic_check ={
    //设置
    .dev_name="mic check",
    .multiplex_sd = 0,//sd cmd复用
    .enable = 1,
    .port = IO_PORTB_01,            //检测IO
    .up = 1,                        // 检测IO上拉使能
    .down = 0,                      // 检测IO下拉使能
    .ad_channel = NO_CONFIG_PORT,   // 检测IO是否使用AD检测
    .ad_vol = 950,                    // AD检测时的阀值

    .scan_time = 150,
    .msg = USER_MSG_SYS_SPK_STATUS, // 发送的消息

    //状态
    .step = 0,
    .stu  = 0,//1
    .online = false,

    .dev_callback_fun = mic_callback_fun,
};
#endif
#ifdef CONFIG_BOARD_AC6966B
USER_DEV_CHECK user_mic_check ={
    //设置
    .dev_name="mic check",
    .multiplex_sd = 1,//sd cmd复用
    .enable = 1,
    .port = IO_PORTC_04,            //检测IO
    .up = 1,                        // 检测IO上拉使能
    .down = 0,                      // 检测IO下拉使能
    .ad_channel = AD_CH_PC4,   // 检测IO是否使用AD检测
    .ad_vol = 950,                    // AD检测时的阀值

    .scan_time = 150,
    .msg =USER_MSG_SYS_SPK_STATUS, // 发送的消息

    //状态
    .step = 0,
    .stu  = 0,//1
    .online = false,

    .dev_callback_fun = mic_callback_fun,
};
#endif
#ifdef CONFIG_BOARD_AC6969D_DEMO
// USER_DEV_CHECK __attribute__((weak)) user_mic_check = {
USER_DEV_CHECK user_mic_check = {
    //设置
    .dev_name="mic check",
    .multiplex_sd = 0,//sd cmd复用
    .enable = 0,
    .port = NO_CONFIG_PORT,            //检测IO
    .up = 1,                        // 检测IO上拉使能
    .down = 0,                      // 检测IO下拉使能
    .ad_channel = NO_CONFIG_PORT,   // 检测IO是否使用AD检测
    .ad_vol = 950,                    // AD检测时的阀值

    .scan_time = 150,
    .msg = USER_MSG_SYS_SPK_STATUS, // 发送的消息

    //状态
    .step = 0,
    .stu  = 0,//1
    .online = false,

    .dev_callback_fun = mic_callback_fun,
};
#endif

bool ex_mic_check_en = 1;
u8 user_mic_check_en(u8 cmd){
  if(0==cmd || 1==cmd){
    ex_mic_check_en = cmd;
    mic_callback_fun(& user_mic_check);
    // printf(" ex mic check en %d\n",ex_mic_check_en);
  }
  return user_mic_check.enable;
}

void mic_callback_fun(void *priv){
  USER_DEV_CHECK * mic_dev = (USER_DEV_CHECK *)priv;
  bool enable = 1;

  if(APP_IDLE_TASK == app_get_curr_task() || APP_FM_TASK == app_get_curr_task()){
    enable = 0;
  }else if(APP_BT_TASK == app_get_curr_task()){
  #if 0//对箱时需要单箱插mic有声音
    if(!(tws_api_get_tws_state() & TWS_STA_SIBLING_DISCONNECTED)){
      enable = 0;
    }
  #endif
  }

  if(1 == user_record_status(0xff)){
    enable = 0;
  }

  if(!ex_mic_check_en){
    enable = 0;
  }

  mic_dev->enable = enable;
}

bool user_get_mic_status(void){
  return user_mic_check.stu?true:false;
}
void user_mic_check_init(void){
  ex_dev_detect_init(& user_mic_check);
}
void user_mic_check_del(void){
  ex_dev_detect_dell(& user_mic_check);
}