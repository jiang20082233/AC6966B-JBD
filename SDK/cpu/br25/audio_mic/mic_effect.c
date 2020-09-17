#include "effect_tool.h"
#include "effect_debug.h"
/* #include "reverb/reverb_api.h" */
/* #include "application/audio_dig_vol.h" */
#include "audio_splicing.h"
/* #include "effect_config.h" */
#include "application/filtparm_api.h"
/* #include "audio_effect/audio_eq.h" */
#include "application/audio_eq_drc_apply.h"
#include "application/audio_output_dac.h"
#include "application/audio_echo_src.h"
#include "clock_cfg.h"
#include "media/audio_stream.h"
#include "media/includes.h"
#include "mic_effect.h"
#include "asm/dac.h"
#include "audio_enc.h"
#include "audio_dec.h"
#define LOG_TAG     "[APP-REVERB]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#if (TCFG_MIC_EFFECT_ENABLE)

extern struct audio_dac_hdl dac_hdl;
extern int *get_outval_addr(u8 mode);

enum {
    MASK_REVERB = 0x0,
    MASK_PITCH,
    MASK_ECHO,
    MASK_NOISEGATE,
    MASK_SHOUT_WHEAT,
    MASK_LOW_SOUND,
    MASK_HIGH_SOUND,
    MASK_EQ,
    MASK_EQ_SEG,
    MASK_EQ_GLOBAL_GAIN,
    MASK_MIC_GAIN,
    MASK_MAX,
};

typedef struct _BFILT_API_STRUCT_ {
    SHOUT_WHEAT_PARM_SET 	shout_wheat;
    LOW_SOUND_PARM_SET 		low_sound;
    HIGH_SOUND_PARM_SET 	high_sound;
    unsigned int			*ptr;         //运算buf指针
    BFILT_FUNC_API			*func_api;    //函数指针
} BFILT_API_STRUCT;

struct __fade {
    int wet;
    u32 delay;
    u32 decayval;
};
extern struct audio_mixer mixer;
struct __mic_effect {
    OS_MUTEX				 		mutex;
    struct __mic_effect_parm     	parm;
    struct __fade	 				fade;
    volatile u32					update_mask;
    mic_stream 						*mic;
    /* PITCH_SHIFT_PARM        		*p_set; */
    /* NOISEGATE_API_STRUCT    		*n_api; */
    BFILT_API_STRUCT 				*filt;
    struct audio_eq_drc             *eq_drc;    //eq drc句柄

    struct audio_stream *stream;		// 音频流
    struct audio_stream_entry entry;	// effect 音频入口
    int out_len;
    int process_len;

    struct audio_mixer_ch mix_ch;//for test

    REVERBN_API_STRUCT 		*p_reverb_hdl;
    ECHO_API_STRUCT 		*p_echo_hdl;
    s_pitch_hdl 			*p_pitch_hdl;
    NOISEGATE_API_STRUCT	*p_noisegate_hdl;
    void            		*d_vol;
    HOWLING_API_STRUCT 		*p_howling_hdl;
    ECHO_SRC_API_STRUCT 	*p_echo_src_hdl;
    struct audio_stream_dac_mix_out *p_last_out;
    u8 pause_mark;
};


static struct __mic_effect *p_effect = NULL;
#define __this  p_effect
#define R_ALIN(var,al)     ((((var)+(al)-1)/(al))*(al))

