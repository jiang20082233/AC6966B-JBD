/*
 ****************************************************************
 *File : audio_dc_linein.c
 *Note :
 *
 ****************************************************************
 */


#include "asm/includes.h"
#include "media/includes.h"
#include "media/pcm_decoder.h"
#include "system/includes.h"
#include "effectrs_sync.h"
#include "application/audio_eq_drc_apply.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_dec.h"
#include "app_config.h"
#include "app_main.h"
#include "audio_enc.h"
#include "clock_cfg.h"
#include "dev_manager.h"

#if (TCFG_LINEIN_ENABLE || TCFG_FM_ENABLE)//外部收音走linein

//////////////////////////////////////////////////////////////////////////////

struct linein_dec_hdl {
    struct audio_stream *stream;	// 音频流
    struct pcm_decoder pcm_dec;		// pcm解码句柄
    struct audio_res_wait wait;		// 资源等待句柄
    struct audio_mixer_ch mix_ch;	// 叠加句柄
#if (RECORDER_MIX_EN)
    struct audio_mixer_ch rec_mix_ch;	// 叠加句柄
#endif/*RECORDER_MIX_EN*/
    struct audio_eq_drc *eq_drc;//eq drc句柄
    u32 id;				// 唯一标识符，随机值
    u32 start : 1;		// 正在解码
    u32 source : 8;		// linein音频源
    void *linein;		// 底层驱动句柄
};


//////////////////////////////////////////////////////////////////////////////

struct linein_dec_hdl *linein_dec = NULL;	// linein解码句柄

#if TCFG_LINEIN_REC_EN
static u32 linein_enc_magic = 0;			// linein录音id标记
#endif

//////////////////////////////////////////////////////////////////////////////
void *linein_eq_drc_open(u16 sample_rate, u8 ch_num);
void linein_eq_drc_close(struct audio_eq_drc *eq_drc);

int linein_sample_size(void *hdl);
int linein_sample_total(void *hdl);


//////////////////////////////////////////////////////////////////////////////

/*----------------------------------------------------------------------------*/
/**@brief    linein解码释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void linein_dec_relaese()
{
    if (linein_dec) {
        audio_decoder_task_del_wait(&decode_task, &linein_dec->wait);
        clock_remove(AUDIO_CODING_PCM);
        local_irq_disable();
        free(linein_dec);
        linein_dec = NULL;
        local_irq_enable();
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    linein解码事件处理
   @param    *decoder: 解码器句柄
   @param    argc: 参数个数
   @param    *argv: 参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void linein_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        if (!linein_dec) {
            log_i("linein_dec handle err ");
            break;
        }

        if (linein_dec->id != argv[1]) {
            log_w("linein_dec id err : 0x%x, 0x%x \n", linein_dec->id, argv[1]);
            break;
        }

        linein_dec_close();
        //audio_decoder_resume_all(&decode_task);
        break;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    linein同步跟随着变化
   @param    in_rate: 跟随输入采样率
   @param    out_rate: 跟随输出采样率
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_linein_set_src_by_dac_sync(int in_rate, int out_rate)
{
    struct linein_dec_hdl *dec = linein_dec;
    if (dec && dec->start && (dec->pcm_dec.dec_no_out_sound == 0)) {
        audio_buf_sync_follow_rate(&dec->mix_ch.sync, in_rate, out_rate);
#if(RECORDER_MIX_EN)
        audio_buf_sync_follow_rate(&dec->rec_mix_ch.sync, in_rate, out_rate);
#endif/*RECORDER_MIX_EN*/
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    linein解码数据输出
   @param    *entry: 音频流句柄
   @param    *in: 输入信息
   @param    *out: 输出信息
   @return   输出长度
   @note     *out未使用
*/
/*----------------------------------------------------------------------------*/
static int linein_dec_data_handler(struct audio_stream_entry *entry,
                                   struct audio_data_frame *in,
                                   struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct pcm_decoder *pcm_dec = container_of(decoder, struct pcm_decoder, decoder);
    struct linein_dec_hdl *dec = container_of(pcm_dec, struct linein_dec_hdl, pcm_dec);
    if (!dec->start) {
        return 0;
    }
    audio_stream_run(&decoder->entry, in);
#if TCFG_LINEIN_REC_EN
    pcm2file_enc_write_pcm(dec->enc, in->data, decoder->process_len);
#endif
    return decoder->process_len;
}

