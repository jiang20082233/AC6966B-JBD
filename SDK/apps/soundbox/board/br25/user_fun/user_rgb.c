#include "user_fun_cfg.h"

#if USER_RGB_EN
#if TCFG_HW_SPI1_ENABLE
#define USER_RGB_DATA   SPI1
#endif
#if TCFG_HW_SPI2_ENABLE
#define USER_RGB_DATA   SPI2
#endif
#ifndef USER_RGB_DATA
error 没有开spi
#endif
RGB_INFO user_rgb_info={
    .power_off = 0,
    .init_flag = 0,
    .rend_flag = 0,
    .updata_flag = 0,
    .number = USER_RGB_NUMBER,
    .spi_scan_time = 20,
    .spi_port = USER_RGB_DATA,//SPI2,
};
RGB_FUN user_rgb_fun = {
    .power_off = 0,
    .step_value = 1,
    .interrupt_id =0,
    .interrupt = 0,
    .info = &user_rgb_info,
    .cur_colour = {0xff,0x0,0x0},
    .cur_mode = USER_RGB_MODE_1,
    .mode_scan_time = 100,
};
#define P_RGB_FUN (&user_rgb_fun)
#else
#define P_RGB_FUN &NULL
#endif

//伪随机数
u32 random_number(u32 start, u32 end){
    if (end <= start) {
        return start;
    }

    return JL_TIMER0->CNT % (end - start + 1) + start;
}

/*
*r,g,b:基色值
*umode：模式
*kk:不进值
*rgb_min_value:另一个基色开始变化时的初始值
*/
void user_rgb_gradient(u16 *r,u16 *g,u16 *b,u8 *umode,u16 kk){
	s16 sr=*r,sg=*g,sb=*b;
	u8 mode = *umode;

    if(0==mode){
        if((sr+sg+sb)!=0xff){
            sr = 0xff;
            sg = sb = 0;
        }
        sg+=kk;
        sr = 0xff-sg;
    	
    	if(0xff <= sg){
            sg = 0xff;
            sr = sb = 0;
    		mode = 1;
    	}
    }else if(1==mode){
        if((sr+sg+sb)!=0xff){
            sg = 0xff;
            sr = sb = 0;
            sb = 0;
        }
        sb+=kk;
        sg = 0xff-sb;
    	
    	if(0xff <= sb){
            sb = 0xff;
            sr = sg = 0;
    		mode =2;
    	}
    }else if(2==mode){
        if((sr+sg+sb)!=0xff){
            sb = 0xff;
            sr = sg = 0;
        }
        sr+=kk;
        sb = 0xff-sr;
    	if(0xff <= sr){
            sr = 0xff;
            sb = sg = 0;
    		mode =3;
    	}
    }else if(3==mode){
        if(sr!=0xff && !sb){
            sr = 0xff;
            sb = sg = 0;
        }
        sg+=kk;
        sr = 0xff;
        sb = 0;
    	if(0xff <= sg){
            sr = sg= 0xff;
            sb = 0;
    		mode =4;
    	}
    }else if(4==mode){
        if((sr+sg+sb)!=(0xff*2)){
            sr = sg= 0xff;
            sb = 0;
        }
        sb+=kk;
        sr = 0xff-sb;
    	
    	if(0xff <= sb){
            sg = sb = 0xff;
            sr = 0;
    		mode =5;
    	}
    }else if(5==mode){
        if((sr+sg+sb)!=(0xff*2)){
            sg = sb = 0xff;
            sr = 0;
        }
        sr+=kk;
        sg = 0xff-sr;
    	
    	if(0xff <= sr){
            sr = sb = 0xff;
            sg = 0;
    		mode =6;
    	}
    }else if(6==mode){
        if(sr!=0xff && !sg){
            sr = sb = 0xff;
            sg = 0;
        }
        sb -=kk;
    	
    	if(0x0>=sb){
            sr = 0xff;
            sg = sb = 0x0;
    		mode =0;
    	}
    }
	*r = sr;
	*g = sg;
	*b = sb;
	*umode = mode;
}


void user_rgb_colour_gradient(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag){
        return;
    }
    static u8 mode = 0;
    
    u16 sr=rgb->cur_colour.r,sg=rgb->cur_colour.g,sb=rgb->cur_colour.b;
    user_rgb_gradient(&sr,&sg,&sb,&mode,1);
    rgb->cur_colour.r=sr;
    rgb->cur_colour.g=sg;
    rgb->cur_colour.b=sb;

}

