#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "aec_user.h"
#include "media/includes.h"
#include "commproc.h"
#include "application/audio_eq_drc_apply.h"
#include "circular_buf.h"
#include "clock_cfg.h"


#define LOG_TAG_CONST       AEC_USER
#define LOG_TAG             "[AEC_USER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define AEC_USER_MALLOC_ENABLE	1
#if (TCFG_CALLING_EN_REVERB)
#define AEC_TOGGLE			0
#else
#define AEC_TOGGLE			1
#endif

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_DONGLE)
#undef AEC_TOGGLE
#define AEC_TOGGLE			0
#endif

#if (TCFG_EQ_ENABLE == 1)
#define AEC_DCCS_EN			1 /*mic去直流滤波eq*/
#define AEC_UL_EQ_EN		1 /*mic 普通eq*/
#else
#define AEC_DCCS_EN			0
#define AEC_UL_EQ_EN		0
#endif


#ifdef CONFIG_FPGA_ENABLE
const u8 CONST_AEC_ENABLE = 0;
#else
const u8 CONST_AEC_ENABLE = 1;
#endif

/*丢包修复使能*/
const u8 CONST_PLC_ENABLE = 0;

/*通话dac限幅器*/
const int CONST_SOFT_LIMITER = 0;/*-12000 = -12dB,放大1000倍,(-24000参考)*/

#if TCFG_AEC_SIMPLEX
/*限幅器-噪声门限*/
const int CONST_NOISE_LIMITER = 1;/*使能*/
const u8 CONST_AEC_SIMPLEX = 1;
#else
/*限幅器-噪声门限*/
const int CONST_NOISE_LIMITER = 0;/*使能*/
const u8 CONST_AEC_SIMPLEX = 0;
#endif

/*
 *延时估计使能
 *点烟器/单工模式需要做延时估计
 *其他的暂时不需要做
 */
#if ((AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM) || TCFG_AEC_SIMPLEX)
const u8 CONST_AEC_DLY_EST = 1;
#else
const u8 CONST_AEC_DLY_EST = 0;
#endif

//////////////////Simplex Parameters(单工调试参数)/////////////////////
/*单工连续清0的帧数*/
#define AEC_SIMPLEX_TAIL 	15
/*
 *远端数据大于CONST_AEC_SIMPLEX_THR,即清零近端数据
 *越小，回音限制得越好，同时也就越容易卡
 */
#define AEC_SIMPLEX_THR		100000	/*default:260000*/

/*
 *数据输出开头丢掉的数据包数
 */
#define AEC_OUT_DUMP_PACKET		10

/*
 *数据输出开头丢掉的数据包数
 */
#define AEC_IN_DUMP_PACKET		1

//////////////////AEC Parameters(通话回声调试参数)/////////////////////

/*aec mode select:AEC_MODE_REDUCE or AEC_MODE_ADVANCE */
#define AEC_MODULE_BIT		AEC_MODE_ADVANCE

/*NLP parameters*/
#define NLP_AGGRESS_FACTOR	4.f	/*range:0~35*/
#define NLP_SUPPRESS_FACTOR	5.f	/*range:0~10*/


//////////////////////////= EDN =//////////////////////////////////////

/*
 *复用lmp rx buf(一般通话的时候复用)
 */

//rx_buf概率产生碎片，导致alloc失败，因此默认配0
#define MALLOC_MULTIPLEX_EN	    0
extern void *lmp_malloc(int);
extern void lmp_free(void *);
void *zalloc_mux(int size)
{
#if MALLOC_MULTIPLEX_EN
    void *p = NULL;
    void bredr_rx_bulk_state();
    bredr_rx_bulk_state();
    do {
        p = lmp_malloc(size);
        if (p) {
            break;
        }
        printf("aec_malloc wait...\n");
        os_time_dly(2);
    } while (1);
    if (p) {
        memset(p, 0, size);
    }
    y_printf("[malloc_mux]p = 0x%x,size = %d\n", p, size);
    return p;
#else
    return zalloc(size);
#endif
}

void free_mux(void *p)
{
#if MALLOC_MULTIPLEX_EN
    y_printf("[free_mux]p = 0x%x\n", p);
    lmp_free(p);
#else
    free(p);
#endif
}

