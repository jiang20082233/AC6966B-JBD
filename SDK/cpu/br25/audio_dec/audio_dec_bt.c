#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/a2dp_decoder.h"
#include "media/esco_decoder.h"
#include "classic/tws_api.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_dec.h"
#include "audio_digital_vol.h"
#include "clock_cfg.h"
#include "btstack/a2dp_media_codec.h"
#include "application/audio_eq_drc_apply.h"
#include "application/audio_equalloudness.h"
#include "application/audio_surround.h"
#include "application/audio_vbass.h"
#include "aec_user.h"
#include "audio_enc.h"
#include "bt_tws.h"

#if TCFG_ESCO_PLC
#include "PLC.h"
#define PLC_FRAME_LEN	60
#endif

#define TCFG_ESCO_LIMITER	1
#if TCFG_ESCO_LIMITER
#include "limiter_noiseGate_api.h"
/*限幅器上限*/
#define LIMITER_THR	 -10000 /*-12000 = -12dB,放大1000倍,(-10000参考)*/
/*小于CONST_NOISE_GATE的当成噪声处理,防止清0近端声音*/
#define LIMITER_NOISE_GATE  -30000 /*-12000 = -12dB,放大1000倍,(-30000参考)*/
/*低于噪声门限阈值的增益 */
#define LIMITER_NOISE_GAIN  (0 << 30) /*(0~1)*2^30*/
#endif/*TCFG_ESCO_LIMITER*/

#define TCFG_ESCO_USE_SPEC_MIX_LEN		0

extern const int CONFIG_A2DP_DELAY_TIME;

//////////////////////////////////////////////////////////////////////////////


struct a2dp_dec_hdl {
    struct a2dp_decoder dec;		// a2dp解码句柄
    struct audio_res_wait wait;		// 资源等待句柄
    struct audio_mixer_ch mix_ch;	// 叠加句柄
#if (RECORDER_MIX_EN)
    struct audio_mixer_ch rec_mix_ch;	// 叠加句柄
#endif//RECORDER_MIX_EN

    struct audio_stream *stream;	// 音频流
    struct audio_eq_drc *eq_drc;    //eq drc句柄
    equal_loudness_hdl *loudness;   //等响度句柄
    surround_hdl *surround;         //环绕音效句柄
    vbass_hdl *vbass;               //虚拟低音句柄
    struct audio_wireless_sync *sync;
};

struct esco_dec_hdl {
    struct esco_decoder dec;		// esco解码句柄
    struct audio_res_wait wait;		// 资源等待句柄
    struct audio_mixer_ch mix_ch;	// 叠加句柄
#if (RECORDER_MIX_EN)
    struct audio_mixer_ch rec_mix_ch;	// 叠加句柄
#endif//RECORDER_MIX_EN

    struct audio_stream *stream;	// 音频流
    struct audio_eq_drc *eq_drc;    //eq drc句柄
    u32 tws_mute_en : 1;	// 静音
    u32 remain : 1;			// 未输出完成

#if TCFG_ESCO_PLC
    void *plc;				// 丢包修护
#endif

#if TCFG_ESCO_LIMITER
    void *limiter;			// 限福器
#endif
    struct audio_wireless_sync *sync;
};

/* extern struct audio_mixer recorder_mixer; */

//////////////////////////////////////////////////////////////////////////////

struct a2dp_dec_hdl *bt_a2dp_dec = NULL;
struct esco_dec_hdl *bt_esco_dec = NULL;

extern s16 mix_buff[];
extern s16 recorder_mix_buff[];

//////////////////////////////////////////////////////////////////////////////

void *a2dp_eq_drc_open(u16 sample_rate, u8 ch_num);
void a2dp_eq_drc_close(struct audio_eq_drc *eq_drc);
void *esco_eq_drc_open(u16 sample_rate, u8 ch_num);
void esco_eq_drc_close(struct audio_eq_drc *eq_drc);
struct audio_wireless_sync *a2dp_output_sync_open(u8 entry_select);
void a2dp_output_sync_close(struct audio_wireless_sync *a2dp_sync);
struct audio_wireless_sync *esco_output_sync_open(u8 entry_select);
void esco_output_sync_close(struct audio_wireless_sync *esco_sync);

extern int lmp_private_esco_suspend_resume(int flag);

