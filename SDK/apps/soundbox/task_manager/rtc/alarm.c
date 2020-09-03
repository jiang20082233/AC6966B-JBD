#include "system/includes.h"
#include "rtc/alarm.h"
#include "common/app_common.h"
#include "system/timer.h"
#include "app_main.h"
#include "tone_player.h"
#include "app_task.h"

#if TCFG_APP_RTC_EN
#ifdef  RTC_ALM_EN
#define ALARM_DEBUG_EN
#ifdef ALARM_DEBUG_EN
#define alarm_printf        printf
#define alarm_putchar       putchar
#define alarm_printf_buf    put_buf
#else
#define alarm_printf(...)
#define alarm_putchar(...)
#define alarm_printf_buf(...)
#endif

#define PRINT_FUN()              alarm_printf("func : %s\n", __FUNCTION__)
#define PRINT_FUN_RETURN_INFO()  alarm_printf("func : %s, line : %d.\n", __FUNCTION__, __LINE__)

static volatile u8 g_alarm_sw = 0;
static volatile u8 g_alarm_map = 0;//存储了当前的所有闹钟信息的map
static volatile u8 g_alarm_active_same = 0;//存储了上次设置的闹钟
static volatile u8 g_alarm_active_flag = 0;

/* volatile u8  g_alarm_ring_max_cnt = 100; */
#define ALARM_RING_MAX 50

volatile u8  g_alarm_backup_sec = 0;
volatile u16 g_alarm_ring_cnt = 0;

static T_ALARM alarm_tab[M_MAX_ALARM_NUMS];
static u8 alarm_name[M_MAX_ALARM_NAME_LEN] = {0};

#define RTC_MASK (0xaa55)
static const u8 month_tab1[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};		 ///<非闰年每月的天数
static const u8 month_tab2[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};		 ///<闰年每月的天数

static void *dev_handle;


/******************************************************** vm op start **********************************************************************************/

static u8 __alarm_vm_check_head(PT_ALARM_VM p)
{
    if (p->head == RTC_MASK) {
        return 1;
    } else {
        return 0;
    }
}

/*
**  函数功能 ：根据闹钟的模式计算出实际时间
**  函数形参 ：pTime-闹钟时间结构体；week-当下星期； mode-闹钟模式
**  返回值   ：void
**  备注     ：无
*/
void alarm_calc_time_by_week_mode(RTC_TIME *cTime, RTC_TIME *pTime, u8 mode)
{
    PRINT_FUN();

    u8 i = 0;
    u8 alarm_week = 0;
    u8 cur_week = 0;
    u8 tmp_mode = 0;

    alarm_puts_rtc_time(cTime);
    cur_week = rtc_calculate_week_val(cTime);
    if (cur_week == 0) {
        cur_week = 7;       //星期天写成7，方便对比计算
    }
    alarm_printf("cur_week : %d\n", cur_week);

    alarm_puts_time(pTime);
    alarm_week = rtc_calculate_week_val(pTime);
    if (alarm_week == 0) {
        alarm_week = 7;       //星期天写成7，方便对比计算
    }

    if (0 == mode) {
        alarm_printf("once\n");
    } else if ((BIT(0)) == mode) {
        alarm_printf("everyday\n");
    } else {

        for (i = 1; i < 8; i++) {
            if (mode & BIT(i)) {
                /* if (i >= cur_week) { */
                if (i >= alarm_week) {
                    tmp_mode = i;
                    break;
                }
            }
        }

        if (i >= 8) {
            for (i = 1; i < 8; i++) {
                if (mode & BIT(i)) {
                    tmp_mode = i;
                    break;
                }
            }
        }
    }

    if ((tmp_mode != 0) && (tmp_mode < 8)) {
        /* if (tmp_mode > cur_week) { */
        if (tmp_mode > alarm_week) {
            alarm_printf("***a***\n");
            /* for (i=0; i<(tmp_mode-cur_week); i++) { */
            for (i = 0; i < (tmp_mode - alarm_week); i++) {
                rtc_calculate_next_day(pTime);
            }
            /* } else if (tmp_mode < cur_week) { */
        } else if (tmp_mode < alarm_week) {
            alarm_printf("***b***\n");
            /* for (i=0; i<(7-(cur_week-tmp_mode)); i++) { */
            for (i = 0; i < (7 - (alarm_week - tmp_mode)); i++) {
                rtc_calculate_next_day(pTime);
            }
            /* } else if (tmp_mode == cur_week) { */
        } else if (tmp_mode == alarm_week) {
            alarm_printf("***c***\n");
            /* if (alarm_week > cur_week) { */
            /*     alarm_printf("***d***\n"); */
            /*     for (i=0; i<6; i++) { */
            /*         rtc_calculate_next_day(pTime); */
            /*     } */
            /* } */
        }
    }

    alarm_puts_time(pTime);

    return;
}


