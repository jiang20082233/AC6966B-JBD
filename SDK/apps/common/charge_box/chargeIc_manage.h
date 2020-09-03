#ifndef _CHARGEIC_MANAGE_H
#define _CHARGEIC_MANAGE_H

#include "printf.h"
#include "cpu.h"
#include "asm/iic_hw.h"
#include "asm/iic_soft.h"
#include "timer.h"
#include "app_config.h"
#include "event.h"
#include "system/includes.h"

typedef struct {
    u8 logo[20];
    u8(*init)(void);
    void (*detect)(void);
    void (*boost_ctrl)(u8 en);
    void (*vol_ctrl)(u8 en);
    void (*vor_ctrl)(u8 en);
    u16(*get_vbat)(void);
} CHARGE_IC_INTERFACE;

struct chargeIc_platform_data {
    u8    iic;
    char  chargeIc_name[20];
};

typedef struct {
    u8   iic_hdl;
    u8   iic_delay;                 //这个延时并非影响iic的时钟频率，而是2Byte数据之间的延时
    int  init_flag;
} CHARGE_IC_INFO;

int chargeIc_init(const void *_data);
u8 chargeIc_command(u8 w_chip_id, u8 register_address, u8 function_command);
u8 chargeIc_get_ndata(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len);
void chargeIc_boost_ctrl(u8 en);
void chargeIc_vol_ctrl(u8 en);
void chargeIc_vor_ctrl(u8 en);
u16 chargeIc_get_vbat(void);

extern CHARGE_IC_INTERFACE chargeIc_dev_begin[];
extern CHARGE_IC_INTERFACE chargeIc_dev_end[];

#define REGISTER_CHARGE_IC(chargeIc) \
	static CHARGE_IC_INTERFACE chargeIc SEC_USED(.chargeIc_dev)

#define list_for_each_chargeIc(c) \
	for (c=chargeIc_dev_begin; c<chargeIc_dev_end; c++)

#define CHARGE_IC_PLATFORM_DATA_BEGIN(data) \
		static const struct chargeIc_platform_data data = {

#define CHARGE_IC_PLATFORM_DATA_END() \
};


extern const struct chargeIc_platform_data chargeIc_data;

#endif


