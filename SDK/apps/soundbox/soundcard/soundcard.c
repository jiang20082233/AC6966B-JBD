#include "soundcard/soundcard.h"
#include "soundcard/lamp.h"
#include "soundcard/peripheral.h"
#include "common/user_msg.h"
#include "audio_reverb.h"
#include "system/timer.h"
#include "os/os_api.h"
#include "asm/audio_linein.h"
#include "key_event_deal.h"
#include "soundcard/notice.h"
#include "app_task.h"
#include "btstack/avctp_user.h"
#include "audio_dec.h"
#include "btstack/avctp_user.h"
#include "pitchshifter/pitchshifter_api.h"
/* #include "effects_config.h" */

#if SOUNDCARD_ENABLE
#define MIC_TYPE_MIC            BIT(0)
#define MIC_TYPE_EAR_MIC        BIT(1)

#define REVERB_EQ_NULL  		0
#define REVERB_EQ_MC  			1  //喊麦
#define REVERB_EQ_BASS  		2
#define REVERB_EQ_TREBLE  		3

struct soundcard {
    u8 mic_mode;
    u8 electric_mode;
    u8 pitch_mode;
    u8 magic_mode;
    u8 boom_mode;
    u8 mic_priority_mode;
    u8 dodge_mode;
    u8 mic_online;

    ////slider, vol
    u8 mic_vol;
    u16 echo_vol;
    u16 rec_vol;
    u16 music_vol;
    u16 monitor_vol;

    u16 bass_level;
    u16 treble_level;

    int reverb_set_timer;
    int mic_set_timer;
};

struct soundcard soundcard_hdl = {
    .mic_mode = 0xff,
    .electric_mode = 0,
    .pitch_mode = 0,
    .magic_mode = 0,
    .boom_mode = 0,
    .mic_priority_mode = 0,
    .dodge_mode = 0,
    .mic_online = 0,
    .reverb_set_timer = 0,
};
#define __this  (&soundcard_hdl)


#define SOUNDCARD_AUDIO_REVERB			0
#define SOUNDCARD_AUDIO_MUSIC  			1


s8 soundcard_audio_get_volume(u8 ch)
{
    switch (ch) {
    case SOUNDCARD_AUDIO_REVERB:
        return __this->echo_vol;
    case SOUNDCARD_AUDIO_MUSIC:
        return __this->music_vol;
    default:
        break;
    }
    return 0;
}


static const u16 boom_mode_tab[] = {
    EFFECTS_MODE_REVERB,
    EFFECTS_MODE_BOOM,
};
static const u16 magic_mode_tab[] = {
    EFFECTS_MODE_REVERB,
    EFFECTS_MODE_MAGIC,
};
static const u16 shouting_wheat_mode_tab[] = {
    EFFECTS_MODE_REVERB,
    EFFECTS_MODE_SHOUTING_WHEAT,
};
static const u16 voice_change_mode_tab[] = {
    EFFECTS_MODE_REVERB,
    EFFECTS_MODE_BOY_TO_GIRL,
    EFFECTS_MODE_GIRL_TO_BOY,
    EFFECTS_MODE_KIDS,
};

static void soundcard_set_boom_mode(u16 mode)
{
    if (mode >= ARRAY_SIZE(boom_mode_tab)) {
        return ;
    }

    set_effects_mode(boom_mode_tab[mode]);
}
static void soundcard_set_magic_mode(u16 mode)
{
    if (mode >= ARRAY_SIZE(magic_mode_tab)) {
        return ;
    }

    set_effects_mode(magic_mode_tab[mode]);
}
static void soundcard_set_shouting_wheat_mode(u16 mode)
{
    if (mode >= ARRAY_SIZE(shouting_wheat_mode_tab)) {
        return ;
    }

    set_effects_mode(shouting_wheat_mode_tab[mode]);
}
static void soundcard_set_voice_change_mode(u16 mode)
{
    if (mode >= ARRAY_SIZE(voice_change_mode_tab)) {
        return ;
    }

    set_effects_mode(voice_change_mode_tab[mode]);
}



static void soundcard_switch_eq_mode(u8 type, u16 gain)
{
    switch (type) {
    case REVERB_EQ_MC:
        reverb_eq_cal_coef(0, 0, gain);
        break;
    case REVERB_EQ_BASS:
        reverb_eq_cal_coef(1, gain, 1);
        break;
    case REVERB_EQ_TREBLE:
        reverb_eq_cal_coef(2, gain, 1);
        break;
    default:
        y_printf("switch eq err\n");
        break;
    }
}