static void alarm_vm_write_mask(void)
{
    PRINT_FUN();

    s32 ret = 0;
    u32 map = 0;
    map = (RTC_MASK << 16) | (g_alarm_active_same << 8) | (g_alarm_map);
    ret = syscfg_write(VM_ALARM_MASK, (u8 *)&map, sizeof(map));
    if (ret < 0) {
        PRINT_FUN_RETURN_INFO();
        return;
    }

    return;
}

static void alarm_vm_read_mask(void)
{
    PRINT_FUN();

    s32 ret = 0;
    u32 map = 0;
    ret = syscfg_read(VM_ALARM_MASK, (u8 *)&map, sizeof(map));
    if (ret < 0) {
        PRINT_FUN_RETURN_INFO();
        return;
    }
    /* 最大限度的避免第一次读vm时的随机数 */
    if (((map >> 16) & 0xffff) != RTC_MASK) {
        g_alarm_map = 0;
    }
    g_alarm_map = map & 0xff;
    g_alarm_active_same = (map >> 8) & 0xff;
    return;
}
static bool leapyear(u16 year)
{
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief	月份换算为月天数
   @param 	month，year
   @return  月份天数
   @note
*/
/*----------------------------------------------------------------------------*/
u16 month_for_day(u8 month, u16 year)
{
    if (leapyear(year)) {
        return (u16)month_tab2[month - 1];
    } else {
        return (u16)month_tab1[month - 1];
    }
    return 0;
}

/******************************************************** print op start *******************************************************************************/
void alarm_puts_time(RTC_TIME *pTime)
{
    u8 week = 0;

#if 0
    alarm_printf("alarm_time : %d-%d-%d,%d:%d:%d\n", pTime->dYear, pTime->bMonth, pTime->bDay, pTime->bHour, pTime->bMin, pTime->bSec);
    week = rtc_calculate_week_val(pTime);
#endif
    alarm_printf("alarm week : %d\n", week);
}

void alarm_puts_rtc_time(RTC_TIME *pTime)
{
    u8 week = 0;

#if 0
    alarm_printf("cur_time  : %d-%d-%d,%d:%d:%d\n",  pTime->dYear, pTime->bMonth, pTime->bDay, pTime->bHour, pTime->bMin, pTime->bSec);
    week = rtc_calculate_week_val(pTime);
    alarm_printf("cur_week  : %d\n", week);
#endif
}
/******************************************************** print op end *********************************************************************************/



/*----------------------------------------------------------------------------*/
/**@brief  计算明天的日期
   @param  data_time--计算日期
   @return none
   @note
*/
/*----------------------------------------------------------------------------*/
void rtc_calculate_next_day(RTC_TIME *data_time)
{
    u16 tmp16 = month_for_day(data_time->bMonth, data_time->dYear);

    if (data_time->bDay < tmp16) {
        data_time->bDay++;
    } else {
        data_time->bMonth++;
        data_time->bDay = 1;
        if (data_time->bMonth > 12) {
            data_time->bMonth = 1;
            data_time->dYear++;
        }
    }
}



void alarm_vm_read_info(void)
{
    PRINT_FUN();
    u8 i;
    s32 ret = 0;

    alarm_vm_read_mask();

    T_ALARM_VM tmp;

    for (i = 0; i < 5; i++) {

        if ((g_alarm_map & 0xff) & BIT(i)) {

            ret = syscfg_read(VM_ALARM_0 + i, &tmp, sizeof(T_ALARM_VM));
            if (ret < 0) {
                alarm_printf("alarm read vm err!\n");
                return;
            }
            if (__alarm_vm_check_head(&tmp)) {
                alarm_printf("find the %d alarm from vm.\n", i);
            } else {
                alarm_printf("can't find the %d alarm from vm.\n", i);
                memset(&(alarm_tab[i]), 0x00, sizeof(T_ALARM));
                continue;
            }

            alarm_printf("vm info : index=%d, sw=%d, mode=%d, h=%d, m=%d, name_len=%d\n", tmp.alarm.index, tmp.alarm.sw, tmp.alarm.mode, \
                         tmp.alarm.time.bHour, tmp.alarm.time.bMin, tmp.alarm.name_len);

            alarm_tab[i].index = tmp.alarm.index;
            alarm_tab[i].sw = tmp.alarm.sw;
            alarm_tab[i].mode = tmp.alarm.mode;
            alarm_tab[i].time.bHour = tmp.alarm.time.bHour;
            alarm_tab[i].time.bMin = tmp.alarm.time.bMin;
            alarm_tab[i].name_len = tmp.alarm.name_len;

            if (alarm_tab[i].sw) {
                g_alarm_sw |= BIT(i);
            }
        }
    }

    return;
}

void alarm_vm_write_info_by_index(u8 index)
{
    PRINT_FUN();

    s32 ret = 0;
    T_ALARM_VM tmp;
    tmp.head = RTC_MASK;
    tmp.alarm.index =  index;
    tmp.alarm.sw    =  alarm_tab[index].sw;
    tmp.alarm.mode  =  alarm_tab[index].mode;
    tmp.alarm.time.bHour =  alarm_tab[index].time.bHour;
    tmp.alarm.time.bMin  =  alarm_tab[index].time.bMin;
    tmp.alarm.name_len =  alarm_tab[index].name_len;
    ret = syscfg_write(VM_ALARM_0 + index, &tmp, sizeof(T_ALARM_VM));
    if (ret < 0) {
        alarm_printf("The %d alarm write vm err!\n", index);
        return;
    }

    return;
}

void alarm_vm_write_name(u8 *p, u8 index)
{
    PRINT_FUN();

    s32 ret = 0;

    ret = syscfg_write(VM_ALARM_NAME_0 + index, p, sizeof(alarm_name));
    if (ret < 0) {
        PRINT_FUN_RETURN_INFO();
        return;
    }

    return;
}

void alarm_vm_read_name(u8 *p, u8 index)
{
    PRINT_FUN();

    s32 ret = 0;

    ret = syscfg_read(VM_ALARM_NAME_0 + index, p, sizeof(alarm_name));
    if (ret < 0) {
        PRINT_FUN_RETURN_INFO();
        return;
    }

    return;
}

void alarm_vm_puts_info(PT_ALARM p)
{
    alarm_printf("index : %d\n", p->index);
    alarm_printf("mode: %d\n", p->mode);
    alarm_printf("sw: %d\n", p->sw);
    /* alarm_puts_time(&(p->time)); */
}
/*----------------------------------------------------------------------------*/
/**@brief  日期转换为星期
   @param  data_time--日期
   @return 星期
   @note
*/
/*----------------------------------------------------------------------------*/
//蔡勒（Zeller）公式：w=y+[y/4]+[c/4]-2c+[26x(m+1)/10]+d-1
u8 rtc_calculate_week_val(RTC_TIME *data_time)
{
    RTC_TIME t_time;
    u32 century, val, year;

    memcpy(&t_time, data_time, sizeof(RTC_TIME));
    if (t_time.bMonth < 3) {
        t_time.bMonth = t_time.bMonth + 12;
        t_time.dYear--;
    }
    year = t_time.dYear % 100;
    century = t_time.dYear / 100;
    val = year + (year / 4) + (century / 4) + (26 * (t_time.bMonth + 1) / 10) + t_time.bDay;
    val = val - century * 2 - 1;

    return (u8)(val % 7);
}



/******************************************************** vm op end ************************************************************************************/


/******************************************************** compare op start *****************************************************************************/


static int __alarm_cmp_time_num(u32 num1, u32 num2)
{
    int ret = -2;

    if (num1 > num2) {
        ret = 1;
    } else if (num1 == num2) {
        ret = 0;
    } else if (num1 < num2) {
        ret = -1;
    }

    return ret;
}

/*
 *  函数功能 ：比较两个闹钟的时间
 *  函数形参 ：time1 - 闹钟1；time2 - 闹钟2
 *  返回值   ：0-两闹钟相等；1-闹钟1时间比对比闹钟2要晚；-1-闹钟1比对比闹钟2早 -2-比较出错
 *  备注     ：无
 *
 * */
static int alarm_cmp_time_member(RTC_TIME *time1, RTC_TIME *time2, TIME_MEMBER_ENUM type)
{
    switch (type) {
    case TIME_MEMBER_YEAR:
        return __alarm_cmp_time_num(time1->dYear,  time2->dYear);
    case TIME_MEMBER_MONTH:
        return __alarm_cmp_time_num(time1->bMonth, time2->bMonth);
    case TIME_MEMBER_DAY:
        return __alarm_cmp_time_num(time1->bDay,   time2->bDay);
    case TIME_MEMBER_HOUR:
        return __alarm_cmp_time_num(time1->bHour,  time2->bHour);
    case TIME_MEMBER_MIN:
        return __alarm_cmp_time_num(time1->bMin,   time2->bMin);
    case TIME_MEMBER_SEC:
        return __alarm_cmp_time_num(time1->bSec,   time2->bSec);

    default:
        return -2;
    }

    return -2;
}

static int alarm_cmp_time(RTC_TIME *time1, RTC_TIME *time2)
{
    u8 i;
    int ret = 0;

    for (i = 0; i < TIME_MEMBER_MAX; i++) {
        ret = alarm_cmp_time_member(time1, time2, i);
        if (ret != 0) {
            break;
        }
    }

    return ret;
}

/******************************************************** compare op end *******************************************************************************/


/******************************************************** hw op start **********************************************************************************/

void alarm_hw_set_sw(u8 sw)
{
    /* printf("alarm sw : %d\n", sw); */
    if (!dev_handle) {
        return ;
    }
    dev_ioctl(dev_handle, IOCTL_SET_ALARM_ENABLE, !!sw);
}

void alarm_hw_write_time(RTC_TIME *alarm_time, u8 sw)
{
    if (!dev_handle) {
        return ;
    }
    struct sys_time time;
    time.year = alarm_time->dYear;
    time.month = alarm_time->bMonth;
    time.day = alarm_time->bDay;
    time.hour = alarm_time->bHour;
    time.min = alarm_time->bMin;
    time.sec = alarm_time->bSec;
    /* struct rtc_data hw_date; */
    /* RTC_TIME_to_rtc_data(&hw_date, alarm_time); */
    alarm_printf("alarm_time : %d-%d-%d,%d:%d:%d\n", alarm_time->dYear, alarm_time->bMonth, alarm_time->bDay, alarm_time->bHour, alarm_time->bMin, alarm_time->bSec);

    alarm_hw_set_sw(0);
    dev_ioctl(dev_handle, IOCTL_SET_ALARM, (u32)&time);
    alarm_hw_set_sw(sw);
    /* set_rtc_alarm(&hw_date); */

}

/******************************************************** hw op end ************************************************************************************/



/******************************************************** calc op start ********************************************************************************/

/*
**  函数功能 ：根据闹钟的时、分计算出它具体的时间（年、月、日、时、分、秒）
**  函数形参 ：带有时、分和闹钟模式的时间结构体
**  返回值   ：void
**  备注     ：无
*/
void alarm_calc_real_time_by_index(RTC_TIME *cTime, u8 index)
{
    u8 week = 0;
    RTC_TIME *pTime ;
    RTC_TIME tmp;
    PT_ALARM pAlarm_tab;
    u8 item = 0;
    int ret = 0;

    if (index > M_MAX_ALARM_INDEX) {
        PRINT_FUN_RETURN_INFO();
        return;
    }

    pAlarm_tab = &(alarm_tab[index]);
    pTime = &(pAlarm_tab->time);

    if (pAlarm_tab->mode > M_MAX_ALARM_MODE) {
        PRINT_FUN_RETURN_INFO();
        return;
    }

    for (item = TIME_MEMBER_HOUR; item < TIME_MEMBER_MAX; item++) {
        ret = alarm_cmp_time_member(cTime, pTime, item);
        if (ret != 0) {
            break;
        }
    }

    if ((1 == ret) || (0 == ret)) {
        alarm_printf("***B***\n");
        memcpy(&tmp, cTime, sizeof(RTC_TIME));
        rtc_calculate_next_day(&tmp);
        pTime->dYear  = tmp.dYear;
        pTime->bMonth = tmp.bMonth;
        pTime->bDay   = tmp.bDay;
        pTime->bSec   = 0;

    } else if (-1 == ret) {
        alarm_printf("***A***\n");
        pTime->dYear  = cTime->dYear;
        pTime->bMonth = cTime->bMonth;
        pTime->bDay   = cTime->bDay;
        pTime->bSec   = 0;
    }

    if ((pAlarm_tab->mode != E_ALARM_MODE_ONCE) && (pAlarm_tab->mode != E_ALARM_MODE_EVERY_DAY)) {
        alarm_calc_time_by_week_mode(cTime, pTime, pAlarm_tab->mode);
    } else {
        alarm_puts_time(pTime);
    }

    return;
}


/******************************************************** calc op end **********************************************************************************/


/******************************************************** name op start ********************************************************************************/

void alarm_name_clear(void)
{
    PRINT_FUN();

    memset(alarm_name, 0x00, sizeof(alarm_name));

    return;
}

void alarm_name_set(u8 *p, u8 index, u8 len)
{
    PRINT_FUN();

    if ((len == 0) || (len > sizeof(alarm_name))) {
        PRINT_FUN_RETURN_INFO();
        return;
    }

    alarm_name_clear();

    alarm_printf("alarm name len : %d\n", len);
    alarm_printf_buf(p, len);

    memcpy(alarm_name, p, len);
    alarm_vm_write_name(alarm_name, index);

    return;
}

u8 alarm_name_get(u8 *p, u8 index)
{
    PRINT_FUN();

    u8 name_len = 0;

    alarm_vm_read_name(alarm_name, index);

    name_len = alarm_tab[index].name_len;
    memcpy(p, alarm_name, name_len);

    alarm_printf("alarm name len : %d\n", name_len);
    alarm_printf_buf(alarm_name, name_len);

    return name_len;
}

/******************************************************** name op end **********************************************************************************/


/******************************************************** get op start *********************************************************************************/

u8 alarm_get_active_index(void)
{
#if 0
    u8 i = 0;
    for (i = 0; i < 5; i++) {
        if (g_alarm_active_same & BIT(i)) {
            return i;
        }
    }
    return i;
#endif
    return g_alarm_active_same;
}

u8 alarm_get_info(PT_ALARM p, u8 index)
{
    u8 ret = E_SUCCESS;

    if (g_alarm_map & BIT(index)) {
        p->index = alarm_tab[index].index;
        p->sw = alarm_tab[index].sw;
        p->mode = alarm_tab[index].mode;
        p->time.bHour = alarm_tab[index].time.bHour;
        p->time.bMin = alarm_tab[index].time.bMin;
        p->name_len = alarm_tab[index].name_len;
    } else {
        ret = E_FAILURE;
    }

    return ret;
}

u8 alarm_get_total(void)
{
    PRINT_FUN();

    u8 total = 0;
    u8 i = 0;

    for (i = 0; i < 5; i++) {
        if (g_alarm_map & BIT(i)) {
            total++;
        }
    }

    alarm_printf("total %d alarm\n", total);

    return total;
}


void alarm_get_the_earliest(void)
{
    PRINT_FUN();

    int ret = 0;
    u8 index = 0;
    u8 i = 0;
    RTC_TIME *pTmp = NULL;

    RTC_TIME  current_alarm;

    g_alarm_active_same = 0;

    alarm_printf("g_alarm_map = %d\n", g_alarm_map);

    for (i = 0; i < 5; i++) {
        if (g_alarm_sw & BIT(i)) {

            pTmp = &(alarm_tab[i].time);

            g_alarm_active_same |= BIT(i);
            index = i;
            break;
        }
    }

    if (i >= 5) {
        alarm_printf("***no alarm***\n");
        alarm_hw_set_sw(0);
        return;
    }

    for (i = index + 1; i < 5; i++) {

        if (g_alarm_sw & BIT(i)) {
            ret = alarm_cmp_time(pTmp, &(alarm_tab[i].time));
            if (0 == ret) {
                g_alarm_active_same |= BIT(i);
                alarm_printf("***A***\n");
            } else if (1 == ret) {
                alarm_printf("***B***\n");
                pTmp = &(alarm_tab[i].time);
                index = i;
                g_alarm_active_same = 0;
                g_alarm_active_same |= BIT(i);
            } else if (-1 == ret) {
                alarm_printf("***C***\n");
            } else if (-2 == ret) {
                alarm_printf("***D***\n");
            } else {
                alarm_printf("***F***\n");
            }
        }
    }

    /* for (i = 0; i< 5; i++) { */
    /*     alarm_printf("%dth alarm:",i); */
    /*     alarm_puts_time(&alarm_tab[i].time); */
    /*     alarm_printf("sw:%d\n",alarm_tab[i].sw); */
    /* } */

    memcpy(&current_alarm, &(alarm_tab[index].time), sizeof(RTC_TIME));
    alarm_puts_time(&current_alarm);
    printf("find the %dth alarm, the same alarm : %d\n", index, g_alarm_active_same);
    alarm_hw_write_time(&current_alarm, alarm_tab[index].sw);

    return;
}

/******************************************************** get op end ***********************************************************************************/


/******************************************************** update op start ******************************************************************************/

void rtc_update_time_api(struct sys_time *time)
{
    if (!dev_handle) {
        return ;
    }
    RTC_TIME current_time = {0};

    dev_ioctl(dev_handle, IOCTL_SET_SYS_TIME, (u32)time);
    current_time.dYear = time->year;
    current_time.bMonth = time->month;
    current_time.bDay = time->day;
    current_time.bHour = time->hour;
    current_time.bMin = time->min;
    current_time.bSec = time->sec;

    alarm_update_all_time(&current_time);
    alarm_get_the_earliest();
    alarm_vm_write_mask();
}


void alarm_update_all_time(RTC_TIME *cTIME)
{
    PRINT_FUN();

    u8 tmp = 0;
    u8 i = 0;

    for (i = 0; i < 5; i++) {
        if (g_alarm_sw & BIT(i)) {
            alarm_calc_real_time_by_index(cTIME, i);
        }
    }

    return;
}

void alarm_update_info_after_isr(void)
{
    PRINT_FUN();

    if (!dev_handle) {
        return ;
    }

    RTC_TIME current_time = {0};
    struct sys_time time = {0};

    dev_ioctl(dev_handle, IOCTL_GET_SYS_TIME, (u32)&time);
    current_time.dYear = time.year;
    current_time.bMonth = time.month;
    current_time.bDay = time.day;
    current_time.bHour = time.hour;
    current_time.bMin = time.min;
    current_time.bSec = time.sec;

    u8 i = 0;


    for (i = 0; i < 5; i++) {
        if (g_alarm_active_same & BIT(i)) {
            if (alarm_tab[i].mode != 0) {
                alarm_calc_real_time_by_index(&current_time, i);
            } else {
                g_alarm_sw &= ~BIT(i);
                alarm_tab[i].sw = 0;
            }
            alarm_vm_write_info_by_index(i);
        }
    }
    alarm_get_the_earliest();

    alarm_vm_write_mask();
}
/******************************************************** update op end ********************************************************************************/


/******************************************************** active flag op start *************************************************************************/
void alarm_active_flag_set(u8 flag)
{
    g_alarm_active_flag = flag;

    return;
}

u8 alarm_active_flag_get(void)
{
    return g_alarm_active_flag;
}
/******************************************************** active flag op end ***************************************************************************/

/******************************************************** other op start ********************************************************************************/
u8 alarm_add(PT_ALARM p, u8 index)
{
    PRINT_FUN();

    u8 ret = E_SUCCESS;

    if (!dev_handle) {
        return E_FAILURE;
    }

    RTC_TIME current_time = {0};
    struct sys_time time = {0};

    dev_ioctl(dev_handle, IOCTL_GET_SYS_TIME, (u32)&time);
    current_time.dYear = time.year;
    current_time.bMonth = time.month;
    current_time.bDay = time.day;
    current_time.bHour = time.hour;
    current_time.bMin = time.min;
    current_time.bSec = time.sec;

    if (index > M_MAX_ALARM_INDEX) {
        PRINT_FUN_RETURN_INFO();
        alarm_printf("alarm is full!\n");
        return E_FAILURE;
    }
    if (p->mode > M_MAX_ALARM_MODE) {
        PRINT_FUN_RETURN_INFO();
        alarm_printf("alarm's mode is error");
        return E_FAILURE;
    }

    g_alarm_map |= BIT(index);

    if (0 == p->sw) {
        alarm_printf("close the %dth alarm!\n", p->index);
        g_alarm_sw &= ~BIT(p->index);
    } else if (1 == p->sw) {
        g_alarm_sw |= BIT(p->index);
    }

    alarm_tab[index].index      = index;
    alarm_tab[index].sw         = p->sw;
    alarm_tab[index].mode       = p->mode;
    alarm_tab[index].time.bHour = p->time.bHour;
    alarm_tab[index].time.bMin  = p->time.bMin;
    alarm_tab[index].name_len   = p->name_len;

    alarm_calc_real_time_by_index(&current_time, index);
    alarm_get_the_earliest();

    alarm_vm_write_info_by_index(index);
    alarm_vm_write_mask();

    return ret;
}

void alarm_delete(u8 index)
{
    PRINT_FUN();

    alarm_printf("delete the %dth alarm!\n", index);
    g_alarm_map &= ~BIT(index);
    g_alarm_sw   &= ~BIT(index);

    alarm_get_the_earliest();
    alarm_vm_write_mask();
    alarm_vm_write_info_by_index(index);

    return;
}

void alarm_ring_cnt_clear(void)
{
    g_alarm_ring_cnt = 0;
}

void alarm_stop(void)
{
    alarm_active_flag_set(0);
    alarm_ring_cnt_clear();
}




void alarm_update()
{
    PRINT_FUN();

    if (!dev_handle) {
        return ;
    }

    RTC_TIME current_time = {0};
    struct sys_time time = {0};
    dev_ioctl(dev_handle, IOCTL_GET_SYS_TIME, (u32)&time);
    current_time.dYear = time.year;
    current_time.bMonth = time.month;
    current_time.bDay = time.day;
    current_time.bHour = time.hour;
    current_time.bMin = time.min;
    current_time.bSec = time.sec;
    alarm_update_all_time(&current_time);
    alarm_get_the_earliest();
}


static void alarm_check(void *priv)
{
    extern APP_VAR app_var;
    extern u32 timer_get_ms(void);
    if ((timer_get_ms() - app_var.start_time) > 3000) {
        struct sys_event e;
        e.type = SYS_DEVICE_EVENT;
        e.arg  = (void *)DEVICE_EVENT_FROM_ALM;
        e.u.dev.event = DEVICE_EVENT_IN;
        e.u.dev.value = 0;
        sys_event_notify(&e);
    } else {
        sys_timeout_add(NULL, alarm_check, 100);
    }
}



static void  __alarm_ring_play(void *p)
{
    struct application *cur;
    if (g_alarm_ring_cnt > 0) {
        u8 app = app_get_curr_task();
        if (app != APP_RTC_TASK) {
            alarm_stop();
            return;
        }

        if (!tone_get_status()) {
            tone_play_by_path(tone_table[IDEX_TONE_NORMAL], 0);
            g_alarm_ring_cnt--;
            sys_timeout_add(NULL, __alarm_ring_play, 500);
        } else {
            sys_timeout_add(NULL, __alarm_ring_play, 500);
        }
    }
}

void alarm_ring_start()
{
    if (g_alarm_ring_cnt == 0) {
        g_alarm_ring_cnt = ALARM_RING_MAX;
        sys_timeout_add(NULL, __alarm_ring_play, 500);
    }
}

static void alarm_event_handler(struct sys_event *event, void *priv)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        if (alarm_active_flag_get()) {
            alarm_stop();
        }
        break;
    default:
        break;
    }
}



