#ifndef __ALARM_H__
#define __ALARM_H__


#define RTC_ALM_EN  1
#include "typedef.h"
#include "system/includes.h"
#pragma pack(1)
typedef struct _RTC_TIME {
    u16 dYear;		///<年份
    u8 	bMonth;		///<月份
    u8 	bDay;		///<天数
    u8 	bHour;		///<时
    u8 	bMin;		///<分
    u8 	bSec;		///<秒
    //    u8 	bWeekday;	///<星期几
} RTC_TIME;
#pragma pack()


typedef struct __ALARM__ {
    u8 index;
    u8 sw;
    u8 mode;
    RTC_TIME time;
    u8 name_len;
} T_ALARM, *PT_ALARM;

typedef struct __alarm_vm__ {
    u16 head;
    T_ALARM alarm;
} T_ALARM_VM, *PT_ALARM_VM;

enum {
    E_ALARM_WRITE_VM_ERR = 0x00,
    E_ALARM_WRITE_VM_SUCC,
};

enum {
    E_ALARM_MODE_ONCE            = 0x00,
    E_ALARM_MODE_EVERY_DAY       = 0x01,
    E_ALARM_MODE_EVERY_MONDAY    = 0x02,
    E_ALARM_MODE_EVERY_TUESDAY   = 0x04,
    E_ALARM_MODE_EVERY_WEDNESDAY = 0x08,
    E_ALARM_MODE_EVERY_THURSDAY  = 0x10,
    E_ALARM_MODE_EVERY_FRIDAY    = 0x20,
    E_ALARM_MODE_EVERY_SATURDAY  = 0x40,
    E_ALARM_MODE_EVERY_SUNDAY    = 0x80,
};

enum {

    E_WEEK_MONDAY = 0x01,
    E_WEEK_TUESDAY,
    E_WEEK_WEDNESDAY,
    E_WEEK_THURSDAY,
    E_WEEK_FRIDAY,
    E_WEEK_SATURDAY,
    E_WEEK_SUNDAY,
};


typedef enum {
    TIME_MEMBER_YEAR = 0x0,
    TIME_MEMBER_MONTH,
    TIME_MEMBER_DAY,
    TIME_MEMBER_HOUR,
    TIME_MEMBER_MIN,
    TIME_MEMBER_SEC,
    TIME_MEMBER_MAX,
} TIME_MEMBER_ENUM;


enum {
    E_SUCCESS = 0x00,
    E_FAILURE,
};

/* macro */
#define M_MAX_ALARM_NUMS          5
#define M_MAX_ALARM_NAME_LEN      32

#define M_MAX_ALARM_INDEX        (M_MAX_ALARM_NUMS-1)
#define M_MAX_ALARM_MODE         (0xFE)

/* 闹铃响的分钟数 */
#define M_MAX_ALARM_RING_MAX_CNT  1


void alarm_init();
void alarm_update();
u8   alarm_add(PT_ALARM p, u8 index);
u8 alarm_get_info(PT_ALARM p, u8 index);
void rtc_update_time_api(struct sys_time *time);


void alarm_ring_start();
u8 alarm_active_flag_get(void);

void alarm_puts_time(RTC_TIME *pTime);
void alarm_puts_rtc_time(RTC_TIME *pTime);
u8 rtc_calculate_week_val(RTC_TIME *data_time);
void rtc_calculate_next_day(RTC_TIME *data_time);//计算明天日期
void alarm_update_all_time(RTC_TIME *cTIME);


void alarm_ring_cnt_clear(void);


u16 month_for_day(u8 month, u16 year);

#endif  //end of __ALARM_H__