void aec_param_dump(struct aec_s_attr *param)
{
    log_info("===========dump aec param==================\n");
    log_info("toggle:%d\n", param->toggle);
    log_info("EnableBit:%x\n", param->EnableBit);
    log_info("ul_eq_en:%x\n", param->ul_eq_en);
    //log_info("AGC_fade:%d\n", (int)(param->AGC_gain_step * 100));
    log_info("AGC_NDT_max_gain:%d\n", (int)(param->AGC_NDT_max_gain * 100));
    log_info("AGC_NDT_min_gain:%d\n", (int)(param->AGC_NDT_min_gain * 100));
    log_info("AGC_NDT_speech_thr:%d\n", (int)(param->AGC_NDT_speech_thr * 100));
    log_info("AGC_DT_max_gain:%d\n", (int)(param->AGC_DT_max_gain * 100));
    log_info("AGC_DT_min_gain:%d\n", (int)(param->AGC_DT_min_gain * 100));
    log_info("AGC_DT_speech_thr:%d\n", (int)(param->AGC_DT_speech_thr * 100));
    log_info("AGC_echo_present_thr:%d\n", (int)(param->AGC_echo_present_thr * 100));
    log_info("AEC_DT_AggressiveFactor:%d\n", (int)(param->AEC_DT_AggressiveFactor * 100));
    log_info("AEC_RefEngThr:%d\n", (int)(param->AEC_RefEngThr * 100));
    log_info("ES_AggressFactor:%d\n", (int)(param->ES_AggressFactor * 100));
    log_info("ES_MinSuppress:%d\n", (int)(param->ES_MinSuppress * 100));
    log_info("ANS_AggressFactor:%d\n", (int)(param->ANS_AggressFactor * 100));
    log_info("ANS_MinSuppress:%d\n", (int)(param->ANS_MinSuppress * 100));
    log_info("=================END=======================\n");
}

void aec_cfg_fill(AEC_CONFIG *cfg)
{
}

struct audio_aec_hdl {
    u8 start;
    u8 inbuf_clear_cnt;
    u8 output_fade_in;
    u8 output_fade_in_gain;
#if AEC_UL_EQ_EN
    struct audio_eq *ul_eq;
    /* struct hw_eq_ch ul_eq_ch; */
#endif
#if AEC_DCCS_EN
    struct audio_eq *dccs_eq;
    /* struct hw_eq_ch dccs_eq_ch; */
#endif
    u16 dump_packet;/*前面如果有杂音，丢掉几包*/
    u8 output_buf[1000];
    cbuffer_t output_cbuf;
    struct aec_s_attr attr;
};
#if AEC_USER_MALLOC_ENABLE
struct audio_aec_hdl *aec_hdl = NULL;
#else
struct audio_aec_hdl aec_handle;
#endif

void audio_aec_ref_start(u8 en)
{
    if (aec_hdl) {
        if (en != aec_hdl->attr.fm_tx_start) {
            aec_hdl->attr.fm_tx_start = en;
            y_printf("fm_tx_start:%d\n", en);
        }
    }
}

#if AEC_DCCS_EN
const int DCCS_8k_Coeff[5] = {
    (943718 << 2),	-(856687 << 2),	(1048576 << 2),	(1887437 << 2),	-(2097152 << 2)
};
const int DCCS_16k_Coeff[5] = {
    (1006633 << 2),	-(967542 << 2),	(1048576 << 2),	(2013266 << 2),	-(2097152 << 2)
};
int aec_dccs_eq_filter(struct audio_eq *eq, int sr, struct audio_eq_filter_info *info)
{
    //r_printf("dccs_eq sr:%d\n", sr);
    if (sr == 16000) {
        info->L_coeff = (void *)DCCS_16k_Coeff;
        info->R_coeff = (void *)DCCS_16k_Coeff;
    } else {
        info->L_coeff = (void *)DCCS_8k_Coeff;
        info->R_coeff = (void *)DCCS_8k_Coeff;
    }
    info->L_gain = 0;
    info->R_gain = 0;
    info->nsection = 1;
    return 0;
}

static int dccs_eq_output(void *priv, void *data, u32 len)
{
    return 0;
}
#endif/*AEC_DCCS_EN*/