void *mic_eq_drc_open(u16 sample_rate, u8 ch_num);
void mic_eq_drc_close(struct audio_eq_drc *eq_drc);
void mic_eq_drc_update();
void mic_high_bass_coeff_cal_init(BFILT_API_STRUCT *filt, u16 sample_rate);
void mic_high_bass_coeff_cal_uninit(BFILT_API_STRUCT *filt);
void mic_effect_echo_parm_parintf(ECHO_PARM_SET *parm);
/*----------------------------------------------------------------------------*/
/**@brief    mic数据流音效处理参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_parm_update(struct __mic_effect *effect)
{
    for (int i = 0; i < MASK_MAX; i++) {
        if (effect->update_mask & BIT(i)) {
            effect->update_mask &= ~BIT(i);
            switch (i) {
            case MASK_REVERB:
                if (effect->p_reverb_hdl) {
                    effect->p_reverb_hdl->func_api->init(effect->p_reverb_hdl->ptr, &effect->p_reverb_hdl->parm);
                }
                break;
            case MASK_PITCH:
                if (effect->p_pitch_hdl) {
                    effect->p_pitch_hdl->ops->init(effect->p_pitch_hdl->databuf, &effect->p_pitch_hdl->parm);
                }
                break;
            case MASK_ECHO:
                if (effect->p_echo_hdl) {
                    effect->p_echo_hdl->func_api->init(effect->p_echo_hdl->ptr, &effect->p_echo_hdl->echo_parm_obj);
                }
                break;
            case MASK_NOISEGATE:
                if (effect->p_noisegate_hdl) {
                    printf("effect->p_noisegate_hdl->parm.attackTime=%d\n", effect->p_noisegate_hdl->parm.attackTime);
                    printf("effect->p_noisegate_hdl->parm.releaseTime=%d\n", effect->p_noisegate_hdl->parm.releaseTime);
                    printf("effect->p_noisegate_hdl->parm.threshold=%d\n", effect->p_noisegate_hdl->parm.threshold);
                    printf("effect->p_noisegate_hdl->parm.low_th_gain=%d\n", effect->p_noisegate_hdl->parm.low_th_gain);
                    printf("effect->p_noisegate_hdl->parm.sampleRate=%d\n", effect->p_noisegate_hdl->parm.sampleRate);
                    printf("effect->p_noisegate_hdl->parm.channel=%d\n", effect->p_noisegate_hdl->parm.channel);

                    noiseGate_init(effect->p_noisegate_hdl->ptr,
                                   effect->p_noisegate_hdl->parm.attackTime,
                                   effect->p_noisegate_hdl->parm.releaseTime,
                                   effect->p_noisegate_hdl->parm.threshold,
                                   effect->p_noisegate_hdl->parm.low_th_gain,
                                   effect->p_noisegate_hdl->parm.sampleRate,
                                   effect->p_noisegate_hdl->parm.channel);
                }
                break;
            case MASK_SHOUT_WHEAT:
                break;
            case MASK_LOW_SOUND:
                break;
            case MASK_HIGH_SOUND:
                break;
            case MASK_EQ:
                break;
            case MASK_EQ_SEG:
                break;
            case MASK_EQ_GLOBAL_GAIN:
                break;
            case MASK_MIC_GAIN:
                break;
            }
        }
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    mic混响参数淡入淡出处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_fade_run(struct __mic_effect *effect)
{
    if (effect == NULL) {
        return ;
    }
    u8 update = 0;
#if 10
    if (effect->p_reverb_hdl) {
        /* printf("reverb_fade_run fade ===========================\n"); */
        if (effect->p_reverb_hdl->parm.wet != effect->fade.wet) {
            update = 1;
            if (effect->p_reverb_hdl->parm.wet > effect->fade.wet) {
                effect->p_reverb_hdl->parm.wet --;
            } else {
                effect->p_reverb_hdl->parm.wet ++;
            }
        }
        if (update) {
            effect->p_reverb_hdl->func_api->init(effect->p_reverb_hdl->ptr, &effect->p_reverb_hdl->parm);
        }
    }
    update = 0;
    if (effect->p_echo_hdl) {
        if (effect->p_echo_hdl->echo_parm_obj.delay != effect->fade.delay) {
            update = 1;
            if (effect->p_echo_hdl->echo_parm_obj.delay > effect->fade.delay) {
                effect->p_echo_hdl->echo_parm_obj.delay --;
            } else {
                effect->p_echo_hdl->echo_parm_obj.delay ++;
            }
        }
        if (effect->p_echo_hdl->echo_parm_obj.decayval != effect->fade.decayval) {
            update = 1;
            if (effect->p_echo_hdl->echo_parm_obj.decayval > effect->fade.decayval) {
                effect->p_echo_hdl->echo_parm_obj.decayval --;
            } else {
                effect->p_echo_hdl->echo_parm_obj.decayval ++;
            }
        }
        if (update) {
            effect->p_echo_hdl->func_api->init(effect->p_echo_hdl->ptr, &effect->p_echo_hdl->echo_parm_obj);
        }
    }
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief    mic数据流串接入口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
/* #define TEST_PORT IO_PORTA_00 */
static u32 mic_effect_effect_run(void *priv, void *in, void *out, u32 inlen, u32 outlen)
{
    struct __mic_effect *effect = (struct __mic_effect *)priv;
    if (effect == NULL) {
        return 0;
    }
    struct audio_data_frame frame;
    frame.channel = 1;//
    /* frame.channel = 2;// */
    frame.sample_rate = effect->parm.sample_rate;
    frame.data_len = inlen;
    frame.data = in;
    effect->out_len = 0;
    effect->process_len = inlen;
    mic_effect_parm_update(effect);//更新参数
    mic_effect_fade_run(effect);//淡入淡出
    if (effect->pause_mark) {
        return outlen;
    }
    /* gpio_direction_output(TEST_PORT, 1); */
    while (1) {
        /* putchar('A'); */
        audio_stream_run(&effect->entry, &frame);
        if (effect->out_len >= effect->process_len) {
            /* putchar('B'); */
            break;
        }
    }
    /* gpio_direction_output(TEST_PORT, 0); */
    return outlen;
}

