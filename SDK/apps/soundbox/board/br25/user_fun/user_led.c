#include "user_fun_cfg.h"


#if USER_LED_EN
LED_IO user_led_io={
    .por = USER_LED_POR,
    .on  = 1,//高亮

    .status = 0xff,
    .init_ok = 0,
};
#endif

#if (USER_LED_EN && USER_RGB_EN && (USER_LED_POR == IO_PORTB_07))
#error 请确保 led与rgb 使用的io不为同一个io
#endif

void user_led_io_fun(void *priv,u8 cmd){
    LED_IO *io = (LED_IO *)priv;
    if(!io){
        return;
    }

#if USER_LED_EN
    // LED_IO *io_p = io;

    if(!io || io->por == NO_CONFIG_PORT){
        puts("led io strl error\n");
        return ;
    }

    if(!io->init_ok && cmd != LED_IO_INIT){
        user_led_io_fun(io,LED_IO_INIT);
    }

    if(LED_IO_INIT == cmd){
        user_print("LED_IO_INIT \n");
        gpio_set_pull_down(io->por, 0);
        gpio_set_pull_up(io->por, 0); 
        gpio_set_die(io->por, 1);
        gpio_set_direction(io->por,0);

        io->init_ok = 1;
        user_led_io_fun(io,LED_POWER_OFF);
    }else if(LED_POWER_ON == cmd){
        user_print("LED_POWER_ON \n");
        gpio_set_output_value(io->por,io->on);
        io->status = cmd;
    }else if(LED_POWER_OFF == cmd){
        user_print("LED_POWER_OFF \n");
        gpio_set_output_value(io->por,!io->on);
        io->status = cmd;
    }else if(LED_IO_FLIP == cmd){
        user_print("LED_IO_FLIP \n");
        if(LED_POWER_OFF == io->status){
            user_led_io_fun(io,LED_POWER_ON);
        }else{
            user_led_io_fun(io,LED_POWER_OFF);
        }
    }
#endif    
}
