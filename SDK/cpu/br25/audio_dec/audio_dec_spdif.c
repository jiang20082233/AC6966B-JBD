/*
 ****************************************************************
 *File : audio_spdif.c
 *Note :
 *
 ****************************************************************
***********************   spdif_dec ******************************/
#include "asm/includes.h"
#include "media/includes.h"
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

#if ((defined TCFG_SPDIF_ENABLE) && (TCFG_SPDIF_ENABLE))

#define SPDIF_EQ_SUPPORT_ASYNC		1

#ifndef CONFIG_EQ_SUPPORT_ASYNC
#undef SPDIF_EQ_SUPPORT_ASYNC
#define SPDIF_EQ_SUPPORT_ASYNC		0
#endif


#if SPDIF_EQ_SUPPORT_ASYNC && TCFG_SPDIF_MODE_EQ_ENABLE
#define SPDIF_EQ_SUPPORT_32BIT		1
#else
#define SPDIF_EQ_SUPPORT_32BIT		0
#endif

#define SPDIF_DEC_DATA_BUF_LEN 1024
struct s_spdif_decode {
    struct audio_decoder decoder;
    struct audio_res_wait wait;
    struct audio_mixer_ch mix_ch;
    u32 coding_type;
    struct audio_fmt fmt;
    cbuffer_t dec_cbuf;
    u8 dec_data_buf[SPDIF_DEC_DATA_BUF_LEN];
    struct audio_src_handle *hw_src;
    u8 status;
    OS_SEM sem;
    u32 clk_sys;
    u8 remain;
    u8 ch_num;


    struct audio_eq_drc *eq_drc;
};


void spdif_eq_drc_open(struct s_spdif_decode *dec, struct audio_fmt *fmt);
void spdif_eq_drc_close(struct s_spdif_decode *dec);

struct audio_spdif_hdl spdif_slave_hdl;
struct s_spdif_decode *spdif_dec_hdl = NULL;
int spdif_dec_open(struct audio_fmt fmt);
void spdif_dec_close(void);
int spdif_non_linear_packet(u16 *pkt, u16 pkt_len)
{
    enum IEC61937DataType data_type;
    u16 search_index = 0;
    int data_cp_begin_idx = 0;
    int data_cp_len = 0;
    u16 *data = pkt;
    u16 *data_ptr = NULL;
    int zero_cnt = 0;
    u32 state = 0;
    int pkt_size_bits, offset, skip_len;
    if (spdif_slave_hdl.length_code > 0) {
        if (spdif_slave_hdl.length_code >= SPDIF_DATA_DAM_LEN * 2) {
            data_cp_begin_idx = 0;
            data_cp_len       = SPDIF_DATA_DAM_LEN * 2;
            spdif_slave_hdl.length_code       -= data_cp_len;
        } else {
            data_cp_begin_idx    = 0;
            data_cp_len = spdif_slave_hdl.length_code;
            spdif_slave_hdl.length_code = 0;
        }
        //写入解码cbuf
        /* int spdif_dec_write_data(s16 *data, int len) */
        spdif_dec_write_data(&data[data_cp_begin_idx], data_cp_len * 2);
        if (spdif_slave_hdl.length_code != 0) {
            return 0;
        } else {
            search_index = data_cp_len;
        }
    }
    if (spdif_slave_hdl.find_sync_word != 0) {
        printf("[last:%d,%x]", search_index, data[search_index]);
        if (data[search_index] == SYNCWORD2) {
            puts("3");
        } else {
            puts("M");
            spdif_slave_hdl.find_sync_word = 0;
            return -1;
        }
    } else {
        while (search_index < pkt_len) {
            if (data[search_index] == SYNCWORD1) {
                spdif_slave_hdl.find_sync_word = 1;
            }
            if (++search_index >= pkt_len) {
                return -1;
            }
            if (spdif_slave_hdl.find_sync_word) {
                if ((data[search_index] == SYNCWORD2) && (data[search_index - 1] == SYNCWORD1)) {
                    break;
                } else {
                    spdif_slave_hdl.find_sync_word = 0;
                }
            }
        }
    }
    spdif_slave_hdl.find_sync_word = 0;

    data_type = data[++search_index];
    spdif_slave_hdl.decoder_type = data_type & 0xff;
    pkt_size_bits = data[++search_index];
    spdif_slave_hdl.length_code = (pkt_size_bits >> 3) >> 1; //16bit
    if (pkt_size_bits % 16) {
        printf("packet not endinf at a 16-bit boundary %d-%d \n", pkt_size_bits, search_index);
        for (int i = 0; i < pkt_len; i++) {
            printf("%x ", data[i]);
        }
    }
    ++search_index;
    data_cp_len    = pkt_len - search_index;
    skip_len = 2048 - (spdif_slave_hdl.length_code << 1) - BURST_HEADER_SZIE;
    if (data_cp_len) {
        spdif_dec_write_data(&data[search_index], data_cp_len * 2);
        data_cp_len = 0;
    }
    return 0;
}
#define SAMPLE_RATE_TABLE_SIZE 14
static u8 old_samplerate_index = 6;
u16 spdif_sample_rate_table[SAMPLE_RATE_TABLE_SIZE] = {
    80,
    110,
    160,
    221,
    240,
    320,
    441,
    480,
    504,
    640,
    882,
    960,
    1764,
    1920,
};

