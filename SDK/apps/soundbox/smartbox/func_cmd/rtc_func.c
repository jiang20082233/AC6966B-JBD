#include "smartbox/func_cmd/rtc_func.h"
#include "smartbox/func_cmd_common.h"
#include "smartbox/function.h"
#include "smartbox/config.h"
#include "smartbox/event.h"
#include "app_task.h"
#include "rtc/alarm.h"
#include "app_action.h"
#include "tone_player.h"

#include "smartbox_music_info_setting.h"
#include "btstack/avctp_user.h"
#include "JL_rcsp_packet.h"
#include "key_event_deal.h"
#include "linein/linein.h"
#include "music_player.h"
#include "app_msg.h"

#if (SMART_BOX_EN)

#if TCFG_RTC_ENABLE

#define RTC_INFO_ATTR_RTC_TIME                 (0)
#define RTC_INFO_ATTR_RTC_ALRAM                (1)
#define RTC_INFO_ATTR_RTC_ALRAM_ACTIVE         (2)
#define RTC_INFO_ATTR_RTC_ALRAM_UNACTIVE       (3)
#define RTC_INFO_ATTR_RTC_ALRAM_STRUCTURE      (4)
#define RTC_INFO_ATTR_RTC_ALRAM_DEFAULT_RING   (5)
#define RTC_INFO_ATTR_RTC_ALRAM_RING_AUDITION  (6)

enum {
    E_ALARM_SET = 0x00,
    E_ALARM_DELETE,
    E_ALARM_UNACTIVE,
};

#pragma pack(1)
typedef struct __APP_ALARM__ {
    u8 index;
    u8 sw;
    u8 mode;
    u8 bHour;
    u8 bMin;
    u8 name_len;
} T_ALARM_APP, *PT_ALARM_APP;

typedef struct __APP_ALARM_EXTRA_DATA__ {
    u8 type;
    u8 dev;
    u32 clust;
    u8 ring_name_len;
    u8 ring_name[16];
} T_ALARM_APP_EXTRA_DATA, *PT_ALARM_APP_EXTRA_DATA;

typedef struct __APP_ALARM_RING_AUDITION__ {
    u8 prev_app_mode;
    u8 ring_op;
    u8 ring_type;
    u8 ring_dev;
    u32 ring_clust;
} T_ALARM_APP_RING_AUDITION, *PT_ALARM_APP_RING_AUDITION;
#pragma pack()

enum {
    ALARM_IDEX_TONE_NUM_0 = 0,
    ALARM_IDEX_TONE_NUM_1 = 1,
    ALARM_IDEX_TONE_NUM_2 = 2,
    ALARM_IDEX_TONE_NUM_3 = 3,
    ALARM_IDEX_TONE_NUM_4 = 4,
    ALARM_IDEX_TONE_NUM_5 = 5,
    ALARM_IDEX_TONE_NUM_6 = 6,
    ALARM_IDEX_TONE_NUM_7 = 7,
    ALARM_IDEX_TONE_NUM_8 = 8,
    ALARM_IDEX_TONE_NUM_9 = 9,

    ALARM_IDEX_TONE_MAX_NUM = 10,
};
static const char *default_ringtone_table[] = {
    [ALARM_IDEX_TONE_NUM_0] 			= TONE_RES_ROOT_PATH"tone/0.*",
    [ALARM_IDEX_TONE_NUM_1] 			= TONE_RES_ROOT_PATH"tone/1.*",
    [ALARM_IDEX_TONE_NUM_2] 			= TONE_RES_ROOT_PATH"tone/2.*",
    [ALARM_IDEX_TONE_NUM_3] 			= TONE_RES_ROOT_PATH"tone/3.*",
    [ALARM_IDEX_TONE_NUM_4] 			= TONE_RES_ROOT_PATH"tone/4.*",
    [ALARM_IDEX_TONE_NUM_5] 			= TONE_RES_ROOT_PATH"tone/5.*",
    [ALARM_IDEX_TONE_NUM_6] 			= TONE_RES_ROOT_PATH"tone/6.*",
    [ALARM_IDEX_TONE_NUM_7] 			= TONE_RES_ROOT_PATH"tone/7.*",
    [ALARM_IDEX_TONE_NUM_8] 			= TONE_RES_ROOT_PATH"tone/8.*",
    [ALARM_IDEX_TONE_NUM_9] 			= TONE_RES_ROOT_PATH"tone/9.*",
};