#if AEC_UL_EQ_EN

static int ul_eq_output(void *priv, void *data, u32 len)
{
    return 0;
}
#endif/*AEC_UL_EQ_EN*/


static int audio_aec_probe(s16 *data, u16 len)
{
#if AEC_DCCS_EN
    if (aec_hdl->dccs_eq) {
        audio_eq_run(aec_hdl->dccs_eq, data, len);
    }
#endif/*AEC_DCCS_EN*/
    return 0;
}

static int audio_aec_post(s16 *data, u16 len)
{
#if AEC_UL_EQ_EN
    if (aec_hdl->ul_eq) {
        audio_eq_run(aec_hdl->ul_eq, data, len);
    }
#endif/*AEC_UL_EQ_EN*/
    return 0;
}

extern void esco_enc_resume(void);
static int audio_aec_output(s16 *data, u16 len)
{
    if (aec_hdl->dump_packet) {
        aec_hdl->dump_packet--;
        memset(data, 0, len);
    } else  {
        if (aec_hdl->output_fade_in) {
            s32 tmp_data;
            //printf("fade:%d\n",aec_hdl->output_fade_in_gain);
            for (int i = 0; i < len / 2; i++) {
                tmp_data = data[i];
                data[i] = tmp_data * aec_hdl->output_fade_in_gain >> 7;
            }
            aec_hdl->output_fade_in_gain += 12;
            if (aec_hdl->output_fade_in_gain >= 128) {
                aec_hdl->output_fade_in = 0;
            }
        }
    }
    u16 wlen = cbuf_write(&aec_hdl->output_cbuf, data, len);
    //printf("wlen:%d-%d\n",len,aec_hdl.output_cbuf.data_len);

    esco_enc_resume();
#if 1
    static u32 aec_output_max = 0;
    if (aec_output_max < aec_hdl->output_cbuf.data_len) {
        aec_output_max = aec_hdl->output_cbuf.data_len;
        y_printf("o_max:%d", aec_output_max);
    }
#endif
    if (wlen != len) {
        putchar('F');
    }
    return wlen;
}

int audio_aec_output_read(s16 *buf, u16 len)
{
    //printf("rlen:%d-%d\n",len,aec_hdl.output_cbuf.data_len);
    local_irq_disable();
    if (!aec_hdl || !aec_hdl->start) {
        printf("audio_aec close now");
        local_irq_enable();
        return -EINVAL;
    }
    u16 rlen = cbuf_read(&aec_hdl->output_cbuf, buf, len);
    if (rlen == 0) {
        //putchar('N');
    }
    local_irq_enable();
    return rlen;
}

static void audio_aec_param_read_config(struct aec_s_attr *p)
{
    AEC_CONFIG cfg;
    int ret = syscfg_read(CFG_AEC_ID, &cfg, sizeof(AEC_CONFIG));
    if (ret == sizeof(AEC_CONFIG)) {
        log_info("audio_aec read config ok\n");
        p->AGC_NDT_fade_in_step = cfg.ndt_fade_in;
        p->AGC_NDT_fade_out_step = cfg.ndt_fade_out;
        p->AGC_DT_fade_in_step = cfg.dt_fade_in;
        p->AGC_DT_fade_out_step = cfg.dt_fade_out;
        p->AGC_NDT_max_gain = cfg.ndt_max_gain;
        p->AGC_NDT_min_gain = cfg.ndt_min_gain;
        p->AGC_NDT_speech_thr = cfg.ndt_speech_thr;
        p->AGC_DT_max_gain = cfg.dt_max_gain;
        p->AGC_DT_min_gain = cfg.dt_min_gain;
        p->AGC_DT_speech_thr = cfg.dt_speech_thr;
        p->AGC_echo_present_thr = cfg.echo_present_thr;
        p->AEC_DT_AggressiveFactor = cfg.aec_dt_aggress;
        p->AEC_RefEngThr = cfg.aec_refengthr;
        p->ES_AggressFactor = cfg.es_aggress_factor;
        p->ES_MinSuppress = cfg.es_min_suppress;
        p->ES_Unconverge_OverDrive = cfg.es_min_suppress;
        p->ANS_AggressFactor = cfg.ans_aggress;
        p->ANS_MinSuppress = cfg.ans_suppress;
        if (cfg.aec_mode == 0) {
            p->toggle = 0;
            p->EnableBit = 0;
        } else if (cfg.aec_mode == 1) {
            p->toggle = 1;
            p->EnableBit = AEC_MODE_REDUCE;
        } else if (cfg.aec_mode == 2) {
            p->toggle = 1;
            p->EnableBit = AEC_MODE_ADVANCE;
        }
        p->ul_eq_en = cfg.ul_eq_en;
        //aec_param_dump(p);
    } else {
        log_error("read audio_aec param err:%x", ret);
    }
}

