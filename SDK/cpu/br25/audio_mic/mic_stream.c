#include "mic_stream.h"
#include "app_config.h"
#include "system/includes.h"
#include "audio_splicing.h"
#include "audio_config.h"
#include "asm/dac.h"
#include "audio_enc.h"
#include "audio_dec.h"
#include "media/includes.h"

#include "media/pcm_decoder.h"

#include "user_fun_cfg.h"
#if (TCFG_MIC_EFFECT_ENABLE)

#define MIC_STREAM_TASK_NAME				"mic_stream"


struct __mic_stream {
    struct adc_mic_ch mic_ch;
    struct audio_adc_output_hdl adc_output;
    struct __mic_stream_parm		*parm;
    struct __mic_stream_io 			out;
    u8								*temp_buf;
    u8								*adc_buf;
    cbuffer_t 						adc_cbuf;
    OS_SEM 							sem;
    struct audio_decoder 			decoder;
    struct audio_mixer_ch 			mix_ch;
    struct audio_res_wait 			wait;
    volatile u8						busy:		1;
        volatile u8						release:	1;
        volatile u8						revert:		6;
        struct pcm_decoder pcm_dec;		// pcm解码句柄
        struct audio_stream *audio_stream;		// 音频流
    };

#define MIC_SIZEOF_ALIN(var,al)     ((((var)+(al)-1)/(al))*(al))

    extern struct audio_dac_hdl dac_hdl;
    extern struct audio_mixer mixer;

    static int pcm_fread(void *hdl, void *buf, u32 len)
{
    len = len / 2;
    memset(buf, 0, len);
    /* putchar('A'); */
    return len;
}
/*----------------------------------------------------------------------------*/
/**@brief    pcm解码数据输出
   @param    *entry: 音频流句柄
   @param    *in: 输入信息
   @param    *out: 输出信息
   @return   输出长度
   @note     *out未使用
*/
/*----------------------------------------------------------------------------*/
static int pcm_dec_data_handler(struct audio_stream_entry *entry,
                                struct audio_data_frame *in,
                                struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct pcm_decoder *pcm_dec = container_of(decoder, struct pcm_decoder, decoder);
    struct __mic_stream *dec = container_of(pcm_dec, struct __mic_stream, pcm_dec);
    audio_stream_run(&decoder->entry, in);
    /* audio_mixer_ch_pause(&dec->mix_ch, 0); */
    return decoder->process_len;
}
static void pcm_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
}
/*----------------------------------------------------------------------------*/
/**@brief    pcm解码数据流激活
   @param    *p: 私有句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void dec_out_stream_resume(void *p)
{
    struct __mic_stream *dec = (struct __mic_stream *)p;

    audio_decoder_resume(&dec->pcm_dec.decoder);
}
/*----------------------------------------------------------------------------*/
/**@brief    pcm解码启动
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int pcm_dec_start(struct __mic_stream *stream)
{
    int err = 0;
    if (stream == NULL) {
        return -EINVAL;
    }
    err = pcm_decoder_open(&stream->pcm_dec, &decode_task);
    if (err) {
        return err;
    }
    pcm_decoder_set_event_handler(&stream->pcm_dec, pcm_dec_event_handler, 0);
    pcm_decoder_set_read_data(&stream->pcm_dec, pcm_fread, stream);
    pcm_decoder_set_data_handler(&stream->pcm_dec, pcm_dec_data_handler);

    audio_mixer_ch_open(&stream->mix_ch, &mixer);
    /* audio_mixer_ch_open_head(&stream->mix_ch, &mixer); // 挂载到mixer最前面 */
    audio_mixer_ch_set_src(&stream->mix_ch, 0, 0);
    /* audio_mixer_ch_set_src(&stream->mix_ch, 1, 1); */
    /* audio_mixer_ch_set_no_wait(&stream->mix_ch, 1, 5); // 超时自动丢数 */
    /* audio_mixer_ch_pause(&stream->mix_ch, 1); */
