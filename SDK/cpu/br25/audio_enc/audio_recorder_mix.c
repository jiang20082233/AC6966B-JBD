#include "audio_recorder_mix.h"
#include "audio_enc.h"
#include "audio_dec.h"
#include "media/pcm_decoder.h"
#include "btstack/btstack_task.h"
#include "btstack/avctp_user.h"
#include "audio_splicing.h"
#include "app_main.h"

#if (RECORDER_MIX_EN)


#define RECORDER_MIX_ZERO_PCM_EN			1


struct __recorder_mix {
    struct audio_stream 			*audio_stream;		// 音频流
    struct audio_decoder 			decoder;
    struct audio_res_wait 			wait;
    struct pcm_decoder 				pcm_dec;		// pcm解码句柄
    struct audio_mixer_ch 			mix_ch;
    struct audio_mixer_ch 			rec_mix_ch;
    cbuffer_t 						*sco_cbuf;
    u8								phone_active;
    u16								timer;
};

#define RECORDER_MIX_MAX_DATA_ONCE			 (512)//不建议修改
#define RECORDER_MIX_SCO_MAX_DATA_ONCE	 	 (240)//不建议修改
#define RECORDER_MIX_SCO_BUF_SIZE			 (1024)//不建议修改
#define	RECORDER_MIX_DEFAULT_SR				 (32000L)//不建议大于32000
#define	RECORDER_MIX_DEFAULT_SR_LIMIT(sr)	 ((sr<16000)?16000:sr)

struct __recorder_mix recorder_hdl_overlay sec(.enc_file_mem);
struct __recorder_mix *recorder_hdl = NULL;
#define __this	recorder_hdl


/* extern struct audio_mixer recorder_mixer; */
/* extern struct audio_mixer mixer; */

static struct recorder_mix_stream *rec_mix = NULL;
static volatile u8 recorder_overlay = 0;
static s16 read_buf[RECORDER_MIX_MAX_DATA_ONCE / 2] sec(.enc_file_mem);
static u8 sco_buf[RECORDER_MIX_SCO_BUF_SIZE] sec(.enc_file_mem);
static cbuffer_t sco_cbuf sec(.enc_file_mem);