static u32 cal_samplerate(void)
{
#if 10
    static u32 pre_sr_cnt = 0;
    static u32 sr_cnt = 0;
    pre_sr_cnt = JL_SS->SR_CNT;
    u8  search_index = 0, i = 0;
    u32 tick_result = 0 ;
    /* printf("sr cnt %8x;%8x \n",sr_cnt,pre_sr_cnt); */
    sr_cnt = pre_sr_cnt >> 3;
    /* printf("sr cnt %x \n",pre_sr_cnt); */
    tick_result = sr_cnt / 100;
    for (i = 0; i < SAMPLE_RATE_TABLE_SIZE; i++) {
        if ((tick_result > spdif_sample_rate_table[i] - 20) && tick_result < spdif_sample_rate_table[i] + 20) {
            search_index = i;
        }
    }
    if ((search_index != 0) && (old_samplerate_index != search_index)) {
        /* printf("SET %d %d ", spdif_sample_rate_table[search_index], tick_result); */
        audio_hw_src_set_rate(spdif_dec_hdl->hw_src, spdif_sample_rate_table[search_index], 48000);
        old_samplerate_index = search_index;
    }

#endif
    return 0;
}
___interrupt
static void spdif_isr_handler()
{
    int index = 0;
    s32 *ptr;
    u8 *info_ptr = NULL;
    s16 data_ptr[SPDIF_DATA_DAM_LEN * SPDIF_CHANNEL_NUMBER];
    u16 *u_data_ptr = (u16 *)data_ptr;
    /* putchar('A'); */
    if (I_PND) { //information pending
        I_PND_CLR;
        index = !(USING_INF_BUF_INDEX >> 13);
        info_ptr = (u8 *) &spdif_slave_hdl.p_info_buf[index][0];
        for (int i = 0; i < SPDIF_INFO_LEN; i++) {
            if (!(info_ptr[0]&INFO_VALIDITY_BIT)) {
                break;
            }
            spdif_slave_hdl.validity_bit_flag = 1;
        }
    }

    if (ERROR_VALUE) { // some error flag
        ERR_CLR;
        putchar('E');
        if (ERROR_VALUE & (BIT(8) | BIT(9))) {
            memset(&spdif_slave_hdl.p_spdif_data_buf[0][0], 0x0, SPDIF_DATA_DAM_LEN * SPDIF_CHANNEL_NUMBER);
            memset(&spdif_slave_hdl.p_spdif_data_buf[1][0], 0x0, SPDIF_DATA_DAM_LEN * SPDIF_CHANNEL_NUMBER);
        }
        if (JL_SS->CON & BIT(11)) {
            spdif_slave_hdl.error_flag = 1;
            //模块复位
        }

    }
    if (D_PND) { //data pending
        D_PND_CLR;
        if (spdif_slave_hdl.validity_bit_flag) { //DST
            spdif_slave_hdl.validity_bit_flag = 0;
            index = !(USING_BUF_INDEX >> 12);
            ptr = (s32 *)&spdif_slave_hdl.p_spdif_data_buf[index][0];
            for (int i = 0; i < SPDIF_DATA_DAM_LEN * 2; i += 2) {
                u_data_ptr[i]   = (u16)(ptr[i] >> 8);
                u_data_ptr[i + 1] = (u16)(ptr[i + 1] >> 8);
            }
            if (!spdif_dec_hdl) {
                struct audio_fmt fmt;
                fmt.channel = 2;
                fmt.coding_type = 	AUDIO_CODING_DTS;
                /* fmt.sample_rate = 16000;		 */
                fmt.sample_rate = 44100;
                spdif_dec_open(fmt);
            } else if (spdif_dec_hdl->fmt.coding_type != AUDIO_CODING_DTS) {
                spdif_dec_close();
            }
            if (spdif_dec_hdl && (spdif_slave_hdl.status == SPDIF_STATE_START)) {
                spdif_non_linear_packet(u_data_ptr, SPDIF_DATA_DAM_LEN * 2);
            }

            /* putchar('D'); */
            return;
        } else { //PCM
            if (!spdif_dec_hdl) {
                putchar('p');
                struct audio_fmt fmt;
                fmt.channel = 2;
                fmt.coding_type = 	AUDIO_CODING_PCM;
                fmt.sample_rate = 44100;
                /* fmt.sample_rate = 96000;						 */
                spdif_dec_open(fmt);
                return;
            } else if (spdif_dec_hdl->fmt.coding_type != AUDIO_CODING_PCM) {
                putchar('p');
                spdif_dec_close();
                return;
            }

            /* putchar('D'); */
            cal_samplerate();
            index = !(USING_BUF_INDEX >> 12);
            ptr = (s32 *) &spdif_slave_hdl.p_spdif_data_buf[index][0];
            for (int i = 0; i < SPDIF_DATA_DAM_LEN * 2; i += 2) {
                data_ptr[i]   = (s16)(ptr[i] >> 8);
                data_ptr[i + 1] = (s16)(ptr[i + 1] >> 8);
            }
            if ((spdif_dec_hdl) && (spdif_slave_hdl.status == SPDIF_STATE_START)) {
                int wlen = spdif_dec_write_data(data_ptr, SPDIF_DATA_DAM_LEN * SPDIF_CHANNEL_NUMBER * 2);
                if (wlen == 0) {
                    printf(" wlen = 0");
                }
            }
        }
    }
    if (CSB_PND) {
        CSB_PND_CLR;
    }
}

