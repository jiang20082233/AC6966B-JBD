#include "user_fun_cfg.h"

extern APP_VAR app_var;
extern int tone_get_status();
// extern void delay_2ms(u32 delay_time);

PA_CTL_IO pa_in_io = {

    /*******单、双io功放复用mute、abd引脚配置*******/
    .port_mute = USER_PA_MUTE_PORT,//双io
    .port_abd = USER_PA_ABD_PORT,//双io
    .port_mute_io_status = USER_PA_MUTE_MODE,
    .port_ab_io_status = USER_PA_ABD_MODE,

    .pa_mute_status = 0xff,
    .pa_class_status = 0xff,
    .pa_power_off = 0,

    .pa_sys_automute = 0,
    .pa_linein_mute = 0,
    .pa_mic_online = 0,
    .pa_manual_mute = 0,
};

PA_IN_STRL pa_in_fun = {
    //最终调用控制功放
    // void (*mute)(void *pa,u8 cmd);
    // void (*abd)(void *pa,u8 cmd);
    .pa_mode    = PA_MODE_MAX,
    .pa_io      = &pa_in_io,//NULL,
    .init       = user_pa_in_pin_init,
    .service    = user_pa_in_service,
    .service_id = 0,
    .io_strl    = user_pa_in_strl,

    //双io控制功放
    .mute_2pin  = user_pa_in_mute,
    .abd_2pin   = user_pa_in_abd,
    //单io控制功放
    .abd_and_mute   =  user_pa_in_abd_and_mute,
};


void user_pa_ex_automute(u8 cmd){
    if(0xff == cmd){
        pa_in_fun.pa_io->pa_sys_automute = !pa_in_fun.pa_io->pa_sys_automute;
    }else{
        pa_in_fun.pa_io->pa_sys_automute = cmd?1:0;
    }
}


void user_pa_ex_mic(u8 cmd){
    if(0xff == cmd){
        pa_in_fun.pa_io->pa_mic_online = !pa_in_fun.pa_io->pa_mic_online;
    }else{
        pa_in_fun.pa_io->pa_mic_online = cmd?1:0;
    }
}

void user_pa_ex_linein(u8 cmd){
    if(0xff == cmd){
        pa_in_fun.pa_io->pa_linein_mute = !pa_in_fun.pa_io->pa_linein_mute;
    }else{
        pa_in_fun.pa_io->pa_linein_mute = cmd?1:0;
    }
}

void user_pa_ex_manual(u8 cmd){
    if(0xff == cmd){
        pa_in_fun.pa_io->pa_manual_mute = !pa_in_fun.pa_io->pa_manual_mute;
    }else{
        pa_in_fun.pa_io->pa_manual_mute = cmd?1:0;
    }
}

//一个io口控制功放 ab d mute
void user_pa_in_abd_and_mute(void *pa,u8 cmd){
    PA_CTL_IO *pa_ctrl = (PA_CTL_IO *)pa;
    puts(">> user_abd_mute\n");

    if((!pa_ctrl) || ((NO_CONFIG_PORT == pa_ctrl->port_mute) || (NO_CONFIG_PORT == pa_ctrl->port_abd) || !pa_ctrl->port_io_init_ok))return;

    //mute脚来控制功放 a、b、mute切换
    //abd脚开机高电平 客户做通用pcb 需要
    if(PA_MUTE == cmd){
        puts(">> PA_MUTE\n");
        gpio_set_direction(pa_ctrl->port_mute,0);
        gpio_set_pull_down(pa_ctrl->port_mute,0);
        gpio_set_pull_up(pa_ctrl->port_mute,0);
        gpio_set_die(pa_ctrl->port_mute,1);
        gpio_set_output_value(pa_ctrl->port_mute,0);
    }else if(PA_UMUTE == cmd){
        if(app_check_curr_task(APP_FM_TASK)){
            user_pa_in_abd_and_mute(pa_ctrl,PA_CLASS_AB);
        }else{
            user_pa_in_abd_and_mute(pa_ctrl,PA_CLASS_D);
        }
    }else if(PA_CLASS_AB == cmd){
        puts(">> PA_CLASS_AB\n");
        gpio_set_direction(pa_ctrl->port_mute,1);
        gpio_set_pull_down(pa_ctrl->port_mute,0);
        gpio_set_pull_up(pa_ctrl->port_mute,0);
    }else if(PA_CLASS_D == cmd){
        puts(">> PA_CLASS_D\n");
        gpio_set_direction(pa_ctrl->port_mute,0);
        gpio_set_pull_down(pa_ctrl->port_mute,0);
        gpio_set_pull_up(pa_ctrl->port_mute,0);
        gpio_set_die(pa_ctrl->port_mute,1);
        gpio_set_output_value(pa_ctrl->port_mute,1);
    }else if(PA_INIT == cmd){
        puts(">> pa PA_INIT\n");
        //abd脚开机高电平
        gpio_set_direction(pa_ctrl->port_abd,0);
        gpio_set_pull_down(pa_ctrl->port_abd,0);
        gpio_set_pull_up(pa_ctrl->port_abd,0);
        gpio_set_die(pa_ctrl->port_abd,1);
        gpio_set_output_value(pa_ctrl->port_abd,1);

        //mute功放
        gpio_set_direction(pa_ctrl->port_mute,0);
        gpio_set_pull_down(pa_ctrl->port_mute,0);
        gpio_set_pull_up(pa_ctrl->port_mute,0);
        gpio_set_die(pa_ctrl->port_mute,1);
        gpio_set_output_value(pa_ctrl->port_mute,0);        
    }
}