void recorder_mix_call_status_change(u8 active)
{
    printf("[%s] = %d\n", __FUNCTION__, active);
    if (active) {
        if (__this && (__this->phone_active == 0)) {
            //如果启动录音的时候不是通话状态， 先停止之前的录音, 通话录音需要手动重新开
            printf("phone rec unactive !!, stop cur rec first\n");
            recorder_mix_stop();
        }
    } else {
        //
        /* recorder_mix_stop(); */
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    通话音频数据混合写接口
   @param
   		data:输入数据地址
		len:输入数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u32 recorder_mix_sco_data_write(u8 *data, u16 len)
{
    //putchar('.');
    u32 wlen = 0;
    if (__this) {
        if (__this->sco_cbuf) {
            u32 wlen = cbuf_write(__this->sco_cbuf, data, len);
            if (!wlen) {
                //putchar('s');
            }
        }
    }
    return len;
}

static int recorder_mix_sco(s16 *data, u16 len, u8 channel)
{
    s32 temp;
    u16 rlen = len;
    if (channel > 2) {
        printf("[%s]not support!!", __FUNCTION__);
    } else if (channel == 2) {
        rlen = len >> 1;
        pcm_dual_to_single(data, data, len);
    }

    if (cbuf_read(__this->sco_cbuf, read_buf, rlen)) {
        for (int i = 0; i < rlen / 2; i++) {
            temp = 	data[i];
            temp += read_buf[i];
            if (temp < -32768) {
                temp = -32768;
            } else if (temp > 32767) {
                temp = 32767;
            }
            data[i] = temp;
        }
    }
    return recorder_userdata_to_enc((s16 *)data, rlen);
}


static int pcm_fread(void *hdl, void *buf, u32 len)
{
    len = len / 2;
    memset(buf, 0, len);
    /* putchar('A'); */
    return len;
}
static int pcm_dec_data_handler(struct audio_stream_entry *entry,
                                struct audio_data_frame *in,
                                struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct pcm_decoder *pcm_dec = container_of(decoder, struct pcm_decoder, decoder);
    audio_stream_run(&decoder->entry, in);
    return decoder->process_len;
}
static void pcm_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
}
static void dec_out_stream_resume(void *p)
{
    struct __recorder_mix *recorder = (struct __recorder_mix *)p;
    audio_decoder_resume(&recorder->pcm_dec.decoder);
}
static int pcm_dec_start(struct __recorder_mix *recorder)
{
    int err = 0;
    if (recorder == NULL) {
        return -EINVAL;
    }
    err = pcm_decoder_open(&recorder->pcm_dec, &decode_task);
    if (err) {
        return err;
    }
    pcm_decoder_set_event_handler(&recorder->pcm_dec, pcm_dec_event_handler, 0);
    pcm_decoder_set_read_data(&recorder->pcm_dec, (void *)pcm_fread, recorder);
    pcm_decoder_set_data_handler(&recorder->pcm_dec, pcm_dec_data_handler);

    audio_mixer_ch_open(&recorder->mix_ch, &mixer);
    audio_mixer_ch_set_src(&recorder->mix_ch, 1, 0);

    audio_mixer_ch_open(&recorder->rec_mix_ch, &recorder_mixer);
    audio_mixer_ch_set_src(&recorder->rec_mix_ch, 1, 0);

// 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &recorder->pcm_dec.decoder.entry;
    entries[entry_cnt++] = &recorder->mix_ch.entry;
    recorder->audio_stream = audio_stream_open(recorder, dec_out_stream_resume);
    audio_stream_add_list(recorder->audio_stream, entries, entry_cnt);

    audio_stream_add_entry(entries[entry_cnt - 2], &recorder->rec_mix_ch.entry);

    err = audio_decoder_start(&recorder->pcm_dec.decoder);
    if (err == 0) {
        printf("pcm_dec_start ok\n");
    }
    return err;
}
static void pcm_dec_stop(struct __recorder_mix *recorder)
{
    printf("recorder mix dec stop \n\n");
    if (recorder) {
        /* audio_decoder_close(&recorder->decoder); */
        /* audio_mixer_ch_close(&recorder->mix_ch);	 */
        pcm_decoder_close(&recorder->pcm_dec);
        audio_mixer_ch_close(&recorder->mix_ch);
        audio_mixer_ch_close(&recorder->rec_mix_ch);
        if (recorder->audio_stream) {
            audio_stream_close(recorder->audio_stream);
            recorder->audio_stream = NULL;
        }
        /* audio_decoder_task_del_wait(&decode_task, &recorder->wait); */
    }
}
static int pcmdec_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    struct __recorder_mix *recorder = container_of(wait, struct __recorder_mix, wait);
    if (event == AUDIO_RES_GET) {
        err = pcm_dec_start(recorder);
    } else if (event == AUDIO_RES_PUT) {
        /* pcm_dec_stop(recorder); */
    }
    return err;
}

static int pcm_dec_open(struct __recorder_mix *recorder)
{
    recorder->pcm_dec.ch_num = 2;
    recorder->pcm_dec.output_ch_num = audio_output_channel_num();
    u16 sr = audio_mixer_get_cur_sample_rate(&mixer);
    recorder->pcm_dec.sample_rate = RECORDER_MIX_DEFAULT_SR_LIMIT(sr);

    recorder->wait.priority = 0;
    recorder->wait.preemption = 0;
    recorder->wait.protect = 1;
    recorder->wait.handler = pcmdec_wait_res_handler;
    printf("[%s], recorder->pcm_dec.sample_rate:%d\n", __FUNCTION__, recorder->pcm_dec.sample_rate);
    return audio_decoder_task_add_wait(&decode_task, &recorder->wait);
}
static void pcm_dec_close(struct __recorder_mix *recorder)
{
    pcm_dec_stop(recorder);
    audio_decoder_task_del_wait(&decode_task, &recorder->wait);
}