int set_reverb_vol(void)
{
#if (USER_DIGITAL_VOLUME_ADJUST_ENABLE != 0)
    reverb_user_digital_volume_set(__this->mic_vol);
#endif
    __this->mic_set_timer = 0;
    return 0;
}


void soundcard_user_msg_deal(int msg, int argc, int *argv)
{
    switch (msg) {
    case USER_MSG_SYS_SOUNDCARD_SLIDE_MIC:
        __this->mic_vol = get_max_sys_vol() * argv[0] / 10;;
        y_printf("mic vol %d \n", __this->mic_vol);
        if (__this->mic_online) {
            reverb_user_digital_volume_set(__this->mic_vol);
        }
        break;
    case USER_MSG_SYS_SOUNDCARD_SLIDE_ECHO:
        y_printf("echo vol %d \n", argv[0]);
        __this->echo_vol = argv[0] * 15;
        set_reverb_wetgain(__this->echo_vol);
        break;
    case USER_MSG_SYS_SOUNDCARD_SLIDE_BASS:
        __this->bass_level = argv[0];//4800 - argv[0]*4800/10;
        y_printf("BASS vol %d \n", argv[0]);
        soundcard_switch_eq_mode(REVERB_EQ_BASS, __this->bass_level);
        break;

    case USER_MSG_SYS_SOUNDCARD_SLIDE_TREBLE:
        __this->treble_level = argv[0];//4800 - argv[0]*4800/10;
        y_printf("treble vol %d \n", argv[0]);
        soundcard_switch_eq_mode(REVERB_EQ_TREBLE, __this->treble_level);
        break;
    case USER_MSG_SYS_SOUNDCARD_SLIDE_REC_VOL:
        __this->rec_vol = get_max_sys_vol() * argv[0] / 30;
        y_printf("rec vol %d \n", argv[0]);
        {
            extern void audio_set_hw_digital_vol_rl(s16 rl);
            extern void audio_set_hw_digital_vol_rr(s16 rr);
            audio_set_hw_digital_vol_rl(__this->rec_vol);
            audio_set_hw_digital_vol_rr(__this->rec_vol);
        }
        break;
    case USER_MSG_SYS_SOUNDCARD_SLIDE_MUSIC_VOL:
        y_printf("music vol %d \n", argv[0]);
        /* __this->music_vol = get_max_sys_vol() * argv[0] / 30; */
        __this->music_vol = 80 * argv[0] / 30;//get_max_sys_vol() * argv[0] / 30;
#if (USER_DIGITAL_VOLUME_ADJUST_ENABLE != 0)
        a2dp_user_digital_volume_set(__this->music_vol);
        linein_user_digital_volume_set(__this->music_vol);

#if TCFG_APP_PC_EN
        pc_user_digital_volume_set(__this->music_vol);
#endif
#endif
        break;
    case USER_MSG_SYS_SOUNDCARD_SLIDE_EAR_VOL:
        __this->monitor_vol = get_max_sys_vol() * argv[0] / 30;
        y_printf("ear vol %d \n", __this->monitor_vol);
        {
            extern void audio_set_hw_digital_vol_fl(s16 fl);
            extern void audio_set_hw_digital_vol_fr(s16 fr);
            audio_set_hw_digital_vol_fl(__this->monitor_vol);
            audio_set_hw_digital_vol_fr(__this->monitor_vol);
        }
        break;
    case USER_MSG_SYS_SOUNDCARD_MIC1_STATUS:
        if (argv[0]) {
            __this->mic_online |= MIC_TYPE_EAR_MIC;
            if (__this->mic_online & MIC_TYPE_MIC) {
                soundcard_ear_mic_mute(1);
            } else {
                soundcard_ear_mic_mute(0);
            }
            if (reverb_if_working()) {
#if (USER_DIGITAL_VOLUME_ADJUST_ENABLE != 0)
                //reverb_user_digital_volume_set(__this->echo_vol);
#endif
                __this->mic_set_timer = sys_timeout_add(NULL, set_reverb_vol, 3000);
            } else {
                y_printf("reverb is idle or mic online\n");
            }
        } else {
            soundcard_ear_mic_mute(1);
            __this->mic_online &= ~MIC_TYPE_EAR_MIC;
            if (reverb_if_working() && (__this->mic_online == 0)) {
                if (__this->mic_set_timer) {
                    sys_timeout_del(__this->mic_set_timer);
                    __this->mic_set_timer = 0;
                }
#if (USER_DIGITAL_VOLUME_ADJUST_ENABLE != 0)
                reverb_user_digital_volume_set(0);
#endif
            } else {
                y_printf("reverb is idle or mic online\n");
            }
        }
        break;

    case USER_MSG_SYS_SOUNDCARD_MIC2_STATUS:
        if (argv[0]) {
            __this->mic_online |= MIC_TYPE_MIC;
            soundcard_ear_mic_mute(1);
            if (reverb_if_working()) {
#if (USER_DIGITAL_VOLUME_ADJUST_ENABLE != 0)
                //reverb_user_digital_volume_set(__this->echo_vol);
#endif
                __this->mic_set_timer = sys_timeout_add(NULL, set_reverb_vol, 1000);
                //sys_timeout_add(NULL, set_reverb_parm_deal, 1000);
            } else {
                y_printf("reverb is idle or mic online\n");
            }
        } else {
            __this->mic_online &= ~MIC_TYPE_MIC;
            if (__this->mic_online & MIC_TYPE_EAR_MIC) {
                soundcard_ear_mic_mute(0);
            }
            if (reverb_if_working() && (__this->mic_online == 0)) {
                if (__this->mic_set_timer) {
                    sys_timeout_del(__this->mic_set_timer);
                    __this->mic_set_timer = 0;
                }
#if (USER_DIGITAL_VOLUME_ADJUST_ENABLE != 0)
                reverb_user_digital_volume_set(0);
#endif

            } else {
                y_printf("reverb is idle or mic online\n");
            }
        }
        break;
    case USER_MSG_SYS_SOUNDCARD_AUX0_STATUS:
        if (argv[0]) {
            linein_user_digital_volume_set(__this->music_vol);
        } else {
            linein_user_digital_volume_set(0);
        }
        break;
    default:
        break;
    }
}

