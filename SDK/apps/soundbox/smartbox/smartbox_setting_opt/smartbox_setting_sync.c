#include "app_config.h"
#include "smartbox_setting_sync.h"
#include "smartbox_adv_common.h"
#include "smartbox_setting_opt.h"
#include "adv_time_stamp_setting.h"

#if (TCFG_EQ_ENABLESMART_BOX_EN && TCFG_USER_TWS_ENABLE)
static void smartbox_sync_tws_func_t(void *data, u16 len, bool rx)
{
    if (rx) {
        deal_sibling_setting((u8 *)data);
    }
}

REGISTER_TWS_FUNC_STUB(adv_tws_sync) = {
    .func_id = TWS_FUNC_ID_ADV_SETTING_SYNC,
    .func    = smartbox_sync_tws_func_t,
};

#if 1//RCSP_SMARTBOX_ADV_EN
static void adv_sync_time_stamp_func_t(void *data, u16 len, bool rx)
{
    if (rx) {
        deal_sibling_time_stamp_setting_switch(data, len);
    }
}

REGISTER_TWS_FUNC_STUB(adv_time_stamp_sync) = {
    .func_id = TWS_FUNC_ID_TIME_STAMP_SYNC,
    .func    = adv_sync_time_stamp_func_t,
};


static void adv_sync_reset_sync_func_t(int args)
{
    extern void cpu_reset();
    cpu_reset();
}

TWS_SYNC_CALL_REGISTER(adv_reset_sync) = {
    .uuid = TWS_FUNC_ID_ADV_RESET_SYNC,
    .func    = adv_sync_reset_sync_func_t,
};
#endif

#endif