static int recorder_mix_stream_data_handler(struct audio_stream_entry *entry,
        struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    struct recorder_mix_stream *hdl = container_of(entry, struct recorder_mix_stream, entry);
    if (in->data_len == 0) {
        /* printf("data = 0\n"); */
        return 0;
    }
    /* putchar('$'); */

    s16 *data = in->data;//(s16 *)(in->data + in->offset / 2);
    u16 data_len = in->data_len;
    int wlen;
    if (__this && __this->sco_cbuf) {
        if (data_len >= RECORDER_MIX_SCO_MAX_DATA_ONCE) {
            data_len = RECORDER_MIX_SCO_MAX_DATA_ONCE;
        }
        wlen = recorder_mix_sco(data, data_len, in->channel);
    } else {
        if (data_len >= RECORDER_MIX_MAX_DATA_ONCE) {
            data_len = RECORDER_MIX_MAX_DATA_ONCE;
        }
        wlen = recorder_userdata_to_enc(data, data_len);
    }
    if (wlen == 0) {
        //数据无法输出直接丢掉， 保证数据流能够顺利激活
        return in->data_len;
    }
    return data_len;
}

static void recorder_mix_stream_output_data_process_len(struct audio_stream_entry *entry,  int len)
{
}

static struct recorder_mix_stream *recorder_mix_stream_open(u8 source)
{
    struct recorder_mix_stream *hdl;
    hdl = zalloc(sizeof(struct recorder_mix_stream));
    hdl->source = source;
    hdl->entry.data_process_len = recorder_mix_stream_output_data_process_len;
    hdl->entry.data_handler = recorder_mix_stream_data_handler;

    return hdl;
}

static void recorder_mix_stream_close(struct recorder_mix_stream *hdl)
{
}

//*----------------------------------------------------------------------------*/
/**@brief    录音数据流节点激活接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void recorder_mix_stream_resume(void)
{
    if (rec_mix)	{
        audio_stream_resume(&rec_mix->entry);
    }
}

static void audio_last_out_stream_resume(void *p)
{
}

static u16 recorder_mix_audio_mixer_check_sr(struct audio_mixer *mixer, u16 sr)
{
    u16 o_sr = audio_output_rate(sr);
    o_sr = RECORDER_MIX_DEFAULT_SR_LIMIT(sr);
    ///对于一些特殊采样率的音频， 如FM的37500, 默认变采样为RECORDER_MIX_DEFAULT_SR
    if (o_sr == 37500) {
        o_sr = RECORDER_MIX_DEFAULT_SR;
    }
    printf("o_sr ============================= %d\n", o_sr);
    return o_sr;
}

//*----------------------------------------------------------------------------*/
/**@brief    混合录音mix节点初始化
   @param
   			mix:通道混合控制句柄
			mix_buf:通道混合后输出的缓存
			buf_size:通道混合输出缓存大小
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void recorder_mix_init(struct audio_mixer *mix, s16 *mix_buf, u16 buf_size)
{
    if (mix == NULL) {
        return ;
    }
    audio_mixer_open(mix);
    audio_mixer_set_event_handler(mix, NULL);
    audio_mixer_set_check_sr_handler(mix, recorder_mix_audio_mixer_check_sr);
    /*初始化mix_buf的长度*/
    audio_mixer_set_output_buf(mix, mix_buf, buf_size);
    u8 ch_num = audio_output_channel_num();
    audio_mixer_set_channel_num(mix, ch_num);
    /* u16 sr = audio_output_nor_rate(); */
    /* if (sr) { */
    // 固定采样率输出
    /* audio_mixer_set_sample_rate(mix, MIXER_SR_SPEC, 16000L); */
    /* } */

    rec_mix = recorder_mix_stream_open(ENCODE_SOURCE_MIX);

    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;

    entries[entry_cnt++] = &mix->entry;
    entries[entry_cnt++] = &rec_mix->entry;

    mix->stream = audio_stream_open(NULL, audio_last_out_stream_resume);
    audio_stream_add_list(mix->stream, entries, entry_cnt);

}