/*----------------------------------------------------------------------------*/
/**@brief    linein解码数据流激活
   @param    *p: 私有句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void linein_dec_out_stream_resume(void *p)
{
    struct linein_dec_hdl *dec = p;
    audio_decoder_resume(&dec->pcm_dec.decoder);
}

/*----------------------------------------------------------------------------*/
/**@brief    linein解码激活
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void linein_dec_resume(void)
{
    if (linein_dec) {
        audio_decoder_resume(&linein_dec->pcm_dec.decoder);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    linein解码开始
   @param
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int linein_dec_start()
{
    int err;
    struct linein_dec_hdl *dec = linein_dec;
    struct audio_mixer *p_mixer = &mixer;

    if (!linein_dec) {
        return -EINVAL;
    }

    err = pcm_decoder_open(&dec->pcm_dec, &decode_task);
    if (err) {
        goto __err1;
    }

    // 打开linein驱动
    dec->linein = linein_sample_open(dec->source, dec->pcm_dec.sample_rate);
    linein_sample_set_resume_handler(dec->linein, linein_dec_resume);

    pcm_decoder_set_event_handler(&dec->pcm_dec, linein_dec_event_handler, dec->id);
    pcm_decoder_set_read_data(&dec->pcm_dec, linein_sample_read, dec->linein);
    pcm_decoder_set_data_handler(&dec->pcm_dec, linein_dec_data_handler);

#if TCFG_PCM_ENC2TWS_ENABLE
    {
        // localtws
        struct audio_fmt enc_f;
        memcpy(&enc_f, &dec->pcm_dec.decoder.fmt, sizeof(struct audio_fmt));
        enc_f.coding_type = AUDIO_CODING_SBC;
        int ret = localtws_enc_api_open(&enc_f, LOCALTWS_ENC_FLAG_STREAM);
        if (ret == true) {
            dec->pcm_dec.dec_no_out_sound = 1;
            // 重定向mixer
            p_mixer = &g_localtws.mixer;
            // 关闭资源等待。最终会在localtws解码处等待
            audio_decoder_task_del_wait(&decode_task, &dec->wait);
        }
    }
#endif

    // 设置叠加功能
    audio_mixer_ch_open_head(&dec->mix_ch, p_mixer);
    audio_mixer_ch_set_no_wait(&dec->mix_ch, 1, 10); // 超时自动丢数
#if (RECORDER_MIX_EN)
    audio_mixer_ch_open_head(&dec->rec_mix_ch, &recorder_mixer);
    audio_mixer_ch_set_no_wait(&dec->rec_mix_ch, 1, 10); // 超时自动丢数
#endif/*RECORDER_MIX_EN*/
    if (dec->pcm_dec.dec_no_out_sound) {
        audio_mixer_ch_set_src(&dec->mix_ch, 1, 0);
#if (RECORDER_MIX_EN)
        audio_mixer_ch_set_src(&dec->rec_mix_ch, 1, 0);
#endif/*RECORDER_MIX_EN*/
    } else {
        struct audio_mixer_ch_sync_info info = {0};
        info.priv = dec->linein;
        info.get_total = linein_sample_total;
        info.get_size = linein_sample_size;
        audio_mixer_ch_set_sync(&dec->mix_ch, &info, 1, 1);
#if (RECORDER_MIX_EN)
        audio_mixer_ch_set_sync(&dec->rec_mix_ch, &info, 1, 1);
#endif/*RECORDER_MIX_EN*/
    }
    dec->eq_drc = linein_eq_drc_open(dec->pcm_dec.sample_rate, dec->pcm_dec.output_ch_num);
    // 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &dec->pcm_dec.decoder.entry;
#if TCFG_EQ_ENABLE && TCFG_LINEIN_MODE_EQ_ENABLE
    if (dec->eq_drc) {
        entries[entry_cnt++] = &dec->eq_drc->entry;
    }
