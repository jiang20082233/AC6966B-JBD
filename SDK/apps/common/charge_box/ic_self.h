#ifndef _IC_SELF_H_
#define _IC_SELF_H_

#include "typedef.h"
#include "os/os_type.h"

#define LOW_LEVEL     0
#define HIGHT_LEVEL   1

//usb在线检测
#define  USB_ONLE_DET_IO          IO_PORTA_06
#define  USB_DET_ONLINE_LEVEL     HIGHT_LEVEL

//满电检测
#define  CHARGE_FULL_DET_IO       IO_PORTB_04
#define  CHARGE_FULL_LEVEL        HIGHT_LEVEL

//boost Io
#define  BOOST_CTRL_IO            IO_PORTA_09

//POWER vl vr IO
#define  VOL_CTRL_IO              IO_PORTB_03
#define  VOR_CTRL_IO              IO_PORTB_03

//LDO //5V升压是否在成功检测
#define CHG_LDO_DET_IO           IO_PORTA_10
#define CHG_LDO_DET_AD_CH        AD_CH_PA10

//仓电池电量检测IO
#define BAT_DET_IO         IO_PORTA_05
#define BAT_DET_AD_CH      AD_CH_PA5
#endif