static void recorder_mix_err_callback(void)
{
    recorder_mix_stop();
}

static void recorder_mix_destroy(struct __recorder_mix **hdl)
{
    if (hdl == NULL || (*hdl == NULL)) {
        return ;
    }
    struct __recorder_mix *recorder = *hdl;

#if (RECORDER_MIX_ZERO_PCM_EN)
    pcm_dec_close(recorder);
#endif/*RECORDER_MIX_ZERO_PCM_EN*/

    if (recorder_overlay) {
        printf("[%s] overlay, noneed free!!\n", __FUNCTION__);
        *hdl = NULL;
    } else {
        printf("[%s] malloc, need free!!\n", __FUNCTION__);
        local_irq_disable();
        free(*hdl);
        *hdl = NULL;
        local_irq_enable();
    }
}

static struct __recorder_mix *recorder_mix_creat(void)
{
    u8 phone_active = 0;
    u8 call_status = get_call_status();
    if ((call_status != BT_CALL_HANGUP) && (call_status == BT_CALL_INCOMING)) {
        printf("phone incomming, no active, no record!!!!\n");
        return NULL;
    }

    if (app_var.siri_stu) {
        printf("siri active, no record!!!\n");
        return NULL;
    }

    if (call_status != BT_CALL_HANGUP) {
        phone_active = 1;
    }

    struct __recorder_mix *recorder = NULL;
    if (phone_active) {
        printf("[%s] phone active,use overlay!!\n", __FUNCTION__);
        recorder_overlay = 1;
        recorder = &recorder_hdl_overlay;
        memset(recorder, 0, sizeof(struct __recorder_mix));
    } else {
        printf("[%s] no phone active,use malloc!!\n", __FUNCTION__);
        recorder_overlay = 0;
        recorder = (struct __recorder_mix *)zalloc(sizeof(struct __recorder_mix));
        if (recorder == NULL) {
            printf("[%s] nomem!!\n", __FUNCTION__);
            return NULL;
        }
    }

    if (phone_active) {
        recorder->sco_cbuf = &sco_cbuf;
        cbuf_init(recorder->sco_cbuf, sco_buf, RECORDER_MIX_SCO_BUF_SIZE);
    } else {
        recorder->sco_cbuf = NULL;
    }

    recorder->phone_active = phone_active;
    return recorder;
}

static int __recorder_mix_start(struct __recorder_mix *recorder)
{
    struct record_file_fmt fmt = {0};
    /* char logo[] = {"sd0"}; */		//可以指定设备
    char folder[] = {REC_FOLDER_NAME};         //录音文件夹名称
    char filename[] = {"AC69****"};     //录音文件名，不需要加后缀，录音接口会根据编码格式添加后缀

#if (TCFG_NOR_REC)
    char logo[] = {"rec_nor"};		//外挂flash录音
#elif (FLASH_INSIDE_REC_ENABLE)
    char logo[] = {"rec_sdfile"};		//内置flash录音
#else
    char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));//普通设备录音，获取最后活动设备