void user_pa_in_mute(void *pa,u8 cmd){
    PA_CTL_IO *pa_ctrl = (PA_CTL_IO *)pa;
    puts(">> user_pa_mute\n");

    if((!pa_ctrl) || (NO_CONFIG_PORT == pa_ctrl->port_mute || !pa_ctrl->port_io_init_ok)){return;}

    if(PA_UMUTE == cmd){
        puts("PA_UMUTE\n");
        gpio_set_output_value(pa_ctrl->port_mute,!(pa_ctrl->port_mute_io_status));
    }else if(PA_MUTE == cmd) {
        puts("PA_MUTE\n");
        gpio_set_output_value(pa_ctrl->port_mute,pa_ctrl->port_mute_io_status);
    }else if(PA_INIT == cmd){
        puts("PA_MUTE INIT\n");
        gpio_set_direction(pa_ctrl->port_mute,0);
        gpio_set_pull_down(pa_ctrl->port_mute,0);
        gpio_set_pull_up(pa_ctrl->port_mute,0);
        gpio_set_die(pa_ctrl->port_mute,1);
        user_pa_in_mute(pa,PA_MUTE);
    }else if(PA_POWER_OFF == cmd){
        puts("PA_POWER_OFF\n");
        user_pa_in_mute(pa,PA_MUTE);
    }

}

void user_pa_in_abd(void *pa,u8 cmd){
    PA_CTL_IO *pa_ctrl = (PA_CTL_IO *)pa;
    puts(">> user_pa_abd\n");

    if((!pa_ctrl) || (NO_CONFIG_PORT == pa_ctrl->port_abd || !pa_ctrl->port_io_init_ok)){return;}

    //功放 ab d类切换必需在mute功放的情况下进行 不然会切换之后一直有杂音
    if(PA_CLASS_AB == cmd || PA_CLASS_D == cmd){
        gpio_set_output_value(pa_ctrl->port_mute,pa_ctrl->port_mute_io_status);//mute
        if(2 == pa_ctrl->port_io_init_ok){
            delay_2ms(25);
        }else{
            delay(10000);
        }
    }

    if(PA_CLASS_AB == cmd){
        puts("PA_CLASS_AB\n");
        gpio_set_output_value(pa_ctrl->port_abd,pa_ctrl->port_ab_io_status);
    }else if(PA_CLASS_D == cmd) {
        puts("PA_CLASS_D\n");
        gpio_set_output_value(pa_ctrl->port_abd,!(pa_ctrl->port_ab_io_status));
    }else if(PA_INIT == cmd){
        puts("PA_ABD INIT\n");
        gpio_set_direction(pa_ctrl->port_abd,0);
        gpio_set_pull_down(pa_ctrl->port_abd,0);
        gpio_set_pull_up(pa_ctrl->port_abd,0);
        gpio_set_die(pa_ctrl->port_abd,1);
        // user_pa_in_abd(pa,PA_CLASS_D);
    }else if(PA_POWER_OFF == cmd){

    }

    if(PA_CLASS_AB == cmd || PA_CLASS_D == cmd){
        if(2 == pa_ctrl->port_io_init_ok){
            delay_2ms(25);
        }else{
            delay(10000);
        }
        gpio_set_output_value(pa_ctrl->port_mute,!(pa_ctrl->port_mute_io_status));//umute
    }

}

