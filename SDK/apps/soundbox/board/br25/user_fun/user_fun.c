#include "user_fun_cfg.h"
#if USER_VBAT_CHECK_EN
USER_POWER_INFO user_power_io={
    .pro = IO_PORTB_06,
    .ch  = AD_CH_PB6,
    .vol = 0,
};
#endif



//设置低电图标 显示、取消；获取低电图标显示状态
bool user_low_power_show(u8 cmd){
    static u8 low_power_icon = 0;

    //获取状态 开机过滤时间 关机状态返回
    if(0xff == cmd || timer_get_sec()<=5 || 0x55 == low_power_icon || low_power_icon == cmd){
        return low_power_icon;
    }

    //关机设置
    if(0x55 == low_power_icon || 0x55 == cmd){
        low_power_icon = cmd;
        // led7_clear_icon(LED7_CHARGE);
        led7_clear_all_icon();
        return 0;
    }


    if(0 == cmd || 1 == cmd){
        low_power_icon = cmd;
        printf(">>>>>     low power icon %d\n",low_power_icon);
    }

    return low_power_icon?1:0;
}
void user_led7_flash_lowpower(void){
    bool ret = user_low_power_show(0xff);
    if(1 == ret){
        led7_flash_icon(LED7_CHARGE);
    }else if(0 == ret){
        led7_clear_icon(LED7_CHARGE);
    }else if(0x55 == ret){
        led7_clear_icon(LED7_CHARGE);
        led7_clear_all_icon();
    }
}


//低电降音量
void user_power_low_dow_sys_vol(u8 vol){
    // return;
    #if (defined(USER_POWER_LOW_DOW_VOL_EN) && USER_POWER_LOW_DOW_VOL_EN)
    puts(">>>>>>>>>>>>> low dow sys vol\n");
    if(tone_get_status()){
        return;
    }
    if(vol < app_audio_get_volume(APP_AUDIO_STATE_MUSIC)){
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, vol, 1);
        extern int user_get_tws_state(void);
        if(1 == user_get_tws_state()){
            bt_tws_sync_volume();
        }
    }
    printf(">>>>>>>>>>>>> low sys vol %d\n",app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
    #endif
}

void user_dow_sys_vol_20(void){

    if(tone_get_status()){
        sys_hi_timeout_add(NULL,user_dow_sys_vol_20,200);
        return;
    }

    user_power_low_dow_sys_vol(20);
    user_bt_tws_sync_msg_send(USER_TWS_SYNC_DOW_VOL_20,0);
}

