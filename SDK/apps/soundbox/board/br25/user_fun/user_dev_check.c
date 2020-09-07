#include "user_fun_cfg.h"
// #include "board_config.h"
// #define user_print(...)  y_printf(__VA_ARGS__);
//#define user_print(...)  printf(__VA_ARGS__);

void ex_dev_io_start(void *arg)
{
    struct ex_dev_opr *io = (struct ex_dev_opr *)arg;
    if (io->init) {
        return ;
    }
    //printf("<<<io_start\n");
    io->init = 1;
    if (io->down) {
        gpio_set_pull_down(io->port, 1);
    } else {
        gpio_set_pull_down(io->port, 0);
    }
    if (io->up) {
        gpio_set_pull_up(io->port, 1);
    } else {
        gpio_set_pull_up(io->port, 0);
    }
    if (io->ad_channel == (u8)NO_CONFIG_PORT) {
        gpio_set_die(io->port, 1);
    } else {
        adc_add_sample_ch(io->ad_channel);
        gpio_set_die(io->port, 0);
    }
    gpio_set_hd(io->port, 0);
    gpio_set_hd0(io->port, 0);
    gpio_direction_input(io->port);
}

void ex_dev_io_stop(void *arg)
{
    struct ex_dev_opr *io = (struct ex_dev_opr *)arg;
    if (!io->init) {
        return ;
    }
    //printf("<<<io_stop\n");
    io->init = 0;

    if (io->ad_channel != (u8)NO_CONFIG_PORT) {
        adc_remove_sample_ch(io->ad_channel);
    }
    gpio_direction_input(io->port);
    gpio_set_pull_up(io->port, 0);
    gpio_set_pull_down(io->port, 0);
    gpio_set_hd(io->port, 0);
    gpio_set_hd0(io->port, 0);
    gpio_set_die(io->port, 0);
}

static int ex_dev_check(void *arg, u8 cnt)
{
    struct ex_dev_opr *dev = (struct ex_dev_opr *)arg;
    u8 cur_stu;

    if (dev->ad_channel == (u8)NO_CONFIG_PORT) {
        cur_stu = gpio_read(dev->port) ? false : true;
    } else {
        cur_stu = adc_get_value(dev->ad_channel) > dev->ad_vol ? false : true;
        /* printf("<%d> ", adc_get_value(dev->ad_channel)); */
    }
    if (!dev->up) {
        cur_stu	= !cur_stu;
    }
    //user_print("cur_stu :%d\n",cur_stu);

    if (cur_stu != dev->stu) {
        dev->stu = cur_stu;
        dev->cnt = 0;
        dev->active = 1;
    } else {
        dev->cnt ++;
    }

    if (dev->cnt < cnt) {
        return DET_HOLD;
    }

    dev->active = 0;

    if ((dev->online) && (!dev->stu)) {
        dev->online = false;
        user_print("<%s off line>\n",dev->dev_name);
        return DET_OFF;
    } else if ((!dev->online) && (dev->stu)) {
        dev->online = true;
        user_print("<%s on line>\n",dev->dev_name);
        return DET_ON;
    }
    return DET_HOLD;
}


extern void adc_enter_occupy_mode(u32 ch);
extern void adc_exit_occupy_mode();
extern u32 adc_occupy_run();
extern u32 adc_get_occupy_value();

int ex_dev_io_sample_detect(void *arg)
{
    struct ex_dev_opr *dev = (struct ex_dev_opr *)arg;
    ex_dev_io_start(dev);

    u8 cur_stu;
    if (dev->ad_channel == (u8)NO_CONFIG_PORT) {
        cur_stu = gpio_read(dev->port) ? false : true;
    } else {
        adc_enter_occupy_mode(dev->ad_channel);
        if (adc_occupy_run()) {
            u32 tp = adc_get_occupy_value();
            // printf("user ad name %s %d\n",dev->dev_name,tp);
            cur_stu = tp > dev->ad_vol ? false : true;
        } else {
            cur_stu = dev->stu;
        }
        adc_exit_occupy_mode();
        /* printf("\n<%d>\n", adc_get_voltage(linein_dev->ad_channel)); */
    }

    /* putchar('A'+cur_stu); */
    /* putchar(cur_stu); */
    ex_dev_io_stop(dev);

    if (cur_stu == dev->stu) {
        dev->cnt = 0;
    } else {
        dev->cnt++;
        if (dev->cnt == 3) {
            dev->cnt = 0;
            dev->stu = cur_stu;
            if (dev->stu == true) {
                dev->online = true;
                user_print("<on line>\n");
                return DET_ON;
            } else {
                dev->online = false;
                user_print("<off line>\n");
                return DET_OFF;
            }
        }
    }

    return DET_HOLD;
}

static void ex_dev_detect(void *arg)
{
    int res;
    struct ex_dev_opr *dev = arg;

    extern s32 sd_io_suspend(u8 sd_io);
    extern s32 sd_io_resume(u8 sd_io);

// #ifdef TCFG_IO_DET_MULTIPLEX_WITH_SD
    if(dev->multiplex_sd){
        if (sd_io_suspend(0) == 0) {
            res = ex_dev_io_sample_detect(arg);
            sd_io_resume(0);
        } else {
            return;
        }
    }else{
// #else
        if (dev->step == 0) {
            dev->step = 1;
            ex_dev_io_start(dev);
            sys_timer_modify(dev->timer, dev->scan_time);
            return ;
        }

        res = ex_dev_check(dev, 3);
        if (!dev->active) {
            dev->step = 0;
            ex_dev_io_stop(dev);
            sys_timer_modify(dev->timer, 500);
        }
    }
// #endif

    if(dev->msg != 0) {
        if (res == DET_ON) {            
            app_task_put_key_msg(dev->msg,1);
        } else if (res == DET_OFF) {
            app_task_put_key_msg(dev->msg,0);
        }
    }
}

char ex_dev_detect_init(void *arg)
{
    if(!arg)return 0;

    struct ex_dev_opr *ex_dev = (struct ex_dev_opr *)arg;
    if (ex_dev->enable) {
        if (ex_dev->port != (u8)NO_CONFIG_PORT) {
            ex_dev->timer = sys_timer_add(ex_dev, ex_dev_detect, ex_dev->scan_time);
        } else {
            ex_dev->online = true;
        }
    }
    return 0;
}