extern struct adc_platform_data adc_data;
int audio_aec_init(u16 sample_rate)
{
    struct aec_s_attr *aec_param;
    /*查看lmp rx buf size*/
    void bredr_rx_bulk_state();
    bredr_rx_bulk_state();
#if AEC_USER_MALLOC_ENABLE
    aec_hdl = zalloc(sizeof(struct audio_aec_hdl));
    if (aec_hdl == NULL) {
        log_error("aec_hdl malloc failed");
        return -ENOMEM;
    }
#else
    aec_hdl = &aec_handle;
#endif/*AEC_USER_MALLOC_ENABLE*/
    cbuf_init(&aec_hdl->output_cbuf, aec_hdl->output_buf, sizeof(aec_hdl->output_buf));
    aec_hdl->start = 1;
    aec_hdl->dump_packet = AEC_OUT_DUMP_PACKET;
    aec_hdl->inbuf_clear_cnt = AEC_IN_DUMP_PACKET;
    aec_hdl->output_fade_in = 1;
    aec_hdl->output_fade_in_gain = 0;
    aec_param = &aec_hdl->attr;

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_DAC)
    aec_param->output_way = 0;
#elif (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
    aec_param->output_way = 1;
#endif

    aec_param->toggle = 1;
    aec_param->EnableBit = AEC_MODULE_BIT;
    aec_param->agc_en = 1;
    aec_param->wideband = 0;
    aec_param->ul_eq_en = 1;
    aec_param->packet_dump = 50;/*0~255(u8)*/

    aec_param->AGC_NDT_fade_in_step = 1.3f;
    aec_param->AGC_NDT_fade_out_step = 0.9f;
    aec_param->AGC_DT_fade_in_step = 1.3f;
    aec_param->AGC_DT_fade_out_step = 0.9f;
    aec_param->AGC_NDT_max_gain = 12.f;
    aec_param->AGC_NDT_min_gain = 0.f;
    aec_param->AGC_NDT_speech_thr = -50.f;
    aec_param->AGC_DT_max_gain = 12.f;
    aec_param->AGC_DT_min_gain = 0.f;
    aec_param->AGC_DT_speech_thr = -40.f;
    aec_param->AGC_echo_look_ahead = 0;
    aec_param->AGC_echo_present_thr = -70.f;
    aec_param->AGC_echo_hold = 400;

    /*AEC*/
    aec_param->AEC_DT_AggressiveFactor = 1.f;	/*范围：1~5，越大追踪越好，但会不稳定,如破音*/
    aec_param->AEC_RefEngThr = -70.f; /*范围：-90 ~ -60 dB*/

    /*ES*/
    aec_param->ES_AggressFactor = -3.0f;	/*范围：-1 ~ -5*/
    aec_param->ES_MinSuppress = 4.f;		/*范围：0 ~ 10*/
    aec_param->ES_Unconverge_OverDrive = aec_param->ES_MinSuppress;

    /*ANS*/
    aec_param->ANS_mode = 1;
    aec_param->ANS_AggressFactor = 1.25f;	/*范围：1~2,动态调整,越大越强(1.25f)*/
    aec_param->ANS_MinSuppress = 0.04f;	/*范围：0~1,静态定死最小调整,越小越强(0.09f)*/
    aec_param->ANS_NoiseLevel = 2.2e4f;

#if TCFG_AEC_SIMPLEX
    aec_param->wn_en = 1;
#else
    aec_param->wn_en = 0;
#endif
    aec_param->wn_gain = 331;
    //aec_param->EchoSupressRateThr = 0.05f;/*范围：0.01f~0.05f*/
    aec_param->SimplexTail = AEC_SIMPLEX_TAIL;
    aec_param->SimplexThr = AEC_SIMPLEX_THR;