equal_loudness_hdl *equal_loudness_open_demo(u16 sample_rate, u8 ch_num);
void equal_loudness_close_demo(equal_loudness_hdl *loudness);
surround_hdl *surround_open_demo(u8 ch_num);
void surround_close(surround_hdl *surround);
vbass_hdl *vbass_open_demo(u16 sample_rate, u8 ch_num);
void vbass_close_demo(vbass_hdl *vbass);
/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码close
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void a2dp_audio_res_close(void)
{
    if (bt_a2dp_dec->dec.start == 0) {
        log_i("bt_a2dp_dec->dec.start == 0");
        return ;
    }

    bt_a2dp_dec->dec.start = 0;
    a2dp_decoder_close(&bt_a2dp_dec->dec);
    a2dp_eq_drc_close(bt_a2dp_dec->eq_drc);
    audio_mixer_ch_close(&bt_a2dp_dec->mix_ch);
#if (RECORDER_MIX_EN)
    audio_mixer_ch_close(&bt_a2dp_dec->rec_mix_ch);
#endif//RECORDER_MIX_EN

    a2dp_output_sync_close(bt_a2dp_dec->sync);
    bt_a2dp_dec->sync = NULL;

    if (bt_a2dp_dec->stream) {
        audio_stream_close(bt_a2dp_dec->stream);
        bt_a2dp_dec->stream = NULL;
    }

    app_audio_state_exit(APP_AUDIO_STATE_MUSIC);
}

/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void a2dp_dec_release()
{
    audio_decoder_task_del_wait(&decode_task, &bt_a2dp_dec->wait);
    a2dp_drop_frame_stop();

    if (bt_a2dp_dec->dec.coding_type == AUDIO_CODING_SBC) {
        clock_remove(DEC_SBC_CLK);
    } else if (bt_a2dp_dec->dec.coding_type == AUDIO_CODING_AAC) {
        clock_remove(DEC_AAC_CLK);
    }

    local_irq_disable();
    free(bt_a2dp_dec);
    bt_a2dp_dec = NULL;
    local_irq_enable();
}

