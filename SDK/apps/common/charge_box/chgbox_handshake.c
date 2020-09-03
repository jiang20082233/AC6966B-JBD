#include "gpio.h"
#include "app_config.h"
#include "system/includes.h"
#include "chgbox_ctrl.h"
#include "asm/adc_api.h"
#include "chgbox_box.h"
#include "device/chargebox.h"
#include "chgbox_handshake.h"

#if(TCFG_CHARGE_BOX_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHG_HS]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

extern void delay_2ms(int cnt);


/*------------------------------------------------------------------------------------*/
/**@brief    关闭所有中断使能
   @param    无
   @return   无
   @note     同时记录下状态，以便恢复
*/
/*------------------------------------------------------------------------------------*/
static u8 irq_ie_tab[10];
void chgbox_irq_disable_backup()
{
    u8 i;
    u8 x, y;
    local_irq_disable();
    memset(irq_ie_tab, 0, sizeof(irq_ie_tab) / sizeof(irq_ie_tab[0]));
    for (i = 0; i < MAX_IRQ_ENTRY_NUM; i++) {
        if (irq_read(i)) {
            x = i / 8;
            y = i % 8;
            irq_ie_tab[x] |= BIT(y);
        }
    }

    for (i = 0; i < MAX_IRQ_ENTRY_NUM; i++) {
        irq_disable(i);
    }
    local_irq_enable();
}

/*------------------------------------------------------------------------------------*/
/**@brief    中断使能恢复
   @param    无
   @return   无
   @note     用于恢复用chgbox_irq_disable_backup关闭的中断
*/
/*------------------------------------------------------------------------------------*/
void chgbox_irq_enable_revert()
{
    u8 i;
    u8 x, y;
    local_irq_disable();
    for (i = 0; i < MAX_IRQ_ENTRY_NUM; i++) {
        x = i / 8;
        y = i % 8;
        if (irq_ie_tab[x] & BIT(y)) {
            irq_enable(i);
        }
    }
    local_irq_enable();
}


//关于handshake部分
//////////////////////////////////////////////////////////////////////////
void chgbox_timer2_delay_us(u8 us);

struct _hs_hdl hs_ctrl = {
    .port = CHGBOX_HANDSHAKE_IO,                //初始化IO
    .send_delay_us = chgbox_timer2_delay_us,    //注册延时函数
};

/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手初始化
   @param    无
   @return   无
   @note     初始化io及注册延时
*/
/*------------------------------------------------------------------------------------*/
void chgbox_handshake_init(void)
{
    //部分封装与其他IO绑定在一起,注意设置成高阻
    gpio_direction_input(IO_PORTC_07);
    gpio_set_die(IO_PORTC_07, 0);
    gpio_set_pull_down(IO_PORTC_07, 0);
    gpio_set_pull_up(IO_PORTC_07, 0);

    handshake_ctrl_init(&hs_ctrl);
}

u8  hs_cur_sys_clk = 0;   //当前时钟匹配值
u32 hs_small_clk = 0;     //小于48m的系统时钟,记录，用来恢复
u16 delay_tap[HS_DELAY_240M + 1][HS_DELAY_16US + 1] = {
    19, 39, 52, 141, 158, 309, 355,//48m
    23, 45, 59, 146, 165, 315, 359,//60m
    27, 51, 66, 148, 170, 318, 367,//80m
    30, 55, 73, 151, 173, 320, 369,//96m
    32, 58, 75, 154, 177, 323, 370,//120m
    37, 60, 79, 157, 178, 325, 374,//160m
    38, 62, 82, 158, 179, 327, 376,//192m
    40, 64, 84, 159, 184, 328, 378,//240m
};