// 写入spdif数据
int spdif_dec_write_data(s16 *data, int len)
{
    if (!spdif_dec_hdl) {
        return 0;
    }
    u16 wlen = cbuf_write(&spdif_dec_hdl->dec_cbuf, data, len);
    if (!wlen) {
        /* putchar(','); */
    }

    os_sem_post(&spdif_dec_hdl->sem);
    /* printf("wl:%d ", wlen); */
    return wlen;
}

/* void spdif_dec_close(void); */
static void spdif_dec_release(void)
{
    audio_decoder_task_del_wait(&decode_task, &spdif_dec_hdl->wait);
    if (spdif_dec_hdl) {
        clock_remove(SPDIF_CLK);
        free(spdif_dec_hdl);
        spdif_dec_hdl = NULL;
    }
}
static void spdif_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        printf("\n--func=%s\n", __FUNCTION__);
        spdif_dec_close();
        break;
    }
}

static int spdif_src_output_handler(void *priv, void *buf, int len)
{
    int wlen = 0;
    int rlen = len;
    struct s_spdif_decode *dec = (struct s_spdif_decode *)priv;
    s16 *data = (s16 *)buf;

    wlen = audio_mixer_ch_write(&dec->mix_ch, data, len);
    return wlen;

}
static int spdif_dec_output_handler(struct audio_decoder *decoder, s16 *data, int len, void *priv)
{
    int wlen = 0;
    int rlen = len;

    struct s_spdif_decode *dec = container_of(decoder, struct s_spdif_decode, decoder);

    if (!dec->remain) {
#if DEC_OUT_OTHER_DATA)
        other_audio_dec_output(decoder, data, len, dec->fmt.channel, dec->fmt.sample_rate);
#endif

    }

    if (dec->hw_src) {
        wlen = audio_src_resample_write(dec->hw_src, data, len);
    } else {
        wlen = audio_mixer_ch_write(&dec->mix_ch, data, len);
    }
    if (wlen == len) {
        dec->remain = 0;
    } else {
        dec->remain = 1;
    }
    return wlen;
}

static const struct audio_dec_handler spdif_dec_handler = {
    /* .dec_probe = spdif_dec_probe_handler,		 */
    .dec_output  = spdif_dec_output_handler,
    /* .dec_post    = spdif_dec_post_handler,	 */
};

static int spdif_dec_read(struct audio_decoder *decoder, void *buf, u32 len)
{
    int rlen = 0;
    os_sem_set(&spdif_dec_hdl->sem, 0);
    while (!rlen) {

        rlen = cbuf_read(&spdif_dec_hdl->dec_cbuf, buf, len);
        /* printf("rlen:%d\n",rlen); */
        if (rlen == 0) {
            if (spdif_dec_hdl->status == SPDIF_STATE_STOP) {
                memset(buf, 0, len);
                rlen = len;
            } else {
                os_sem_pend(&spdif_dec_hdl->sem, 1);
            }

        }
    }
    return rlen;
}