/*----------------------------------------------------------------------------*/
/**@brief   释放mic数据流资源
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_destroy(struct __mic_effect **hdl)
{
    if (hdl == NULL || *hdl == NULL) {
        return ;
    }
    struct __mic_effect *effect = *hdl;
    if (effect->mic) {
        log_i("mic_stream_destroy\n\n\n");
        mic_stream_destroy(&effect->mic);
    }
    if (effect->p_noisegate_hdl) {
        log_i("close_noisegate\n\n\n");
        close_noisegate(effect->p_noisegate_hdl);
    }
    if (effect->p_howling_hdl) {
        log_i("close_howling\n\n\n");
        close_howling(effect->p_howling_hdl);
    }

    if (effect->eq_drc) {
        log_i("mic_eq_drc_close\n\n\n");
        mic_eq_drc_close(effect->eq_drc);
    }
    if (effect->p_pitch_hdl) {
        log_i("close_pitch\n\n\n");
        close_pitch(effect->p_pitch_hdl);
    }

    if (effect->p_reverb_hdl) {
        log_i("close_reverb\n\n\n");
        close_reverb(effect->p_reverb_hdl);
    }
    if (effect->p_echo_hdl) {
        log_i("close_echo\n\n\n");
        close_echo(effect->p_echo_hdl);
    }


    if (effect->filt) {
        log_i("mic_high_bass_coeff_cal_uninit\n\n\n");
        mic_high_bass_coeff_cal_uninit(effect->filt);
    }
    if (effect->d_vol) {
        audio_stream_del_entry(audio_dig_vol_entry_get(effect->d_vol));
        audio_dig_vol_close(effect->d_vol);
    }
    if (effect->p_echo_src_hdl) {
        log_i("close_echo src\n\n\n");
        close_echo_src(effect->p_echo_src_hdl);
    }
    if (effect->p_last_out) {
        audio_stream_dac_mix_out_close(effect->p_last_out);
    }
    if (effect->stream) {
        audio_stream_close(effect->stream);
    }
    local_irq_disable();
    free(effect);
    *hdl = NULL;
    local_irq_enable();

    mem_stats();
    clock_remove_set(REVERB_CLK);
}
/*----------------------------------------------------------------------------*/
/**@brief    串流唤醒
   @param
   @return
   @note 暂未使用
*/
/*----------------------------------------------------------------------------*/
static void mic_stream_resume(void *p)
{
    struct __mic_effect *effect = (struct __mic_effect *)p;
    /* audio_decoder_resume_all(&decode_task); */
}

/*----------------------------------------------------------------------------*/
/**@brief    串流数据处理长度回调
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_data_process_len(struct audio_stream_entry *entry, int len)
{

    struct __mic_effect *effect = container_of(entry, struct __mic_effect, entry);
    effect->out_len += len;
    /* printf("out len[%d]",effect->out_len); */
}

/*----------------------------------------------------------------------------*/
/**@brief    (mic数据流)混响打开接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
bool mic_effect_start(void)
{
    bool ret = false;
    printf("\n--func=%s\n", __FUNCTION__);
    if (__this) {
        log_e("reverb is already start \n");
        return ret;
    }
    struct __mic_effect *effect = (struct __mic_effect *)zalloc(sizeof(struct __mic_effect));
    if (effect == NULL) {
        return false;
    }
    clock_add_set(REVERB_CLK);
    os_mutex_create(&effect->mutex);
    memcpy(&effect->parm, &effect_parm_default, sizeof(struct __mic_effect_parm));
    struct __mic_stream_parm *mic_parm = (struct __mic_stream_parm *)&effect_mic_stream_parm_default;

    if ((effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_REVERB))
        && (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_ECHO))) {
        log_e("effect config err ?? !!!, cann't support echo && reverb at the same time\n");
        mic_effect_destroy(&effect);
        return false;
    }
    u8 ch_num = 1; //??
    ///reverb 初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_REVERB)) {
        ch_num = 2;
        effect->fade.wet = effect_reverb_parm_default.wet;
        effect->p_reverb_hdl = open_reverb(&effect_reverb_parm_default, effect->parm.sample_rate);
    }
    ///echo 初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_ECHO)) {
        effect->fade.decayval = effect_echo_parm_default.decayval;
        effect->fade.delay = effect_echo_parm_default.delay;
        log_i("open_echo\n\n\n");
        effect->p_echo_hdl = open_echo(&effect_echo_parm_default, effect->parm.sample_rate);
    }
    ///pitch 初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_PITCH)) {
        log_i("open_pitch\n\n\n");
        effect->p_pitch_hdl = open_pitch(&effect_pitch_parm_default);
    }
    ///声音门限初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_NOISEGATE)) {
        effect->p_noisegate_hdl = open_noisegate(&effect_noisegate_parm_default, 0, 0);
    }
    ///初始化数字音量
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_DVOL)) {
        /* effect->d_vol = audio_dig_vol_open(&effect_dvol_default_parm); */
    }
    ///滤波器初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_FILT)) {
        effect->filt = zalloc(sizeof(BFILT_API_STRUCT));
        if (effect->filt) {
            mic_high_bass_coeff_cal_init(effect->filt, effect->parm.sample_rate);
        } else {
            log_e("mic filt malloc err\n");
        }
    }

    ///啸叫抑制初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_HOWLING)) {
        log_i("open_howling\n\n\n");
        effect->p_howling_hdl = open_howling(NULL, effect->parm.sample_rate, 0, 1);
    }
    ///eq初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_EQ)) {
        effect->eq_drc = mic_eq_drc_open(effect->parm.sample_rate, ch_num);
    }
    //打开混响变采i样
    effect->p_echo_src_hdl = open_echo_src(effect->parm.sample_rate, 44118, ch_num);
    /* effect->p_echo_src_hdl = open_echo_src(effect->parm.sample_rate,44100,ch_num); */
    // dac mix open
    effect->p_last_out = audio_stream_dac_mix_out_open(mic_parm->dac_delay);
    effect->entry.data_process_len = mic_effect_data_process_len;


