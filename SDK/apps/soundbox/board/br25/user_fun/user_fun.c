#include "user_fun_cfg.h"

#if USER_VBAT_CHECK_EN
USER_POWER_INFO user_power_io={
    .pro = IO_PORTC_02,
    .ch  = AD_CH_PC2,
    .vol = 0,
};
#endif

void user_vbat_check_init(void){
    #if USER_VBAT_CHECK_EN
    adc_add_sample_ch(user_power_io.ch);          //注意：初始化AD_KEY之前，先初始化ADC

    gpio_set_die(user_power_io.pro, 0);
    gpio_set_direction(user_power_io.pro, 1);
    gpio_set_pull_down(user_power_io.pro, 0);
    gpio_set_pull_up(user_power_io.pro, 0);
    #endif
    return;
}

u16 user_fun_get_vbat(void){
    u16 tp = 0;
    #if USER_VBAT_CHECK_EN
    u32 vddio_m = 0;
    user_power_io.vol = adc_get_value(user_power_io.ch);
    vddio_m = (TCFG_LOWPOWER_VDDIOM_LEVEL<=3)?(220+TCFG_LOWPOWER_VDDIOM_LEVEL*20):(300+(TCFG_LOWPOWER_VDDIOM_LEVEL-VDDIOM_VOL_30V)*20);

    // printf("user vbat >>>> %d %dV\n",user_power_io.vol,(vddio_m*2*user_power_io.vol)/0x3ffL);
    user_power_io.vol = (vddio_m*2*user_power_io.vol)/0x3ffL;
    tp = user_power_io.vol;
    return tp;
    #endif
    return tp;
}


void user_fm_vol_set(bool cmd){
#if (defined(USER_FM_MODE_SYS_VOL) && USER_FM_MODE_SYS_VOL)

    static u8 user_fm_sys_vol = 0xff;
    u8 tp_vol = 0;

    if(cmd){//进fm 保存音量
        user_fm_sys_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
        while (user_fm_sys_vol > app_audio_get_volume(APP_AUDIO_STATE_MUSIC)){
            app_audio_volume_down(1);
        }        

    }else{//退出FM

        while (user_fm_sys_vol > app_audio_get_volume(APP_AUDIO_STATE_MUSIC))
        {
            app_audio_volume_up(1);
        }
        while (user_fm_sys_vol < app_audio_get_volume(APP_AUDIO_STATE_MUSIC))
        {
            app_audio_volume_down(1);
        }        
    }

#endif
}

//sd pg power
void user_sd_power(u8 cmd){
    #if(defined(USER_SD_POWER_SWITCH_EN) && USER_SD_POWER_SWITCH_EN)
    //绑定引脚高阻态
    #if (USER_SD_POWER_IO != NO_CONFIG_PORT)
    gpio_set_die(USER_SD_POWER_IO, 0);
    gpio_set_direction(USER_SD_POWER_IO, 1);
    gpio_set_pull_down(USER_SD_POWER_IO, 0);
    gpio_set_pull_up(USER_SD_POWER_IO, 0);
    #endif

    sdpg_config(cmd?1:0);
    #endif
}

//关机
void user_power_off(void){
    user_led_io_fun(USER_IO_LED,LED_POWER_OFF);
    pa_ex_fun.strl(PA_POWER_OFF);
    user_sd_power(0);
}

//开机 io口初始化
void user_fun_io_init(void){
    pa_ex_fun.pa_io_init();
    user_sd_power(1);
    user_vbat_check_init();
}

//开机 功能初始化
void user_fun_init(void){
    pa_ex_fun.pa_fun_init();

    // #if (USER_IC_MODE == USER_IC_6966B)
    // if(user_4ad_fun.init){
    //     user_4ad_fun.init(&user_4ad_io);
    // }
    // #endif

    user_led_io_fun(USER_IO_LED,LED_IO_INIT);
    user_led_io_fun(USER_IO_LED,LED_POWER_ON);

    // #if (USER_IC_MODE == USER_IC_6966B)
    // ex_dev_detect_init(& user_mic_check);
    // #endif
    ex_dev_detect_init(& user_mic_check);


}
