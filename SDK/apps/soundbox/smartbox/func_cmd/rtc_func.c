#include "smartbox/func_cmd/rtc_func.h"
#include "smartbox/func_cmd_common.h"
#include "smartbox/function.h"
#include "smartbox/config.h"
#include "smartbox/event.h"
#include "app_task.h"
#include "rtc/alarm.h"
#include "app_action.h"

#if (SMART_BOX_EN)

#if TCFG_RTC_ENABLE

#define RTC_INFO_ATTR_RTC_TIME                 (0)
#define RTC_INFO_ATTR_RTC_ALRAM                (1)
#define RTC_INFO_ATTR_RTC_ALRAM_ACTIVE         (2)
#define RTC_INFO_ATTR_RTC_ALRAM_UNACTIVE       (3)

enum {
    E_ALARM_SET = 0x00,
    E_ALARM_DELETE,
    E_ALARM_UNACTIVE,
};

typedef struct __APP_ALARM__ {
    u8 index;
    u8 sw;
    u8 mode;
    u8 bHour;
    u8 bMin;
    u8 name_len;
} T_ALARM_APP, *PT_ALARM_APP;

static u8 smart_rtc_get_alarm_info(PT_ALARM_APP p, u8 index)
{
    extern u8 alarm_get_info(PT_ALARM p, u8 index);
    u8 ret = 0;
    T_ALARM alarm_param;
    ret = alarm_get_info(&alarm_param, index);
    p->index = alarm_param.index;
    p->sw = alarm_param.sw;
    p->mode = alarm_param.mode;
    p->bHour = alarm_param.time.bHour;
    p->bMin = alarm_param.time.bMin;
    p->name_len = alarm_param.name_len;
    return ret;
}

static u8 smartbox_rtc_get_alarm_total(void)
{
    extern u8 alarm_get_total(void);
    u8 total = 0;
    total = alarm_get_total();
    return total;
}

static u8 m_func_alarm_get_active_index(void)
{
    extern u8 alarm_get_active_index(void);
    return alarm_get_active_index();
}

static u8 m_func_alarm_name_get(u8 *p, u8 index)
{
    extern u8 alarm_name_get(u8 * p, u8 index);
    return alarm_name_get(p, index);
}

static void smart_rtc_update_time(RTC_TIME *p)
{
    extern void rtc_update_time_api(struct sys_time * time);
    rtc_update_time_api((struct sys_time *)p);
}

static u8 mfunc_alarm_deal_data(PT_ALARM_APP p)
{
    extern u8 alarm_add(PT_ALARM p, u8 index);
    T_ALARM tmp_alarm 	 = {0};
    tmp_alarm.index 	 = p->index;
    tmp_alarm.sw         = p->sw;
    tmp_alarm.mode       = p->mode;
    tmp_alarm.time.bHour = p->bHour;
    tmp_alarm.time.bMin  = p->bMin;
    tmp_alarm.name_len   = p->name_len;
    return alarm_add(&tmp_alarm, p->index);
}

static void m_func_alarm_name_set(u8 *p, u8 index, u8 len)
{
    extern void alarm_name_set(u8 * p, u8 index, u8 len);
    alarm_name_set(p, index, len);
}

static void m_func_alarm_dealte(u8 index)
{
    extern void alarm_delete(u8 index);
    alarm_delete(index);
}

static u8 smart_rtc_alarm_deal(void *priv, u8 *p, u8 len)
{
    u8 ret = E_SUCCESS;
    u8 op = 0;
    u8 nums = 0;
    u8 index = 0;
    u8 *pTmp = 0;
    u8 i = 0;

    T_ALARM_APP alarm_tab;

    if (len >= 3) {
        op = p[2];
        printf("op = %d\n", op);
    }
    if (len >= 4) {
        nums = p[3];
        printf("nums = %d\n", nums);
    }
    if (nums > M_MAX_ALARM_NUMS) {
        printf("nums is error\n");
        return E_FAILURE;
    }

    switch (op) {
    case E_ALARM_SET:
        printf("E_ALARM_SET\n");
        for (i = 0; i < nums; i++) {
            pTmp = &(p[4 + i * (6 + alarm_tab.name_len)]);
            alarm_tab.index = pTmp[0];
            alarm_tab.sw = pTmp[1];
            alarm_tab.mode = pTmp[2];
            alarm_tab.bHour = pTmp[3];
            alarm_tab.bMin = pTmp[4];
            alarm_tab.name_len = pTmp[5];

            printf("index : %d, sw : %d, mode : %d, hour : %d, min : %d, name_len : %d\n", pTmp[0], pTmp[1], pTmp[2], pTmp[3], pTmp[4], pTmp[5]);
            ret = mfunc_alarm_deal_data(&alarm_tab);
            if (E_SUCCESS == ret) {
                m_func_alarm_name_set(&(pTmp[6]), alarm_tab.index, alarm_tab.name_len);
            }
        }
        break;
    case E_ALARM_DELETE:
        printf("E_ALARM_DELETE\n");
        for (i = 0; i < nums; i++) {
            index = p[4 + i];
            m_func_alarm_dealte(index);
        }
        break;
    case E_ALARM_UNACTIVE:
        printf("E_ALARM_UNACTIVE\n");
        extern void alarm_stop(void);
        alarm_stop();
        //smartbox_msg_post(USER_MSG_SMARTBOX_RTC_UPDATE_STATE, 1, (int)priv);
        smartbox_function_update(RTC_FUNCTION_MASK, BIT(RTC_INFO_ATTR_RTC_ALRAM_UNACTIVE));
        break;
    default:
        printf("alarm no action!\n");
        break;
    }
    return ret;
}