// 数据流串联
    struct audio_stream_entry *entries[10] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &effect->entry;
    if (effect->p_noisegate_hdl) {
        entries[entry_cnt++] = &effect->p_noisegate_hdl->entry;
    }
    if (effect->p_howling_hdl) {
        entries[entry_cnt++] = &effect->p_howling_hdl->entry;
    }
    if (effect->eq_drc) {
        entries[entry_cnt++] = &effect->eq_drc->entry;
    }
    if (effect->p_pitch_hdl) {
        entries[entry_cnt++] = &effect->p_pitch_hdl->entry;
    }

    if (effect->p_reverb_hdl) {
        entries[entry_cnt++] = &effect->p_reverb_hdl->entry;
    }
    if (effect->p_echo_hdl) {
        entries[entry_cnt++] = &effect->p_echo_hdl->entry;
    }

    if (effect->d_vol) {
        entries[entry_cnt++] = audio_dig_vol_entry_get(effect->d_vol);//&effect->d_vol->entry;
    }

    if (effect->p_echo_src_hdl) {
        entries[entry_cnt++] = &effect->p_echo_src_hdl->entry;
    }
    if (effect->p_last_out) {
        entries[entry_cnt++] = &effect->p_last_out->entry;
    }

    effect->stream = audio_stream_open(effect, mic_stream_resume);
    audio_stream_add_list(effect->stream, entries, entry_cnt);


    ///mic 数据流初始化
    effect->mic = mic_stream_creat(mic_parm);
    if (effect->mic == NULL) {
        mic_effect_destroy(&effect);
        return false;
    }
    mic_stream_set_output(effect->mic, (void *)effect, mic_effect_effect_run);
    mic_stream_start(effect->mic);

    __this = effect;
    log_info("--------------------------effect start ok\n");
    mem_stats();
    mic_effect_change_mode(0);
    return true;
}
/*----------------------------------------------------------------------------*/
/**@brief    (mic数据流)混响关闭接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_stop(void)
{
    mic_effect_destroy(&__this);
}
/*----------------------------------------------------------------------------*/
/**@brief    (mic数据流)混响暂停接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_pause(u8 mark)
{
    if (__this) {
        __this->pause_mark = mark ? 1 : 0;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    (mic数据流)混响状态获取接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 mic_effect_get_status(void)
{
    printf("\n--func=%s\n", __FUNCTION__);
    return ((__this) ? 1 : 0);
}

/*----------------------------------------------------------------------------*/
/**@brief    数字音量调节接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_set_dvol(u8 vol)
{
    if (__this == NULL) {
        return ;
    }
    audio_dig_vol_set(__this->d_vol, 3, vol);
}
u8 mic_effect_get_dvol(void)
{
    if (__this) {
        return audio_dig_vol_get(__this->d_vol, 1);
    }
    return 0;
}


/*----------------------------------------------------------------------------*/
/**@brief    reverb 效果声增益调节接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_set_reverb_wet(int wet)
{
    if (__this == NULL || __this->p_reverb_hdl == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);
    __this->fade.wet = wet;
    os_mutex_post(&__this->mutex);
}

int mic_effect_get_reverb_wet(void)
{
    if (__this && __this->p_reverb_hdl) {
        return __this->fade.wet;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    echo 回声延时调节接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_set_echo_delay(u32 delay)
{
    if (__this == NULL || __this->p_echo_hdl == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);
    __this->fade.delay = delay;
    os_mutex_post(&__this->mutex);
}
u32 mic_effect_get_echo_delay(void)
{
    if (__this && __this->p_echo_hdl) {
        return __this->fade.delay;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    echo 回声衰减系数调节接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_set_echo_decay(u32 decay)
{
    if (__this == NULL || __this->p_echo_hdl == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);
    __this->fade.decayval = decay;
    os_mutex_post(&__this->mutex);
}

u32 mic_effect_get_echo_decay(void)
{
    if (__this && __this->p_echo_hdl) {
        return __this->fade.decayval;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    设置各类音效运行标记
   @param
   @return
   @note 暂不使用
*/
/*----------------------------------------------------------------------------*/
void mic_effect_set_function_mask(u32 mask)
{
    if (__this == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);
    __this->parm.effect_run = mask;
    os_mutex_post(&__this->mutex);
}
/*----------------------------------------------------------------------------*/
/**@brief    获取各类音效运行标记
   @param
   @return
   @note 暂不使用
*/
/*----------------------------------------------------------------------------*/
u32 mic_effect_get_function_mask(void)
{
    if (__this) {
        return __this->parm.effect_run;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    喊mic效果系数计算
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_shout_wheat_cal_coef(int sw)
{
    log_info("%s %d\n", __FUNCTION__, __LINE__);
    if (__this == NULL || __this->filt == NULL) {
        return ;
    }
#if 1
    os_mutex_pend(&__this->mutex, 0);
    BFILT_API_STRUCT *filt = __this->filt;
    filt->func_api->init(
        filt->ptr,
        filt->shout_wheat.center_frequency,
        filt->shout_wheat.bandwidth,
        TYPE_BANDPASS,
        __this->parm.sample_rate,
        0);
    if (sw) {
        filt->func_api->cal_coef(filt->ptr, get_outval_addr(0), filt->shout_wheat.occupy, 0);
        log_i("shout_wheat_cal_coef on\n");
    } else {
        filt->func_api->cal_coef(filt->ptr, get_outval_addr(0), 0, 0);
        log_i("shout_wheat_cal_coef off\n");
    }
    os_mutex_post(&__this->mutex);
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief    低音系数计算
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_low_sound_cal_coef(int gainN)
{
    log_info("%s %d\n", __FUNCTION__, __LINE__);
    if (__this == NULL || __this->filt == NULL) {
        return ;
    }
#if 1
    os_mutex_pend(&__this->mutex, 0);
    BFILT_API_STRUCT *filt = __this->filt;
    filt->func_api->init(
        filt->ptr,
        filt->low_sound.cutoff_frequency,
        1024,
        TYPE_LOWPASS,
        __this->parm.sample_rate,
        1);
    gainN = filt->low_sound.lowest_gain
            + gainN * (filt->low_sound.highest_gain - filt->low_sound.lowest_gain) / 10;
    log_i("low sound gainN %d\n", gainN);
    log_i("lowest_gain %d\n", filt->low_sound.lowest_gain);
    log_i("highest_gain %d\n", filt->low_sound.highest_gain);
    if ((gainN >= filt->low_sound.lowest_gain) && (gainN <= filt->low_sound.highest_gain)) {
        filt->func_api->cal_coef(filt->ptr, get_outval_addr(1), gainN, 1);
    }

    os_mutex_post(&__this->mutex);
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief    高音系数计算
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_high_sound_cal_coef(int gainN)
{
    log_info("%s %d\n", __FUNCTION__, __LINE__);
    if (__this == NULL || __this->filt == NULL) {
        return ;
    }
#if 1
    os_mutex_pend(&__this->mutex, 0);
    BFILT_API_STRUCT *filt = __this->filt;
    filt->func_api->init(
        filt->ptr,
        filt->high_sound.cutoff_frequency,
        1024,
        TYPE_HIGHPASS,
        __this->parm.sample_rate,
        2);
    gainN = filt->high_sound.lowest_gain + gainN * (filt->high_sound.highest_gain - filt->high_sound.lowest_gain) / 10;
    log_i("high gainN %d\n", gainN);
    log_i("lowest_gain %d\n", filt->high_sound.lowest_gain);
    log_i("highest_gain %d\n", filt->high_sound.highest_gain);
    if ((gainN >= filt->high_sound.lowest_gain) && (gainN <= filt->high_sound.highest_gain)) {
        filt->func_api->cal_coef(filt->ptr, get_outval_addr(2), gainN, 2);
    }
    os_mutex_post(&__this->mutex);
#endif
}

/*
 *混响高低音调节
 *filtN为0 与 sw 组合：控制喊麦开关
 *filtN为1或者2时，改变gainN值 调节高低音值
 * */
void mic_effect_cal_coef(u8 filtN, int gainN, u8 sw)
{
#if 1
    if (filtN == 0) {
        mic_effect_shout_wheat_cal_coef(sw);
    } else if (filtN == 1) {
        mic_effect_low_sound_cal_coef(gainN);
    } else if (filtN == 2) {
        mic_effect_high_sound_cal_coef(gainN);
    }
    mic_eq_drc_update();
#endif
}

void reverb_eq_mode_set(u8 type, u32 gainN)
{
#if 1
    log_i("filN %d, gainN %d\n", type, gainN);
    if (type == MIC_EQ_MODE_SHOUT_WHEAT) {
        mic_effect_shout_wheat_cal_coef(gainN);
    } else if (type == MIC_EQ_MODE_LOW_SOUND) {
        mic_effect_low_sound_cal_coef(gainN);
    } else if (type == MIC_EQ_MODE_HIGH_SOUND) {
        mic_effect_high_sound_cal_coef(gainN);
    }
    mic_eq_drc_update();
#endif
}


static int outval[3][5]; //开3个2阶滤波器的空间，给硬件eq存系数的
__attribute__((weak))int *get_outval_addr(u8 mode)
{
    //高低音系数表地址
    return outval[mode];
}

u8 mic_effect_eq_section_num(void)
{
    return (EQ_SECTION_MAX + 3);
}
/*----------------------------------------------------------------------------*/
/**@brief    reverb 混响参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_reverb_parm_fill(REVERBN_PARM_SET *parm, u8 fade, u8 online)
{

#if 1
    if (__this == NULL || __this->p_reverb_hdl == NULL) {
        return ;
    }
    if (parm == NULL) {
        __this->parm.effect_run &= ~BIT(MIC_EFFECT_CONFIG_REVERB);
        pause_reverb(__this->p_reverb_hdl, 1);
        return ;
    }
    mic_effect_reverb_parm_printf(parm);
    REVERBN_PARM_SET tmp;
    os_mutex_pend(&__this->mutex, 0);
    memcpy(&tmp, parm, sizeof(REVERBN_PARM_SET));
    if (fade) {
        //针对需要fade的参数，读取旧值，通过fade来更新对应的参数
        tmp.wet = __this->p_reverb_hdl->parm.wet;///读取旧值,暂时不更新
        if (online) {
            __this->fade.wet = parm->wet;///设置wet fade 目标值, 通过fade更新
        } else {
            __this->fade.wet = __this->p_reverb_hdl->parm.wet;//值不更新, 通过外部按键更新， 如旋钮
        }
    }
    memcpy(&__this->p_reverb_hdl->parm, &tmp, sizeof(REVERBN_PARM_SET));
    __this->update_mask |= BIT(MASK_REVERB);
    __this->parm.effect_run |= BIT(MIC_EFFECT_CONFIG_REVERB);
    pause_reverb(__this->p_reverb_hdl, 0);

    os_mutex_post(&__this->mutex);
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief    echo 混响参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_echo_parm_fill(ECHO_PARM_SET *parm, u8 fade, u8 online)
{

    log_info("%s %d\n", __FUNCTION__, __LINE__);
#if 1
    if (__this == NULL || __this->p_echo_hdl == NULL) {
        return ;
    }
    if (parm == NULL) {
        __this->parm.effect_run &= ~BIT(MIC_EFFECT_CONFIG_ECHO);
        pause_echo(__this->p_echo_hdl, 1);
        return ;
    }
    ECHO_PARM_SET tmp;
    os_mutex_pend(&__this->mutex, 0);
    memcpy(&tmp, parm, sizeof(ECHO_PARM_SET));
    if (fade) {
        //针对需要fade的参数，读取旧值，通过fade来更新对应的参数
        tmp.delay = __this->p_echo_hdl->echo_parm_obj.delay;
        tmp.decayval = __this->p_echo_hdl->echo_parm_obj.decayval;
        if (online) {
            __this->fade.delay = parm->delay;///设置wet fade 目标值, 通过fade更新
            __this->fade.decayval = parm->decayval;///设置wet fade 目标值, 通过fade更新
        } else {
            __this->fade.delay = __this->p_echo_hdl->echo_parm_obj.delay;///值不更新, 通过外部按键更新， 如旋钮
            __this->fade.decayval = __this->p_echo_hdl->echo_parm_obj.decayval;///值不更新, 通过外部按键更新， 如旋钮
        }
    }
    memcpy(&__this->p_echo_hdl->echo_parm_obj, &tmp, sizeof(ECHO_PARM_SET));
    __this->update_mask |= BIT(MASK_ECHO);
    __this->parm.effect_run |= BIT(MIC_EFFECT_CONFIG_ECHO);
    pause_echo(__this->p_echo_hdl, 0);
    os_mutex_post(&__this->mutex);
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief    变声参数直接更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void set_pitch_para(u32 shiftv, u32 sr, u8 effect, u32 formant_shift)
{
    if (__this == NULL || __this->p_pitch_hdl == NULL) {
        return ;
    }
    PITCH_SHIFT_PARM *p_pitch_parm = get_pitch_parm();

    p_pitch_parm->shiftv = shiftv;


    /* p_pitch_parm->sr = sr; */

    p_pitch_parm->effect_v = effect;

    p_pitch_parm->formant_shift = formant_shift;
    printf("\n\n\nshiftv[%d],sr[%d],effect[%d],formant_shift[%d] \n\n", p_pitch_parm->shiftv, p_pitch_parm->sr, p_pitch_parm->effect_v, p_pitch_parm->formant_shift);
    update_pict_parm(__this->p_pitch_hdl);

}
/*----------------------------------------------------------------------------*/
/**@brief    变声参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_pitch_parm_fill(PITCH_PARM_SET2 *parm, u8 fade, u8 online)
{

    log_info("%s %d\n", __FUNCTION__, __LINE__);
#if 1
    if (__this == NULL || __this->p_pitch_hdl == NULL) {
        return ;
    }
    if (parm == NULL) {
        __this->parm.effect_run &= ~BIT(MIC_EFFECT_CONFIG_PITCH);
        pause_pitch(__this->p_pitch_hdl, 1);
        return ;
    }
    mic_effect_pitch_parm_printf(parm);

    os_mutex_pend(&__this->mutex, 0);
    PITCH_SHIFT_PARM *pitch = &__this->p_pitch_hdl->parm;
    pitch->effect_v = parm->effect_v;
    pitch->formant_shift = parm->formant_shift;
    pitch->shiftv = parm->pitch;
    __this->update_mask |= BIT(MASK_PITCH);
    __this->parm.effect_run |= BIT(MIC_EFFECT_CONFIG_PITCH);
    pause_pitch(__this->p_pitch_hdl, 0);
    os_mutex_post(&__this->mutex);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    噪声抑制参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_noisegate_parm_fill(NOISE_PARM_SET *parm, u8 fade, u8 online)
{
    if (__this == NULL || __this->p_noisegate_hdl == NULL) {
        return ;
    }
    if (parm == NULL) {
        __this->parm.effect_run &= ~BIT(MIC_EFFECT_CONFIG_NOISEGATE);
        pause_noisegate(__this->p_noisegate_hdl, 1);
        return ;
    }
    mic_effect_noisegate_parm_printf(parm);

    os_mutex_pend(&__this->mutex, 0);
    NOISEGATE_PARM *noisegate = &__this->p_noisegate_hdl->parm;
    noisegate->attackTime = parm->attacktime;
    noisegate->releaseTime = parm->releasetime;
    noisegate->threshold = parm->threadhold;
    noisegate->low_th_gain = parm->gain;
    __this->update_mask |= BIT(MASK_NOISEGATE);
    __this->parm.effect_run |= BIT(MIC_EFFECT_CONFIG_NOISEGATE);
    pause_noisegate(__this->p_noisegate_hdl, 0);
    os_mutex_post(&__this->mutex);
}
/*----------------------------------------------------------------------------*/
/**@brief    喊mic参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_shout_wheat_parm_fill(SHOUT_WHEAT_PARM_SET *parm, u8 fade, u8 online)
{
    log_info("%s %d\n", __FUNCTION__, __LINE__);
    if (__this == NULL || __this->filt == NULL || parm == NULL) {
        return ;
    }
    mic_effect_shout_wheat_parm_printf(parm);

    SHOUT_WHEAT_PARM_SET tmp;
    os_mutex_pend(&__this->mutex, 0);
    memcpy(&tmp, parm, sizeof(SHOUT_WHEAT_PARM_SET));
    if (fade) {
        //针对需要fade的参数，读取旧值，通过fade来更新对应的参数
    }
    memcpy(&__this->filt->shout_wheat, &tmp, sizeof(SHOUT_WHEAT_PARM_SET));
    __this->update_mask |= BIT(MASK_SHOUT_WHEAT);
    os_mutex_post(&__this->mutex);
}
void mic_effect_low_sound_parm_fill(LOW_SOUND_PARM_SET *parm, u8 fade, u8 online)
{
    log_info("%s %d\n", __FUNCTION__, __LINE__);
    if (__this == NULL || __this->filt == NULL || parm == NULL) {
        return ;
    }
    mic_effect_low_sound_parm_printf(parm);

    LOW_SOUND_PARM_SET tmp;
    os_mutex_pend(&__this->mutex, 0);
    memcpy(&tmp, parm, sizeof(LOW_SOUND_PARM_SET));
    if (fade) {
        //针对需要fade的参数，读取旧值，通过fade来更新对应的参数
    }
    memcpy(&__this->filt->low_sound, &tmp, sizeof(LOW_SOUND_PARM_SET));
    __this->update_mask |= BIT(MASK_LOW_SOUND);
    os_mutex_post(&__this->mutex);
}
void mic_effect_high_sound_parm_fill(HIGH_SOUND_PARM_SET *parm, u8 fade, u8 online)
{
    log_info("%s %d\n", __FUNCTION__, __LINE__);
    if (__this == NULL || __this->filt == NULL || parm == NULL) {
        return ;
    }
    mic_effect_high_sound_parm_printf(parm);

    HIGH_SOUND_PARM_SET tmp;
    os_mutex_pend(&__this->mutex, 0);
    memcpy(&tmp, parm, sizeof(HIGH_SOUND_PARM_SET));
    if (fade) {
        //针对需要fade的参数，读取旧值，通过fade来更新对应的参数
    }
    memcpy(&__this->filt->high_sound, &tmp, sizeof(HIGH_SOUND_PARM_SET));
    __this->update_mask |= BIT(MASK_HIGH_SOUND);
    os_mutex_post(&__this->mutex);
}

/*----------------------------------------------------------------------------*/
/**@brief    mic增益调节
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_mic_gain_parm_fill(EFFECTS_MIC_GAIN_PARM *parm, u8 fade, u8 online)
{
    log_info("%s %d\n", __FUNCTION__, __LINE__);
    if (__this == NULL || parm == NULL) {
        return ;
    }
    audio_mic_set_gain(parm->gain);
}
/*----------------------------------------------------------------------------*/
/**@brief    mic效果模式切换（数据流音效组合切换）
   @param
   @return
   @note 使用效果配置文件时生效
*/
/*----------------------------------------------------------------------------*/
void mic_effect_change_mode(u16 mode)
{
    effect_cfg_change_mode(mode);
}
/*----------------------------------------------------------------------------*/
/**@brief    获取mic效果模式（数据流音效组合）
   @param
   @return
   @note 使用效果配置文件时生效
*/
/*----------------------------------------------------------------------------*/
u16 mic_effect_get_cur_mode(void)
{
    return effect_cfg_get_cur_mode();
}




/*----------------------------------------------------------------------------*/
/**@brief    eq接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_eq_drc_update()
{
    local_irq_disable();
    if (__this && __this->eq_drc && __this->eq_drc->eq) {
        __this->eq_drc->eq->updata = 1;
    }
    local_irq_enable();
    putchar('P');
}


void mic_high_bass_coeff_cal_init(BFILT_API_STRUCT *filt, u16 sample_rate)
{
#if 1
    printf("\n--func=%s\n", __FUNCTION__);
    log_info("%s %d\n", __FUNCTION__, __LINE__);
    filt->func_api = get_bfiltTAB_func_api();
    filt->ptr = zalloc(filt->func_api->needbuf());
    filt->func_api->open(filt->ptr);

    memcpy(&filt->shout_wheat, &effect_shout_wheat_default, sizeof(SHOUT_WHEAT_PARM_SET));
    memcpy(&filt->low_sound, &effect_low_sound_default, sizeof(LOW_SOUND_PARM_SET));
    memcpy(&filt->high_sound, &effect_high_sound_default, sizeof(HIGH_SOUND_PARM_SET));
    // 运算buf, 中心频率，带宽设置， 滤波器类型，采样率，滤波器index
    filt->func_api->init(filt->ptr, filt->shout_wheat.center_frequency, filt->shout_wheat.bandwidth, TYPE_BANDPASS, sample_rate, 0); //喊麦滤波器
    filt->func_api->init(filt->ptr, filt->low_sound.cutoff_frequency, 1024, TYPE_LOWPASS, sample_rate, 1);//低音滤波器
    filt->func_api->init(filt->ptr, filt->high_sound.cutoff_frequency, 1024, TYPE_HIGHPASS, sample_rate, 2);//高音滤波器

    filt->func_api->cal_coef(filt->ptr, get_outval_addr(0), 0, 0);
    filt->func_api->cal_coef(filt->ptr, get_outval_addr(1), 0, 1);
    filt->func_api->cal_coef(filt->ptr, get_outval_addr(2), 0, 2);

#endif

}
void mic_high_bass_coeff_cal_uninit(BFILT_API_STRUCT *filt)
{
    local_irq_disable();
    if (filt && filt->ptr) {
        free(filt->ptr);
        filt->ptr = NULL;
    }
    if (filt) {
        free(filt);
        filt = NULL;
    }
    local_irq_enable();
}

__attribute__((weak))int mic_eq_get_filter_info(struct audio_eq *eq, int sr, struct audio_eq_filter_info *info)
{
    log_info("mic_eq_get_filter_info \n");
    int *coeff = NULL;

    coeff = outval;
    info->L_coeff = info->R_coeff = (void *)coeff;
    info->L_gain = info->R_gain = 0;
    info->nsection = 3;
    return 0;
}


void *mic_eq_drc_open(u16 sample_rate, u8 ch_num)
{

#if TCFG_EQ_ENABLE

    printf("sample_rate %d %d\n", sample_rate, ch_num);
    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

    effect_parm.eq_en = 1;

    if (effect_parm.eq_en) {
        effect_parm.async_en = 0;
        effect_parm.out_32bit = 0;
        effect_parm.online_en = 0;
        effect_parm.mode_en = 0;
    }

    effect_parm.eq_name = mic_eq_mode;

    effect_parm.ch_num = ch_num;
    effect_parm.sr = sample_rate;
    effect_parm.eq_cb = mic_eq_get_filter_info;

    eq_drc = audio_eq_drc_open(&effect_parm);

    clock_add(EQ_CLK);
    return eq_drc;
#else
    return NULL;
#endif//TCFG_EQ_ENABLE

}

void mic_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
    /* #if TCFG_BT_MUSIC_EQ_ENABLE */
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
    }
    /* #endif */
#endif
    return;
}




#endif//TCFG_MIC_EFFECT_ENABLE