#endif
    entries[entry_cnt++] = &dec->mix_ch.entry;
    dec->stream = audio_stream_open(dec, linein_dec_out_stream_resume);
    audio_stream_add_list(dec->stream, entries, entry_cnt);

#if (RECORDER_MIX_EN)
    audio_stream_add_entry(entries[entry_cnt - 2], &dec->rec_mix_ch.entry);
#endif/*RECORDER_MIX_EN*/

    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);

    dec->start = 1;
    err = audio_decoder_start(&dec->pcm_dec.decoder);
    if (err) {
        goto __err3;
    }
    clock_set_cur();
    return 0;
__err3:
    dec->start = 0;
    linein_eq_drc_close(dec->eq_drc);
    if (dec->linein) {
        linein_sample_close(dec->linein);
        dec->linein = NULL;
    }
    audio_mixer_ch_close(&dec->mix_ch);
#if (RECORDER_MIX_EN)
    audio_mixer_ch_close(&dec->rec_mix_ch);
#endif/*RECORDER_MIX_EN*/
#if TCFG_PCM_ENC2TWS_ENABLE
    if (linein_dec->pcm_dec.dec_no_out_sound) {
        linein_dec->pcm_dec.dec_no_out_sound = 0;
        localtws_enc_api_close();
    }
#endif
    if (dec->stream) {
        audio_stream_close(dec->stream);
        dec->stream = NULL;
    }
    pcm_decoder_close(&dec->pcm_dec);
