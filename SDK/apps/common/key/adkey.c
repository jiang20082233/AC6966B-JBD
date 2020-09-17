#include "board_config.h"
#include "key_driver.h"
#include "adkey.h"
#include "gpio.h"
#include "system/event.h"
#include "app_config.h"


#if TCFG_ADKEY_ENABLE

#if (KEY_AD_NUM_MAX!=ADKEY_MAX_NUM)
#error adkey 两个宏buxiangdeng
#endif

static const struct adkey_platform_data *__this = NULL;

u8 ad_get_key_value(void);
//按键驱动扫描参数列表
struct key_driver_para adkey_scan_para = {
    .scan_time 	  	  = 10,				//按键扫描频率, 单位: ms
    .last_key 		  = NO_KEY,  		//上一次get_value按键值, 初始化为NO_KEY;
    .filter_time  	  = 2,				//按键消抖延时;
    .long_time 		  = 75,  			//按键判定长按数量
    .hold_time 		  = (75 + 15),  	//按键判定HOLD数量
    .click_delay_time = 20,				//按键被抬起后等待连击延时数量
    .key_type		  = KEY_DRIVER_TYPE_AD,
    .get_value 		  = ad_get_key_value,
};
u8 user_adkey_mapping(u8 key){
    u8 tp = key;    
#if (defined(USER_ADKEY_MAPPING_EN) && USER_ADKEY_MAPPING_EN)    

    u8 ad_key_table[][2]={
        {0,3},//mode、tws
        {2,5},//LED OFF、RGB mode
        {4,8},//eq
        {5,1},//+
        {7,2},//-
        {8,0},//pp
    };
    if(NO_KEY == tp)return tp;

    for(int i = 0;i<(sizeof(ad_key_table)/sizeof(ad_key_table[0]));i++){
        tp = key;
        if(ad_key_table[i][0] == tp){
            tp = ad_key_table[i][1];
            // printf(">>>>>> key %d %d\n",key,tp);tp = NO_KEY;
            break;
        }else{
            tp = NO_KEY;
        }
    }
#endif
    return tp;
}
u8 ad_get_key_value(void)
{
    u8 i;
    u16 ad_data;

    if (!__this->enable) {
        return NO_KEY;
    }

    /* ad_data = adc_get_voltage(__this->ad_channel); */
    ad_data = adc_get_value(__this->ad_channel);
    // printf("ad_value = %d \n", ad_data); 
    //return NO_KEY;
    for (i = 0; i < ADKEY_MAX_NUM; i++) {
        if ((ad_data <= __this->ad_value[i]) && (__this->ad_value[i] < 0x3ffL)) {
            #if (defined(USER_ADKEY_MAPPING_EN) && USER_ADKEY_MAPPING_EN)
            return user_adkey_mapping(__this->key_value[i]);
            #else
            return __this->key_value[i];
            #endif
        }
    }
    return NO_KEY;
}

int adkey_init(const struct adkey_platform_data *adkey_data)
{
    __this = adkey_data;
    if (!__this) {
        return -EINVAL;
    }

    if (!__this->enable) {
        return KEY_NOT_SUPPORT;
    }
    adc_add_sample_ch(__this->ad_channel);          //注意：初始化AD_KEY之前，先初始化ADC

    gpio_set_die(__this->adkey_pin, 0);
    gpio_set_direction(__this->adkey_pin, 1);
    gpio_set_pull_down(__this->adkey_pin, 0);
    if (__this->extern_up_en) {
        gpio_set_pull_up(__this->adkey_pin, 0);
    } else {
        gpio_set_pull_up(__this->adkey_pin, 1);
    }

    return 0;
}


#endif  /* #if TCFG_ADKEY_ENABLE */