#if ((AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM) || (TCFG_AEC_SIMPLEX))
    aec_param->dly_est = 1;
#else
    aec_param->dly_est = 0;
#endif
    aec_param->dst_delay = 50;
    aec_param->aec_probe = audio_aec_probe;
    aec_param->aec_post = audio_aec_post;
    aec_param->output_handle = audio_aec_output;

    audio_aec_param_read_config(aec_param);

#if TCFG_AEC_SIMPLEX
    aec_param->EnableBit = AEC_MODE_SIMPLEX;
    if (sample_rate == 8000) {
        aec_param->wideband = 0;
        aec_param->hw_delay_offset = 75;
        aec_param->SimplexTail = aec_param->SimplexTail / 2;
        clock_add(AEC8K_SPX_CLK);
    } else {
        aec_param->wideband = 1;
        aec_param->hw_delay_offset = 50;
        clock_add(AEC16K_SPX_CLK);
    }
    if (aec_param->dly_est == 0) {
        aec_param->dst_delay = 30;
    }
    //printf("aec SimplexMode\n");
#else
    if (sample_rate == 16000) {
        aec_param->wideband = 1;
        aec_param->hw_delay_offset = 50;
        if (aec_param->EnableBit == AEC_MODE_ADVANCE) {
            clock_add(AEC16K_ADV_CLK);
        } else {
            clock_add(AEC16K_CLK);
        }
    } else {
        aec_param->wideband = 0;
        aec_param->hw_delay_offset = 75;
        if (aec_param->EnableBit == AEC_MODE_ADVANCE) {
            clock_add(AEC8K_ADV_CLK);
        } else {
            clock_add(AEC8K_CLK);
        }
    }
#endif/*TCFG_AEC_SIMPLEX*/


#if AEC_UL_EQ_EN
    if (aec_param->ul_eq_en) {
        /* memset(&aec_hdl->ul_eq, 0, sizeof(struct audio_eq)); */
        /* memset(&aec_hdl->ul_eq_ch, 0, sizeof(struct hw_eq_ch)); */
        /* aec_hdl->ul_eq.eq_ch = &aec_hdl->ul_eq_ch; */
        struct audio_eq_param ul_eq_param = {0};
        ul_eq_param.sr = sample_rate;
        ul_eq_param.channels = 1;
        ul_eq_param.online_en = 1;
        ul_eq_param.mode_en = 0;
        ul_eq_param.remain_en = 0;
        ul_eq_param.max_nsection = EQ_SECTION_MAX;
        ul_eq_param.cb = aec_ul_eq_filter;
        ul_eq_param.eq_name = aec_eq_mode;
        aec_hdl->ul_eq = audio_dec_eq_open(&ul_eq_param);
        /* audio_eq_open(&aec_hdl->ul_eq, &ul_eq_param); */
        /* audio_eq_set_samplerate(&aec_hdl->ul_eq, sample_rate); */
        /* audio_eq_set_output_handle(&aec_hdl->ul_eq, ul_eq_output, NULL); */
        /* audio_eq_start(&aec_hdl->ul_eq); */
    }
#endif/*AEC_UL_EQ_EN*/

#if AEC_DCCS_EN
    if (adc_data.mic_capless) {
        /* memset(&aec_hdl->dccs_eq, 0, sizeof(struct audio_eq)); */
        /* memset(&aec_hdl->dccs_eq_ch, 0, sizeof(struct hw_eq_ch)); */

        /* .eq_ch = &aec_hdl->dccs_eq_ch; */
        struct audio_eq_param dccs_eq_param = {0};
        dccs_eq_param.sr = sample_rate;
        dccs_eq_param.channels = 1;
        dccs_eq_param.online_en = 0;
        dccs_eq_param.mode_en = 0;
        dccs_eq_param.remain_en = 0;
        dccs_eq_param.max_nsection = EQ_SECTION_MAX;
        dccs_eq_param.cb = aec_dccs_eq_filter;
        aec_hdl->dccs_eq = audio_dec_eq_open(&dccs_eq_param);
        /* audio_eq_open(&aec_hdl->dccs_eq, &dccs_eq_param); */
        /* audio_eq_set_samplerate(&aec_hdl->dccs_eq, sample_rate); */
        /* audio_eq_set_output_handle(&aec_hdl->dccs_eq, dccs_eq_output, NULL); */
        /* audio_eq_start(&aec_hdl->dccs_eq); */
    }
