#ifndef __SMARTBOX_MISC_SETTING__
#define __SMARTBOX_MISC_SETTING__

#include "system/includes.h"

#define REGISTER_APP_MISC_SETTING_OPT(smart_misc_opt) \
int smart_misc_opt##_setting_init(void) {\
   return register_smartbox_setting_misc_setting(&smart_misc_opt);\
}\
late_initcall(smart_misc_opt##_setting_init);

#define SMARTBOX_REVERBERATION_SETTING	1
#define SMARTBOX_DRC_VAL_SETTING		1

enum {
    MISC_SETTING_REVERBERATION = 0,
    MISC_SETTING_DRC_VAL = 1,
    FUNCTION_MASK_MAX = 32, // 最多只支持32个
};

typedef struct _SMARTBOX_MISC_SETTING_OPT {
    struct _SMARTBOX_MISC_SETTING_OPT *next;
    u32 misc_data_len;
    int misc_setting_type;
    int misc_syscfg_id;

    // misc_set_setting函数参数：
    // misc_setting : 传入数据
    // is_conversion : 1 - 需要转换，从大端转为小端
    // 				   0 - 不需要转换
    int (*misc_set_setting)(u8 *misc_setting, u8 is_conversion);

    // misc_get_setting函数参数：
    // misc_setting : 传出数据
    // is_conversion : 1 - 需要转换，从小端转为大端
    // 				   0 - 不需要转换
    int (*misc_get_setting)(u8 *misc_setting, u8 is_conversion);
    int (*misc_state_update)(void);

    // 上面是必填，下面是选填
    int (*misc_custom_setting_init)(void);
    int (*misc_custom_write_vm)(u8 *misc_setting);
} SMARTBOX_MISC_SETTING_OPT;

int register_smartbox_setting_misc_setting(void *misc_setting);
#endif