void user_dow_sys_vol_10(void){

    if(tone_get_status()){
        sys_hi_timeout_add(NULL,user_dow_sys_vol_10,200);
        return;
    }

    user_power_low_dow_sys_vol(10);
    user_bt_tws_sync_msg_send(USER_TWS_SYNC_DOW_VOL_10,0);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws info同步
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
#if TCFG_USER_TWS_ENABLE
static void bt_tws_user_info_sync(void *_data, u16 len, bool rx)
{
    if (rx/*&& len==2*/) {
        u8 *data = (u8 *)_data;
        //SYNC LED
        if(USER_TWS_SYNC_LED == data[0]){
            user_led_io_fun(USER_IO_LED,data[1]);
        }

        //SYNC RGB
        if(USER_TWS_SYNC_RGB == data[0]){
            user_led_io_fun(USER_IO_LED,data[1]);
        }


        // //DOW VOL
        // if(USER_TWS_SYNC_DOW_VOL==data[0]){
        //     // user_dow_sys_vol();
        // }

        // //DOW VOL 10
        // if(USER_TWS_SYNC_DOW_VOL_10==data[0]){
        //     user_dow_sys_vol_10();
        // }

        // //DOW VOL 20
        // if(USER_TWS_SYNC_DOW_VOL_20==data[0]){
        //     user_dow_sys_vol_20();
        // }

        printf(">>>> tws sync DATA0:%d DATA1:%d\n",data[0],data[1]);
    }

}

//TWS同步信息
REGISTER_TWS_FUNC_STUB(app_led_mode_sync_stub) = {
    .func_id = USER_TWS_FUNC_ID_USER_INFO_SYNC,
    .func    = bt_tws_user_info_sync,
};

void user_bt_tws_sync_msg_send(u8 sync_type,u8 value){
    u8 data[2];
    if((tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) && (sync_type < USER_TWS_SYNC_MAX)){
        data[0] = sync_type;
        data[1] = value;
        tws_api_send_data_to_sibling(data, 2, USER_TWS_FUNC_ID_USER_INFO_SYNC);
    }
}
#endif

void user_tws_sync_info(void){
    #if TCFG_USER_TWS_ENABLE
    if(APP_BT_TASK != app_get_curr_task() && (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)){
        return;
    }

    int ret = user_led_io_fun(USER_IO_LED,LED_IO_STATUS);
    if(-1 != ret){
        // user_bt_tws_send_led_mode(0,ret);
        user_bt_tws_sync_msg_send(USER_TWS_SYNC_LED,ret);
    }

    ret = user_rgb_mode_set(USER_RGB_STATUS,NULL);
    if(ret){
        // user_bt_tws_send_led_mode(1,ret);
        user_bt_tws_sync_msg_send(USER_TWS_SYNC_RGB,ret);
    }

    #endif
}

//adkey 与 irkey复用时滤波
bool user_adkey_mult_irkey(u8 key_type){
    u32 static ir_time = 0;
    bool tp = 0;
    #if ((TCFG_ADKEY_ENABLE && TCFG_IRKEY_ENABLE) && (TCFG_IRKEY_PORT == TCFG_ADKEY_PORT))
    if(KEY_DRIVER_TYPE_IR == key_type){
        ir_time = timer_get_ms();
    }else if(KEY_DRIVER_TYPE_AD == key_type){
        if(timer_get_ms()-ir_time<200){
            // printf(">>> %d %d\n",timer_get_ms(),ir_time);
            tp = 1;
        }
    }
    #endif
    return tp;
}

//设置、获取 录音状态
u8 user_record_status(u8 cmd){
    static u8 user_record_status  = 0;
    #if USER_RECORD_EN
    if(0 == cmd || 1 == cmd){
        user_record_status = cmd;

        // user_mic_check_en(!user_record_status);
        // printf(">>>>>>>>>>>>>>>>>>>>  record status  %d\n",user_record_status);
    }
    #endif
    return user_record_status;//USER_KEY_RECORD_START
}

/*
cmd:    1、其他值
    1：已启用 其他：获取状态
ret：   1：已启用 0：为启用过
*/
u8 user_eq_init_ok(u8 cmd){
    static u8 sys_init_ok_flag = 0;

    if(1 == cmd){
        sys_init_ok_flag = cmd;
    }

    return sys_init_ok_flag;
}
void user_eq_mode_sw(void){
    #if USER_EQ_FILE_ADD_EQ_TABLE
        static int user_eq_mode = 0;

        if(!user_eq_init_ok(0xff)){
            return;
        }

        user_eq_mode++;
        if(user_eq_mode>EQ_MODE_COUNTRY){
            user_eq_mode = EQ_MODE_NORMAL;
        }
        printf(">>>> eq mode %d\n",user_eq_mode);

        #if USER_EQ_LIVE_UPDATE
        int tp_bass = (eq_tab_custom[USER_EQ_BASS_INDEX].gain)>>20;
        int tp_terble = (eq_tab_custom[USER_EQ_TERBLE_INDEX].gain)>>20;
        #endif

        // if(user_eq_mode<EQ_MODE_CUSTOM){
        //     memcpy(eq_tab_custom,eq_type_tab[user_eq_mode],sizeof(EQ_CFG_SEG)*SECTION_MAX);
        // }else{
        //     memcpy(eq_tab_custom,user_eq_tab_custom,sizeof(EQ_CFG_SEG)*SECTION_MAX);
        // }

        memcpy(eq_tab_custom,eq_type_tab[user_eq_mode],sizeof(EQ_CFG_SEG)*SECTION_MAX);

        #if USER_EQ_LIVE_UPDATE
        eq_tab_custom[USER_EQ_BASS_INDEX].gain = tp_bass<<20;
        eq_tab_custom[USER_EQ_TERBLE_INDEX].gain = tp_terble<<20;
        #endif

        eq_mode_set(EQ_MODE_CUSTOM);
#if TCFG_UI_ENABLE
    ui_set_tmp_menu(MENU_SET_EQ, 1000, user_eq_mode, NULL);
#endif
    #else
    eq_mode_sw();
    #endif
}

void user_eq_bass_terble_set(int bass,int terble){
    #if USER_EQ_LIVE_UPDATE
    eq_tab_custom[USER_EQ_BASS_INDEX].gain = bass<<20;
    eq_tab_custom[USER_EQ_TERBLE_INDEX].gain = terble<<20;

    eq_mode_set(EQ_MODE_CUSTOM);
    #endif
    return;
}

void user_bass_terble_updata(u32 bass_ad,u32 terble_ad){
    static int bass_old =0,terble_old = 0;

    s32 bass  = bass_ad,terble = terble_ad;

    //无效值
    if((512-20) < bass_ad || (512+20) > terble_ad  || user_record_status(0xff) || \
    !user_eq_init_ok(0xff) || timer_get_sec()<6 || APP_RECORD_TASK == app_get_curr_task() || \
    app_get_curr_task() == APP_FM_TASK){
        return;
    }

    if((USER_EQ_BASS_AD_MIN-10) > bass_ad){
        bass_ad = USER_EQ_BASS_AD_MIN;
    }

    if((USER_EQ_TERBLE_AD_MIN-10) > terble){
        terble = USER_EQ_TERBLE_AD_MIN;
    }


    bass = ((bass-USER_EQ_BASS_AD_MIN)*(USER_EQ_BASS_GAIN_MAX-USER_EQ_BASS_GAIN_MIN))/(USER_EQ_BASS_AD_MAX-USER_EQ_BASS_AD_MIN);
    terble = ((terble-USER_EQ_TERBLE_AD_MIN)*(USER_EQ_TERBLE_GAIN_MAX-USER_EQ_TERBLE_GAIN_MIN))/(USER_EQ_TERBLE_AD_MAX-USER_EQ_TERBLE_AD_MIN);
    bass += USER_EQ_BASS_GAIN_MIN;
    terble += USER_EQ_TERBLE_GAIN_MIN;

    if(bass_old!=bass || terble_old!=terble){
        if(bass_old!=bass){
            bass_old = bass;
        }
        if(terble_old!=terble){
            terble_old = terble;
        }

        // printf(">>>>> eq bass %d terble %d\n",bass_old,terble_old);
        user_eq_bass_terble_set(bass_old,terble_old);
    }
}
void user_mic_en(u8 en){
    SFR(JL_ANA->ADA_CON0,29,1,en);
}

void user_mic_vol_update(u8 vol){
    
    static int mic_effect_dvol = -1;
    static bool mic_status_old = 0;
    static u8 mic_old = 0;
    
    bool mic_in_status = user_get_mic_status();
    // printf(">>>>>> mic in status %d\n",mic_in_status);
    if(!user_record_status(0xff) && \
    ((mic_in_status && !mic_status_old)||
     mic_old != vol)){

        mic_old = vol;
        mic_status_old = mic_in_status;

        // printf(">>>>> mic_old %d vol %d\n",mic_old,vol);
        #if USER_MIC_MUSIC_VOL_SEPARATE
        mic_effect_set_dvol(mic_old);
        #else
        if(mic_old <= 0){
            mic_effect_dvol = 1;
            // mic_effect_dvol = mic_effect_get_dvol();
            // mic_effect_set_dvol(0);
            user_mic_en(0);

        }else{
            if(mic_effect_dvol >= 0){
                mic_effect_dvol = -1;
                // mic_effect_set_dvol(mic_effect_dvol);
                
                user_mic_en(1);
            }
        }
        audio_mic_set_gain(mic_old);
        #endif
    }
}
u8 user_mic_ad_2_vol(u8 cmd,u32 vol_ad){
    static int mic_old =-1;

#if (defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE)

    s32 mic_vol = 0;

    //获取mic 旋钮音量
    if(1 == cmd){
        return mic_old;
    }

    //ad 512 无效
    if((USER_EQ_MIC_AD_MIN > vol_ad) || ((USER_EQ_MIC_AD_MAX+20)) < vol_ad){

        return mic_old;
    }

    u32 tp_ad=vol_ad;
    tp_ad = tp_ad>USER_EQ_MIC_AD_MAX?USER_EQ_MIC_AD_MAX:tp_ad;
    tp_ad = tp_ad<USER_EQ_MIC_AD_MIN?USER_EQ_MIC_AD_MIN:tp_ad;

    // 0 600 615 650 670 700 740 820 900
    u32 level[]={0,600,620,640,670,700,800,890,1024,1024};
    for(int i = 0;(i+1)<sizeof(level);i++){
        if((tp_ad>level[i]) && (tp_ad<=level[i+1])){
            mic_vol = i;
            break;
        }
    }
    mic_vol = mic_vol?mic_vol+USER_EQ_MIC_GAIN_MIN:mic_vol;
    // printf(">>>>> mic_old %d mic_vol %d ad %d\n",mic_old,mic_vol,vol_ad);

    mic_old = mic_vol;

    user_mic_vol_update(mic_old);

#endif
    return mic_old;
}

u8 user_ex_mic_get_vol(void){
    int tp_vol = user_mic_ad_2_vol(1,0);
    if(tp_vol<0){
        return 0xff;
    }
    return tp_vol;
}


void user_mic_reverb_updata(u32 vol){
    #if (defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE)
    static int rev_old =0;
    
    static int mic_effect_dvol = -1;
    static bool mic_status_old = 0;
    
    if(!user_record_status(0xff) && \
    ((user_get_mic_status() && !mic_status_old)||
     rev_old != vol)){
        rev_old = vol;
        mic_status_old = user_get_mic_status();
        mic_effect_set_echo_delay(rev_old);
     }

    #endif
}

int user_mic_ad_2_reverb(u8 cmd, u32 reverb_ad){
    static int reverb_value =-1;
    s32 tp_value = 0;

    //获取reverb 旋钮值
    if(1 == cmd){
        return reverb_value;
    }

    //ad 512 无效    
    if((USER_EQ_REV_AD_MAX < reverb_ad) || ((USER_EQ_REV_AD_MIN) > reverb_ad)){
        return reverb_value;
    }

    u32 tp_ad=reverb_ad;
    tp_ad = tp_ad>USER_EQ_REV_AD_MAX?USER_EQ_REV_AD_MAX:tp_ad;
    tp_ad = tp_ad<USER_EQ_REV_AD_MIN?USER_EQ_REV_AD_MIN:tp_ad;

    tp_value = ((tp_ad-USER_EQ_REV_AD_MIN)*(USER_EQ_REV_GAIN_MAX-USER_EQ_REV_GAIN_MIN)*10)/(USER_EQ_REV_AD_MAX-USER_EQ_REV_AD_MIN);    
    tp_value = (tp_value/10)+(tp_value%10>5?1:0);
    tp_value += USER_EQ_REV_GAIN_MIN;

    // printf(">>>>> reverb %d mic_vol %d ad %d\n",reverb_value,tp_value,reverb_ad);

    reverb_value = tp_value;
    user_mic_reverb_updata(reverb_value);
    return reverb_value;
}

int user_ex_mic_get_reverb(void){
    int tp_vol = user_mic_ad_2_reverb(1,0);
    if(tp_vol<0){
        return 0xff;
    }
    return tp_vol;
}

void user_4ad_fun_features(u32 *vol){
    user_bass_terble_updata(vol[USER_EQ_BASS_BIT],vol[USER_EQ_TERBLE_BIT]);
    user_mic_ad_2_vol(0,vol[USER_MIC_VOL_BIT]);
    user_mic_ad_2_reverb(0,vol[USER_REVER_BOL_BIT]);
}
  

void user_sys_vol_callback_fun(u32 *vol){
    #define USER_SYS_VOL_AD_MAX 1000
    #define USER_SYS_VOL_AD_MIN 20

    //Rate of change
    static u32 dif_ad_old = 0;
    u32 cur_vol_ad = 0;//当前系统音量转换成ad值
    u32 cur_ad_vol = 0;//当前ad值转换成系统音量
    u32 level_ad = (USER_SYS_VOL_AD_MAX-USER_SYS_VOL_AD_MIN)/app_audio_get_max_volume();
    u32 cur_ad = vol[0];
    u32 cur_vol = 0;
    bool sys_vol_update_flag = 0;

    if(timer_get_sec<4 || tone_get_status()){
        return;
    }

    //滤除两端ad值
    cur_ad = cur_ad>USER_SYS_VOL_AD_MAX?USER_SYS_VOL_AD_MAX:cur_ad;
    cur_ad = cur_ad<USER_SYS_VOL_AD_MIN?0:cur_ad;

    //当前系统音量转换成ad值
    cur_vol_ad = 10*(USER_SYS_VOL_AD_MAX-USER_SYS_VOL_AD_MIN)*app_audio_get_volume(APP_AUDIO_STATE_MUSIC)/app_audio_get_max_volume();
    cur_vol_ad = cur_vol_ad/10+(cur_vol_ad%10>5?1:0);

    //当前ad值转换成系统音量
    cur_ad_vol = (cur_ad * app_audio_get_max_volume()*10 / (USER_SYS_VOL_AD_MAX-USER_SYS_VOL_AD_MIN));
    cur_ad_vol = cur_ad_vol/10+(cur_ad_vol%10>5?1:0);
    cur_ad_vol = cur_ad_vol>app_audio_get_max_volume()?app_audio_get_max_volume():cur_ad_vol;

    if(DIFFERENCE(cur_ad,cur_vol_ad)>=level_ad/*2*level_ad/3*/){
        sys_vol_update_flag = 1;
    }else if((cur_vol_ad!=app_audio_get_max_volume() && cur_ad_vol == app_audio_get_max_volume())){
        sys_vol_update_flag = 1;
    }else if(!cur_ad_vol && app_audio_get_max_volume()){
        sys_vol_update_flag = 1;
    }else if(DIFFERENCE(cur_ad,dif_ad_old)>60){
        sys_vol_update_flag = 1;
    }

    if(sys_vol_update_flag){
        if(app_audio_get_volume(APP_AUDIO_STATE_MUSIC) != cur_ad_vol){
            u8 volume = cur_ad_vol;
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, volume, 1);
            UI_SHOW_MENU(MENU_MAIN_VOL, 1000, app_audio_get_volume(APP_AUDIO_STATE_MUSIC), NULL);
        }
    }

    dif_ad_old = vol[0];
    printf(">>>> sys vol ad %d %d %d %d %d %d\n",vol[0],cur_ad,sys_vol_update_flag,cur_ad_vol,cur_vol_ad,app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
}

static u16 auto_time_id = 0;
void user_music_play_finle_number(void *priv){
    int *filenum = (int *)priv;
    auto_time_id = 0;
    printf(">>> kkkk number %d %d\n",*filenum);
    if(*filenum){
        music_player_play_by_number(music_player_get_dev_cur(),*filenum);
        // music_play_msg_post(3, MPLY_MSG_FILE_BY_DEV_FILENUM, (int)music_play_get_cur_dev(), *filenum);
    }
    *filenum = 0;
    printf(">>>> 000 %d\n",*filenum);
}
void user_music_set_file_number(int number){
    #if USER_IR_PLAY_FILE_NUMBER
    static int filenum = 0;
    s32 tp = filenum*10+number;

    printf(">>>>>music_play_get_file_total_file %d\n",music_player_get_file_total());

    if(auto_time_id){
        sys_timeout_del(auto_time_id);
    }

    if((tp>music_player_get_file_total()) || /*!tp ||*/ tp/10000){
        filenum = 0;
        #if TCFG_UI_ENABLE
        ui_set_tmp_menu(MENU_RECODE_ERR, 3000, 0, NULL);
        #endif
        return;
    }else{
        filenum = tp;
    }

    printf(">>>>> filenum %d\n",filenum);


    #if TCFG_UI_ENABLE
    ui_set_tmp_menu(MENU_FILENUM, 4000, filenum, NULL);
    #endif

    if(filenum>1000){
        user_music_play_finle_number(&filenum);
        filenum = 0;
    }else{
        auto_time_id = sys_timeout_add(&filenum,user_music_play_finle_number,3000);
    }
    #endif
}


//spi pb6 clk 引脚复用设置
void user_fun_spi_pb6_mult(void){
    #if (defined(USER_VBAT_CHECK_EN) && USER_VBAT_CHECK_EN)
    gpio_set_die(user_power_io.pro, 0);
    gpio_set_direction(user_power_io.pro, 1);
    gpio_set_pull_down(user_power_io.pro, 0);
    gpio_set_pull_up(user_power_io.pro, 0);
    #endif
}

void user_vbat_check_init(void){
    #if (defined(USER_VBAT_CHECK_EN) && USER_VBAT_CHECK_EN)
    user_fun_spi_pb6_mult();
    adc_add_sample_ch(user_power_io.ch);          //注意：初始化AD_KEY之前，先初始化ADC
    // if(timer_get_ms()>3000){
    //     adc_set_sample_freq(user_power_io.ch,10);
    // }
    #endif
    return;
}

u16 user_fun_get_vbat(void){
    static u16 tp = 0;
    #if (defined(USER_VBAT_CHECK_EN) && USER_VBAT_CHECK_EN)
    u32 vddio_m = 0;
    u32 tp_ad = adc_get_value(user_power_io.ch);

    // static u32 vbat_scan_time = 0;
    // if(timer_get_ms()>3000 && timer_get_ms()-vbat_scan_time>30){
    //     vbat_scan_time = timer_get_ms();
    //     adc_remove_sample_ch(user_power_io.ch);
    //     user_vbat_check_init();
    // }else{
    //     return tp;
    // }

    vddio_m  = 220+(TCFG_LOWPOWER_VDDIOM_LEVEL-VDDIOM_VOL_22V)*20;

    // printf("user vbat >>>> ad:%d vbat:%dV\n",tp_ad,(vddio_m*2*tp_ad)/0x3ffL);
    user_power_io.vol = (vddio_m*2*tp_ad)/0x3ffL;
    tp = user_power_io.vol;

    #endif
    return tp;
}


void user_fm_vol_set(bool cmd){
#if (defined(USER_FM_MODE_SYS_VOL) && USER_FM_MODE_SYS_VOL)

    static u8 user_fm_sys_vol = 0xff;
    u8 tp_vol = 0;

    if(cmd){//进fm 保存音量
        tp_vol = user_fm_sys_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
        while (tp_vol > USER_FM_MODE_SYS_VOL){
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, USER_FM_MODE_SYS_VOL, 1);
            tp_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
        }
    }else{//退出FM
        while(app_audio_get_volume(APP_AUDIO_STATE_MUSIC)!=user_fm_sys_vol){
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, user_fm_sys_vol, 1);
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

//获取、设置 关机类型
//cmd 1:低电关机 2:遥控器关机
u8 user_power_off_class(u8 cmd){
    static u8 power_class = 0;

    if(1 == cmd){//低电关机
        power_class = cmd;
    }else if((1 != power_class) && (2 == cmd)){//遥控器关机
        power_class = cmd;        
    }

    return power_class;
}
//关机
void user_power_off(void){
    user_low_power_show(0x55);
    UI_SHOW_MENU(MENU_POWER_OFF, 0, 0, NULL);
    user_led_io_fun(USER_IO_LED,LED_POWER_OFF);
    // user_pa_ex_strl(PA_POWER_OFF);
    // user_sd_power(0);
    user_rgb_mode_set(USER_RGB_POWER_OFF,NULL);

    user_mic_check_en(0);
}

//注销 定时器
void user_del_time(void){
    user_mic_check_del();
    user_ad_total_check_del();
    user_rgb_fun_del();
    user_pa_ex_strl(PA_POWER_OFF);
    user_pa_ex_del();
}

//开机 io口初始化
void user_fun_io_init(void){
    user_pa_ex_io_init();
    user_sd_power(1);
    user_vbat_check_init();
}

//开机 功能初始化
void user_fun_init(void){
    user_pa_ex_init();

    user_4ad_check_init(user_4ad_fun_features);
    user_sys_vol_ad_check_init(user_sys_vol_callback_fun);

    user_led_io_fun(USER_IO_LED,LED_IO_INIT);
    user_led_io_fun(USER_IO_LED,LED_POWER_ON);

    // #if (USER_IC_MODE == USER_IC_6966B)
    // ex_dev_detect_init(& user_mic_check);
    // #endif
    //第一次上电开机 mic上线的消息会丢失 延时检测
    sys_timeout_add(NULL,user_mic_check_init,3000);

    user_rgb_fun_init();
    user_low_power_show(0);
}