void alarm_init()
{
    u8 i = 0;
    if (dev_handle) {
        return ;
    }
    dev_handle = dev_open("rtc", NULL);
    if (!dev_handle) {
        ASSERT(0, "%s %d \n", __func__, __LINE__);
    }


    T_ALARM alarm = {0};
    alarm_vm_read_mask();
    if (g_alarm_map == 0) {
        for (i = 0; i < M_MAX_ALARM_NUMS; i++) {
            alarm.index = i;
            alarm_add(&alarm, i);
        }
    }

    alarm_vm_read_info();
    if (!alarm_active_flag_get()) { //判断是否闹钟在响
        alarm_update();
    } else {
        sys_timeout_add(NULL, alarm_check, 100);
    }

    register_sys_event_handler(SYS_ALL_EVENT, 0, NULL, alarm_event_handler);
}

void alm_wakeup_isr(void)
{
    if (!dev_handle) {
        alarm_active_flag_set(true);
    } else {
        alarm_active_flag_set(true);
        struct sys_event e;
        e.type = SYS_DEVICE_EVENT;
        e.arg  = (void *)DEVICE_EVENT_FROM_ALM;
        e.u.dev.event = DEVICE_EVENT_IN;
        e.u.dev.value = 0;
        sys_event_notify(&e);
    }
}