bool rtc_func_set(void *priv, u8 *data, u16 len)
{
    u8 ret = 0;
    u8 offset = 0;
    RTC_TIME time_info;
    while (offset < len) {
        u8 len_tmp = data[offset];
        u8 type = data[offset + 1];
        printf("rtc info:\n");
        put_buf(&data[offset], len_tmp + 1);

        switch (type) {
        case RTC_INFO_ATTR_RTC_TIME:
            printf("RTC_INFO_ATTR_RTC_TIME\n");
            memcpy((u8 *)&time_info, data + 2, sizeof(time_info));
            time_info.dYear = app_htons(time_info.dYear);
            smart_rtc_update_time(&time_info);
            break;
        case RTC_INFO_ATTR_RTC_ALRAM:
            printf("RTC_INFO_ATTR_RTC_ALRAM\n");
            put_buf(data, len);
            ret = smart_rtc_alarm_deal(priv, data, len);
            break;
        }
        offset += len_tmp + 1;
    }
    return (E_SUCCESS == ret);
}

u32 rtc_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    u16 offset = 0;
    if (mask & BIT(RTC_INFO_ATTR_RTC_TIME)) {
        printf("RTC_INFO_ATTR_RTC_TIME\n");
        RTC_TIME time_info = {
            .dYear = 2020,
            .bMonth = 5,
            .bDay = 15,
            .bHour = 19,
            .bMin = 55,
            .bSec = 40,
        };
        offset += add_one_attr(buf, buf_size, offset, RTC_INFO_ATTR_RTC_TIME, (u8 *)&time_info, sizeof(time_info));
    }

    if (mask & BIT(RTC_INFO_ATTR_RTC_ALRAM)) {
        printf("RTC_INFO_ATTR_RTC_ALRAM\n");
        u8 alarm_data[M_MAX_ALARM_NUMS * (sizeof(T_ALARM_APP) + M_MAX_ALARM_NAME_LEN)];
        u8 total = 0;
        u8 index = 0;
        u8 name_len = 0;
        u8 *pTmp;
        u8 data_len = 0;
        u8 ret = 0;

        pTmp = alarm_data;

        total = smartbox_rtc_get_alarm_total();
        printf("total %d alarm!\n", total);

        pTmp[0] = total;

        pTmp++;
        data_len++;

        for (index = 0; index < 5; index++) {
            ret = smart_rtc_get_alarm_info((PT_ALARM_APP)pTmp, index);
            if (0 == ret) {
                pTmp += sizeof(T_ALARM_APP);
                data_len += sizeof(T_ALARM_APP);

                name_len = m_func_alarm_name_get(pTmp, index);
                pTmp += name_len;
                data_len += name_len;
                printf("data_len = %d\n", data_len);
            }
        }

        put_buf(alarm_data, data_len);
        offset += add_one_attr(buf, buf_size, offset, RTC_INFO_ATTR_RTC_ALRAM, alarm_data, data_len);
    }

    if (mask & BIT(RTC_INFO_ATTR_RTC_ALRAM_ACTIVE)) {
        printf("RTC_INFO_ATTR_RTC_ALRAM_ACTIVE\n");
        u8 index_mask = 0;
        index_mask = m_func_alarm_get_active_index();
        printf("active alarm index : %d\n", index_mask);
        offset += add_one_attr(buf, buf_size, offset, RTC_INFO_ATTR_RTC_ALRAM_ACTIVE, (u8 *)&index_mask, sizeof(index_mask));

        extern void alarm_update_info_after_isr(void);
        alarm_update_info_after_isr();
    }

    if (mask & BIT(RTC_INFO_ATTR_RTC_ALRAM_UNACTIVE)) {
        printf("RTC_INFO_ATTR_RTC_ALRAM_UNACTIVE\n");
        offset += add_one_attr(buf, buf_size, offset, RTC_INFO_ATTR_RTC_ALRAM_UNACTIVE, NULL, 0);
    }
    return offset;
}

void smartbot_rtc_msg_deal(int msg)
{
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL) {
        return ;
    }

    switch (msg) {
    case (int)-1 :
        smartbox_function_update(RTC_FUNCTION_MASK, BIT(RTC_INFO_ATTR_RTC_ALRAM_ACTIVE));
        break;
    }
}

#else

u32 rtc_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    return 0;
}

bool rtc_func_set(void *priv, u8 *data, u16 len)
{
    return true;
}

void smartbot_rtc_msg_deal(int msg)
{

}
#endif

#endif
