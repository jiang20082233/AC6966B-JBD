#ifndef _USER_LED_H_
#define _USER_LED_H_
// #include "cpu.h"

//LED
enum {
    //parm
    LED_IO_INIT = 0,
    LED_POWER_ON,
    LED_POWER_OFF,
    LED_IO_FLIP,//IO翻转
};

typedef struct USER_LED_IO{
    u32 por;//io
    bool on;

    bool init_ok;
    u8 status;
    // void (* strl)(void *io,u8 cmd);
}LED_IO;

#if USER_LED_EN
extern LED_IO user_led_io;
#define USER_IO_LED (&user_led_io)
#else
#define USER_IO_LED (NULL)
#endif
void user_led_io_fun(void *priv,u8 cmd);
#endif