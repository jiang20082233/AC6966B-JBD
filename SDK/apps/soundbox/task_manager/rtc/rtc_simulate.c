#include "app_config.h"
#include "rtc/alarm.h"
#include "app_action.h"
#include "system/includes.h"
#include "clock_cfg.h"
#include "ioctl_cmds.h"
#include "system/sys_time.h"

#if (TCFG_APP_RTC_EN && TCFG_APP_RTC_SIMULATE_EN)

#define WRITE_ALARM     BIT(1)
#define READ_ALARM      BIT(5)

#define WRITE_RTC       BIT(0)
#define READ_RTC        BIT(4)

typedef struct _RTC_SIMULATE_VAR {
    struct sys_time sys_simulate_time;
    struct sys_time sys_alarm_time;
    struct device device;
} RTC_SIMULATE_VAR;

#define __this	(&rtc_smulate_var)
static RTC_SIMULATE_VAR rtc_smulate_var = {0};

static u8 g_alarm_flag = 1;

void __attribute__((weak)) alm_wakeup_isr(void) {}

static void rtc_smulate_timer_opt(u8 on_off, u32 msec, void (*func)(void *priv), void *priv)
{
    static u16 rtc_smulate_time = 0;
    if (on_off) {
        if (0 == rtc_smulate_time) {
            rtc_smulate_time = sys_timer_add(priv, func, msec);
        }
    } else {
        if (rtc_smulate_time) {
            sys_timer_del(rtc_smulate_time);
            rtc_smulate_time = 0;
        }
    }
}

static void calc_time_and_carry(struct sys_time *pt_time, u8 time_type, u32 time_step)
{
    u8 *current = NULL;
    u32 max = 0;
    switch (time_type) {
    case TIME_MEMBER_YEAR :
        pt_time->year += time_step;
        return;
    case TIME_MEMBER_MONTH :
        current = &pt_time->month;
        max = 13;
        break;
    case TIME_MEMBER_DAY :
        current = &pt_time->day;
        max = month_for_day(pt_time->month, pt_time->year) + 1;
        break;
    case TIME_MEMBER_HOUR :
        current = &pt_time->hour;
        max = 24;
        break;
    case TIME_MEMBER_MIN :
        current = &pt_time->min;
        max = 60;
        break;
    case TIME_MEMBER_SEC :
        current = &pt_time->sec;
        max = 60;
        break;
    default:
        return;
    }

    u32 current_time = *current + time_step;
    if (current_time >= max) {
        calc_time_and_carry(pt_time, time_type - 1, current_time / max);
        current_time = current_time % max;
    }
    *current = (u8)(current_time & 0xFF);

    switch (time_type) {
    case TIME_MEMBER_MONTH :
    case TIME_MEMBER_DAY :
        *current = 0 == *current ? 1 : *current;
        break;
    }
}

static u8 is_the_same_time(struct sys_time *time1, struct sys_time *time2)
{
    return (time1->year == time2->year &&
            time1->month == time2->month &&
            time1->day == time2->day &&
            time1->hour == time2->hour &&
            time1->min == time2->min &&
            time1->sec == time2->sec);
}

static void rtc_smulate_time_func(void *priv)
{
    static u32 time = 0;
    calc_time_and_carry(&__this->sys_simulate_time, TIME_MEMBER_SEC, sys_timer_get_ms() / 1000L - time);
    /* log_i("sys_time year:month:day, %02d :%02d :%02d", __this->sys_simulate_time.year, __this->sys_simulate_time.month, __this->sys_simulate_time.day); */
    /* log_i("sys_time hour:min:sec, %02d :%02d :%02d", __this->sys_simulate_time.hour, __this->sys_simulate_time.min, __this->sys_simulate_time.sec); */
    if (g_alarm_flag && is_the_same_time(&__this->sys_alarm_time, &__this->sys_simulate_time)) {
        alm_wakeup_isr();
    }
    time = sys_timer_get_ms() / 1000L;
}

static int rtc_simulate_init(const struct dev_node *node, void *arg)
{
    __this->sys_simulate_time.year = 2020;
    __this->sys_simulate_time.month = 1;
    __this->sys_simulate_time.day = 1;
    __this->sys_simulate_time.hour = 0;
    __this->sys_simulate_time.min = 0;
    __this->sys_simulate_time.sec = 0;
    return 0;
}

static int rtc_simulate_open(const char *name, struct device **device, void *arg)
{
    *device = &__this->device;
    rtc_smulate_timer_opt(1, 500, rtc_smulate_time_func, NULL);
    return 0;
}

static void write_alarm(struct sys_time *alarm_time)
{
    memcpy(&__this->sys_alarm_time, alarm_time, sizeof(struct sys_time));
}

static void write_sys_time(struct sys_time *curr_time)
{
    memcpy(&__this->sys_simulate_time, curr_time, sizeof(struct sys_time));
}

static void read_sys_time(struct sys_time *curr_time)
{
    memcpy(curr_time, &__this->sys_simulate_time, sizeof(struct sys_time));
}

static void set_alarm_ctrl(u8 set_alarm)
{
    g_alarm_flag = set_alarm;
}

static int rtc_simulate_ioctl(struct device *device, u32 cmd, u32 arg)
{
    int err = 0;
    switch (cmd) {
    case IOCTL_SET_ALARM:
        write_alarm((struct sys_time *)arg);
        break;
    case IOCTL_SET_ALARM_ENABLE:
        if (arg) {
            set_alarm_ctrl(1);
        } else {
            set_alarm_ctrl(0);
        }
        break;
    case IOCTL_GET_SYS_TIME:
        read_sys_time((struct sys_time *)arg);
        break;
    case IOCTL_SET_SYS_TIME:
        write_sys_time((struct sys_time *)arg);
        break;
    }
    return err;
}

const struct device_operations rtc_simulate_ops = {
    .init   = rtc_simulate_init,
    .open   = rtc_simulate_open,
    .ioctl  = rtc_simulate_ioctl,
    .read   = NULL,
    .write  = NULL,
};

#endif