void user_pa_in_strl(void *pa,u8 cmd){
    PA_IN_STRL *pa_ctrl = (PA_IN_STRL *)pa;
    u8 *pa_mute_status = &(pa_ctrl->pa_io->pa_mute_status);
    u8 *pa_power_off   = &(pa_ctrl->pa_io->pa_power_off);

    void (*mute_strl)(void *pa,u8 cmd) = NULL;
    void (*abd_strl)(void *pa,u8 cmd) = NULL;

    //初始化的时候根据单io还是双io 功放 来注册对应的控制函数
    mute_strl = pa_ctrl->mute;
    abd_strl = pa_ctrl->abd;

    if(!pa_ctrl || *pa_power_off || !pa_ctrl->pa_io->port_io_init_ok || !mute_strl || !abd_strl || !pa_in_fun.pa_io->port_io_init_ok){
        return;
    }

    if(PA_MUTE == cmd && cmd != *pa_mute_status){
        puts(">>>PA STL  PA_MUTE\n");

        *pa_mute_status = cmd;
        mute_strl(pa_ctrl->pa_io,cmd);

    }else if(PA_UMUTE == cmd && cmd != *pa_mute_status){
        puts(">>>PA STL  PA_UMUTE\n");

        *pa_mute_status = cmd;
        mute_strl(pa_ctrl->pa_io,cmd);

    }else if(PA_CLASS_AB == cmd){
        puts(">>>PA STL  PA_CLASS_AB\n");

        abd_strl(pa_ctrl->pa_io,cmd);

    }else if(PA_CLASS_D == cmd){
        puts(">>>PA STL  PA_CLASS_D\n");

        abd_strl(pa_ctrl->pa_io,cmd);

    }else if(PA_POWER_OFF == cmd){
        puts(">>>PA STL  PA_POWER_OFF\n");

        mute_strl(pa_ctrl->pa_io,PA_MUTE);
        *pa_power_off = 1;

    }else if(PA_INIT == cmd){
        puts(">>>PA STL  PA_INIT\n");

        mute_strl(pa_ctrl->pa_io,PA_INIT);
        abd_strl(pa_ctrl->pa_io,PA_INIT);

    }
}

void user_pa_ex_strl(u8 cmd){
    if(pa_in_fun.pa_io->port_io_init_ok){
        pa_in_fun.io_strl(&pa_in_fun,cmd);
    }else{
        puts("pa no init\n");
    }
}


void user_pa_in_service(void *pa){
    PA_CTL_IO *pa_ctrl = ((PA_IN_STRL *)pa)->pa_io;
    bool pa_sys_auto_mute;

    if(!pa_ctrl->port_io_init_ok){
        return;
    }

    //自动mute
    pa_sys_auto_mute = pa_ctrl->pa_sys_automute?1:0;
    // if(pa_sys_auto_mute){
    //     puts(">> H\n");
    // }

    //各个模式特有mute

    if (app_check_curr_task(APP_BT_TASK)){
        if ((get_call_status() == BT_CALL_ACTIVE) ||
            (get_call_status() == BT_CALL_OUTGOING) ||
            (get_call_status() == BT_CALL_ALERT) ||
            (get_call_status() == BT_CALL_INCOMING)) {
            //通话过程不允许mute pa
            pa_sys_auto_mute = 0;
            puts(">> bt L\n");
        }else{
            //防止切歌时开关功放
            if(BT_MUSIC_STATUS_STARTING == a2dp_get_status()){
                pa_sys_auto_mute = 0;
            }else if(BT_MUSIC_STATUS_SUSPENDING == a2dp_get_status()){
                pa_sys_auto_mute = 1;
            }
        }

    }
    #if TCFG_APP_MUSIC_EN
    else if (app_check_curr_task(APP_MUSIC_TASK)){
        // extern bool user_file_dec_is_pause(void);
        // pa_sys_auto_mute = user_file_dec_is_pause();
        // pa_sys_auto_mute = 0;
        pa_sys_auto_mute = (music_player_get_play_status() == FILE_DEC_STATUS_PLAY)?0:1;
    }
    #endif
    else if(app_check_curr_task(APP_FM_TASK)){
        pa_sys_auto_mute = 0;
    }else if(app_check_curr_task(APP_LINEIN_TASK)){
        pa_sys_auto_mute = pa_ctrl->pa_linein_mute;
    }

    /*******************************mute*******************************/
    //音量为0
    if(!app_var.music_volume){
        // puts(">> L\n");
        pa_sys_auto_mute = 1;
    }

    //按键mute
    if(pa_ctrl->pa_manual_mute){
        pa_sys_auto_mute = 1;
    }


    /*******************************umute*******************************/
    //提示音
    if(tone_get_status()){
        pa_sys_auto_mute = 0;
    }

    //mic 插入
    if(pa_ctrl->pa_mic_online){
        pa_sys_auto_mute = 0;
    }

    //录音
    if(user_record_status(0xff)){
        pa_sys_auto_mute = 0;
    }

    // printf(">>>>>>>>>>>  mute flag %d\n",pa_sys_auto_mute);
    if(pa_sys_auto_mute){
        ((PA_IN_STRL *)pa)->io_strl(pa,PA_MUTE);
    }else{
        ((PA_IN_STRL *)pa)->io_strl(pa,PA_UMUTE);
    }

    ((PA_IN_STRL *)pa)->service_id = sys_hi_timeout_add(pa,((PA_IN_STRL *)pa)->service,pa_sys_auto_mute?100:1000);
}

