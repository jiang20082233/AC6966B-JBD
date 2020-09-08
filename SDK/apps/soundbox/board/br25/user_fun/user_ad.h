#ifndef _USER_AD_H_
#define _USER_AD_H_
// #include "board_config.h"

typedef struct USER_IO3_AD4{
    u32 por;//io
    u32 select_por[2];//功能选择io 相当于片选io
    u32 vol[4];//ad值
    u8  ch_bumber;//与vol对应 取值为0~3
    u8  ch;//通道
    int up;

    bool init_ok;
    u16 check_id;
    u32 check_time;//ms

    void (* callback)(u32 *vol);
}IO3_AD4;

void user_4ad_check_init(void (* callback)(u32 *vol));
#endif