#include "key_driver.h"
#include "slidekey.h"
#include "gpio.h"
#include "system/event.h"
#include "app_config.h"
#include "asm/clock.h"
#include "app_action.h"

#if (TCFG_SLIDE_KEY_ENABLE)

#define MAX_CH   10
static const struct slidekey_platform_data *__this = NULL;

static u32 slide_old_ad[MAX_CH];

struct key_driver_para slidekey_scan_para = {
    .scan_time 	  	  = 300,
    .last_key 		  = NO_KEY,
    .filter_time  	  = 4,
    .long_time 		  = 75,
    .hold_time 		  = (75 + 15),
    .click_delay_time = 20,
    .key_type		  = KEY_DRIVER_TYPE_SLIDEKEY,
    .get_value 		  = slide_get_key_value,
};

u8 slide_get_key_value(void)
{
    u8 i = 0;
    u32 ad_value_cur = 0;
    u32 offset;
    u32 level = 0;

    if (!__this->enable) {
        return NO_KEY;
    }

    for (i = 0; i < __this->num; i++) {
        ad_value_cur = adc_get_value(__this->port[i].ad_channel);
        //printf("nl = %d,nd = %d\n",slide_now_level[i],adc_get_value(__this->port[i].ad_channel));
        if (ad_value_cur > slide_old_ad[i]) {
            offset = ad_value_cur - slide_old_ad[i];
        } else {
            offset = slide_old_ad[i] - ad_value_cur;
        }
        level = 1023 / (__this->port[i].level + 1);
        if (offset > level / 2) {
            slide_old_ad[i] = ad_value_cur;
            level =  slide_old_ad[i] / level;
            if (level > __this->port[i].level) {
                level = __this->port[i].level;
            }
            y_printf("key\n");
            app_task_msg_post(__this->port[i].msg, 1, level);
        }

    }
    return NO_KEY;
}



int slidekey_init(const struct slidekey_platform_data *slidekey_data)
{
    u8 i = 0;

    __this = slidekey_data;
    if (__this == NULL) {
        return -EINVAL;
    }
    //if(!__this->enable)
    //{
//
    //}
    y_printf("slide key init\n");
    for (i = 0; i < __this->num; i++) {
        adc_add_sample_ch(__this->port[i].ad_channel);

        gpio_set_die(__this->port[i].io, 0);
        gpio_set_direction(__this->port[i].io, 1);
        gpio_set_pull_down(__this->port[i].io, 0);
        if (__this->port[i].io_up_en) {
            gpio_set_pull_up(__this->port[i].io, 1);
        } else {
            gpio_set_pull_up(__this->port[i].io, 0);
        }
    }
    return 0;
}


#endif  /* #if TCFG_SLIDE_KEY_ENABLE */