void is_alm_come(void)
{
#if (defined(TCFG_USE_FAKE_RTC) && (TCFG_USE_FAKE_RTC))
    struct sys_time alm_time = {0};
    struct sys_time time = {0};

    if (!dev_handle) {
        return ;
    }

    extern void read_alarm(struct sys_time * alarm_time);
    extern void read_sys_time(struct sys_time * curr_time);
    extern u8 get_alarm_ctrl(void);

    if (!get_alarm_ctrl()) {
        return;
    }

    read_alarm(&alm_time);
    read_sys_time(&time);
    /* dev_ioctl(dev_handle, IOCTL_GET_SYS_TIME, (u32)&time); */
    /* dev_ioctl(dev_handle, IOCTL_GET_ALARM, (u32)&alm_time); */
    if (time.year == alm_time.year && \
        time.month == alm_time.month && \
        time.day == alm_time.day && \
        time.hour == alm_time.hour && \
        time.min == alm_time.min && \
        time.sec == alm_time.sec) {
        alm_wakeup_isr();
        extern void set_alarm_ctrl(u8 set_alarm);
        set_alarm_ctrl(0);
    }
#endif
}


int alarm_sys_event_handler(struct sys_event *event)
{
    struct application *cur;
    if ((u32)event->arg == DEVICE_EVENT_FROM_ALM) {
        if (event->u.dev.event == DEVICE_EVENT_IN) {
            printf("alarm_sys_event_handler\n");
            alarm_update_info_after_isr();

            u8 app = app_get_curr_task();
            if (app == APP_RTC_TASK) {
                alarm_ring_start();
                return false;
            }
            return true;
        }
    }

    return false;
}

#endif  //end of RTC_ALM_EN
#endif