__err1:
    linein_dec_relaese();
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    fm解码关闭
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void __linein_dec_close(void)
{
    if (linein_dec && linein_dec->start) {
        linein_dec->start = 0;

#if TCFG_LINEIN_REC_EN
        linein_pcm_enc_stop();
#endif
        pcm_decoder_close(&linein_dec->pcm_dec);
        linein_sample_close(linein_dec->linein);
        linein_dec->linein = NULL;

        linein_eq_drc_close(linein_dec->eq_drc);
        audio_mixer_ch_close(&linein_dec->mix_ch);
#if (RECORDER_MIX_EN)
        audio_mixer_ch_close(&linein_dec->rec_mix_ch);
#endif/*RECORDER_MIX_EN*/
#if TCFG_PCM_ENC2TWS_ENABLE
        if (linein_dec->pcm_dec.dec_no_out_sound) {
            linein_dec->pcm_dec.dec_no_out_sound = 0;
            localtws_enc_api_close();
        }
#endif
        if (linein_dec->stream) {
            audio_stream_close(linein_dec->stream);
            linein_dec->stream = NULL;
        }
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    linein解码资源等待
   @param    *wait: 句柄
   @param    event: 事件
   @return   0：成功
   @note     用于多解码打断处理
*/
/*----------------------------------------------------------------------------*/
static int linein_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    log_i("linein_wait_res_handler, event:%d\n", event);
    if (event == AUDIO_RES_GET) {
        // 启动解码
        err = linein_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        // 被打断
        __linein_dec_close();
    }

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    打开linein解码
   @param    source: 音频源
   @param    sample_rate: 采样率
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int linein_dec_open(u8 source, u32 sample_rate)
{
    int err;
    struct linein_dec_hdl *dec;
    dec = zalloc(sizeof(*dec));
    if (!dec) {
        return -ENOMEM;
    }
    linein_dec = dec;
    dec->id = rand32();

    switch (source) {
    case BIT(0):
    case BIT(1):
        dec->pcm_dec.ch_num = 1;
        break;
    case BIT(2):
    case BIT(3):
        dec->pcm_dec.ch_num = 1;
        break;
    default:
        log_e("Linein can only select a single channel\n");
        ASSERT(0, "err\n");
        break;
    }
    dec->source = source;

    dec->pcm_dec.output_ch_num = audio_output_channel_num();
    dec->pcm_dec.sample_rate = sample_rate;

    dec->wait.priority = 2;
    dec->wait.preemption = 0;
    dec->wait.snatch_same_prio = 1;
    dec->wait.handler = linein_wait_res_handler;
    clock_add(AUDIO_CODING_PCM);


#if TCFG_DEC2TWS_ENABLE
    // 设置localtws重播接口
    localtws_globle_set_dec_restart(linein_dec_push_restart);
#endif

    err = audio_decoder_task_add_wait(&decode_task, &dec->wait);
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    关闭linein解码
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void linein_dec_close(void)
{
    if (!linein_dec) {
        return;
    }

    __linein_dec_close();
    linein_dec_relaese();
    clock_set_cur();
    log_i("linein dec close \n\n ");
}

/*----------------------------------------------------------------------------*/
/**@brief    linein解码重新开始
   @param    id: 文件解码id
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int linein_dec_restart(int id)
{
    if ((!linein_dec) || (id != linein_dec->id)) {
        return -1;
    }
    u8 source = linein_dec->source;
    u32 sample_rate = linein_dec->pcm_dec.sample_rate;
    linein_dec_close();
    int err = linein_dec_open(source, sample_rate);
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    推送linein解码重新开始命令
   @param
   @return   true：成功
   @return   false：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int linein_dec_push_restart(void)
{
    if (!linein_dec) {
        return false;
    }
    int argv[3];
    argv[0] = (int)linein_dec_restart;
    argv[1] = 1;
    argv[2] = (int)linein_dec->id;
    os_taskq_post_type(os_current_task(), Q_CALLBACK, ARRAY_SIZE(argv), argv);
    return true;
}


/***********************linein pcm enc******************************/
#if TCFG_LINEIN_REC_EN

/*----------------------------------------------------------------------------*/
/**@brief    linein录音事件处理
   @param    *decoder: 编码器句柄
   @param    argc: 参数个数
   @param    *argv: 参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void linein_pcm_enc_event_handler(struct audio_encoder *encoder, int argc, int *argv)
{
    log_i("linein_pcm_enc_event_handler, argv[]:%d, %d ", argv[0], argv[1]);
    linein_pcm_enc_stop();
}

/*----------------------------------------------------------------------------*/
/**@brief    linein录音停止
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void linein_pcm_enc_stop(void)
{
    void *ptr;
    if (linein_dec && linein_dec->enc) {
        ptr = linein_dec->enc;
        linein_dec->enc = NULL;
        linein_enc_magic++;
        pcm2file_enc_close(ptr);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    linein录音开始
   @param
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int linein_pcm_enc_start(void)
{
    void *ptr;
    if (!linein_dec) {
        return -1;
    }
    linein_pcm_enc_stop();
    struct audio_fmt fmt = {0};
    fmt.coding_type = AUDIO_CODING_MP3;
    /* fmt.coding_type = AUDIO_CODING_WAV; */
    fmt.bit_rate = 128;
    fmt.channel = linein_dec->fmt.channel;
    fmt.sample_rate = linein_dec->fmt.sample_rate;
    linein_dec->enc = pcm2file_enc_open(&fmt, dev_manager_find_active(0));
    if (!linein_dec->enc) {
        return -1;
    }
    pcm2file_enc_set_evt_handler(linein_dec->enc, linein_pcm_enc_event_handler, linein_enc_magic);
    pcm2file_enc_start(linein_dec->enc);
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    检测linein是否在录音
   @param
   @return   true：正录音
   @return   false：不在
   @note
*/
/*----------------------------------------------------------------------------*/
bool linein_pcm_enc_check()
{
    if (linein_dec && linein_dec->enc) {
        return true;
    }
    return false;
}
#endif /*TCFG_LINEIN_REC_EN*/

/*----------------------------------------------------------------------------*/
/**@brief    linein模式 eq drc 打开
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
void *linein_eq_drc_open(u16 sample_rate, u8 ch_num)
{
#if TCFG_EQ_ENABLE

    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};
#if TCFG_LINEIN_MODE_EQ_ENABLE
    effect_parm.eq_en = 1;

#if TCFG_DRC_ENABLE
#if TCFG_LINEIN_MODE_DRC_ENABLE
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
#endif
    return NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    linein模式 eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void linein_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
#if TCFG_LINEIN_MODE_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
#if TCFG_DRC_ENABLE
#if TCFG_LINEIN_MODE_DRC_ENABLE
        clock_remove(EQ_DRC_CLK);
#endif
#endif
    }
#endif
#endif
    return;
}

#endif