static const struct audio_dec_input spdif_dec_input_pcm = {
    .coding_type = AUDIO_CODING_PCM,
    .data_type   = AUDIO_INPUT_FILE,
    .ops = {
        .file = {
            .fread = spdif_dec_read,
        }
    }
};
static const struct audio_dec_input spdif_dec_input_dts = {
    .coding_type = AUDIO_CODING_DTS,
    .data_type   = AUDIO_INPUT_FILE,
    .ops = {
        .file = {
            .fread = spdif_dec_read,
        }
    }
};


int spdif_dec_start(void)
{
    int err = 0;
    struct audio_fmt f;
    if (!spdif_dec_hdl) {
        return -EINVAL;
    }
    if (spdif_dec_hdl->fmt.coding_type == AUDIO_CODING_DTS) {
        puts("dts \n");
        err = audio_decoder_open(&spdif_dec_hdl->decoder, &spdif_dec_input_dts, &decode_task);
    } else {
        puts("pcm \n");
        err = audio_decoder_open(&spdif_dec_hdl->decoder, &spdif_dec_input_pcm, &decode_task);
    }
    printf("\n--func=%s\n", __FUNCTION__);
    if (err) {
        goto __err1;
    }
    audio_decoder_set_handler(&spdif_dec_hdl->decoder, &spdif_dec_handler);
    audio_decoder_set_event_handler(&spdif_dec_hdl->decoder, spdif_dec_event_handler, 0);

    f.coding_type = spdif_dec_hdl->fmt.coding_type;
    f.sample_rate = spdif_dec_hdl->fmt.sample_rate ;
    f.channel     = spdif_dec_hdl->fmt.channel;
    err = audio_decoder_set_fmt(&spdif_dec_hdl->decoder, &f);
    if (err) {
        goto __err2;
    }

    spdif_dec_hdl->ch_num = f.channel;
    audio_mixer_ch_open_head(&spdif_dec_hdl->mix_ch, &mixer);
    printf("mixer sample_rate :%d", audio_mixer_get_sample_rate(&mixer));
#if 10
    if (f.sample_rate != audio_mixer_get_sample_rate(&mixer)) {
        spdif_dec_hdl->hw_src = zalloc(sizeof(struct audio_src_handle));
        if (spdif_dec_hdl->hw_src) {
            u8 ch_num = spdif_dec_hdl->ch_num;
            audio_hw_src_open(spdif_dec_hdl->hw_src, ch_num, SRC_TYPE_RESAMPLE);
            /* audio_hw_src_open(spdif_dec_hdl->hw_src,f.channel,SRC_TYPE_RESAMPLE); */
            audio_hw_src_set_rate(spdif_dec_hdl->hw_src, f.sample_rate, 48000);
            /* audio_hw_src_set_rate(spdif_dec_hdl->hw_src,f.sample_rate,audio_mixer_get_sample_rate(&mixer)); */
            audio_src_set_output_handler(spdif_dec_hdl->hw_src, spdif_dec_hdl, spdif_src_output_handler);
        }
    } else {


    }
#endif
    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);
    audio_mixer_ch_set_sample_rate(&spdif_dec_hdl->mix_ch, audio_output_rate(f.sample_rate));
    spdif_eq_drc_open(spdif_dec_hdl, &f);


    spdif_dec_hdl->status = SPDIF_STATE_START;
    err = audio_decoder_start(&spdif_dec_hdl->decoder);
    if (err) {
        goto __err3;
    }
    clock_set_cur();
    return 0;
__err3:
    spdif_dec_hdl->status = 0;
    audio_mixer_ch_close(&spdif_dec_hdl->mix_ch);
    if (spdif_dec_hdl->hw_src) {
        audio_hw_src_stop(spdif_dec_hdl->hw_src);
        audio_hw_src_close(spdif_dec_hdl->hw_src);
        free(spdif_dec_hdl->hw_src);
        spdif_dec_hdl->hw_src = NULL;
    }


__err2:
    audio_decoder_close(&spdif_dec_hdl->decoder);
    spdif_eq_drc_close(spdif_dec_hdl);
