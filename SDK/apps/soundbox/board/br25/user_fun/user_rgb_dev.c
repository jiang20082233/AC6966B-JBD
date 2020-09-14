#include "user_fun_cfg.h"

#if USER_RGB_EN



//把单个rgb数据转换成spi数
void user_rgb_set_only(SPI_COLOUR *spi_buff,RGB_COLOUR *rgb){
    if(!spi_buff || !rgb){
        return;
    }

    for(int i=0;i<8;i++){
        if(rgb->r&BIT(7-i)){
            spi_buff->r[i]=USER_RGB_CODE1;
        }else{
            spi_buff->r[i]=USER_RGB_CODE0;
        }
        if(rgb->g&BIT(7-i)){
            spi_buff->g[i]=USER_RGB_CODE1;
        }else{
            spi_buff->g[i]=USER_RGB_CODE0;
        }
        if(rgb->b&BIT(7-i)){
            spi_buff->b[i]=USER_RGB_CODE1;
        }else{
            spi_buff->b[i]=USER_RGB_CODE0;
        }
    }
}

//把所有颜色数据转换成spi数据
void user_rgb_set_all(SPI_COLOUR *spi_buff,RGB_COLOUR *rgb,u8 number){
    if(!spi_buff || !rgb || !number){
        return;
    }

    for(int i=0;i<number;i++){
        user_rgb_set_only(&(spi_buff[i]),&(rgb[i]));
    }
    return;
}

//设置第几颗rgb颜色
/*
priv：rgb 配置数据
colour：rgb 颜色
number:离io口最近的那颗灯为第一颗，最后一颗可以表示为 -1或者(USER_RGB_NUMBER-1)
*/
void user_rgb_colour_only_set(void *priv,RGB_COLOUR *colour,s16 number){
    
    RGB_INFO *rgb = (RGB_INFO *)priv;
    s16 mapp_number = number;

    if(!rgb || !colour){
        return;
    }

    mapp_number%=rgb->number;

    mapp_number = mapp_number<0?(rgb->number+mapp_number):mapp_number;

    rgb->rgb_buff[mapp_number]=*colour;

    return;
}

//设置为同样颜色
void user_rgb_same_colour(void *priv,RGB_COLOUR *colour){
    RGB_INFO *rgb = (RGB_INFO *)priv;
    if(!rgb || !colour || !rgb->number){
        return;
    }

    for(int i=0;i<rgb->number;i++){
        user_rgb_colour_only_set(rgb,colour,i);
    }
}

//清除颜色
void user_rgb_clear_colour(void *priv){
    RGB_INFO *rgb = (RGB_INFO *)priv;
    RGB_COLOUR colour={0,0,0};

    if(!rgb || !rgb->number){
        return;
    }

    user_rgb_same_colour(rgb,&colour);
}

//发送spi数据
void user_rgb_send(void *priv){
    RGB_INFO *rgb = (RGB_INFO *)priv;

    if(!rgb){
        return;
    }

    if(rgb->power_off){
        // spi_open(rgb->spi_port);
        return;
    }

    if(rgb->init_flag && rgb && !rgb->updata_flag){
        rgb->rend_flag = 1;
        user_rgb_set_all(rgb->spi_buff,rgb->rgb_buff,rgb->number);
        rgb->rend_flag = 0;
        int send_ret = spi_dma_send(rgb->spi_port,(u8*)rgb->spi_buff,sizeof(SPI_COLOUR)*rgb->number);
        if(send_ret<0){
            printf("user rgb spi send data error\n");
        }
        sys_timeout_add(rgb,user_rgb_send,rgb->spi_scan_time);
    }else{
        sys_timeout_add(rgb,user_rgb_send,10);
    }
}

#endif

void user_rgb_delay_close_spi(void *priv){
    RGB_INFO *rgb = (RGB_INFO *)priv;
    if(!rgb){
        return;
    }

    spi_close(rgb->spi_port);
}

void user_rgb_power_off(void *priv){
    RGB_INFO *rgb = (RGB_INFO *)priv;
    if(!rgb || rgb->power_off){
        return;
    }

    rgb->power_off = 1;
    rgb->updata_flag = 1;
    rgb->rend_flag = 1;
    
    user_rgb_clear_colour(rgb);
    user_rgb_set_all(rgb->spi_buff,rgb->rgb_buff,rgb->number);
    spi_dma_send(rgb->spi_port,(u8*)rgb->spi_buff,sizeof(SPI_COLOUR)*rgb->number);
    sys_timeout_add(rgb,user_rgb_delay_close_spi,30);

}
//初始化
void user_rgb_init(void *priv){
    
    #if USER_RGB_EN
    RGB_INFO *rgb = (RGB_INFO *)priv;
    if(!rgb || SPI_MAX_HW_NUM<=(rgb->spi_port)){
        return;
    }
    int ret = spi_open(rgb->spi_port);
    if(ret<0){
        return;
    }
    rgb->init_flag = 1;

    if(!(rgb->spi_scan_time)){
        rgb->spi_scan_time = 100;
    }

    sys_timeout_add(rgb,user_rgb_send,rgb->spi_scan_time);//打开spi之后不能马上去发送数据 不然rgb会闪烁一下
    #endif
}