static const char *default_ringtone_name_table[] = {
    [ALARM_IDEX_TONE_NUM_0] 			= "提示音0",
    [ALARM_IDEX_TONE_NUM_1] 			= "1",
    [ALARM_IDEX_TONE_NUM_2] 			= "2",
    [ALARM_IDEX_TONE_NUM_3] 			= "3",
    [ALARM_IDEX_TONE_NUM_4] 			= "4",
    [ALARM_IDEX_TONE_NUM_5] 			= "5",
    [ALARM_IDEX_TONE_NUM_6] 			= "6",
    [ALARM_IDEX_TONE_NUM_7] 			= "7",
    [ALARM_IDEX_TONE_NUM_8] 			= "8",
    [ALARM_IDEX_TONE_NUM_9] 			= "9",
};

static u8 rtc_func_structure_flag = 1;
static T_ALARM_APP_RING_AUDITION g_ring_audition = {
    .prev_app_mode = -1,
};

static u8 smart_rtc_get_alarm_info(PT_ALARM_APP p, u8 index)
{
    extern u8 alarm_get_info(PT_ALARM p, u8 index);
    u8 ret = 0;
    T_ALARM alarm_param;
    ret = alarm_get_info(&alarm_param, index);
    p->index = alarm_param.index;
    p->sw = alarm_param.sw;
    p->mode = alarm_param.mode;
    p->bHour = alarm_param.time.hour;
    p->bMin = alarm_param.time.min;
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
    struct sys_time time = {0};
    time.year = p->dYear;
    time.month = p->bMonth;
    time.day = p->bDay;
    time.hour = p->bHour;
    time.min = p->bMin;
    time.sec = p->bSec;
    rtc_update_time_api(&time);
}

