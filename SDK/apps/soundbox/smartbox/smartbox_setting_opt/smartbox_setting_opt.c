#include "app_config.h"

#include "smartbox_setting_opt.h"
#include "smartbox_setting_sync.h"
#include "btstack/avctp_user.h"
#include "bt_tws.h"

#include "smartbox_bt_manage.h"
#include "smartbox_rcsp_manage.h"

#if (SMART_BOX_EN)

static SMARTBOX_SETTING_OPT *g_opt_link_head = NULL;

int register_smartbox_setting_opt_setting(void *opt_param)
{
    SMARTBOX_SETTING_OPT *opt = g_opt_link_head;
    SMARTBOX_SETTING_OPT *item = (SMARTBOX_SETTING_OPT *)opt_param;
    while (opt && item) {
        if (opt->setting_type == item->setting_type) {
            return 0;
        }
        opt = opt->next;
    }

    if (item) {
        item->next = g_opt_link_head;
        g_opt_link_head = item;
        return 0;
    }

    return -1;
}

static u32 setting_event_flag = (u32) - 1;
static void set_setting_event_flag(u32 flag)
{
    setting_event_flag = flag;
}

static u32 get_setting_event_flag(void)
{
    return setting_event_flag;
}

static u8 deal_setting_string_item(u8 *des, u8 *src, u8 src_len, u8 type)
{
    des[0] = type;
    memcpy(des + 1, src, src_len);
    return src_len + sizeof(type);
}

void update_info_from_vm_info(void)
{
    SMARTBOX_SETTING_OPT *opt = g_opt_link_head;
    while (opt) {
        if (get_setting_event_flag() & BIT(opt->setting_type)) {
            if (opt->custom_vm_info_update) {
                opt->custom_vm_info_update();
            } else if (opt->deal_opt_setting) {
                opt->deal_opt_setting(NULL, 1, 0);
            }
        }
        opt = opt->next;
    }
    set_setting_event_flag(0);
}

u8 smartbox_read_data_from_vm(u8 syscfg_id, u8 *buf, u8 buf_len)
{
    int len = 0;
    u8 i = 0;

    len = syscfg_read(syscfg_id, buf, buf_len);

    if (len > 0) {
        for (i = 0; i < buf_len; i++) {
            if (buf[i] != 0xff) {
                return (buf_len == len);
            }
        }
    }

    return 0;
}

static u32 g_opt_total_size = 0;
void smart_setting_init(void)
{
    SMARTBOX_SETTING_OPT *opt = g_opt_link_head;
    u32 opt_total_size = 0;
    while (opt) {
        u8 *data = zalloc(opt->data_len);
        if (opt->custom_setting_init) {
            opt->custom_setting_init();
        } else if (smartbox_read_data_from_vm(opt->syscfg_id, data, opt->data_len)) {
            if (opt->set_setting && opt->deal_opt_setting) {
                opt->set_setting(data);
                opt->deal_opt_setting(NULL, 0, 0);
            }
        }

        if (data) {
            free(data);
        }
        opt_total_size += (opt->data_len + 1);
        opt = opt->next;
    }

    if (opt_total_size) {
        g_opt_total_size = opt_total_size + 1;
    }
}

// type : 0 ~ 8
// mode : 0 - 从vm读出并更新全局变量数据 // 1 - 同步
void update_smartbox_setting(u16 type)
{
    u8 offset = 1;
    u8 *setting_to_sync = zalloc(g_opt_total_size);
    SMARTBOX_SETTING_OPT *opt = g_opt_link_head;

    while (opt) {
        if (type & BIT(opt->setting_type)) {
            u8 *data = zalloc(opt->data_len);
            if (opt->custom_setting_update)	{
                opt->custom_setting_update(data);
            } else if (opt->get_setting) {
                opt->get_setting(data);
            }
            offset += deal_setting_string_item(setting_to_sync + offset, data, opt->data_len, opt->setting_type);
            if (data) {
                free(data);
            }
        }
        opt = opt->next;
    }

    if (offset > 1) {
        setting_to_sync[0] = offset;
        if (type == ((u16) - 1)) {
            tws_api_send_data_to_sibling(setting_to_sync, g_opt_total_size, TWS_FUNC_ID_ADV_SETTING_SYNC);
            //tws_api_sync_call_by_uuid('T', SYNC_CMD_APP_RESET_LED_UI, 300);
        } else {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                tws_api_send_data_to_sibling(setting_to_sync, g_opt_total_size, TWS_FUNC_ID_ADV_SETTING_SYNC);
                /*     tws_api_sync_call_by_uuid('T', SYNC_CMD_APP_RESET_LED_UI, 300); */
            }
        }
    }

    if (setting_to_sync) {
        free(setting_to_sync);
    }
}

void deal_sibling_setting(u8 *buf)
{
    u8 type;
    u8 len = buf[0];
    u8 offset = 1;
    u8 *data;
    set_setting_event_flag(0);
    while (offset < len) {
        type = buf[offset++];
        data = buf + offset;
        SMARTBOX_SETTING_OPT *opt = g_opt_link_head;
        while (opt) {
            if (type == opt->setting_type) {
                if (opt->custom_sibling_setting_deal) {
                    opt->custom_sibling_setting_deal(data);
                } else if (opt->set_setting) {
                    opt->set_setting(data);
                }
                offset += opt->data_len;
                set_setting_event_flag(get_setting_event_flag() | BIT(type));
                break;
            } else {
                return;
            }
            opt = opt->next;
        }
    }
    // 发送事件
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_ADV_SETTING_UPDATE, NULL, 0);
}

void set_smartbox_opt_setting(u16 setting_type, u8 *data)
{
    SMARTBOX_SETTING_OPT *opt = g_opt_link_head;
    while (opt) {
        if (opt->setting_type == setting_type && opt->deal_opt_setting) {
            opt->deal_opt_setting(data, 1, 1);
        }
        opt = opt->next;
    }
}

void get_smartbox_opt_setting(u16 setting_type, u8 *data)
{
    SMARTBOX_SETTING_OPT *opt = g_opt_link_head;
    while (opt) {
        if (opt->setting_type == setting_type && opt->get_setting) {
            opt->get_setting(data);
        }
        opt = opt->next;
    }
}

void smartbox_opt_release(void)
{
    SMARTBOX_SETTING_OPT *opt = g_opt_link_head;
    while (opt) {
        if (opt->custom_setting_release) {
            opt->custom_setting_release();
        }
    }
}

#if 1//RCSP_SMARTBOX_ADV_EN
void modify_bt_name_and_reset(u32 msec)
{
    tws_api_sync_call_by_uuid(TWS_FUNC_ID_ADV_RESET_SYNC, 0, msec);
}
#endif

#endif