//清除打断标志
void user_rgb_clean_interrupt(void *priv){

    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb){
        return;
    }
    rgb->interrupt_id = 0;
    rgb->interrupt = 0;
}
//bass状态显示
void user_rgb_bass_display(void *priv,void *data){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    RGB_DISPLAY_DATA *play_data = (RGB_DISPLAY_DATA*)data;

    if(!rgb || !play_data || !rgb->info || rgb->info->rend_flag || !rgb->info->number || !play_data->display_time || rgb->power_off){
        return;
    }    
    
    rgb->interrupt = 1;
    rgb->info->updata_flag = 1;
    if(rgb->interrupt_id){
        sys_timeout_del(rgb->interrupt_id);
    }
    
    user_rgb_clear_colour(rgb->info);
    RGB_COLOUR colour={0x0,0x0,0x0};
    if(play_data->bass){
        colour.r = 0xff;
        colour.b = 0;
        colour.g = 0;
    }else{
        colour.r = 0xff;
        colour.b = 0xff;
        colour.g = 0;
    }

    user_rgb_same_colour(rgb->info,&colour);

    rgb->info->updata_flag = 0;
    rgb->interrupt_id = sys_timeout_add(rgb,user_rgb_clean_interrupt,1000*(play_data->display_time));    

}

//音量显示
void user_rgb_vol_display(void *priv,void *data){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    RGB_DISPLAY_DATA *play_data = (RGB_DISPLAY_DATA*)data;

    if(!rgb || !play_data || !rgb->info || rgb->info->rend_flag || !rgb->info->number || !play_data->display_time || rgb->power_off){
        return;
    }
    rgb->interrupt = 1;
    rgb->info->updata_flag = 1;
    if(rgb->interrupt_id){
        sys_timeout_del(rgb->interrupt_id);
    }
    
    user_rgb_clear_colour(rgb->info);

    u16 tp = 0;
    u16 number = (play_data->sys_vol*rgb->info->number/play_data->sys_vol_max);//被点亮的颗数
    RGB_COLOUR colour={0x0,0x0,0x0};
    
    for(int i=0;i<number;i++){
        tp = i*(0xff-0xa)/rgb->info->number;
        tp = !tp?0xa:tp;
        tp = tp>0xff?0xff:tp;
        colour.r = tp;

        user_rgb_colour_only_set(rgb->info,&colour,i);
    }

    rgb->info->updata_flag = 0;
    rgb->interrupt_id = sys_timeout_add(rgb,user_rgb_clean_interrupt,1000*(play_data->display_time));
}

//vol显示外部调用接口
void user_rgb_display_vol(u8 vol,u16 display_time){
    RGB_DISPLAY_DATA data;
    data.display_time = display_time;
    data.sys_vol_max = app_audio_get_max_volume();
    data.sys_vol = vol;//app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    printf(">>>>>> max %d vol %d",data.sys_vol_max,data.sys_vol);
    user_rgb_mode_set(USER_RGB_SYS_VOL,&data);
}

//bass状态显示外部调用接口
void user_rgb_display_bass(u8 bass,u16 display_time){
    RGB_DISPLAY_DATA data;
    data.display_time = display_time;
    data.bass = bass;
    
    user_rgb_mode_set(USER_RGB_EQ_BASS,&data);
}
//淡入淡出 环形灯圈
void user_rgb_display_mode_1(void *priv){

    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || !rgb->info->number){
        return;
    }

    RGB_COLOUR tp_buff;
    u32 color_brightness = 0;
    u32 tp_brightness = 0;

    /*
        颜色： r:g:b(比值)
        亮度：0~255(光的强弱)
        效果：颜色与亮度的叠加
    */
    for(int i = 0;i<rgb->info->number;i++){
        if(rgb->brightness_table[i]<=(RGB_BRIGHTNESS_LEVEL/RGB_BRIGHTNESS_SEGMENT)){
            color_brightness = (255/RGB_BRIGHTNESS_LEVEL)*RGB_BRIGHTNESS_SEGMENT*rgb->brightness_table[i];
        }else if (rgb->brightness_table[i]<((RGB_BRIGHTNESS_LEVEL/RGB_BRIGHTNESS_SEGMENT)*2)){
            color_brightness = (255/RGB_BRIGHTNESS_LEVEL)*(RGB_BRIGHTNESS_LEVEL-RGB_BRIGHTNESS_SEGMENT*(rgb->brightness_table[i]%(RGB_BRIGHTNESS_LEVEL/RGB_BRIGHTNESS_SEGMENT)));
        }else{
            color_brightness = 0;
        }

        u16 tp_value = rgb->brightness_table[i];
        tp_value = (rgb->step_value+tp_value)%0xff;
        rgb->brightness_table[i]= tp_value;
        rgb->brightness_table[i] %= ((RGB_BRIGHTNESS_LEVEL/rgb->info->number))*(rgb->info->number);//RGB_BRIGHTNESS_LEVEL;

        tp_brightness = (color_brightness*rgb->cur_colour.r)/0xff;
        tp_buff.r = tp_brightness;

        tp_brightness = (color_brightness*rgb->cur_colour.g)/0xff;
        tp_buff.g = tp_brightness;

        tp_brightness = (color_brightness*rgb->cur_colour.b)/0xff;
        tp_buff.b = tp_brightness;

        user_rgb_colour_only_set(rgb->info,&tp_buff,(rgb->info->number-i-1));
    }

}