/*------------------------------------------------------------------------------------*/
/**@brief    获取当前的时钟,设置对应的表值
   @param    无
   @return   无
   @note     该值匹配delay_tap
*/
/*------------------------------------------------------------------------------------*/
void set_hs_cur_sys_clk(void)
{
    u32 cur_clock = 0;
    cur_clock = clk_get("sys");
    if (cur_clock < 48000000) { //小于48m时，转成48m，结束后恢复原来的时钟
        hs_small_clk = cur_clock;
        clk_set("sys", 48 * 1000000L);
        hs_cur_sys_clk = HS_DELAY_48M;
        puts("rest 48m\n");
    } else {
        switch (cur_clock) {
        case 48000000:
            hs_cur_sys_clk = HS_DELAY_48M;
            break;
        case 60000000:
            hs_cur_sys_clk = HS_DELAY_60M;
            break;
        case 80000000:
            hs_cur_sys_clk = HS_DELAY_80M;
            break;
        case 96000000:
            hs_cur_sys_clk = HS_DELAY_96M;
            break;
        case 120000000:
            hs_cur_sys_clk = HS_DELAY_120M;
            break;
        case 160000000:
            hs_cur_sys_clk = HS_DELAY_160M;
            break;
        case 192000000:
            hs_cur_sys_clk = HS_DELAY_192M;
            break;
        case 240000000:
            hs_cur_sys_clk = HS_DELAY_240M;
            break;
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    握手后重设置时钟
   @param    无
   @return   无
   @note     有改过时钟才重设置
*/
/*------------------------------------------------------------------------------------*/
void after_handshake_resume_small_clk(void)
{
    u32 cur_clock = 0;
    if (hs_small_clk) {
        clk_set("sys", hs_small_clk);
        hs_small_clk = 0;//清0
        cur_clock = clk_get("sys");
        printf("after handshake app :%d\n", cur_clock);
    }
}
/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手延时
   @param    无
   @return   无
   @note     提供不同的us级延时
*/
/*------------------------------------------------------------------------------------*/
SEC(.chargebox_code)
void chgbox_timer2_delay_us(u8 us)
{
    ////delay 值要根据不同的频率去调整，小于48m的要先设置48m，方便延时
    switch (us) {
    case HS_DELAY_2US:
        JL_TIMER2->PRD = delay_tap[hs_cur_sys_clk][HS_DELAY_2US];//24*2;
        break;
    case HS_DELAY_3US:
        JL_TIMER2->PRD = delay_tap[hs_cur_sys_clk][HS_DELAY_3US];//24*3;
        break;
    case HS_DELAY_4US:
        JL_TIMER2->PRD = delay_tap[hs_cur_sys_clk][HS_DELAY_4US];//24*4;
        break;
    case HS_DELAY_7US:
        JL_TIMER2->PRD = delay_tap[hs_cur_sys_clk][HS_DELAY_7US];//24*7;
        break;
    case HS_DELAY_8US:
        JL_TIMER2->PRD = delay_tap[hs_cur_sys_clk][HS_DELAY_8US];//24*8;
        break;
    case HS_DELAY_14US:
        JL_TIMER2->PRD = delay_tap[hs_cur_sys_clk][HS_DELAY_14US];//24*14;
        break;
    case HS_DELAY_16US:
        JL_TIMER2->PRD = delay_tap[hs_cur_sys_clk][HS_DELAY_16US];//24*16;
        break;
    }
    JL_TIMER2->CON = BIT(0) | BIT(3) | BIT(14); //1分频,osc时钟,24m，24次就1us
    while (!(JL_TIMER2->CON & BIT(15))); //等pending
    JL_TIMER2->CON = 0;
}


/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手
   @param    无
   @return   无
   @note     注意：为了精确的时间，会关闭其他中断
*/
/*------------------------------------------------------------------------------------*/
void chgbox_handshake_run_app(void)
{
    delay_2ms(2);
    chgbox_irq_disable_backup();
    set_hs_cur_sys_clk();
    handshake_send_app(HS_CMD0);
    chgbox_irq_enable_revert();

    delay_2ms(2);

    chgbox_irq_disable_backup();
    set_hs_cur_sys_clk();
    handshake_send_app(HS_CMD1);
    chgbox_irq_enable_revert();

    delay_2ms(2);

    chgbox_irq_disable_backup();
    set_hs_cur_sys_clk();
    handshake_send_app(HS_CMD2);
    chgbox_irq_enable_revert();

    delay_2ms(2);

    chgbox_irq_disable_backup();
    set_hs_cur_sys_clk();
    handshake_send_app(HS_CMD3);
    chgbox_irq_enable_revert();

    after_handshake_resume_small_clk();
}

//握手n次处理
#define  HS_REPEAT_MAX_TIMES  10
static u8 chgbox_hs_repeat_times = 0;  //重复次数
static u8 hs_repeat_cnt = 0;           //时间间隔计数,间隔时间==hs_repeat_cnt*HS_REPEAT_MAX_TIMES*调用间隔（比如：100ms或半秒）
/*------------------------------------------------------------------------------------*/
/**@brief    lighting重复握手初始化
   @param    times:重复次数
   @return   无
   @note     初始化多次握手的次数
*/
/*------------------------------------------------------------------------------------*/
void chgbox_handshake_set_repeat(u8 times)
{
    chgbox_hs_repeat_times = times;
    hs_repeat_cnt = 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting重复握手
   @param    无
   @return   无
   @note     usb上线后多次握手，增强兼容性,次数由 chgbox_hs_repeat_times决定
*/
/*------------------------------------------------------------------------------------*/
void chgbox_handshake_repeatedly(void)
{
    if (chgbox_hs_repeat_times) {
        hs_repeat_cnt++;
        if (hs_repeat_cnt == HS_REPEAT_MAX_TIMES) {
            hs_repeat_cnt = 0;
            chgbox_hs_repeat_times--;
            chgbox_handshake_run_app();
        }
    }
}

#endif
