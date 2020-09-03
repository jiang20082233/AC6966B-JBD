#include "soundcard/lamp.h"
#include "system/device/vm.h"
#include "asm/rtc.h"
#include "asm/power/p33.h"
#include "asm/gpio.h"
#include "soundcard/soundcard.h"
#include "system/timer.h"

#if SOUNDCARD_ENABLE

static u8 my_led_status = 0;
static u8 my_led_status_self = 0;  //独立的LED
static void led_port_set_hz(u8 port)
{
    gpio_set_pull_down(port, 0);
    gpio_set_pull_up(port, 0);
    gpio_set_direction(port, 1);
}
void my_led_clear_all(void)
{
    led_port_set_hz(TCFG_MY_LED1_PORT);
    led_port_set_hz(TCFG_MY_LED2_PORT);
    led_port_set_hz(TCFG_MY_LED3_PORT);
    led_port_set_hz(TCFG_MY_LED4_PORT);
}
static void __my_led_scan(void *param)
{
    static u8 cnt = 0;
    my_led_clear_all();
    if (get_vm_statu()) {
        return;
    }
    switch (cnt) {
    case 0:
        gpio_direction_output(TCFG_MY_LED1_PORT, 0);
        if (my_led_status & UI_LED_ELECTRIC_SOUND) {
            gpio_direction_output(TCFG_MY_LED2_PORT, 1);
        }
        if (my_led_status & UI_LED_PITCH_SOUND) {
            gpio_direction_output(TCFG_MY_LED3_PORT, 1);
        }
        if (my_led_status & UI_LED_MAGIC_SOUND) {
            gpio_direction_output(TCFG_MY_LED4_PORT, 1);
        }
        break;
    case 1:
        gpio_direction_output(TCFG_MY_LED2_PORT, 0);
        if (my_led_status & UI_LED_BOOM_SOUND) {
            gpio_direction_output(TCFG_MY_LED3_PORT, 1);
        }
        if (my_led_status & UI_LED_MIC_SOUND) {
            gpio_direction_output(TCFG_MY_LED4_PORT, 1);
        }
        break;
    case 2:
        gpio_direction_output(TCFG_MY_LED4_PORT, 0);
        if (my_led_status_self & UI_LED_SELF_DODGE_SOUND) {
            gpio_direction_output(TCFG_MY_LED3_PORT, 1);
        }
        break;
    default:
        putchar('a');
        break;
    }
    cnt = (cnt >= 2) ? 0 : cnt + 1;
}

static void __my_led2_scan(void *param)
{
    static u8 cnt = 0;

    if (my_led_status_self & UI_LED_SELF_RED) {
        gpio_set_output_value(TCFG_MY_LED_RED_PORT, 1);
    } else {
        gpio_set_output_value(TCFG_MY_LED_RED_PORT, 0);
    }

    if (my_led_status_self & UI_LED_SELF_BLUE_FLASH) {
        if (cnt) {
            gpio_set_output_value(TCFG_MY_LED_BLUE_PORT, 1);
        } else {
            gpio_set_output_value(TCFG_MY_LED_BLUE_PORT, 0);
        }
    } else {
        gpio_set_output_value(TCFG_MY_LED_BLUE_PORT, 1);
    }

    cnt = (cnt >= 1) ? 0 : cnt + 1;
}

void soundcard_led_set(u8 led, u8 sw)
{
    y_printf("%s:%d", __func__, led);
    if (sw) {
        my_led_status = led;
    } else {
        my_led_status = 0;
    }
}

void soundcard_led_self_set(u8 led, u8 sw)
{
    y_printf("%s:%d", __func__, led);

    if (sw) {
        my_led_status_self |= led;
    } else {
        my_led_status_self &= ~led;
    }

}

void soundcard_led_mode(u8 mode, u8 sw)
{
    switch (mode) {
    case ECHO_KTV_MODE:
        soundcard_led_set(UI_LED_OFF, sw);
        break;
    case ECHO_ELECTRIC_MODE:
        soundcard_led_set(UI_LED_ELECTRIC_SOUND, sw);
        break;
    case ECHO_PITCH_MODE:
        soundcard_led_set(UI_LED_PITCH_SOUND, sw);
        break;
    case ECHO_MAGIC_MODE:
        soundcard_led_set(UI_LED_MAGIC_SOUND, sw);
        break;
    case ECHO_BOOM_MODE:
        soundcard_led_set(UI_LED_BOOM_SOUND, sw);
        break;
    case ECHO_MIC_PRIORITY_MODE:
        soundcard_led_set(UI_LED_MIC_SOUND, sw);
        break;
    case ECHO_DODGE_MODE:
        soundcard_led_self_set(UI_LED_SELF_DODGE_SOUND, sw);
        break;
    default:
        break;
    }
}


void soundcard_led_init(void)
{
    y_printf("%s", __func__);

    my_led_clear_all();

    gpio_set_direction(TCFG_MY_LED_BLUE_PORT, 0);
    gpio_set_direction(TCFG_MY_LED_RED_PORT, 0);
    soundcard_led_self_set(UI_LED_SELF_RED, 1);
    soundcard_led_self_set(UI_LED_SELF_BLUE_FLASH, 1);

    sys_hi_timer_add(NULL, __my_led_scan, 2); //2ms
    sys_hi_timer_add(NULL, __my_led2_scan, 500); //2ms
}

#endif//SOUNDCARD_ENABLE