//升降 环形灯圈
void user_rgb_display_mode_2(void *priv){

    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || !rgb->info->number){
        return;
    }
    // random_number(7,30);
    // timer_get_ms();
    
    s32 display_number = timer_get_sec()%(rgb->info->number);
    // printf(">>>>>>>>>>>>%d\n",display_number);
    display_number = display_number>=(rgb->info->number>>1)?(rgb->info->number>>1)-(display_number%(rgb->info->number>>1))-2:display_number;
    
    for(int i = 0;i<=display_number;i++){
        user_rgb_colour_only_set(rgb->info,&rgb->cur_colour,(rgb->info->number/2)-i-(!(rgb->info->number%2)));
        user_rgb_colour_only_set(rgb->info,&rgb->cur_colour,(rgb->info->number/2)+i);
    }
}

//渐变
void user_rgb_display_mode_3(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || !rgb->info->number){
        return;
    }

    user_rgb_same_colour(rgb->info,&rgb->cur_colour);
}

//三色旋转
void user_rgb_display_mode_4(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || !rgb->info->number){
        return;
    }

    RGB_COLOUR colour[3] = {
        {0xff,0x00,0x00},
        {0x00,0xff,0x00},
        {0x00,0x00,0xff}
    };

    u16 grade = rgb->info->number/3;
    s16 rand  = random_number(7,20);
    for(int i=0;i<rgb->info->number;i++){
        user_rgb_colour_only_set(rgb->info,&colour[i/grade],i+rand);
    }
}

void user_rgb_mode_scan(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if(!rgb || !rgb->info || rgb->info->rend_flag || rgb->power_off){
        return;
    }

    rgb->info->updata_flag = 1;
    
    if(!rgb->interrupt){        
        user_rgb_colour_gradient(rgb);
        user_rgb_clear_colour(rgb->info);

        switch (rgb->cur_mode){
        case USER_RGB_MODE_1:
            if(30 != rgb->mode_scan_time){
                rgb->mode_scan_time = 30;
            }
            user_rgb_display_mode_1(rgb);   
            break;
        case USER_RGB_MODE_2:
            if(100 != rgb->mode_scan_time){
                rgb->mode_scan_time = 100;
            }
            user_rgb_display_mode_2(rgb);  
            break;
        case USER_RGB_MODE_3:        
            if(100 != rgb->mode_scan_time){
                rgb->mode_scan_time = 100;
            }
            user_rgb_display_mode_3(rgb);
            break;
        case USER_RGB_MODE_4:
            if(100 != rgb->mode_scan_time){
                rgb->mode_scan_time = 100;
            }
            user_rgb_display_mode_4(rgb);
            break;
        case USER_RGB_MODE_OFF:
            break; 
        default:
            break;
        }
    }

    rgb->info->updata_flag = 0;
    sys_timeout_add(rgb,user_rgb_mode_scan,rgb->mode_scan_time);
}

void user_rgb_fun_power_off(void *priv){
    RGB_FUN *rgb = (RGB_FUN *)priv;
    if (!rgb || !rgb->info || rgb->power_off){
        return;
    }

    rgb->cur_mode = USER_RGB_POWER_OFF;
    rgb->power_off = 1;
    user_rgb_power_off(rgb->info);
}

//模式设置
void user_rgb_mode_set(USER_GRB_MODE mode,void *priv){
    if (!P_RGB_FUN || !P_RGB_FUN->info || USER_RGB_POWER_OFF==P_RGB_FUN->cur_mode){
        printf(">>>>> rgb p error\n");
        return;
    }

    switch (mode){
    case USER_RGB_AUTO_SW:
        if(P_RGB_FUN->cur_mode+1>=USER_RGB_MODE_MAX){
            P_RGB_FUN->cur_mode = USER_RGB_MODE_1;
        }else{
            P_RGB_FUN->cur_mode++;
        }
        printf("user rgb mode %d\n",P_RGB_FUN->cur_mode);
        break;
    case USER_RGB_SYS_VOL:
        puts("USER_RGB_SYS_VOL\n");
        user_rgb_vol_display(P_RGB_FUN,priv);
        break;
    case USER_RGB_EQ_BASS:
        puts("USER_RGB_EQ_BASS\n");
        user_rgb_bass_display(P_RGB_FUN,priv);
        break;
    case USER_RGB_POWER_OFF:
        user_rgb_fun_power_off(P_RGB_FUN);
        break;
    default:
        break;
    }

}


void user_rgb_fun_init(void){
#if USER_RGB_EN
    if (!P_RGB_FUN || !P_RGB_FUN->info){
        printf(">>>>> rgb p error\n");
        return;
    }
    
    // printf(">>>>>>>>>>>>>> user spi %d\n",user_rgb_fun.info->spi_port);
    for(int i = 0;i<P_RGB_FUN->info->number;i++){
        P_RGB_FUN->brightness_table[i]=(i+1)*(RGB_BRIGHTNESS_LEVEL/P_RGB_FUN->info->number);
        printf("xx %d %d\n",i,P_RGB_FUN->brightness_table[i]);
    }

    user_rgb_same_colour(P_RGB_FUN->info,&user_rgb_fun.cur_colour);

    user_rgb_init(P_RGB_FUN->info);

    user_rgb_mode_scan(P_RGB_FUN);
#endif
}