void soundcard_key_event_deal(u8 key_event)
{
    y_printf("key_event:%d\n", key_event);
    switch (key_event) {
    case  KEY_ELECTRIC_MODE:
        /* soundcard_led_set(UI_LED_ELECTRIC_SOUND,1); */
        if (__this->mic_mode == ECHO_ELECTRIC_MODE) {
            __this->electric_mode++;
            if (__this->electric_mode > get_effects_electric_mode_max()) {
                __this->electric_mode = 0;
            }
        }
        __this->mic_mode = ECHO_ELECTRIC_MODE;
        soundcard_make_notice_electric(__this->electric_mode);
        soundcard_led_mode(__this->mic_mode, 1);

        set_effects_electric_mode(__this->electric_mode);

        break;
    case  KEY_PITCH_MODE:
        if (__this->mic_mode == ECHO_PITCH_MODE) {
            __this->pitch_mode++;
        } else {
            __this->pitch_mode = 1;
        }
        __this->mic_mode = ECHO_PITCH_MODE;
        switch (__this->pitch_mode) {
        case 0:
            //tone_play_index(IDEX_TONE_ELECTRIC_A, 0);
            break;
        case 1:
            //tone_play_index(IDEX_TONE_ELECTRIC_AS, 0);
            break;
        case 2:
            //tone_play_index(IDEX_TONE_ELECTRIC_A, 0);
            break;
        case 3:
            //tone_play_index(IDEX_TONE_ELECTRIC_AS, 0);
            break;
        default:
            __this->pitch_mode = 0;
            __this->mic_mode = ECHO_KTV_MODE;
            //tone_play_index(IDEX_TONE_ELECTRIC_A, 0);
            break;
        }

        soundcard_led_mode(__this->mic_mode, 1);
        soundcard_set_voice_change_mode(__this->pitch_mode);
        break;
    case  KEY_MAGIC_MODE:
        if (__this->mic_mode == ECHO_MAGIC_MODE) {
            __this->magic_mode++;
        } else {
            __this->magic_mode = 1;
        }
        __this->mic_mode = ECHO_MAGIC_MODE;
        switch (__this->magic_mode) {
        case 0:
            //tone_play_index(IDEX_TONE_ELECTRIC_A, 0);
            break;
        case 1:
            //tone_play_index(IDEX_TONE_ELECTRIC_AS, 0);
            break;
        default:
            __this->magic_mode = 0;
            __this->mic_mode = ECHO_KTV_MODE;
            //tone_play_index(IDEX_TONE_ELECTRIC_A, 0);
            break;
        }
        soundcard_led_mode(__this->mic_mode, 1);
        set_effects_mode(EFFECTS_MODE_MAGIC);
        soundcard_set_magic_mode(__this->magic_mode);
        break;
    case KEY_BOOM_MODE:
        if (__this->mic_mode == ECHO_BOOM_MODE) {
            __this->boom_mode++;
        } else {
            __this->boom_mode = 1;
        }
        __this->mic_mode = ECHO_BOOM_MODE;
        switch (__this->boom_mode) {
        case 0:
            //tone_play_index(IDEX_TONE_ELECTRIC_A, 0);
            break;
        case 1:
            //tone_play_index(IDEX_TONE_ELECTRIC_AS, 0);
            break;
        default:
            __this->boom_mode = 0;
            __this->mic_mode = ECHO_KTV_MODE;
            //tone_play_index(IDEX_TONE_ELECTRIC_A, 0);
            break;
        }
        soundcard_led_mode(__this->mic_mode, 1);
        soundcard_set_boom_mode(__this->boom_mode);
        break;
    case  KEY_MIC_PRIORITY_MODE:
        if (__this->mic_mode == ECHO_MIC_PRIORITY_MODE) {
            __this->mic_priority_mode++;
        } else {
            __this->mic_priority_mode = 1;
        }
        __this->mic_mode = ECHO_MIC_PRIORITY_MODE;
        switch (__this->mic_priority_mode) {
        case 0:
            //tone_play_index(IDEX_TONE_ELECTRIC_A, 0);
            break;
        case 1:
            //tone_play_index(IDEX_TONE_ELECTRIC_AS, 0);
            break;
        default:
            __this->mic_priority_mode = 0;
            __this->mic_mode = ECHO_KTV_MODE;
            //tone_play_index(IDEX_TONE_ELECTRIC_A, 0);
            break;
        }
        soundcard_led_mode(__this->mic_mode, 1);
        soundcard_set_shouting_wheat_mode(__this->mic_priority_mode);
        soundcard_switch_eq_mode(REVERB_EQ_MC, __this->mic_priority_mode);
        break;
    case  KEY_DODGE_MODE:
        y_printf("KEY_DODGE_MODE\n");
        if (__this->dodge_mode) {
            __this->dodge_mode = 0;
        } else {
            __this->dodge_mode = 1;
        }
        soundcard_led_mode(ECHO_DODGE_MODE, __this->dodge_mode);
        {
#if 1
            extern void reverb_dodge_open_api(u8 dodge_en);
            reverb_dodge_open_api(__this->dodge_mode);
#endif
        }
        break;
    case  KEY_TONE_huanhu:
    case  KEY_TONE_ganga:
    case  KEY_TONE_qiangsheng:
    case  KEY_TONE_bishi:
    case  KEY_TONE_chuchang:
    case  KEY_TONE_feiwen:
#if 0
    {
        update_reverb_parm_debug(key_event - KEY_TONE_huanhu, 1, 0);
    }
    break;
#endif
    case  KEY_TONE_xiaosheng:
    case  KEY_TONE_zhangsheng:
    case  KEY_TONE_guanzhu:
    case  KEY_TONE_momoda:
    case  KEY_TONE_zeilala:
    case  KEY_TONE_feichengwurao:
#if 0
    {
        update_reverb_parm_debug(key_event - KEY_TONE_xiaosheng, 0, 0);
    }
#endif
    soundcard_make_some_noise(key_event - KEY_TONE_huanhu);
    break;
    case  KEY_USB_MIC_CH_SWITCH:
        printf("KEY_USB_MIC_CH_SWITCH\n");
#if TCFG_APP_PC_EN
        {
            u8 cur_filt_state = usb_audio_mix_out_ext0_filt_switch_state();
            if (cur_filt_state) {
            } else {
            }
            usb_audio_mix_out_ext0_filt_switch();
        }
#endif
        break;
    case  KEY_KTV_TEST:
        y_printf("KEY_KTV_TEST\n");
        static u8 test_flag = 0;
        extern void mic_2_dac_rl(u8 r_en, u8 l_en);
        if (reverb_if_working() == 0) {

            mic_2_dac_rl(0, 0);
            start_reverb_mic2dac(NULL);
            mic_2_dac_rl(0, 0);
            set_mic_gain(0);
            //extern void set_reverb_wetgain(u16 value);
            set_reverb_wetgain(0);
        } else {
            y_printf("mic on\n");
            set_mic_gain(7);
            stop_reverb_mic2dac();
            mic_2_dac_rl(1, 1);
        }
        break;
    default:
        break;
    }
}

void soundcard_bt_connect_status_event(u8 event)
{
    switch (event) {
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        soundcard_led_self_set(UI_LED_SELF_BLUE_FLASH, 0);
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        soundcard_led_self_set(UI_LED_SELF_BLUE_FLASH, 1);
        break;
    default:
        break;
    }
}

#endif//SOUNDCARD_ENABLE