__err1:
    spdif_dec_release();
    return err;
}
int spdif_dec_stop(void)
{
    spdif_dec_hdl->status = SPDIF_STATE_STOP;
    if (spdif_dec_hdl) {
        audio_decoder_stop(&spdif_dec_hdl->decoder);
        audio_decoder_close(&spdif_dec_hdl->decoder);
        if (spdif_dec_hdl->hw_src) {
            audio_hw_src_stop(spdif_dec_hdl->hw_src);
            audio_hw_src_close(spdif_dec_hdl->hw_src);
            free(spdif_dec_hdl->hw_src);
            spdif_dec_hdl->hw_src = NULL;
        }
        audio_mixer_ch_close(&spdif_dec_hdl->mix_ch);
        spdif_eq_drc_close(spdif_dec_hdl);

    }
    return 0;
}
static int spdif_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    if (event == AUDIO_RES_GET) {
        err = spdif_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        err = spdif_dec_stop();
    }
    return err;
}
static void spdif_status_check(void)
{
    /* printf("spdif status \n");	 */
    if (spdif_dec_hdl && (spdif_slave_hdl.status == SPDIF_STATE_START || spdif_slave_hdl.status == SPDIF_STATE_STOP)) {
        /* printf("spdif status \n");	 */
        if (spdif_slave_hdl.error_flag == 1) {
            printf("spdif reset \n");
            /* audio_spdif_slave_close(&spdif_slave_hdl); */
            spdif_slave_hdl.error_flag = 0;
            spdif_dec_close();
            audio_spdif_slave_open(&spdif_slave_hdl);
            audio_spdif_slave_start(&spdif_slave_hdl);
        }
    }
}

void spdif_init(void)
{
    audio_spdif_slave_init(&spdif_slave_hdl);
    request_irq(IRQ_SPDIF_IDX, 2, spdif_isr_handler, 0);
    //gpio_set_direction(IO_PORTA_02 , 1);
    //gpio_direction_output(IO_PORTA_02 , 1);
    sys_timer_add(NULL, spdif_status_check, 250);
}

int spdif_dec_open(struct audio_fmt fmt)
{
    int err;
    if (spdif_dec_hdl) {
        return -1;
    }
    spdif_dec_hdl = zalloc(sizeof(struct s_spdif_decode));
    if (!spdif_dec_hdl) {
        return -ENOMEM;
    }

    clock_add(SPDIF_CLK);
    spdif_dec_hdl->clk_sys = clk_get("sys");
    os_sem_create(&spdif_dec_hdl->sem, 0);
    printf("\n--func=%s\n", __FUNCTION__);
    cbuf_init(&spdif_dec_hdl->dec_cbuf, spdif_dec_hdl->dec_data_buf, SPDIF_DEC_DATA_BUF_LEN);
    spdif_dec_hdl->fmt.channel = fmt.channel;
    spdif_dec_hdl->fmt.coding_type = fmt.coding_type;
    spdif_dec_hdl->fmt.sample_rate = fmt.sample_rate;
    spdif_dec_hdl->coding_type = fmt.coding_type;
    spdif_dec_hdl->wait.priority = 2;
    spdif_dec_hdl->wait.preemption = 0;
    spdif_dec_hdl->wait.snatch_same_prio = 1;
    spdif_dec_hdl->wait.handler = spdif_wait_res_handler;
    err = audio_decoder_task_add_wait(&decode_task, &spdif_dec_hdl->wait);
    spdif_dec_hdl->status = SPDIF_STATE_OPEN;
    return err;
}

void spdif_dec_close(void)
{
    printf("\n--func1=%s\n", __FUNCTION__);
    if (!spdif_dec_hdl) {
        return;
    }
    printf("\n--func2=%s\n", __FUNCTION__);
    spdif_dec_stop();
    /* audio_decoder_close(&spdif_dec_hdl->decoder); */
    /* audio_mixer_ch_close(&spdif_dec_hdl->mix_ch); */
    printf("\n--func3=%s\n", __FUNCTION__);
    spdif_dec_release();
    clock_set_cur();
}

bool spdif_dec_check(void)
{
    if (spdif_dec_hdl) {

        return true;
    }
    return false;
}


#if USER_DIGITAL_VOLUME_ADJUST_ENABLE != 0
/*
 *spdif播歌数字音量调节
 * */
void spdif_user_digital_volume_set(u8 vol)
{
    if (spdif_dec_hdl && spdif_dec_hdl->user_hdl && spdif_dec_hdl->user_hdl->dvol_hdl) {
        user_audio_digital_volume_set(spdif_dec_hdl->user_hdl->dvol_hdl, vol);
    }
}