#endif

    fmt.dev = logo;
    fmt.folder = folder;
    fmt.filename = filename;

    //编码格式：AUDIO_CODING_WAV, AUDIO_CODING_MP3
    if (recorder->phone_active) {
        ///通话需要固定采样率、编码格式、通道数
        fmt.coding_type = AUDIO_CODING_WAV;
        fmt.channel = 1;

    } else {
        fmt.coding_type = AUDIO_CODING_MP3;
        fmt.channel = audio_output_channel_num();//跟mix通道数一致
        if (fmt.channel > 2) {
            ASSERT(0, "[%s]no support!!", __FUNCTION__);
        }
    }
    u16 sr = audio_mixer_get_cur_sample_rate(&recorder_mixer);
    fmt.sample_rate = RECORDER_MIX_DEFAULT_SR_LIMIT(sr);
    printf("[%s], fmt.sample_rate = %d\n", __FUNCTION__, fmt.sample_rate);
    fmt.cut_head_time = 300;            //录音文件去头时间,单位ms
    fmt.cut_tail_time = 300;            //录音文件去尾时间,单位ms
    fmt.limit_size = 3000;              //录音文件大小最小限制， 单位byte
    fmt.source = ENCODE_SOURCE_MIX;     //录音输入源
    fmt.err_callback = recorder_mix_err_callback;

    /* audio_mixer_set_sample_rate(&recorder_mixer, MIXER_SR_SPEC, fmt.sample_rate); */

    int ret = recorder_encode_start(&fmt);
    if (ret) {
        log_e("[%s] fail !!, dev = %s\n", __FUNCTION__, logo);
    } else {
        log_e("[%s] succ !!, dev = %s\n", __FUNCTION__, logo);
    }
    return ret;
}

static void __recorder_mix_stop(struct __recorder_mix *recorder)
{
    recorder_encode_stop();
}

static void __recorder_mix_timer(void *priv)
{
    u32 sec = recorder_get_encoding_time();
    printf("%d\n", sec);
}

//*----------------------------------------------------------------------------*/
/**@brief    混合录音开始
   @param
   @return   0成功， 非0失败
   @note
   			混合录音支持录制内容：
				BT sbc（蓝牙高级音频）
				BT sco（蓝牙通话，通话接通后才可以启动）
				FM（内置FM）
				Linein(外部音源输入,单声道, 696系列需要外部硬件合并左右声道)
			录音参数配置：
				请在__recorder_mix_start函数内部修改参数
				1、支持设备选择, 如：sd0、udisk0等
				2、修改文件名称及文件夹名称, 默认文件夹名称为JL_REC，文件名AC69****
				3、编码格式(资源受限，通话支持adpcm wav)
				4、支持砍头砍尾处理
			注意：
				1、通话录音录制的是adpcm wav格式，其他音源录音为mp3
				2、通话录音不支持录制来电铃声
				3、普通录制过程， 来电或去电会先停止原来的录音,通话录音需要重新启动
				4、通话结束，如是通话录音， 录音会主动停止
*/
/*----------------------------------------------------------------------------*/
int recorder_mix_start(void)
{
    if (__this) {
        recorder_mix_stop();
    }
    struct __recorder_mix *recorder = recorder_mix_creat();
    if (recorder == NULL) {
        return -1;
    }

#if (RECORDER_MIX_ZERO_PCM_EN)
    //启动时间轴解码(填充0)， 用于确保混合录音始终有数据写入编码器
    int err = pcm_dec_open(recorder);
    if (err) {
        recorder_mix_destroy(&recorder);
        return -1;
    }
#endif/*RECORDER_MIX_ZERO_PCM_EN*/

    int ret = __recorder_mix_start(recorder);
    if (ret) {
        recorder_mix_destroy(&recorder);
        return -1;
    }

    local_irq_disable();
    __this = recorder;
    local_irq_enable();

    __this->timer = sys_timer_add((void *)__this, __recorder_mix_timer, 1000);

    printf("[%s] start ok\n", __FUNCTION__);

    return 0;
}

//*----------------------------------------------------------------------------*/
/**@brief    混合录音停止
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void recorder_mix_stop(void)
{
    if (__this && __this->timer) {
        sys_timer_del(__this->timer);
    }
    __recorder_mix_stop(__this);
    recorder_mix_destroy(&__this);
    printf("[%s] stop ok\n", __FUNCTION__);
}
//*----------------------------------------------------------------------------*/
/**@brief    获取混合录音状态
   @param
   @return
  			1:正在录音状态
			0:录音停止状态
   @note
*/
/*----------------------------------------------------------------------------*/
int recorder_mix_get_status(void)
{
    return (__this ? 1 : 0);
}


#endif//RECORDER_MIX_EN