#endif/*AEC_DCCS_EN*/

    //aec_param->toggle = 1;
    //aec_param->EnableBit = AEC_MODULE_BIT;
    //aec_param_dump(aec_param);

#if AEC_TOGGLE
    aec_init(aec_param);
#endif
    bredr_rx_bulk_state();
    return 0;
}

void audio_aec_close(void)
{
    printf("audio_aec_close:%x", aec_hdl);
    struct aec_s_attr *aec_param;
    if (aec_hdl) {
        aec_hdl->start = 0;

        aec_param = &aec_hdl->attr;
#if TCFG_AEC_SIMPLEX
        if (aec_param->wideband) {
            clock_remove(AEC16K_SPX_CLK);
        } else {
            clock_remove(AEC8K_SPX_CLK);
        }
#else
        if (aec_param->wideband) {
            if (aec_param->EnableBit == AEC_MODE_ADVANCE) {
                clock_remove(AEC16K_ADV_CLK);
            } else {
                clock_remove(AEC16K_CLK);
            }
        } else {
            if (aec_param->EnableBit == AEC_MODE_ADVANCE) {
                clock_remove(AEC8K_ADV_CLK);
            } else {
                clock_remove(AEC8K_CLK);
            }
        }
#endif/*TCFG_AEC_SIMPLEX*/

#if AEC_TOGGLE
        aec_exit();
#endif/*AEC_TOGGLE*/
#if AEC_DCCS_EN
        if (aec_hdl->dccs_eq) {
            audio_dec_eq_close(aec_hdl->dccs_eq);
        }
#endif/*AEC_DCCS_EN*/
#if AEC_UL_EQ_EN
        if (aec_hdl->ul_eq) {
            audio_dec_eq_close(aec_hdl->ul_eq);
        }
#endif/*AEC_UL_EQ_EN*/
        local_irq_disable();
#if AEC_USER_MALLOC_ENABLE
        free(aec_hdl);
#endif/*AEC_USER_MALLOC_ENABLE*/
        aec_hdl = NULL;
        local_irq_enable();
    }
}

void audio_aec_inbuf(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
#if AEC_TOGGLE
        if (aec_hdl->inbuf_clear_cnt) {
            aec_hdl->inbuf_clear_cnt--;
            memset(buf, 0, len);
        }
        int ret = aec_fill_in_data(buf, len);
        if (ret == -1) {
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_DAC)
            /* log_info("fill dac data\n");
            u8 tmp_buf[64] = {0};
            for (u8 i = 0; i < 512 / sizeof(tmp_buf); i++) {
                app_audio_output_write(tmp_buf, sizeof(tmp_buf));
            } */
#endif
        } else if (ret == -2) {
            log_error("aec inbuf full\n");
        }
#else
        audio_aec_output(buf, len);
#endif/*AEC_TOGGLE*/
    }
}

void audio_aec_refbuf(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
#if AEC_TOGGLE
        aec_fill_ref_data(buf, len);
#endif/*AEC_TOGGLE*/
    }
}

int audio_aec_refbuf_data_len()
{
    if (aec_hdl && aec_hdl->start) {
#if AEC_TOGGLE
        //aec_ref_total_len();
        //aec_ref_data_len();
#endif/*AEC_TOGGLE*/
    }
    return 0;
}

/* void aec_estimate_dump(int DlyEst)
{
	printf("DlyEst:%d\n",DlyEst);
} */
/********************************************************
 *
 *		aec parameters debug online
 *
 *******************************************************/
#if AEC_DEBUG_ONLINE
#include "string.h"
#include "asm/crc16.h"
#include "app_main.h"

enum {
    AEC_MIC_AGAIN	= 0u,
    AEC_DAC_AGAIN		,
    AEC_NDT_MAX_GAIN	,
    AEC_NDT_MIN_GAIN	,
    AEC_NDT_FADE_SPEED	,
    AEC_NEAREND_THR		,
    AEC_NLP_AGGRESS		,
    AEC_NLP_SUPPRESS	,
    AEC_MODE			,/*0:disable,1:REDUCE,2:ADVANCE*/
    AEC_UL_EQ	   	,    /*0:disable,1:enable*/
};

