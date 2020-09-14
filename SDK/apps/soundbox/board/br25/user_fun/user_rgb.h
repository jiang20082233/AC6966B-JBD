#ifndef _USER_RGB_H_
#define _USER_RGB_H_

/*
最大亮度等级 注意数据类型 数值不要超范围
平常使用8位灯珠最大值不超255 使用4k的灯珠请修改数据类型
等级
*/
#define RGB_BRIGHTNESS_LEVEL  (200)
/*
作为控制灯的 淡入 淡出 阀值使用
把亮度分为几个等级 (段)
eg.
RGB_BRIGHTNESS_LEVEL ：120
RGB_BRIGHTNESS_SEGMENT：4
亮度小于 120/4 的灯为淡入
亮度大于 120/4 且小于 (120/4)*2为淡出
*/
#define RGB_BRIGHTNESS_SEGMENT (4)
typedef struct _USER_RGB_DISPLAY_DATA_{
    int sys_vol_max;
    int sys_vol;
    int bass;
    u16 display_time;//打断时间 单位s
}RGB_DISPLAY_DATA;

typedef enum{
    USER_RGB_MODE_1=0,
    USER_RGB_MODE_2,
    USER_RGB_MODE_3,
    USER_RGB_MODE_4,
    USER_RGB_MODE_OFF,
    USER_RGB_MODE_MAX,
    USER_RGB_POWER_OFF,
    USER_RGB_AUTO_SW,//自动切换
    USER_RGB_SYS_VOL,//音量显示
    USER_RGB_EQ_BASS,//BASS状态显示
}USER_GRB_MODE;

typedef struct _USER_RGB_FUN_{
    RGB_INFO *info;
    bool power_off;
    bool interrupt;//打断标志  用于音量等其他非模式效果的显示
    u16 interrupt_id;//打断任务 id bass、vol显示公用此id

    u8 brightness_table[USER_RGB_NUMBER];//亮度表
    u8 step_value; //亮度步进值

    USER_GRB_MODE cur_mode;//当前模式
    u16 mode_scan_time;//模式扫描时间
    RGB_COLOUR cur_colour;//当前颜色
}RGB_FUN;


void user_rgb_fun_init(void);
void user_rgb_mode_set(USER_GRB_MODE mode,void *priv);
void user_rgb_display_vol(u8 vol,u16 display_time);
void user_rgb_display_bass(u8 bass,u16 display_time);
#endif