/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码事件返回
   @param    *decoder: 解码器句柄
   @param    argc: 参数个数
   @param    *argv: 参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void a2dp_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        log_i("AUDIO_DEC_EVENT_END\n");
        a2dp_dec_close();
        break;
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码数据流激活
   @param    *p: 私有句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void a2dp_dec_out_stream_resume(void *p)
{
    struct a2dp_dec_hdl *dec = (struct a2dp_dec_hdl *)p;

    audio_decoder_resume(&dec->dec.decoder);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙收数激活解码
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void a2dp_rx_notice_to_decode(void)
{
    if (bt_a2dp_dec) {
        a2dp_decoder_resume_form_bluetooeh(&bt_a2dp_dec->dec);
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    开始a2dp解码
   @param
   @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int a2dp_dec_start(void)
{
    int err;
    struct audio_fmt *fmt;
    struct a2dp_dec_hdl *dec = bt_a2dp_dec;
    u8 ch_num = 0;

    if (!bt_a2dp_dec) {
        return -EINVAL;
    }

    log_i("a2dp_dec_start: in\n");

    err = a2dp_decoder_open(&dec->dec, &decode_task);
    if (err) {
        goto __err1;
    }
    audio_decoder_set_event_handler(&dec->dec.decoder, a2dp_dec_event_handler, 0);

    err = audio_decoder_get_fmt(&dec->dec.decoder, &fmt);
    if (err) {
        goto __err2;
    }

    ch_num = audio_output_channel_num();
    a2dp_decoder_set_output_channel(&dec->dec, ch_num);

    audio_mixer_ch_open_head(&dec->mix_ch, &mixer); // 挂载到mixer最前面
    audio_mixer_ch_set_src(&dec->mix_ch, 1, 0);
    audio_mixer_ch_set_no_wait(&dec->mix_ch, 1, 10); // 超时自动丢数
    audio_mixer_ch_sample_sync_enable(&dec->mix_ch, 1);

#if (RECORDER_MIX_EN)
    audio_mixer_ch_open_head(&dec->rec_mix_ch, &recorder_mixer); // 挂载到mixer最前面
    audio_mixer_ch_set_src(&dec->rec_mix_ch, 1, 0);
    audio_mixer_ch_set_no_wait(&dec->rec_mix_ch, 1, 10); // 超时自动丢数
    /* audio_mixer_ch_sample_sync_enable(&dec->rec_mix_ch, 1); */
#endif//RECORDER_MIX_EN

    dec->eq_drc = a2dp_eq_drc_open(fmt->sample_rate, ch_num);

    //dec->loudness = equal_loudness_open_demo(fmt->sample_rate, ch_num);
    //dec->surround = surround_open_demo(ch_num);
    //dec->vbass = vbass_open_demo(fmt->sample_rate, ch_num);

    dec->sync = a2dp_output_sync_open(0);

    /*使能同步，配置延时时间*/
    a2dp_decoder_stream_sync_enable(&dec->dec, dec->sync->context, fmt->sample_rate, CONFIG_A2DP_DELAY_TIME);

    // 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &dec->dec.decoder.entry;
    if (dec->sync) {
        entries[entry_cnt++] = dec->sync->entry;
    }
#if TCFG_EQ_ENABLE && TCFG_BT_MUSIC_EQ_ENABLE
    if (dec->eq_drc) {
        entries[entry_cnt++] = &dec->eq_drc->entry;
    }
#endif
    /* entries[entry_cnt++] = &dec->loudness->entry; */
    /* entries[entry_cnt++] = &dec->surround->entry; */
    /* entries[entry_cnt++] = &dec->vbass->entry; */
    entries[entry_cnt++] = &dec->mix_ch.entry;
    dec->stream = audio_stream_open(dec, a2dp_dec_out_stream_resume);
    audio_stream_add_list(dec->stream, entries, entry_cnt);

#if (RECORDER_MIX_EN)
    audio_stream_add_entry(entries[entry_cnt - 2], &dec->rec_mix_ch.entry);
#endif//RECORDER_MIX_EN

    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);

    log_i("dec->ch:%d, fmt->channel:%d\n", dec->dec.ch, fmt->channel);

    a2dp_drop_frame_stop();
#if TCFG_USER_TWS_ENABLE
    if (tws_network_audio_was_started()) {
        /*a2dp播放中副机加入*/
        tws_network_local_audio_start();
        a2dp_decoder_join_tws(&dec->dec);
    }
#endif

    dec->dec.start = 1;
    err = audio_decoder_start(&dec->dec.decoder);
    if (err) {
        goto __err3;
    }

    clock_set_cur();

    return 0;

__err3:
    dec->dec.start = 0;
    a2dp_eq_drc_close(dec->eq_drc);
    audio_mixer_ch_close(&dec->mix_ch);
#if (RECORDER_MIX_EN)
    audio_mixer_ch_close(&dec->rec_mix_ch);
#endif//RECORDER_MIX_EN

    if (dec->sync) {
        a2dp_output_sync_close(dec->sync);
        dec->sync = NULL;
    }
    if (dec->stream) {
        audio_stream_close(dec->stream);
        dec->stream = NULL;
    }
__err2:
    a2dp_decoder_close(&dec->dec);
__err1:
    a2dp_dec_release();

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码资源等待
   @param    *wait: 句柄
   @param    event: 事件
   @return   0：成功
   @note     用于多解码打断处理
*/
/*----------------------------------------------------------------------------*/
static int a2dp_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    log_i("a2dp_wait_res_handler: %d\n", event);

    if (event == AUDIO_RES_GET) {
        err = a2dp_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        if (bt_a2dp_dec->dec.start) {
            a2dp_audio_res_close();
            a2dp_drop_frame_start();
        }
    }
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    打开a2dp解码
   @param    media_type: 媒体类型
   @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
int a2dp_dec_open(int media_type)
{
    struct a2dp_dec_hdl *dec;

    if (bt_a2dp_dec) {
        return 0;
    }

    log_i("a2dp_dec_open: %d\n", media_type);

    dec = zalloc(sizeof(*dec));
    if (!dec) {
        return -ENOMEM;
    }

    switch (media_type) {
    case A2DP_CODEC_SBC:
        log_i("a2dp_media_type:SBC");
        dec->dec.coding_type = AUDIO_CODING_SBC;
        clock_add(DEC_SBC_CLK);
        break;
    case A2DP_CODEC_MPEG24:
        log_i("a2dp_media_type:AAC");
        dec->dec.coding_type = AUDIO_CODING_AAC;
        clock_add(DEC_AAC_CLK);
        break;
    default:
        log_i("a2dp_media_type unsupoport:%d", media_type);
        free(dec);
        return -EINVAL;
    }

    bt_a2dp_dec = dec;
    dec->wait.priority = 1;
    dec->wait.preemption = 0;
    dec->wait.snatch_same_prio = 1;
    dec->wait.handler = a2dp_wait_res_handler;
    audio_decoder_task_add_wait(&decode_task, &dec->wait);

    if (bt_a2dp_dec && (bt_a2dp_dec->dec.start == 0)) {
        a2dp_drop_frame_start();
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    关闭a2dp解码
   @param
   @return   0: 没有a2dp解码
   @return   1: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
int a2dp_dec_close(void)
{
    if (!bt_a2dp_dec) {
        return 0;
    }
    if (bt_a2dp_dec->dec.start) {
        a2dp_audio_res_close();
    }
    a2dp_dec_release();/*free bt_a2dp_dec*/
    clock_set_cur();
    log_i("a2dp_dec_close: exit\n");
    return 1;
}


#if TCFG_ESCO_PLC
/*----------------------------------------------------------------------------*/
/**@brief    esco丢包修护初始化
   @param
   @return   丢包修护句柄
   @note
*/
/*----------------------------------------------------------------------------*/
static void *esco_plc_init(void)
{
    void *plc = malloc(PLC_query()); /*buf_size:1040*/
    //plc = zalloc_mux(PLC_query());
    log_i("PLC_buf:0x%x,size:%d\n", (int)plc, PLC_query());
    if (!plc) {
        return NULL;
    }
    int err = PLC_init(plc);
    if (err) {
        log_i("PLC_init err:%d", err);
        free(plc);
        plc = NULL;
    }
    return plc;
}
/*----------------------------------------------------------------------------*/
/**@brief    esco丢包修护关闭
   @param    *plc: 丢包修护句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_plc_close(void *plc)
{
    free(plc);
}
/*----------------------------------------------------------------------------*/
/**@brief    esco丢包修护运行
   @param    *data: 数据
   @param    len: 数据长度
   @param    repair: 修护标记
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_plc_run(s16 *data, u16 len, u8 repair)
{
    u16 repair_point, tmp_point;
    s16 *p_in, *p_out;
    p_in    = data;
    p_out   = data;
    tmp_point = len / 2;

#if 0	//debug
    static u16 repair_cnt = 0;
    if (repair) {
        repair_cnt++;
        y_printf("[E%d]", repair_cnt);
    } else {
        repair_cnt = 0;
    }
    //log_i("[%d]",point);
#endif/*debug*/

    while (tmp_point) {
        repair_point = (tmp_point > PLC_FRAME_LEN) ? PLC_FRAME_LEN : tmp_point;
        tmp_point = tmp_point - repair_point;
        PLC_run(p_in, p_out, repair_point, repair);
        p_in  += repair_point;
        p_out += repair_point;
    }
}
#endif


#if TCFG_ESCO_LIMITER
/*----------------------------------------------------------------------------*/
/**@brief    esco限福器初始化
   @param    sample_rate: 解码采样率
   @return   限福器句柄
   @note
*/
/*----------------------------------------------------------------------------*/
static void *esco_limiter_init(u16 sample_rate)
{
    void *limiter = malloc(need_limiter_noiseGate_buf(1));
    log_i("limiter size:%d\n", need_limiter_noiseGate_buf(1));
    if (!limiter) {
        return NULL;
    }
    //限幅器启动因子 int32(exp(-0.65/(16000 * 0.005))*2^30)   16000为采样率  0.005 为启动时间(s)
    int limiter_attfactor = 1065053018;
    //限幅器释放因子 int32(exp(-0.15/(16000 * 0.1))*2^30)     16000为采样率  0.1   为释放时间(s)
    int limiter_relfactor = 1073641165;
    //限幅器阈值(mdb)
    //int limiter_threshold = CONST_LIMITER_THR;

    //噪声门限启动因子 int32(exp(-1/(16000 * 0.1))*2^30)       16000为采样率  0.1   为释放时间(s)
    int noiseGate_attfactor = 1073070945;
    //噪声门限释放因子 int32(exp(-1/(16000 * 0.005))*2^30)     16000为采样率  0.005 为启动时间(s)
    int noiseGate_relfactor = 1060403589;
    //噪声门限(mdb)
    //int noiseGate_threshold = -25000;
    //低于噪声门限阈值的增益 (0~1)*2^30
    //int noise
    //Gate_low_thr_gain = 0 << 30;

    if (sample_rate == 8000) {
        limiter_attfactor = 1056434522;
        limiter_relfactor =  1073540516;
        noiseGate_attfactor = 1072400485;
        noiseGate_relfactor =  1047231044;
    }

    limiter_noiseGate_init(limiter,
                           limiter_attfactor,
                           limiter_relfactor,
                           noiseGate_attfactor,
                           noiseGate_relfactor,
                           LIMITER_THR,
                           LIMITER_NOISE_GATE,
                           LIMITER_NOISE_GAIN,
                           sample_rate, 1);

    return limiter;
}
/*----------------------------------------------------------------------------*/
/**@brief    esco限福器关闭
   @param    *limiter: 限福器句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_limiter_close(void *limiter)
{
    free(limiter);
}
#endif /* TCFG_ESCO_LIMITER */


/*----------------------------------------------------------------------------*/
/**@brief    esco解码输出数据处理
   @param    *entry: 数据流入口
   @param    *in: 输入数据
   @param    *out: 输出数据
   @return   处理了多长数据
   @note
*/
/*----------------------------------------------------------------------------*/
static int esco_dec_data_handler(struct audio_stream_entry *entry,
                                 struct audio_data_frame *in,
                                 struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct esco_decoder *esco_dec = container_of(decoder, struct esco_decoder, decoder);
    struct esco_dec_hdl *dec = container_of(esco_dec, struct esco_dec_hdl, dec);

    if (dec->remain == 0) {
        if (dec->tws_mute_en) {
            memset(in->data, 0, in->data_len);
        }

#if TCFG_ESCO_PLC
        if (dec->plc && out && out->data) {
            esco_plc_run(in->data, in->data_len, *(u8 *)out->data);
        }
#endif/*TCFG_ESCO_PLC*/

#if TCFG_ESCO_LIMITER
        if (dec->limiter) {
            limiter_noiseGate_run(dec->limiter, in->data, in->data, in->data_len / 2);
        }
#endif/*TCFG_ESCO_LIMITER*/

    }

    int wlen = esco_decoder_output_handler(&dec->dec, in);

    if (in->data_len != wlen) {
        dec->remain = 1;
    } else {
        dec->remain = 0;
    }
    return wlen;
}


/*----------------------------------------------------------------------------*/
/**@brief    esco解码释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void esco_dec_release()
{
    audio_decoder_task_del_wait(&decode_task, &bt_esco_dec->wait);

    if (bt_esco_dec->dec.coding_type == AUDIO_CODING_MSBC) {
        clock_remove(DEC_MSBC_CLK);
    } else if (bt_esco_dec->dec.coding_type == AUDIO_CODING_CVSD) {
        clock_remove(DEC_CVSD_CLK);
    }

    local_irq_disable();
    free(bt_esco_dec);
    bt_esco_dec = NULL;
    local_irq_enable();
}

/*----------------------------------------------------------------------------*/
/**@brief    esco解码事件返回
   @param    *decoder: 解码器句柄
   @param    argc: 参数个数
   @param    *argv: 参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        log_i("AUDIO_DEC_EVENT_END\n");
        esco_dec_close();
        break;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    esco解码close
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_audio_res_close(void)
{
    /*
     *先关闭aec，里面有复用到enc的buff，再关闭enc，
     *如果没有buf复用，则没有先后顺序要求。
     */
    if (!bt_esco_dec->dec.start) {
        return ;
    }
    bt_esco_dec->dec.start = 0;
    bt_esco_dec->dec.enc_start = 0;
    audio_aec_close();
    esco_enc_close();
    esco_decoder_close(&bt_esco_dec->dec);
    esco_eq_drc_close(bt_esco_dec->eq_drc);
    audio_mixer_ch_close(&bt_esco_dec->mix_ch);
#if (RECORDER_MIX_EN)
    audio_mixer_ch_close(&bt_esco_dec->rec_mix_ch);
#endif//RECORDER_MIX_EN

    esco_output_sync_close(bt_esco_dec->sync);
    bt_esco_dec->sync = NULL;

#if TCFG_ESCO_PLC
    if (bt_esco_dec->plc) {
        esco_plc_close(bt_esco_dec->plc);
        bt_esco_dec->plc = NULL;
    }
#endif/*TCFG_ESCO_PLC*/

#if TCFG_ESCO_LIMITER
    if (bt_esco_dec->limiter) {
        esco_limiter_close(bt_esco_dec->limiter);
        bt_esco_dec->limiter = NULL;
    }
#endif /*TCFG_ESCO_LIMITER*/

    if (bt_esco_dec->stream) {
        audio_stream_close(bt_esco_dec->stream);
        bt_esco_dec->stream = NULL;
    }

#if TCFG_ESCO_USE_SPEC_MIX_LEN
    /*恢复mix_buf的长度*/
    audio_mixer_set_output_buf(&mixer, mix_buff, AUDIO_MIXER_LEN);
#endif /*TCFG_ESCO_USE_SPEC_MIX_LEN*/
    app_audio_state_exit(APP_AUDIO_STATE_CALL);
    bt_esco_dec->dec.start = 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    esco解码数据流激活
   @param    *p: 私有句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_dec_out_stream_resume(void *p)
{
    struct esco_dec_hdl *dec = (struct esco_dec_hdl *)p;

    audio_decoder_resume(&dec->dec.decoder);
}

/*----------------------------------------------------------------------------*/
/**@brief    开始esco解码
  @param
  @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int esco_dec_start()
{
    int err;
    struct esco_dec_hdl *dec = bt_esco_dec;

    if (!bt_esco_dec) {
        return -EINVAL;
    }
    err = esco_decoder_open(&dec->dec, &decode_task);
    if (err) {
        goto __err1;
    }

    audio_decoder_set_event_handler(&dec->dec.decoder, esco_dec_event_handler, 0);

#if TCFG_ESCO_USE_SPEC_MIX_LEN
    /*
     *(1)bt_esco_dec输出是120或者240，所以通话的时候，修改mix_buff的长度，提高效率
     *(2)其他大部分时候解码输出是512的倍数，通话结束，恢复mix_buff的长度，提高效率
     */
    audio_mixer_set_output_buf(&mixer, mix_buff, AUDIO_MIXER_LEN / 240 * 240);
#endif /*TCFG_ESCO_USE_SPEC_MIX_LEN*/

    audio_mixer_ch_open_head(&dec->mix_ch, &mixer); // 挂载到mixer最前面
    audio_mixer_ch_set_src(&dec->mix_ch, 1, 0);
    audio_mixer_ch_set_no_wait(&dec->mix_ch, 1, 10); // 超时自动丢数
    audio_mixer_ch_sample_sync_enable(&dec->mix_ch, 1);

#if (RECORDER_MIX_EN)
    audio_mixer_ch_open_head(&dec->rec_mix_ch, &recorder_mixer); // 挂载到mixer最前面
    audio_mixer_ch_set_src(&dec->rec_mix_ch, 1, 0);
    audio_mixer_ch_set_no_wait(&dec->rec_mix_ch, 1, 10); // 超时自动丢数
    audio_mixer_ch_sample_sync_enable(&dec->rec_mix_ch, 1);
    audio_mixer_ch_set_sample_rate(&dec->mix_ch, dec->dec.sample_rate);
    /* audio_mixer_ch_set_sample_rate(&dec->rec_mix_ch, dec->dec.sample_rate); */
    printf("[%s], dec->dec.sample_rate = %d\n", __FUNCTION__, dec->dec.sample_rate);
#endif//RECORDER_MIX_EN

    dec->eq_drc = esco_eq_drc_open(dec->dec.sample_rate, dec->dec.ch_num);

    dec->dec.decoder.entry.data_handler = esco_dec_data_handler;

    dec->sync = esco_output_sync_open(0);
    /*使能同步，配置延时时间*/
    esco_decoder_stream_sync_enable(&dec->dec, dec->sync->context, dec->dec.sample_rate, 25);
    // 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &dec->dec.decoder.entry;
    if (dec->sync) {
        entries[entry_cnt++] = dec->sync->entry;
    }
#if TCFG_EQ_ENABLE && TCFG_PHONE_EQ_ENABLE
    if (dec->eq_drc) {
        entries[entry_cnt++] = &dec->eq_drc->entry;
    }
#endif
    entries[entry_cnt++] = &dec->mix_ch.entry;
    dec->stream = audio_stream_open(dec, esco_dec_out_stream_resume);
    audio_stream_add_list(dec->stream, entries, entry_cnt);

#if (RECORDER_MIX_EN)
    audio_stream_add_entry(entries[entry_cnt - 2], &dec->rec_mix_ch.entry);
#endif//RECORDER_MIX_EN

    audio_output_set_start_volume(APP_AUDIO_STATE_CALL);

#if TCFG_ESCO_PLC
    dec->plc = esco_plc_init();
#endif

#if TCFG_ESCO_LIMITER
    dec->limiter = esco_limiter_init(dec->dec.sample_rate);
#endif /* TCFG_ESCO_LIMITER */

#if TCFG_USER_TWS_ENABLE
    if (tws_network_audio_was_started()) {
        tws_network_local_audio_start();
        esco_decoder_join_tws(&dec->dec);
    }
#endif
    lmp_private_esco_suspend_resume(2);
    dec->dec.start = 1;
    err = audio_decoder_start(&dec->dec.decoder);
    if (err) {
        goto __err3;
    }
    dec->dec.frame_get = 0;

    err = audio_aec_init(dec->dec.sample_rate);
    if (err) {
        log_i("audio_aec_init failed:%d", err);
        //goto __err3;
    }
    err = esco_enc_open(dec->dec.coding_type, dec->dec.esco_len);
    if (err) {
        log_i("audio_enc_open failed:%d", err);
        //goto __err3;
    }
    dec->dec.enc_start = 1;

    clock_set_cur();
    return 0;

__err3:
    dec->dec.start = 0;
    esco_eq_drc_close(dec->eq_drc);
    audio_mixer_ch_close(&dec->mix_ch);
#if (RECORDER_MIX_EN)
    audio_mixer_ch_close(&dec->rec_mix_ch);
#endif//RECORDER_MIX_EN

    if (dec->sync) {
        esco_output_sync_close(dec->sync);
        dec->sync = NULL;
    }
    if (dec->stream) {
        audio_stream_close(dec->stream);
        dec->stream = NULL;
    }
__err2:
    esco_decoder_close(&dec->dec);
__err1:
    esco_dec_release();
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    esco解码资源等待
   @param    *wait: 句柄
   @param    event: 事件
   @return   0：成功
   @note     用于多解码打断处理
*/
/*----------------------------------------------------------------------------*/
static int esco_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    log_d("esco_wait_res_handler %d\n", event);
    if (event == AUDIO_RES_GET) {
        err = esco_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        if (bt_esco_dec->dec.start) {
            lmp_private_esco_suspend_resume(1);
            esco_audio_res_close();
        }
    }

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    打开esco解码
   @param    *param: 媒体信息
   @param    mutw: 静音
   @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
int esco_dec_open(void *param, u8 mute)
{
    int err;
    struct esco_dec_hdl *dec;
    u32 esco_param = *(u32 *)param;
    int esco_len = esco_param >> 16;
    int codec_type = esco_param & 0x000000ff;

    log_i("esco_dec_open, type=%d,len=%d\n", codec_type, esco_len);

    dec = zalloc(sizeof(*dec));
    if (!dec) {
        return -ENOMEM;
    }

#if TCFG_DEC2TWS_ENABLE
    localtws_media_disable();
#endif

    bt_esco_dec = dec;

    dec->tws_mute_en = mute;
    dec->dec.esco_len = esco_len;
    dec->dec.out_ch_num = audio_output_channel_num();
    if (codec_type == 3) {
        dec->dec.coding_type = AUDIO_CODING_MSBC;
        dec->dec.sample_rate = 16000;
        dec->dec.ch_num = 1;
        clock_add(DEC_MSBC_CLK);
    } else if (codec_type == 2) {
        dec->dec.coding_type = AUDIO_CODING_CVSD;
        dec->dec.sample_rate = 8000;
        dec->dec.ch_num = 1;
        clock_add(DEC_CVSD_CLK);
    }

    dec->wait.priority = 2;
    dec->wait.preemption = 0;
    dec->wait.snatch_same_prio = 1;
    dec->wait.handler = esco_wait_res_handler;
    err = audio_decoder_task_add_wait(&decode_task, &dec->wait);
    if (bt_esco_dec->dec.start == 0) {
        lmp_private_esco_suspend_resume(1);
    }

#if AUDIO_OUTPUT_AUTOMUTE
    extern void mix_out_automute_skip(u8 skip);
    mix_out_automute_skip(1);
#endif

    return err;
}


/*----------------------------------------------------------------------------*/
/**@brief    关闭esco解码
   @param    :
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void esco_dec_close()
{
    if (!bt_esco_dec) {
        return;
    }

    esco_audio_res_close();
    esco_dec_release();

    log_i("esco_dec_close: exit\n");
#if AUDIO_OUTPUT_AUTOMUTE
    extern void mix_out_automute_skip(u8 skip);
    mix_out_automute_skip(0);
#endif
    clock_set_cur();
#if TCFG_DEC2TWS_ENABLE
    localtws_media_enable();
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙音频正在运行
   @param
   @return   1: 正在运行
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_audio_is_running(void)
{
    return (bt_a2dp_dec || bt_esco_dec);
}
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙播放正在运行
   @param
   @return   1: 正在运行
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_media_is_running(void)
{
    return bt_a2dp_dec != NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙电话正在运行
   @param
   @return   1: 正在运行
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_phone_dec_is_running()
{
    return bt_esco_dec != NULL;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙解码idle判断
   @param
   @return   1: idle
   @return   0: busy
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 bt_dec_idle_query()
{
    if (bt_audio_is_running()) {
        return 0;
    }

    return 1;
}
REGISTER_LP_TARGET(bt_dec_lp_target) = {
    .name = "bt_dec",
    .is_idle = bt_dec_idle_query,
};



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 eq drc 打开
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
void *a2dp_eq_drc_open(u16 sample_rate, u8 ch_num)
{

#if TCFG_EQ_ENABLE
    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

#if TCFG_BT_MUSIC_EQ_ENABLE
    effect_parm.eq_en = 1;

#if TCFG_DRC_ENABLE
#if TCFG_BT_MUSIC_DRC_ENABLE
    effect_parm.drc_en = 1;
    effect_parm.drc_cb = drc_get_filter_info;

#endif
#endif



    if (effect_parm.eq_en) {
        effect_parm.async_en = 1;
        effect_parm.out_32bit = 1;
        effect_parm.online_en = 1;
        effect_parm.mode_en = 1;
    }

    effect_parm.eq_name = song_eq_mode;


    effect_parm.ch_num = ch_num;
    effect_parm.sr = sample_rate;
    effect_parm.eq_cb = eq_get_filter_info;

    eq_drc = audio_eq_drc_open(&effect_parm);

    clock_add(EQ_CLK);
    if (effect_parm.drc_en) {
        clock_add(EQ_DRC_CLK);
    }
#endif
    return eq_drc;
#endif//TCFG_EQ_ENABLE

    return NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void a2dp_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
#if TCFG_BT_MUSIC_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
#if TCFG_DRC_ENABLE
#if TCFG_BT_MUSIC_DRC_ENABLE
        clock_remove(EQ_DRC_CLK);
#endif
#endif
    }
#endif
#endif
    return;
}
/*----------------------------------------------------------------------------*/
/**@brief    通话模式 eq drc 打开
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
void *esco_eq_drc_open(u16 sample_rate, u8 ch_num)
{
    mix_out_high_bass_dis(AUDIO_EQ_HIGH_BASS_DIS, 1);
#if TCFG_EQ_ENABLE
    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

#if TCFG_PHONE_EQ_ENABLE
    effect_parm.eq_en = 1;

    if (effect_parm.eq_en) {
        effect_parm.async_en = 1;
        effect_parm.online_en = 1;
        effect_parm.mode_en = 0;
    }

    effect_parm.eq_name = call_eq_mode;

    effect_parm.ch_num = ch_num;
    effect_parm.sr = sample_rate;
    effect_parm.eq_cb = eq_phone_get_filter_info;

    eq_drc = audio_eq_drc_open(&effect_parm);

    clock_add(EQ_CLK);
#endif
    return eq_drc;
#endif//TCFG_EQ_ENABLE

    return NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    通话模式 eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void esco_eq_drc_close(struct audio_eq_drc *eq_drc)
{

#if TCFG_EQ_ENABLE
#if TCFG_PHONE_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
    }
#endif
#endif
    mix_out_high_bass_dis(AUDIO_EQ_HIGH_BASS_DIS, 0);
    return;
}

/*----------------------------------------------------------------------------*/
/**@brief    a2dp软件数字音量设置
   @param
   @return
   @note    该函数由应用实现
*/
/*----------------------------------------------------------------------------*/
void set_a2dp_digital_vol_demo(u8 vol)
{
//调用数字音量接口设置 音量值
}


/*----------------------------------------------------------------------------*/
/**@brief    a2dp软件数字音量获取
   @param
   @return  返回软件数字音量值
   @note    该函数由应用实现
*/
/*----------------------------------------------------------------------------*/
static int get_a2dp_cur_vol_test()
{
    u8 vol =  0;
//调用数字音量接口获取 音量值
    return vol;
}

static float alpha_tab[] = {0.2, 0.4, 0.6, 0.8, 1};
/*----------------------------------------------------------------------------*/
/**@brief    根据获取的软件数字音量值，选用不同的alpha值，提供给等响度模块使用
   @param
   @return  返回软件数字音量值
   @note    该函数由使用者实现
*/
/*----------------------------------------------------------------------------*/
static int get_alpha(float *alpha, u8 *volume, u8 threadhold_vol)
{
    int vol = get_a2dp_cur_vol_test();
    if (vol < threadhold_vol) {
        float tmp_alpha = alpha_tab[4];
        if (vol < 3) {
            tmp_alpha = alpha_tab[0];
        } else if (vol < 6) {
            tmp_alpha = alpha_tab[1];
        } else if (vol < 9) {
            tmp_alpha = alpha_tab[2];
        } else if (vol < 12) {
            tmp_alpha = alpha_tab[3];
        }
        *alpha = tmp_alpha;
        *volume = vol;
        return 0;
    }
    return -1;
}
/*----------------------------------------------------------------------------*/
/**@brief    等响度模块打开 例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
equal_loudness_hdl *equal_loudness_open_demo(u16 sample_rate, u8 ch_num)
{
    equal_loudness_hdl *loudness = NULL;
    equalloudness_open_parm parm = {0};
    parm.threadhold_vol = 17;
    parm.sr = sample_rate;
    parm.channel = ch_num;
    parm.alpha_cb = get_alpha;
    loudness = audio_equal_loudness_open(&parm);
#if 0
    if (loudness) {
        equalloudness_update_parm parm = {0};
        parm.threadhold_vol = 18;
        audio_equal_loudness_parm_update(loudness, 0, &parm);
    }
#endif

    clock_add(DEC_LOUDNES_CLK);
    return loudness;

}
/*----------------------------------------------------------------------------*/
/**@brief    等响度模块关闭例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

void equal_loudness_close_demo(equal_loudness_hdl *loudness)
{
    if (loudness) {
        audio_equal_loudness_close(loudness);
        loudness = NULL;
    }
    clock_remove(DEC_LOUDNES_CLK);
}

/*----------------------------------------------------------------------------*/
/**@brief    环绕音效切换测试例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void surround_switch_test(void *p)
{

    if (!p) {
        return;
    }

    static u8 cnt_type = 0;
    audio_surround_parm_update(p, cnt_type, NULL);
    if (++cnt_type > EFFECT_FOUR_SENSION_BATTLEFIELD) {
        cnt_type = EFFECT_3D_PANORAMA;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    环绕音效模块打开例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
surround_hdl *surround_open_demo(u8 ch_num)
{
    surround_hdl *surround = NULL;
    surround_open_parm parm = {0};
    parm.channel = ch_num;
    surround = audio_surround_open(&parm);
#if 0
    if (surround) {
        audio_surround_parm_update(surround, EFFECT_3D_PANORAMA, NULL);
    }
#endif
    clock_add(DEC_3D_CLK);
    /* sys_timer_add(surround, surround_switch_test, 10000); */
    return surround;
}
/*----------------------------------------------------------------------------*/
/**@brief    环绕音效关闭例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void surround_close(surround_hdl *surround)
{
    if (surround) {
        audio_surround_close(surround);
        surround = NULL;
    }
    clock_remove(DEC_3D_CLK);
}



/*----------------------------------------------------------------------------*/
/**@brief    虚拟低音打开例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
vbass_hdl *vbass_open_demo(u16 sample_rate, u8 ch_num)
{
    vbass_hdl *vbass = NULL;
    vbass_open_parm parm = {0};
    parm.sr = sample_rate;
    parm.channel = ch_num;
    vbass = audio_vbass_open(&parm);
#if 0
    if (vbass) {
        vbass_update_parm def_parm = {0};
        def_parm.bass_f = 300;
        def_parm.level = 8192;
        audio_vbass_parm_update(vbass, 0, &def_parm);
    }
#endif
    clock_add(DEC_VBASS_CLK);

    return vbass;
}

/*----------------------------------------------------------------------------*/
/**@brief    虚拟低音关闭例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void vbass_close_demo(vbass_hdl *vbass)
{
    if (vbass) {
        audio_vbass_close(vbass);
        vbass = NULL;
    }
    clock_remove(DEC_VBASS_CLK);
}
