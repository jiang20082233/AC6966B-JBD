#ifndef _CHGBOX_WIRELESS_H_
#define _CHGBOX_WIRELESS_H_

#include "device/wireless_charge.h"

extern _wireless_hdl wl_hdl;

//dcdc en IO
#define DCDC_EN_IO          IO_PORTB_09
#define DCDC_CTRL_EN        1            //是否使用dcdc 控制

//wpc io
#define WPC_IO              IO_PORTB_10

//ad 选择
#define WL_AD_DET_IO        IO_PORTB_08
#define WL_AD_DET_CH        AD_CH_PB8

//api函数声明
u16 get_wireless_power(void);
void wireless_api_close(void);
void wireless_api_open(void);
void wireless_init_api(void);
void wireless_100ms_run_app(void);

#endif