struct _AEC_ONLINE {
    char tag[4];/*AEC:0x41 0x45 0x43 0x00	*/
    u16 crc;	/*crc = crc16(len+cmd+data)	*/
    u16 len;	/*data length				*/
    u16 cmd;	/*cmd index					*/
    s32 data;	/*cmd value					*/
} __attribute__((packed));
typedef struct _AEC_ONLINE AEC_ONLINE;
AEC_ONLINE aec_online;

void aec_param_deal(u16 cmd, s32 data)
{
    //log_info(">>>>aec cmd:%d \t data:%d\n", cmd, data);
    switch (cmd) {
    case AEC_MIC_AGAIN:
        app_var.aec_mic_gain = (u16)data;
        log_info("aec_mic:%d\n", app_var.aec_mic_gain);
        break;
    case AEC_DAC_AGAIN:
        app_var.aec_dac_gain = (u16)data;
        log_info("aec_dac:%d\n", app_var.aec_dac_gain);
        break;
    case AEC_NDT_MAX_GAIN:
        aec_param.AGC_max_gain = (u16)data;
        log_info("aec_max_gain:%d\n", aec_param.AGC_max_gain);
        break;
    case AEC_NDT_MIN_GAIN:
        aec_param.AGC_min_gain = (u16)data;
        log_info("aec_min_gain:%d\n", aec_param.AGC_min_gain);
        break;
    case AEC_NDT_FADE_SPEED:
        aec_param.AGC_fade = (u16)data;
        log_info("aec_fade_speed:%d\n", aec_param.AGC_fade);
        break;
    case AEC_NEAREND_THR:
        aec_param.AGC_threshold = (u16)data;
        log_info("aec_neadend_thr:%d\n", aec_param.AGC_threshold);
        break;
    case AEC_NLP_AGGRESS:
        memcpy(&aec_param.ES_AggressFactor, &data, sizeof(float));
        aec_param.ES_AggressFactor = -aec_param.ES_AggressFactor;
        log_info("Aggress:%d\n", (int)aec_param.ES_AggressFactor);
        break;
    case AEC_NLP_SUPPRESS:
        //memcpy(&aec_param.ES_MinSuppress, &data, sizeof(float));
        //log_info("Suppress:%d\n", (int)aec_param.ES_MinSuppress);
        break;
    case AEC_MODE:	/*0:disable,1:REDUCE,2:ADVANCE*/
        if (data == 0) {
            log_info("aec_mode:OFF\n");
            aec_param.toggle = 0;
        } else if (data == 1) {
            log_info("aec_mode:REDUCE\n");
            aec_param.toggle = 1;
            aec_param.EnableBit = AEC_MODE_REDUCE;
        } else if (data == 2) {
            log_info("aec_mode:ADVANCE\n");
            aec_param.toggle = 1;
            aec_param.EnableBit = AEC_MODE_ADVANCE;
        }
        break;
    case AEC_UL_EQ:
        aec_param.ul_eq_en = (u8)data;
        log_info("aec_ul_eq:%d\n", aec_param.ul_eq_en);
        break;
    default:
        log_info("unknown AEC cmd\n");
        break;
    }
}

#include "tone_player.h"
s8 aec_debug_online(void *buf, u16 size)
{
    log_info("AEC_rx:%d\n", size);
    log_info_hexdump(buf, size);
    memcpy(&aec_online, buf, sizeof(AEC_ONLINE));
    if (strcmp(aec_online.tag, "AEC") == 0) {
        if (aec_online.crc == CRC16(&aec_online.len, aec_online.len + sizeof(aec_online.len)))	{
            log_info("aec_online OK\n");
            aec_param_deal(aec_online.cmd, aec_online.data);
            tone_play_by_path(tone_table[IDEX_TONE_NORMAL], 1);
            return 0;
        } else {
            log_info("aec_online ERROR\n");
            return -1;
        }
    } else {
        log_info("unknown data\n");
        return -1;
    }
}
#endif