static u8 mfunc_alarm_deal_data(PT_ALARM_APP p)
{
    extern u8 alarm_add(PT_ALARM p, u8 index);
    T_ALARM tmp_alarm 	 = {0};
    tmp_alarm.index 	 = p->index;
    tmp_alarm.sw         = p->sw;
    tmp_alarm.mode       = p->mode;
    tmp_alarm.time.hour  = p->bHour;
    tmp_alarm.time.min   = p->bMin;
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

static u8 smart_rtc_alarm_extra_data_set(u8 index, u8 *p, u8 len)
{
    u8 offset = 0;
    T_ALARM_APP_EXTRA_DATA data = {0};
    data.type = p[offset++];
    data.dev = p[offset++];
    data.clust = READ_BIG_U32(p + offset);
    offset += sizeof(data.clust);
    data.ring_name_len = p[offset++];
    memcpy(data.ring_name, p + offset, data.ring_name_len);
    offset += data.ring_name_len;
    syscfg_write(VM_ALARM_RING_NAME_0 + index, &data, sizeof(data));
    return offset;
}

static u8 smart_rtc_alarm_deal(void *priv, u8 *p, u8 len)
{
    u8 ret = E_SUCCESS;
    u8 op = 0;
    u8 nums = 0;
    u8 index = 0;
    u8 *pTmp = 0;
    u8 i = 0;
    u8 ring_info_offset = 0;

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
            pTmp = &(p[4 + i * (6 + alarm_tab.name_len) + ring_info_offset]);
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

            if (rtc_func_structure_flag) {
                ring_info_offset += smart_rtc_alarm_extra_data_set(i, &pTmp[6 + alarm_tab.name_len], len);
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
        smartbox_function_update(RTC_FUNCTION_MASK, BIT(RTC_INFO_ATTR_RTC_ALRAM_UNACTIVE));
        break;
    default:
        printf("alarm no action!\n");
        break;
    }
    return ret;
}

static u8 smartbox_rtc_play_dev_ring(PT_ALARM_APP_RING_AUDITION ring_param)
{
    u8 ret = 0;
    // 设备音乐响铃，还没实现!!
    printf("neet to play the dev music, clust %x\n", ring_param->ring_clust);
    return ret;
}

static u8 smart_rtc_ring_audition_deal(PT_ALARM_APP_RING_AUDITION ring_param)
{
    u8 ret = 0;
    switch (ring_param->ring_op) {
    case 0:
        printf("ring stop");
        if (0 == ring_param->ring_type) {
            // 停止提示音
            tone_play_stop_by_path(default_ringtone_table[ring_param->ring_clust]);
        } else if (1 == ring_param->ring_type) {
            // 停止设备音乐,还没实现!!
            printf("need to stop the dev music, clust = %x\n", ring_param->ring_clust);
        }
        break;
    case 1:
        printf("ringing\n");
        if (0 == ring_param->ring_type) {
            tone_play_by_path(default_ringtone_table[ring_param->ring_clust], 0);
        } else if (1 == ring_param->ring_type) {
            // 播放设备音乐
            smartbox_rtc_play_dev_ring(ring_param);
        }
        break;
    }
    return ret;
}

static u8 smart_rtc_ring_audition_prepare(void *priv, u8 *data, u16 len)
{
    u8 ret = 0;
    struct smartbox *smart = (struct smartbox *)priv;
    g_ring_audition.ring_op = data[0];
    switch (g_ring_audition.ring_op) {
    case 0:
        // 回到原来的模式
        if ((u8) - 1 != g_ring_audition.prev_app_mode) {
            smartbox_msg_post(USER_MSG_SMARTBOX_MODE_SWITCH, 2, (int)smart, g_ring_audition.prev_app_mode);
            g_ring_audition.prev_app_mode = -1;
        } else {
            smart_rtc_ring_audition_deal(&g_ring_audition);
        }
        break;
    case 1:
        g_ring_audition.ring_type = data[1];
        g_ring_audition.ring_dev = data[2];
        g_ring_audition.ring_clust = READ_BIG_U32(data + 3);
        // 进入rtc模式
        if (RTC_FUNCTION != smart->cur_app_mode) {
            g_ring_audition.prev_app_mode = smart->cur_app_mode;
            // 切换rtc模式
            smartbox_msg_post(USER_MSG_SMARTBOX_MODE_SWITCH, 2, (int)smart, RTC_FUNCTION_MASK);
        } else {
            smart_rtc_ring_audition_deal(&g_ring_audition);
        }
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
        case RTC_INFO_ATTR_RTC_ALRAM_RING_AUDITION:
            printf("RTC_INFO_ATTR_RTC_ALRAM_RING_AUDITION\n");
            ret = smart_rtc_ring_audition_prepare(priv, data + offset + 2, len);
        }
        offset += len_tmp + 1;
    }
    return (E_SUCCESS == ret);
}