/*
检测单双线功放
可以指定功放类型 
*/
int user_pa_set_mode(void *pa){
    PA_IN_STRL *pa_ctrl = (PA_IN_STRL *)pa;
    PA_CTL_IO *pa_info = pa_ctrl->pa_io;

    if(!pa_ctrl || !pa_info || (NO_CONFIG_PORT == pa_info->port_mute) || (NO_CONFIG_PORT == pa_info->port_abd)){
        puts("user check pa mode error\n");
        return -1;
    }

    gpio_set_direction(pa_info->port_mute,1);
    gpio_set_pull_down(pa_info->port_mute,0);
    gpio_set_pull_up(pa_info->port_mute,0);
    gpio_set_die(pa_info->port_mute,1);

    gpio_set_direction(pa_info->port_abd,1);
    gpio_set_pull_down(pa_info->port_abd,0);
    gpio_set_pull_up(pa_info->port_abd,0);
    gpio_set_die(pa_info->port_abd,1);

    delay(60);
    //单双io类型检测
    if(gpio_read(pa_info->port_abd)){
        pa_ctrl->pa_mode = PA_MODE_1;//双线 
        puts(">>>>>> pa mode pin 2\n");
    }else{
        pa_ctrl->pa_mode = PA_MODE_2;//单线
        puts(">>>>>> pa mode pin 1\n");
    }

    if(USER_PA_MODE_2 == USER_PA_CLASS){
        pa_ctrl->pa_mode = PA_MODE_2;        
    }else if(USER_PA_MODE_1 == USER_PA_CLASS){
        pa_ctrl->pa_mode = PA_MODE_1;//双线 
    }

    if(USER_PA_MODE_1 == USER_PA_CLASS){
        pa_ctrl->pa_mode = PA_MODE_1;        
    }

    //功放 mute abd控制方式检测
    if(PA_MODE_1 == pa_ctrl->pa_mode){//双线        
        if(gpio_read(pa_info->port_mute)){
            pa_info->port_mute_io_status = USER_PA_MUTE_H;//高mute
        }else{
            pa_info->port_mute_io_status = USER_PA_MUTE_L;//低mute
        }

        pa_info->port_ab_io_status = USER_PA_AB_L;//高ab类
    }else if(PA_MODE_2 == pa_ctrl->pa_mode){//单线
        gpio_set_direction(pa_info->port_mute,0);
        gpio_set_output_value(pa_info->port_mute,1);
        delay(60);
        if(gpio_read(pa_info->port_abd)){
            pa_ctrl->pa_mode = PA_MODE_2;//电压控制
        }else{
            pa_ctrl->pa_mode = PA_MODE_3;//脉冲控制  
        }
    }

    if(USER_PA_MODE_1 == USER_PA_CLASS){
        pa_ctrl->pa_mode = PA_MODE_1;        
    }else if(USER_PA_MODE_2 == USER_PA_CLASS){
        pa_ctrl->pa_mode = PA_MODE_2;
    }else if(USER_PA_MODE_3 == USER_PA_CLASS){
        pa_ctrl->pa_mode = PA_MODE_3;        
    }

    printf(">>>>>>>>  pa mode %d ab%d mute%d\n",pa_ctrl->pa_mode,pa_info->port_ab_io_status,pa_info->port_mute_io_status);
    return 0;
}
/*
1.通过判断单io功放引脚是否赋值来确定 功放是单io功放还是双io功放
2.初始化io
*/
int user_pa_in_pin_init(void *pa){
    PA_IN_STRL *pa_ctrl = (PA_IN_STRL *)pa;

    user_pa_set_mode(pa);

    if((PA_MODE_MAX == pa_ctrl->pa_mode) || !pa_ctrl || !(pa_ctrl->pa_io) \
    || (NO_CONFIG_PORT == pa_ctrl->pa_io->port_mute) || (NO_CONFIG_PORT == pa_ctrl->pa_io->port_abd)\
    ){
        puts("pa in init error\n");
        pa_ctrl->pa_io->port_io_init_ok = 0;
        return -1;
    }

    
    if(PA_MODE_2 == pa_ctrl->pa_mode){
        //单io控制条件不满足 
        if(!pa_ctrl->abd_and_mute){
            puts("pa 2 pin error\n");
            pa_ctrl->pa_io->port_io_init_ok = 0;
            return -1;
        }

        pa_ctrl->mute = pa_ctrl->abd_and_mute;
        pa_ctrl->abd = pa_ctrl->abd_and_mute;
    }else if(PA_MODE_1 == pa_ctrl->pa_mode){
        //双io控制条件不满足
        if( (!pa_ctrl->abd_2pin) || (!pa_ctrl->mute_2pin)){
            puts("pa 1 pin error\n");
            pa_ctrl->pa_io->port_io_init_ok = 0;
            return -1;
        }
        pa_ctrl->mute = pa_ctrl->mute_2pin;
        pa_ctrl->abd = pa_ctrl->abd_2pin;
    }else{
        puts("pa in init error ii\n");
        pa_ctrl->pa_io->port_io_init_ok = 0;
        return -1;
    }

    pa_ctrl->io_strl(pa,PA_INIT);
    return 0;
}