// 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &stream->pcm_dec.decoder.entry;
    entries[entry_cnt++] = &stream->mix_ch.entry;
    stream->audio_stream = audio_stream_open(stream, dec_out_stream_resume);
    audio_stream_add_list(stream->audio_stream, entries, entry_cnt);

    /* audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC); */
    err = audio_decoder_start(&stream->pcm_dec.decoder);
    if (err == 0) {
        printf("pcm_dec_start ok\n");
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    pcm解码停止
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void pcm_dec_stop(struct __mic_stream *stream)
{
    printf("mic stream dec stop \n\n");
    if (stream) {
        /* audio_decoder_close(&stream->decoder); */
        /* audio_mixer_ch_close(&stream->mix_ch);	 */
        pcm_decoder_close(&stream->pcm_dec);
        audio_mixer_ch_close(&stream->mix_ch);
        if (stream->audio_stream) {
            audio_stream_close(stream->audio_stream);
            stream->audio_stream = NULL;
        }
        /* audio_decoder_task_del_wait(&decode_task, &stream->wait); */
    }
}

static int pcmdec_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    struct __mic_stream *stream = container_of(wait, struct __mic_stream, wait);
    if (event == AUDIO_RES_GET) {
        err = pcm_dec_start(stream);
    } else if (event == AUDIO_RES_PUT) {
        /* pcm_dec_stop(stream); */
    }

    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    打开pcm解码
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int pcm_dec_open(struct __mic_stream *stream)
{
    stream->pcm_dec.ch_num = 2;
    stream->pcm_dec.output_ch_num = audio_output_channel_num();
    /* stream->pcm_dec.sample_rate = stream->parm->sample_rate; */
    stream->pcm_dec.sample_rate = TCFG_REVERB_SAMPLERATE_DEFUAL;

    stream->wait.priority = 0;
    stream->wait.preemption = 0;
    stream->wait.protect = 1;
    stream->wait.handler = pcmdec_wait_res_handler;
    return audio_decoder_task_add_wait(&decode_task, &stream->wait);
}

/*----------------------------------------------------------------------------*/
/**@brief    关闭pcm解码
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void pcm_dec_close(struct __mic_stream *stream)
{
    pcm_dec_stop(stream);
    audio_decoder_task_del_wait(&decode_task, &stream->wait);
}


/*----------------------------------------------------------------------------*/
/**@brief    唤醒mic数据处理任务
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_stream_adc_resume(void *priv)
{
    struct __mic_stream *stream = (struct __mic_stream *)priv;
    if (stream != NULL && (stream->release == 0)) {
        os_sem_set(&stream->sem, 0);
        os_sem_post(&stream->sem);
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    mic数据处理函数
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int mic_stream_effects_run(struct __mic_stream *stream)
{
    u32 tmp_len, wlen, data_len;
    s16 *read_buf = (s16 *)(stream->temp_buf + stream->parm->point_unit * 2 * 3);
    s16 *dual_buf = (s16 *)(stream->temp_buf + stream->parm->point_unit * 2 * 2);
    s16 *qual_buf = (s16 *)stream->temp_buf;

    u8 dac_chls = audio_dac_get_channel(&dac_hdl);
    if (cbuf_get_data_size(&stream->adc_cbuf) < stream->parm->point_unit * 2) {
        int res = os_sem_pend(&stream->sem, 0);
        if (res) {
            return -1;
        }
    }
    if (stream->release) {
        return -1;
    }

    wlen = cbuf_read(&stream->adc_cbuf, read_buf, stream->parm->point_unit * 2);
    if (wlen) {
        if (stream->out.func) {
            /* pcm_single_to_dual(dual_buf, read_buf, stream->parm->point_unit * 2); */
            /* wlen = stream->out.func(stream->out.priv, dual_buf, dual_buf, stream->parm->point_unit * 2 * 2, stream->parm->point_unit * 2 * 2); */
            wlen = stream->out.func(stream->out.priv, read_buf, dual_buf, stream->parm->point_unit * 2, stream->parm->point_unit * 2 * 2);
        } else {
            if (dac_chls == 1) {
                tmp_len = stream->parm->point_unit * 2;
                wlen = audio_dac_mix_write(&dac_hdl, read_buf, tmp_len);
            }
            if (dac_chls == 2) {
                pcm_single_to_dual(dual_buf, read_buf, stream->parm->point_unit * 2);
                tmp_len = stream->parm->point_unit * 2 * 2;
                wlen = audio_dac_mix_write(&dac_hdl, dual_buf, tmp_len);
            } else if (dac_chls == 4) {
                pcm_single_to_qual(qual_buf, read_buf, stream->parm->point_unit * 2);
                tmp_len = stream->parm->point_unit * 2 * 4;
                wlen = audio_dac_mix_write(&dac_hdl, qual_buf, tmp_len);
            }
            if (wlen < tmp_len) {
                putchar('D');
            }
        }
    } else {
        /* putchar('R'); */
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    mic数据处理任务
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_stream_task_deal(void *p)
{
    int res = 0;
    struct __mic_stream *stream = (struct __mic_stream *)p;
    stream->busy = 1;
    while (1) {
        res = mic_stream_effects_run(stream);
        if (res) {
            ///等待删除线程
            stream->busy = 0;
            while (1) {
                os_time_dly(10000);
            }
        }
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    创建mic数据流
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
struct __mic_stream *mic_stream_creat(struct __mic_stream_parm *parm)
{
    int err = 0;
    struct __mic_stream_parm *p = parm;
    if (parm == NULL) {
        printf("%s parm err\n", __FUNCTION__);
        return NULL;
    }
    printf("p->dac_delay = %d\n", p->dac_delay);
    printf("p->point_unit = %d\n", p->point_unit);
    printf("p->sample_rate = %d\n", p->sample_rate);

    u32 offset = 0;
    u32 buf_size = MIC_SIZEOF_ALIN(sizeof(struct __mic_stream), 4)
                   + MIC_SIZEOF_ALIN((p->point_unit * 4 * 2), 4)
                   + MIC_SIZEOF_ALIN((p->point_unit * 2 * 3), 4);

    u8 *buf = zalloc(buf_size);
    if (buf == NULL) {
        return NULL;
    }

    struct __mic_stream *stream = (struct __mic_stream *)buf;
    offset += MIC_SIZEOF_ALIN(sizeof(struct __mic_stream), 4);

    stream->temp_buf = (u8 *)buf + offset;
    offset += MIC_SIZEOF_ALIN((p->point_unit * 4 * 2), 4);

    stream->adc_buf = (u8 *)buf + offset;
    offset += MIC_SIZEOF_ALIN((p->point_unit * 2 * 3), 4);

    stream->parm = p;

    os_sem_create(&stream->sem, 0);
    cbuf_init(&stream->adc_cbuf, stream->adc_buf, MIC_SIZEOF_ALIN((p->point_unit * 2 * 3), 4));

    /* audio_dac_mix_ch_open(&dac_hdl, p->dac_delay);//初始化dac混合通道 */

    err = task_create(mic_stream_task_deal, (void *)stream, MIC_STREAM_TASK_NAME);
    if (err != OS_NO_ERR) {
        printf("%s creat fail %x\n", __FUNCTION__,  err);
        free(stream);
        return NULL;
    }
#if 10
    err = pcm_dec_open(stream);
    if (err) {
        mic_stream_destroy(&stream);
        return NULL;
    }
#endif

    printf("mic stream creat ok\n");
    return stream;
}
/*----------------------------------------------------------------------------*/
/**@brief    设置mic处理函数的回调处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_stream_set_output(struct __mic_stream  *stream, void *priv, u32(*func)(void *priv, void *in, void *out, u32 inlen, u32 outlen))
{
    if (stream) {
        stream->out.priv = priv;
        stream->out.func = func;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    mic中断数据输出回调函数
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void adc_output_to_buf(void *priv, s16 *data, int len)
{
    struct __mic_stream *stream = (struct __mic_stream *)priv;
    int wlen = 0;
    if (stream != NULL && (stream->release == 0)) {
        wlen = cbuf_write(&stream->adc_cbuf, data, len);
        os_sem_set(&stream->sem, 0);
        os_sem_post(&stream->sem);
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    打开mic
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
bool mic_stream_start(struct __mic_stream  *stream)
{
    if (stream) {
        u8 tp_mic_vol = user_ex_mic_get_vol();
        if(0xff!= tp_mic_vol){
            tp_mic_vol = USER_MIC_DEFAULT_GAIN;
        }
        
        if (audio_mic_open(&stream->mic_ch, stream->parm->sample_rate, tp_mic_vol) == 0) {
        // if (audio_mic_open(&stream->mic_ch, stream->parm->sample_rate, 2) == 0) {
            stream->adc_output.handler = adc_output_to_buf;
            stream->adc_output.priv = stream;
            audio_mic_add_output(&stream->adc_output);
            audio_mic_start(&stream->mic_ch);
            log_i("mic_stream_start ok 11\n");
            return true;
        }
    }
    return false;
}
/*----------------------------------------------------------------------------*/
/**@brief    关闭mic数据流
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_stream_destroy(struct __mic_stream **hdl)
{
    int err = 0;
    if ((hdl == NULL) || (*hdl == NULL)) {
        return ;
    }

    struct __mic_stream *stream = *hdl;
    stream->release = 1;
    audio_mic_close(&stream->mic_ch, &stream->adc_output);

    os_sem_set(&stream->sem, 0);
    os_sem_post(&stream->sem);

    while (stream->busy) {
        os_time_dly(5);
    }

    printf("%s wait busy ok!!!\n", __FUNCTION__);

    err = task_kill(MIC_STREAM_TASK_NAME);
    os_sem_del(&stream->sem, 0);

    pcm_dec_close(stream);
    /* audio_dac_mix_ch_close(&dac_hdl); */

    local_irq_disable();
    free(*hdl);
    *hdl = NULL;
    /* p_stream = NULL; */
    local_irq_enable();
}

#endif//TCFG_MIC_EFFECT_ENABLE


