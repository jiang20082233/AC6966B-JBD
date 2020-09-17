#include "app_config.h"
#include "syscfg_id.h"
#include "smartbox_misc_setting.h"

#include "smartbox_setting_sync.h"
#include "smartbox_setting_opt.h"
#include "le_smartbox_module.h"

#if (SMART_BOX_EN)

extern int get_bt_tws_connect_status();

static int misc_setting_init(void);
static void get_misc_setting_info(u8 *misc_setting);
static void set_misc_setting_info(u8 *misc_setting);
static void deal_misc_setting(u8 *misc_setting, u8 write_vm, u8 tws_sync);

static SMARTBOX_SETTING_OPT misc_setting_opt = {
    .data_len = 4, // 长度为0，zalloc会发送异常，初始值是mask的长度
    .setting_type = ATTR_TYPE_MISC_SETTING,
    .deal_opt_setting = deal_misc_setting,
    .set_setting = set_misc_setting_info,
    .get_setting = get_misc_setting_info,
    .custom_setting_init = misc_setting_init,
};
REGISTER_APP_SETTING_OPT(misc_setting_opt);

static SMARTBOX_MISC_SETTING_OPT *g_misc_opt_link_head = NULL;

int register_smartbox_setting_misc_setting(void *misc_setting)
{
    // 需要排序
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    SMARTBOX_MISC_SETTING_OPT *item = (SMARTBOX_SETTING_OPT *)misc_setting;
    SMARTBOX_MISC_SETTING_OPT *existed_item = NULL;
    while (misc_opt && item) {
        if (misc_opt->misc_setting_type == item->misc_setting_type) {
            return 0;
        } else if (item->misc_setting_type > misc_opt->misc_setting_type) {
            existed_item = misc_opt;
        }
        misc_opt = misc_opt->next;
    }

    if (!g_misc_opt_link_head) {
        item->next = g_misc_opt_link_head;
        g_misc_opt_link_head = item;
        return 0;
    } else if (item && existed_item) {
        item->next = existed_item->next;
        existed_item->next = item;
    }

    return -1;
}

static int misc_setting_init(void)
{
    u32 total_len = sizeof(u32); // 1个mask的长度
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        u8 *data = zalloc(misc_opt->misc_data_len);
        if (misc_opt->misc_custom_setting_init) {
            misc_opt->misc_custom_setting_init();
        } else if (smartbox_read_data_from_vm(misc_opt->misc_syscfg_id, data, misc_opt->misc_data_len)) {
            if (misc_opt->misc_set_setting) {
                misc_opt->misc_set_setting(data, 0);
            }
        }
        if (data) {
            free(data);
        }
        total_len += misc_opt->misc_data_len;
        misc_opt = misc_opt->next;
    }
    if (g_misc_opt_link_head && g_misc_opt_link_head != misc_opt) {
        misc_setting_opt.data_len = total_len;
        deal_misc_setting(NULL, 0, 0);
    }
    return 0;
}

static void get_misc_setting_info(u8 *misc_setting)
{
    u32 mask = 0;
    u32 offset = sizeof(mask);
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        if (misc_opt->misc_get_setting) {
            misc_opt->misc_get_setting(misc_setting + offset, 1);
        }
        offset += misc_opt->misc_data_len;
        mask |= BIT(misc_opt->misc_setting_type);
        misc_opt = misc_opt->next;
    }

    misc_setting[0] = (mask >> 24) & 0xFF;
    misc_setting[1] = (mask >> 16) & 0xFF;
    misc_setting[2] = (mask >> 8) & 0xFF;
    misc_setting[3] = (mask) & 0xFF;

}

static void set_misc_setting_info(u8 *misc_setting)
{
    u32 offset = 0;
    u32 mask = (misc_setting[offset++] << 24 | misc_setting[offset++] << 16 | misc_setting[offset++] << 8 | misc_setting[offset++]);
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        if (mask & BIT(misc_opt->misc_setting_type)) {
            if (misc_opt->misc_set_setting) {
                misc_opt->misc_set_setting(misc_setting + offset, 1);
            }
            offset += misc_opt->misc_data_len;
        }
        misc_opt = misc_opt->next;
    }
}

static int write_misc_setting_vm_value(int syscfg_id, u8 *data, u32 data_len)
{
    if (!data_len) {
        goto end;
    }
    u8 *vm_data = zalloc(data_len);
    syscfg_read(syscfg_id, vm_data, data_len);
    if (memcmp(vm_data, data, data_len)) {
        syscfg_write(syscfg_id, data, data_len);
    }
    if (vm_data) {
        free(vm_data);
    }
end:
    return 0;
}

static void update_misc_setting_vm_value(u8 *misc_setting)
{
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        if (misc_opt->misc_custom_write_vm) {
            misc_opt->misc_custom_write_vm(misc_setting);
        } else if (misc_opt->misc_syscfg_id && misc_opt->misc_get_setting) {
            u8 *data = zalloc(misc_opt->misc_data_len);
            misc_opt->misc_get_setting(data, 0);
            write_misc_setting_vm_value(misc_opt->misc_syscfg_id, data, misc_opt->misc_data_len);
            if (data) {
                free(data);
            }
        }
        misc_opt = misc_opt->next;
    }
}

static void misc_setting_sync(u8 *misc_setting)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_smartbox_setting(BIT(ATTR_TYPE_MISC_SETTING));
    }
#endif
}

static void misc_setting_state_update(void)
{
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        if (misc_opt->misc_state_update) {
            misc_opt->misc_state_update();
        }
        misc_opt = misc_opt->next;
    }
}

static void deal_misc_setting(u8 *misc_setting, u8 write_vm, u8 tws_sync)
{
    if (misc_setting) {
        set_misc_setting_info(misc_setting);
    }
    if (write_vm) {
        update_misc_setting_vm_value(misc_setting);
    }
    if (tws_sync) {
        misc_setting_sync(misc_setting);
    }
    misc_setting_state_update();
}

u32 get_misc_setting_data_len(void)
{
    return misc_setting_opt.data_len;
}


#else
u32 get_misc_setting_data_len(void)
{
    return 0;
}

#endif