static void user_pa_dac_pupu(){
    user_pa_ex_strl(PA_MUTE);
    delay_2ms(10);
	extern struct audio_dac_hdl dac_hdl;
	extern int audio_dac_start(struct audio_dac_hdl *dac);
	extern int audio_dac_stop(struct audio_dac_hdl *dac);
	audio_dac_start(&dac_hdl);
	audio_dac_stop(&dac_hdl);
    delay_2ms(25);
}

void user_pa_ex_io_init(void){

    puts("pa io init\n");

    if(pa_in_fun.pa_io){
        pa_in_fun.pa_io = pa_in_fun.pa_io;
    }else{
        puts("pa ex io init io error\n");
        for(;;){;}
    }

#if USER_PA_EN

    // if(pa_in_fun.init && 0==pa_in_fun.init(&pa_in_fun)){
    if(0 == user_pa_in_pin_init(&pa_in_fun)){
        pa_in_fun.pa_io->port_io_init_ok = 1;
    }else{
        puts("pa ex io init fun error\n");
        for(;;){;}
    }

    user_pa_ex_strl(PA_INIT);
#endif
}

void user_pa_ex_del(void){
#if USER_PA_EN
    pa_in_fun.pa_io->port_io_init_ok = 0;
    if(pa_in_fun.service_id){
        sys_hi_timeout_del(pa_in_fun.service_id);
        pa_in_fun.service_id = 0;
    }
#endif
}

void user_pa_ex_init(void){
#if USER_PA_EN
    if(pa_in_fun.pa_io->port_io_init_ok){
        pa_in_fun.pa_io->port_io_init_ok = 2;
        user_pa_dac_pupu();
        pa_in_fun.service(&pa_in_fun);
    }else{
        pa_in_fun.pa_io->port_io_init_ok = 0;
    }
    user_pa_ex_strl(PA_CLASS_D);
#endif
}