static u8 smart_rtc_alarm_extra_data_get(u8 *p, u8 index, u8 is_conversion)
{
    u32 offset = 0;
    T_ALARM_APP_EXTRA_DATA data = {0};
    if (sizeof(data) != syscfg_read(VM_ALARM_RING_NAME_0 + index, &data, sizeof(data))) {
        // 默认数据
        data.type = 0;
        data.dev = 0;
        data.clust = 0;
        memcpy(data.ring_name, default_ringtone_name_table[data.clust], strlen(default_ringtone_name_table[data.clust]) + 1);
        data.ring_name_len = strlen(data.ring_name) + 1;
    }
    p[offset++] = data.type;
    p[offset++] = data.dev;
    if (is_conversion) {
        WRITE_BIG_U32(p + offset, data.clust);
    } else {
        memcpy(p + offset, &data.clust, sizeof(data.clust));
    }
    offset += sizeof(data.clust);
    p[offset++] = data.ring_name_len;
    memcpy(p + offset, data.ring_name, data.ring_name_len);
    offset += data.ring_name_len;
    return offset;
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
        u8 alarm_data[M_MAX_ALARM_NUMS * (sizeof(T_ALARM_APP) + M_MAX_ALARM_NAME_LEN + sizeof(T_ALARM_APP_EXTRA_DATA))];
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

                if (rtc_func_structure_flag) {
                    name_len = smart_rtc_alarm_extra_data_get(pTmp, index, 1);
                    pTmp += name_len;
                    data_len += name_len;
                }

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

        /* extern void alarm_update_info_after_isr(void); */
        /* alarm_update_info_after_isr(); */
    }

    if (mask & BIT(RTC_INFO_ATTR_RTC_ALRAM_UNACTIVE)) {
        printf("RTC_INFO_ATTR_RTC_ALRAM_UNACTIVE\n");
        offset += add_one_attr(buf, buf_size, offset, RTC_INFO_ATTR_RTC_ALRAM_UNACTIVE, NULL, 0);
    }

    if (mask & BIT(RTC_INFO_ATTR_RTC_ALRAM_STRUCTURE)) {
        printf("RTC_INFO_ATTR_RTC_ALRAM_STRUCTURE\n");
        offset += add_one_attr(buf, buf_size, offset, RTC_INFO_ATTR_RTC_ALRAM_STRUCTURE, &rtc_func_structure_flag, sizeof(rtc_func_structure_flag));
    }

    if (mask & BIT(RTC_INFO_ATTR_RTC_ALRAM_DEFAULT_RING)) {
        printf("RTC_INFO_ATTR_RTC_ALRAM_DEFAULT_RING\n");
        u8 i;
        // total + index
        u8 ring_info_len = 1 + ALARM_IDEX_TONE_MAX_NUM;
        for (i = 0; i < ALARM_IDEX_TONE_MAX_NUM; i++) {
            // name_len + name_data
            ring_info_len += 1 + strlen(default_ringtone_name_table[i]) + 1;
        }

        u8 default_offset = 0;
        u8 ring_name_len = 0;
        u8 *default_ring = zalloc(ring_info_len);
        default_ring[default_offset++] = ALARM_IDEX_TONE_MAX_NUM;
        for (i = 0; i < ALARM_IDEX_TONE_MAX_NUM; i++) {
            ring_name_len = strlen(default_ringtone_name_table[i]) + 1;
            default_ring[default_offset++] = i;
            default_ring[default_offset++] = ring_name_len;
            if (ring_name_len) {
                memcpy(default_ring + default_offset, default_ringtone_name_table[i], ring_name_len);
            }
            default_offset += ring_name_len;
        }

        offset += add_one_attr(buf, buf_size, offset, RTC_INFO_ATTR_RTC_ALRAM_DEFAULT_RING, default_ring, ring_info_len);
        if (default_ring) {
            free(default_ring);
        }

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

u8 rtc_app_alarm_ring_play(u8 alarm_state)
{
    // 防止g_ring_audition成为临界资源，所以定义多一个局部变量
    static T_ALARM_APP_RING_AUDITION ringing_info = {0};
    u8 index;
    u8 active_flag = alarm_active_flag_get();
    if (0 == active_flag && 0 == rtc_func_structure_flag) {
        return rtc_func_structure_flag;
    }

    if (0 == alarm_state) {
        memset(&ringing_info, 0, sizeof(ringing_info));
        return rtc_func_structure_flag;
    }

    for (index = 0; index < 5; index++) {
        if (active_flag & BIT(index)) {
            break;
        }
    }

    if (index < 5) {
        if (0 == ringing_info.prev_app_mode) {
            // 响铃会进入始终模式，这个时全局变量prev_app_mode应该清除
            g_ring_audition.prev_app_mode = -1;
            // 暂停正在播放的提示音或设备音乐
            g_ring_audition.ring_op = 0;
            smart_rtc_ring_audition_deal(&g_ring_audition);

            T_ALARM_APP_EXTRA_DATA data = {0};
            u32 data_len = 0;
            data_len = smart_rtc_alarm_extra_data_get(&data, index, 0);

            ringing_info.prev_app_mode = -1;
            ringing_info.ring_op = 1;
            ringing_info.ring_type = data.type;
            ringing_info.ring_dev = data.dev;
            ringing_info.ring_clust = data.clust;
        }

        if (ringing_info.ring_type) {
            // 播放设备音乐
            smartbox_rtc_play_dev_ring(&ringing_info);
        } else {
            tone_play_by_path(default_ringtone_table[ringing_info.ring_clust], 0);
        }
    }
    return rtc_func_structure_flag;
}

u8 smartbox_rtc_ring_tone(void)
{
    u8 ret = true;
    if ((u8) - 1 != g_ring_audition.prev_app_mode) {
        smart_rtc_ring_audition_deal(&g_ring_audition);
        ret = false;
    }
    return ret;
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

u8 smartbox_rtc_ring_tone(void)
{
    return true;
}
#endif

#endif