u8 spdif_user_audio_digital_volume_get()
{
    if (!spdif_dec_hdl) {
        return 0;
    }
    if (!spdif_dec_hdl->user_hdl) {
        return 0;
    }
    if (!spdif_dec_hdl->user_hdl->dvol_hdl) {
        return 0;
    }
    return user_audio_digital_volume_get(spdif_dec_hdl->user_hdl->dvol_hdl);
}

/*
 *user_vol_max:音量级数
 *user_vol_tab:自定义音量表,自定义表长user_vol_max+1
 *注意：如需自定义音量表，须在volume_set前调用 ,否则会在下次volume_set时生效
 */
void spdif_user_digital_volume_tab_set(u16 *user_vol_tab, u8 user_vol_max)
{
    if (spdif_dec_hdl && spdif_dec_hdl->user_hdl && spdif_dec_hdl->user_hdl->dvol_hdl) {
        user_audio_digital_set_volume_tab(spdif_dec_hdl->user_hdl->dvol_hdl, user_vol_tab, user_vol_max);
    }
}



#endif

#if SPDIF_EQ_SUPPORT_32BIT
void spdif_eq_32bit_out(struct s_spdif_decode *dec)
{
    int wlen = 0;

    if (dec->hw_src) {
        wlen = audio_src_resample_write(dec->hw_src, &dec->eq_out_buf[dec->eq_out_points], (dec->eq_out_total - dec->eq_out_points) * 2);
    } else {
        wlen = audio_mixer_ch_write(&dec->mix_ch, &dec->eq_out_buf[dec->eq_out_points], (dec->eq_out_total - dec->eq_out_points) * 2);
    }
    dec->eq_out_points += wlen / 2;
}


int spdif_eq_32bit_to_16bit(struct s_spdif_decode *dec, s32 *data, u32 len)
{

    if ((!dec->eq_out_buf) || (dec->eq_out_buf_len < len / 2)) {
        if (dec->eq_out_buf) {
            free(dec->eq_out_buf);
        }
        dec->eq_out_buf_len = len / 2;
        dec->eq_out_buf = malloc(dec->eq_out_buf_len);
        ASSERT(dec->eq_out_buf);
    }
    s32 *idat = data;
    s16 *odat = dec->eq_out_buf;
    for (int i = 0; i < len / 4; i++) {
        s32 outdat = *idat++;
        if (outdat > 32767) {
            outdat = 32767;
        } else if (outdat < -32768) {
            outdat = -32768;
        }
        *odat++ = outdat;
    }
    dec->eq_out_points = 0;
    dec->eq_out_total = len / 4;

    spdif_eq_32bit_out(dec);
    return len;
}
#endif /*SPDIF_EQ_SUPPORT_32BIT*/


static int spdif_eq_output(void *priv, s16 *data, u32 len)
{
    int wlen = 0;
    int rlen = len;
    struct s_spdif_decode *dec = priv;
#if SPDIF_EQ_SUPPORT_ASYNC


    if (!dec->eq_remain) {
#if SPDIF_EQ_SUPPORT_32BIT
        if (dec->eq_out_buf && (dec->eq_out_points < dec->eq_out_total)) {
            spdif_eq_32bit_out(dec);
            if (dec->eq_out_points < dec->eq_out_total) {
                return 0;
            }
        }
#endif /*SPDIF_EQ_SUPPORT_32BIT*/

#if TCFG_SPDIF_MODE_DRC_ENABLE
        if (dec->p_drc) {
            audio_drc_run(dec->p_drc, data, len);
        }

#endif
    }

#if SPDIF_EQ_SUPPORT_32BIT
    return spdif_eq_32bit_to_16bit(dec, data, len);
#endif /*SPDIF_EQ_SUPPORT_32BIT*/



    if (dec->hw_src) {
        wlen = audio_src_resample_write(dec->hw_src, data, len);
    } else {
        wlen = audio_mixer_ch_write(&dec->mix_ch, data, len);
    }


    if (wlen == len) {
        dec->eq_remain = 0;
    } else {
        dec->eq_remain = 1;
    }

    return wlen;
#endif
    return len;
}



void spdif_eq_drc_open(struct s_spdif_decode *dec, struct audio_fmt *fmt)
{
    if (!dec) {
        return;
    }

}


void spdif_eq_drc_close(struct s_spdif_decode *dec)
{
    if (!dec) {
        return;
    }
}


#endif      // TCFG_SPDIF_ENABLE
/***********************spdif dec end*****************************/
