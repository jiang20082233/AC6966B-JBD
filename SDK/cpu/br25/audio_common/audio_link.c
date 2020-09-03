#include "asm/iis.h"
#include "gpio.h"
#include "media/includes.h"

#include "app_config.h"
#include "audio_link.h"
#define alink_printf      g_printf

#if TCFG_IIS_ENABLE

extern u32 audio_output_rate(int input_rate);
extern u32 audio_output_channel_num(void);
extern int audio_output_set_start_volume(u8 state);
extern void audio_hw_resample_close(struct audio_src_handle *hdl);
extern struct audio_decoder_task decode_task;
extern struct audio_mixer mixer;

static struct s_alink_hdl *p_alink_hdl[ALINK_MODULE_NUM];

#define IIS_DAC_DBG    1

#define PCM_DEC_ENABLE 10

typedef struct {
    iis_param user_data;
    u8 iis_use;
} iis_module_param;

enum {
    MCLK_PLL_192M = 0,
    MCLK_PLL_160M,
    MCLK_FMCKO,
};

enum {
    PLL_DIV_17 = 0,
    PLL_DIV_13,
    PLL_DIV_FM_8,
    PLL_DIV_FM_4,
};

enum {
    MCLK_EXTERNAL	= 0u,
    MCLK_SYS_CLK		,
    MCLK_OSC_CLK 		,
    MCLK_PLL_CLK		,
};

enum {
    MCLK_DIV_1		= 0u,
    MCLK_DIV_2			,
    MCLK_DIV_4			,
    MCLK_DIV_8			,
    MCLK_DIV_16			,
};

enum {
    MCLK_LRDIV_EX	= 0u,
    MCLK_LRDIV_64FS		,
    MCLK_LRDIV_128FS	,
    MCLK_LRDIV_192FS	,
    MCLK_LRDIV_256FS	,
    MCLK_LRDIV_384FS	,
    MCLK_LRDIV_512FS	,
    MCLK_LRDIV_768FS	,
};

static iis_module_param  iis_module_data[ALINK_MODULE_NUM];


enum {
    IIS_IO_MCLK = 0u,
    IIS_IO_SCLK 	,
    IIS_IO_LRCK 	,
    IIS_IO_CH0 		,
    IIS_IO_CH1 		,
    IIS_IO_CH2 		,
    IIS_IO_CH3 		,
};

static const u8 alink0_IOS_portA[] = {
    IO_PORTA_02, IO_PORTA_03, IO_PORTA_04, IO_PORTA_00, IO_PORTA_01, IO_PORTA_05, IO_PORTA_06,
};
static const u8 alink0_IOS_portB[] = {
    IO_PORTC_02, IO_PORTA_05, IO_PORTA_06, IO_PORTA_00, IO_PORTA_02, IO_PORTA_03, IO_PORTA_04,
};
/* static const u8 alink1_IOS_portA[] = { */
/* IO_PORTB_00, IO_PORTC_00, IO_PORTC_01, IO_PORTC_02, IO_PORTC_03, IO_PORTC_04, IO_PORTC_05, */
/* }; */

static void iis_set_sr(ALINK_PORT port, u32 rate, u8 soe)
{
    u8 module = ALINK_PORT_TO_MODULE(port);
    /* if(iis_module_data[module].user_data.rate == rate){ */
    /*     return; */
    /* } */

    alink_printf("ALINK_SR = %d\n", rate);

    switch (rate) {
    case 48000:
        /*12.288Mhz 256fs*/
        PLL_ALINK_SEL(module, MCLK_PLL_160M);
        PLL_ALINK_DIV(module, PLL_DIV_13);
        ALINK_MDIV(module, MCLK_DIV_1);
        if (soe) {
            ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        } else {
            ALINK_LRDIV(module, MCLK_LRDIV_EX);
        }
        break ;

    case 44100:
        /*11.289Mhz 256fs*/
        PLL_ALINK_SEL(module, MCLK_PLL_192M);
        PLL_ALINK_DIV(module, PLL_DIV_17);
        ALINK_MDIV(module, MCLK_DIV_1);
        if (soe) {
            ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        } else {
            ALINK_LRDIV(module, MCLK_LRDIV_EX);
        }
        break ;

    case 32000:
        /*12.288Mhz 384fs*/
        PLL_ALINK_SEL(module, MCLK_PLL_160M);
        PLL_ALINK_DIV(module, PLL_DIV_13);
        ALINK_MDIV(module, MCLK_DIV_1);
        if (soe) {
            ALINK_LRDIV(module, MCLK_LRDIV_384FS);
        } else {
            ALINK_LRDIV(module, MCLK_LRDIV_EX);
        }
        break ;

    case 24000:
        /*12.288Mhz 512fs*/
        PLL_ALINK_SEL(module, MCLK_PLL_160M);
        PLL_ALINK_DIV(module, PLL_DIV_13);
        ALINK_MDIV(module, MCLK_DIV_2);
        if (soe) {
            ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        } else {
            ALINK_LRDIV(module, MCLK_LRDIV_EX);
        }
        break ;

    case 22050:
        /*11.289Mhz 512fs*/
        PLL_ALINK_SEL(module, MCLK_PLL_192M);
        PLL_ALINK_DIV(module, PLL_DIV_17);
        ALINK_MDIV(module, MCLK_DIV_2);
        if (soe) {
            ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        } else {
            ALINK_LRDIV(module, MCLK_LRDIV_EX);
        }
        break ;

    case 16000:
        /*12.288/2Mhz 384fs*/
        PLL_ALINK_SEL(module, MCLK_PLL_160M);
        PLL_ALINK_DIV(module, PLL_DIV_13);
        ALINK_MDIV(module, MCLK_DIV_2);
        if (soe) {
            ALINK_LRDIV(module, MCLK_LRDIV_384FS);
        } else {
            ALINK_LRDIV(module, MCLK_LRDIV_EX);
        }
        break ;

    case 12000:
        /*12.288/2Mhz 512fs*/
        PLL_ALINK_SEL(module, MCLK_PLL_160M);
        PLL_ALINK_DIV(module, PLL_DIV_13);
        ALINK_MDIV(module, MCLK_DIV_4);
        if (soe) {
            ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        } else {
            ALINK_LRDIV(module, MCLK_LRDIV_EX);
        }
        break ;

    case 11025:
        /*11.289/2Mhz 512fs*/
        PLL_ALINK_SEL(module, MCLK_PLL_192M);
        PLL_ALINK_DIV(module, PLL_DIV_17);
        ALINK_MDIV(module, MCLK_DIV_4);
        if (soe) {
            ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        } else {
            ALINK_LRDIV(module, MCLK_LRDIV_EX);
        }
        break ;

    case 8000:
        /*12.288/4Mhz 384fs*/
        PLL_ALINK_SEL(module, MCLK_PLL_160M);
        PLL_ALINK_DIV(module, PLL_DIV_13);
        ALINK_MDIV(module, MCLK_DIV_4);
        if (soe) {
            ALINK_LRDIV(module, MCLK_LRDIV_384FS);
        } else {
            ALINK_LRDIV(module, MCLK_LRDIV_EX);
        }
        break ;

    default:
        //44100
        /*11.289Mhz 256fs*/
        PLL_ALINK_SEL(module, MCLK_PLL_192M);
        PLL_ALINK_DIV(module, PLL_DIV_17);
        ALINK_MDIV(module, MCLK_DIV_1);
        if (soe) {
            ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        } else {
            ALINK_LRDIV(module, MCLK_LRDIV_EX);
        }
        break;
    }
}

___interrupt
static void alink0_isr(void)
{
    u16 reg;
    u8  i = 0;
    s16 *buf_addr = NULL ;

    reg = JL_ALNK0->CON2;

    ALINK_CPND(0, 0xf);/*clear pending*/

    for (; i < ALINK_CH_MAX; i++) {
        if ((iis_module_data[0].user_data.ch[i].enable) && (reg & BIT(i + 4))) {
            if (iis_module_data[0].user_data.isr_cbfun) {
                buf_addr = (JL_ALNK0->CON0 & BIT(i + 12)) ? (iis_module_data[0].user_data.ch[i].dma_adr) : (iis_module_data[0].user_data.ch[i].dma_adr) + iis_module_data[0].user_data.frame_len * 2;
                iis_module_data[0].user_data.isr_cbfun(i, buf_addr, iis_module_data[0].user_data.frame_len);
            }
        }
    }
}

___interrupt
static void alink1_isr(void)
{
    u16 reg;
    u8 i = 0;
    s16 *buf_addr = NULL ;

    reg = JL_ALNK1->CON2;

    ALINK_CPND(1, 0xf);/*clear pending*/

    for (; i < ALINK_CH_MAX; i++) {
        if ((iis_module_data[1].user_data.ch[i].enable) && (reg & BIT(i + 4))) {
            if (iis_module_data[1].user_data.isr_cbfun) {
                buf_addr = (JL_ALNK1->CON0 & BIT(i + 12)) ? (iis_module_data[1].user_data.ch[i].dma_adr) : (iis_module_data[1].user_data.ch[i].dma_adr) + iis_module_data[1].user_data.frame_len * 2;
                iis_module_data[1].user_data.isr_cbfun(i, buf_addr, iis_module_data[1].user_data.frame_len);
            }
        }
    }
}

static void iis_ch_en(iis_param *param)
{
    u16 ch_mode = 0;
    u16 ch_dir = 0;/*0:Send 1:Receive*/
    u16 ch_len = 0;/*0:16bit,1:24bit*/
    u8  i = 0;

    for (; i < ALINK_CH_MAX; i++) {
        if (param->ch[i].enable) {
            alink_printf("iis ch%d en \n", i);
            /* ch_mode |= param->ch[i].ch_md    << (i * 4 + 0); #<{(|TxMOD|)}># */
            /* ch_len  |= param->ch[i].bit_wide << (i * 4 + 1); #<{(|TxLEN|)}># */
            /* ch_dir  |= param->ch[i].dir  	   << (i * 4 + 3); #<{(|TxDIR|)}># */

            ch_mode |= (param->ch[i].ch_md & 0x03)    << (i * 4 + 0); /*TxMOD*/
            ch_len  |= (param->ch[i].bit_wide & 0x01) << (i * 4 + 2); /*TxLEN*/
            ch_dir  |= (param->ch[i].dir &  0x01)	      << (i * 4 + 3); /*TxDIR*/
        }
    }

    if (ALINK_PORT_TO_MODULE(param->port) == 0) {
        JL_ALNK0->CON1 = 0; /*reset ch reg*/
        JL_ALNK0->CON1 |= (ch_mode | ch_len | ch_dir);
    } else {
        JL_ALNK1->CON1 = 0; /*reset ch reg*/
        JL_ALNK1->CON1 |= (ch_mode | ch_len | ch_dir);
    }
}

int iis_open(iis_param *param)
{
    u8 i = 0;
    u8 module = 0;

    if (param == NULL) {
        return -1;
    }

    module = ALINK_PORT_TO_MODULE(param->port);

    g_printf("iis module:%d", module);
    if (iis_module_data[module].iis_use == 0) {
        memcpy(&(iis_module_data[module].user_data), param, sizeof(iis_param));
        iis_module_data[module].iis_use = 1;
    } else {
        alink_printf("iis already use!");
        return -1;
    }
    switch (param->port) {
    case ALINK0_PORTA: //MCLK:PA8  SCLK:PA2  lRCK:PA3  CH0:PA4  CH1:PA5  CH2:PA6  CH3:PA7
        if (param->soe) {
            gpio_set_direction(alink0_IOS_portA[IIS_IO_SCLK], 0);
            gpio_set_direction(alink0_IOS_portA[IIS_IO_LRCK], 0);
            gpio_set_die(alink0_IOS_portA[IIS_IO_SCLK], 1);
            gpio_set_die(alink0_IOS_portA[IIS_IO_LRCK], 1);
        } else {
            gpio_set_direction(alink0_IOS_portA[IIS_IO_SCLK], 1);
            gpio_set_direction(alink0_IOS_portA[IIS_IO_LRCK], 1);
            /* gpio_set_die(alink0_IOS_portA[IIS_IO_SCLK], 1); */
            /* gpio_set_die(alink0_IOS_portA[IIS_IO_LRCK], 1);			 */
        }
        if (param->moe) {
            gpio_set_direction(alink0_IOS_portA[IIS_IO_MCLK], 0);
            gpio_set_die(alink0_IOS_portA[IIS_IO_MCLK], 1);
        } else {
            /* gpio_set_direction(alink0_IOS_portA[IIS_IO_MCLK], 1); */
        }
        for (i = 0; i < ALINK_CH_MAX; i++) {
            if (param->ch[i].enable) {
                gpio_set_direction(alink0_IOS_portA[IIS_IO_CH0 + i], param->ch[i].dir);

                gpio_set_die(alink0_IOS_portA[IIS_IO_CH0 + i], 1);
                ALINK_SET_ADR(module, i, (volatile unsigned int)param->ch[i].dma_adr);
            }
        }
        JL_IOMAP->CON0 &= ~BIT(11);/*ALINK0_IO_SEL_PORTA*/
        request_irq(IRQ_ALINK0_IDX, 3, alink0_isr, 0) ;
        break;

    case ALINK0_PORTB: //MCLK:PA15 SCLK:PA9  LRCK:PA10 CH0:PA11 CH1:PA12 CH2:PA13 CH3:PA14
        if (param->soe) {
            gpio_set_direction(alink0_IOS_portB[IIS_IO_SCLK], 0);
            gpio_set_direction(alink0_IOS_portB[IIS_IO_LRCK], 0);
            gpio_set_die(alink0_IOS_portB[IIS_IO_SCLK], 1);
            gpio_set_die(alink0_IOS_portB[IIS_IO_LRCK], 1);
        } else {
            gpio_set_direction(alink0_IOS_portB[IIS_IO_SCLK], 1);
            gpio_set_direction(alink0_IOS_portB[IIS_IO_LRCK], 1);
        }
        if (param->moe) {
            gpio_set_direction(alink0_IOS_portB[IIS_IO_MCLK], 0);
            gpio_set_die(alink0_IOS_portB[IIS_IO_MCLK], 1);
        }
        for (i = 0; i < ALINK_CH_MAX; i++) {
            if (param->ch[i].enable) {
                gpio_set_direction(alink0_IOS_portB[IIS_IO_CH0 + i], param->ch[i].dir);
                gpio_set_die(alink0_IOS_portB[IIS_IO_CH0 + i], 1);
                ALINK_SET_ADR(module, i, (volatile unsigned int)param->ch[i].dma_adr);
            }
        }
        JL_IOMAP->CON0 |=  BIT(11);/*ALINK_IO_SEL_PORTB*/
        request_irq(IRQ_ALINK0_IDX, 3, alink0_isr, 0) ;
        break;
    default:
        alink_printf("unkown alink port!\n");
        return -1;
    }
    ALINK_MSRC(module, MCLK_PLL_CLK);	/*MCLK source*/
    ALINK_SOE(module, param->soe);   //使能alink soe
    ALINK_MOE(module, param->moe);   //使能alink moe
    ALINK_DSPE(module, param->dsp);   //选择alink模式，0:普通模式   1:扩展模式
    ALINK_SET_LEN(module, param->frame_len);
    iis_set_sr(param->port, param->rate, param->soe);              //设置采样率
    iis_ch_en(param);

    ALINK_EN(module, 1);
#if 0
    if (!module) {
        alink_printf("ALINK0_CON0:0x%x \n", JL_ALNK0->CON0);
        alink_printf("ALINK0_CON1:0x%x \n", JL_ALNK0->CON1);
        alink_printf("ALINK0_CON2:0x%x \n", JL_ALNK0->CON2);
        alink_printf("ALINK0_CON3:0x%x \n", JL_ALNK0->CON3);
    } else {
        alink_printf("ALINK1_CON0:0x%x", JL_ALNK1->CON0);
        alink_printf("ALINK1_CON1:0x%x", JL_ALNK1->CON1);
        alink_printf("ALINK1_CON2:0x%x", JL_ALNK1->CON2);
        alink_printf("ALINK1_CON3:0x%x", JL_ALNK1->CON3);
    }
    alink_printf("CLK_CON2 :0x%x", JL_CLOCK->CLK_CON2);
    alink_printf("PLL_CON1 :0x%x", JL_CLOCK-> PLL_CON1);
#endif
    return 0;
}
int close(u8 module)
{
    ALINK_EN((module ? 1 : 0), 0);
    iis_module_data[module].iis_use = 0;
    return 0;
}
void iis0_cb(u8 ch_idx, void *buf, u32 len)
{
    int rlen = 0;
    int wlen = 0;
    if (iis_module_data[0].user_data.ch[ch_idx].dir == ALINK_DIR_TX) {
        if (p_alink_hdl[0]->param.dsp == 0) { //只支持立体声
            rlen = cbuf_read(&p_alink_hdl[0]->pcm_cbuf[ch_idx], buf, len * 2 * 2);
            if (!rlen) {
                memset(buf, 0x0, len * 2 * 2);
            } else {
                /* printf("iis0 ch:%d read success%d \n",ch_idx,len*2);	 */
            }
        } else { //单声道
            rlen = cbuf_read(&p_alink_hdl[0]->pcm_cbuf[ch_idx], buf, len * 2);
            if (!rlen) {
                memset(buf, 0x0, len * 2);
            } else {
                /* printf("iis0 ch:%d read success%d \n",ch_idx,len*2);	 */
            }
        }

    } else {
        if (p_alink_hdl[0]->param.dsp == 0) { //立体声
            wlen =	cbuf_write(&p_alink_hdl[0]->pcm_cbuf[ch_idx], buf, len * 2 * 2);
            if (!wlen) {
                putchar('W');
            } else {
                /* printf("w len %d\n",len*4);	 */
            }
        } else {
            cbuf_write(&p_alink_hdl[0]->pcm_cbuf[ch_idx], buf, len * 2);
        }
    }
}

void iis1_cb(u8 ch_idx, void *buf, u32 len)
{
    int rlen = 0;
    /* printf("ch:%d, dir:%d ", ch_idx, iis_module_data[1].user_data.ch[ch_idx].dir); */
    if (iis_module_data[1].user_data.ch[ch_idx].dir == ALINK_DIR_TX) {
        if (p_alink_hdl[1]->param.dsp == 0) { //只支持立体声
            rlen = cbuf_read(&p_alink_hdl[1]->pcm_cbuf[ch_idx], buf, len * 2 * 2);
            if (!rlen) {
                memset(buf, 0x0, len * 2 * 2);
            } else {
                /* printf("iis1 ch:%d read success%d \n",ch_idx,len*2);	 */
            }
        } else {
            rlen = cbuf_read(&p_alink_hdl[1]->pcm_cbuf[ch_idx], buf, len * 2);
            if (!rlen) {
                memset(buf, 0x0, len * 2);
            } else {
                /* printf("iis1 ch:%d read success%d \n",ch_idx,len*2);	 */
            }
        }

    } else {
        if (p_alink_hdl[1]->param.dsp == 0) { //只支持立体声
            cbuf_write(&p_alink_hdl[1]->pcm_cbuf[ch_idx], buf, len * 2 * 2);
#if PCM_DEC_ENABLE
            audio_decoder_resume(&p_alink_hdl[1]->dec->decoder);
#endif
            /* printf("iis receive len %d",len*4); */
        } else {
            cbuf_write(&p_alink_hdl[1]->pcm_cbuf[ch_idx], buf, len * 2);
#if PCM_DEC_ENABLE
            audio_decoder_resume(&p_alink_hdl[1]->dec->decoder);
#endif
        }
    }
}

#define CH_CBUF_DATA_LEN   (1024*4)
#define IIS_POINT_NUM      (32)
#if 0
static s16 alink_dma_buf0[2][IIS_POINT_NUM * 2] __attribute__((aligned(4)));
static s16 alink_dma_buf1[2][IIS_POINT_NUM * 2] __attribute__((aligned(4)));
static s16 alink_dma_buf2[2][IIS_POINT_NUM * 2] __attribute__((aligned(4)));
static s16 alink_dma_buf3[2][IIS_POINT_NUM * 2] __attribute__((aligned(4)));
static s16 *p_alink_dma_buf[ALINK_CH_MAX] = {(s16 *)alink_dma_buf0, (s16 *)alink_dma_buf1, (s16 *)alink_dma_buf2, (s16 *)alink_dma_buf3};
#endif

static u8 ch_en_bitmap[ALINK_MODULE_NUM] = {0};
// RX TX 单通道立体声
/* static u8 ch_en_bitmap = BIT(0); */
/* static u8 ch_en_bitmap = BIT(1); */
/* static u8 ch_en_bitmap = BIT(2); */
/* static u8 ch_en_bitmap = BIT(3); */

//双通道输出立体声
/* u8 static ch_en_bitmap = BIT(0)|BIT(1); */

//4声道(前L前R后L后R) 4通道输出
/* u8 static ch_en_bitmap = BIT(0)|BIT(1)|BIT(2)|BIT(3); */


/* static struct s_alink_hdl *p_alink_hdl[ALINK_MODULE_NUM]; */


/******************pcm dec ***************************/
#if PCM_DEC_ENABLE
#define PCM_RATE_MAX_STEP		50
#define PCM_RATE_INC_STEP       5
#define PCM_RATE_DEC_STEP       5

static u8 pcm_dec_maigc = 0;
static void pcm_dec_close(u8 module);
static int alink_src_output_handler(void *priv, void *buf, int len)
{
    int wlen = 0;
    int rlen = len;
    struct s_pcm_dec *dec = (struct s_pcm_dec *) priv;
    s16 *data = (s16 *)buf;
    /* return len;		 */
    do {
        wlen = audio_mixer_ch_write(&dec->mix_ch, data, rlen);
        if (!wlen) {
            break;
        }
        data += wlen / 2 ;
        rlen -= wlen;
    } while (rlen);
    /* printf("src,l:%d, wl:%d \n", len, len-rlen); */
    return (len - rlen);
}

static int pcm_dec_sync_init(struct s_pcm_dec *dec)
{
#if 10
    dec->sync_start = 0;
    dec->begin_size = dec->dec_pcm_cbuf->total_len * 60 / 100;
    dec->top_size = dec->dec_pcm_cbuf->total_len * 70 / 100;
    dec->bottom_size = dec->dec_pcm_cbuf->total_len * 40 / 100;

    u16 out_sr = dec->src_out_sr;
    printf("out_sr:%d, dsr:%d, dch:%d \n", out_sr, dec->source_sr, dec->out_ch_num);
    dec->audio_new_rate = out_sr;
    dec->audio_max_speed = out_sr + PCM_RATE_MAX_STEP;
    dec->audio_min_speed = out_sr - PCM_RATE_MAX_STEP;

    /* return 0; */
    dec->src_sync = zalloc(sizeof(struct audio_src_handle));
    if (!dec->src_sync) {
        return -ENODEV;
    }
    audio_hw_src_open(dec->src_sync, dec->out_ch_num, SRC_TYPE_AUDIO_SYNC);

    audio_hw_src_set_rate(dec->src_sync, dec->source_sr, dec->audio_new_rate);

    audio_src_set_output_handler(dec->src_sync, dec, alink_src_output_handler);
#endif
    return 0;
}
static void pcm_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        if ((u8)argv[1] != (u8)(pcm_dec_maigc - 1)) {
            log_i("maigc err, %s\n", __FUNCTION__);
            break;
        }
        /* pcm_dec_close(); */
        break;
    }
}
static int pcm_dec_stream_sync(struct s_pcm_dec *dec, int data_size)
{
    if (!dec->src_sync) {
        return 0;
    }

    if (data_size < dec->bottom_size) {
        dec->audio_new_rate += PCM_RATE_INC_STEP;
        /*printf("rate inc\n");*/
    }

    if (data_size > dec->top_size) {
        dec->audio_new_rate -= PCM_RATE_DEC_STEP;
        /*printf("rate dec : %d\n", __this->audio_new_rate);*/
    }

    if (dec->audio_new_rate < dec->audio_min_speed) {
        dec->audio_new_rate = dec->audio_min_speed;
    } else if (dec->audio_new_rate > dec->audio_max_speed) {
        dec->audio_new_rate = dec->audio_max_speed;
    }

    audio_hw_src_set_rate(dec->src_sync, dec->source_sr, dec->audio_new_rate);

    return 0;
}

static int pcm_dec_probe_handler(struct audio_decoder *decoder)
{
    struct s_pcm_dec *dec = container_of(decoder, struct s_pcm_dec, decoder);
#if 10
    if (!dec->sync_start) {
        if (cbuf_get_data_len(dec->dec_pcm_cbuf) > dec->begin_size) {
            dec->sync_start = 1;
            /* printf("sync start \n"); */
            return 0;
        } else {
            /* printf("decode suspend \n"); */
            audio_decoder_suspend(&dec->decoder, 0);
            return -EINVAL;
        }
    }

    pcm_dec_stream_sync(dec, cbuf_get_data_len(dec->dec_pcm_cbuf));
#endif
    /* printf("bs%d;data len %d",dec->begin_size,cbuf_get_data_len(dec->dec_pcm_cbuf)); */
    return 0;
}

static int pcm_dec_output_handler(struct audio_decoder *decoder, s16 *data, int len, void *priv)
{
    int wlen = 0;
    int rlen = len;
    struct s_pcm_dec *dec = container_of(decoder, struct s_pcm_dec, decoder);
    do {
        /* wlen = pcm_dec_output(decoder, data, rlen, priv); */
        /* wlen = audio_mixer_ch_write(&p_alink_hdl[module]->dec->mix_ch, data, rlen); */
        if (dec->src_sync) {
            wlen = audio_src_resample_write(dec->src_sync, data, rlen);
        } else {
            wlen = audio_mixer_ch_write(&dec->mix_ch, data, rlen);
        }

        if (!wlen) {
            /* putchar('O'); */
            break;
        }
        data += wlen / 2 ;
        rlen -= wlen;
    } while (rlen);
    /* printf("iis dec,l:%d, wl:%d \n", len, len-rlen); */
    return (len - rlen);
}

static const struct audio_dec_handler pcm_dec_handler = {
    .dec_probe = pcm_dec_probe_handler,
    .dec_output = pcm_dec_output_handler,
    /* .dec_post   = linein_dec_post_handler, */
};
static int pcm_fread(struct audio_decoder *decoder, void *buf, u32 len)
{
    int rlen = 0;
    int i, j;
    s16 *data = (s16 *)buf;
    u8 module = ALINK_PORT_TO_MODULE(TCFG_IIS_INPUT_PORT);
    struct s_pcm_dec *dec = container_of(decoder, struct s_pcm_dec, decoder);

    switch (dec->source_ch_num) {
    case 0://1通道  立体声数据
    case 1://1通道  单声道数据
#if 10
        /* rlen = cbuf_read(&dec->pcm_cbuf[], (void *)((int)buf + (len / 2)), len / 2); */
        if (ch_en_bitmap[module]&BIT(0)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[0], buf, len);
        }
        if (ch_en_bitmap[module]&BIT(1)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[1], buf, len);
        }
        if (ch_en_bitmap[module]&BIT(2)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[2], buf, len);
        }
        if (ch_en_bitmap[module]&BIT(3)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[3], buf, len);
        }
#endif
        break;
    case 2:
        j = 0;
        if (ch_en_bitmap[module]&BIT(0)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[0], dec->dec_pcm_buf, len / 3);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 3] = dec->dec_pcm_buf[i];
                }
                j++;
            }

        }
        if (ch_en_bitmap[module]&BIT(1)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[1], dec->dec_pcm_buf, len / 3);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 2 + j] = dec->dec_pcm_buf[i];
                }
            }
            j++;
        }
        if (j == dec->source_ch_num) {
            break;
        }
        if (ch_en_bitmap[module]&BIT(2)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[2], dec->dec_pcm_buf, len / 3);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 2 + j] = dec->dec_pcm_buf[i];
                }
            }
            j++;
        }
        if (j == dec->source_ch_num) {
            break;
        }
        if (ch_en_bitmap[module]&BIT(3)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[3], dec->dec_pcm_buf, len / 3);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 2 + j] = dec->dec_pcm_buf[i];
                }
            }
        }
        break;
    case 3:
        j = 0;
        if (ch_en_bitmap[module]&BIT(0)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[0], dec->dec_pcm_buf, len / 2);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 3] = dec->dec_pcm_buf[i];
                }
                j++;
            }

        }
        if (ch_en_bitmap[module]&BIT(1)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[1], dec->dec_pcm_buf, len / 2);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 3 + j] = dec->dec_pcm_buf[i];
                }
            }
            j++;
        }
        if (ch_en_bitmap[module]&BIT(2)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[2], dec->dec_pcm_buf, len / 2);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 3 + j] = dec->dec_pcm_buf[i];
                }
            }
            j++;
        }
        if (j == dec->source_ch_num) {
            break;
        }
        if (ch_en_bitmap[module]&BIT(3)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[3], dec->dec_pcm_buf, len / 2);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 3 + j] = dec->dec_pcm_buf[i];
                }
            }
        }
        break;
    case 4:
        if (ch_en_bitmap[module]&BIT(0)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[0], dec->dec_pcm_buf, len / 4);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 4] = dec->dec_pcm_buf[i];
                }
            }

        }
        if (ch_en_bitmap[module]&BIT(1)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[1], dec->dec_pcm_buf, len / 4);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 4 + 1] = dec->dec_pcm_buf[i];
                }
            }

        }
        if (ch_en_bitmap[module]&BIT(2)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[2], dec->dec_pcm_buf, len / 4);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 4 + 2] = dec->dec_pcm_buf[i];
                }
            }

        }
        if (ch_en_bitmap[module]&BIT(3)) {
            rlen = cbuf_read(&p_alink_hdl[module]->pcm_cbuf[3], dec->dec_pcm_buf, len / 4);
            if (rlen) {
                for (i = 0; i < rlen / 2; i++) {
                    data[i * 4 + 3] = dec->dec_pcm_buf[i];
                }
            }

        }
        break;
    }

    if (rlen == 0) {
        /* putchar('R'); */
        /* memset(buf,0,len); */
        /* return len; */
        return -1;
    }
    /* printf("fread len %d %d\n",len,rlen); */
    return rlen;
}
static const struct audio_dec_input pcm_input = {
    .coding_type = AUDIO_CODING_PCM,
    .data_type   = AUDIO_INPUT_FILE,
    .ops = {
        .file = {
#if VFS_ENABLE == 0
#undef fread
#undef fseek
#undef flen
#endif
            .fread = pcm_fread,
            /* .fseek = file_fseek, */
            /* .flen  = linein_flen, */
        }
    }
};

static int pcm_dec_start(void)
{
    int err;
    struct audio_fmt f;
    u8 module = ALINK_PORT_TO_MODULE(TCFG_IIS_INPUT_PORT);
    struct s_pcm_dec *dec = p_alink_hdl[module]->dec;

    printf("\n--func=%s\n", __FUNCTION__);
    err = audio_decoder_open(&dec->decoder, &pcm_input, &decode_task);
    if (err) {
        goto __err1;
    }
    dec->out_ch_num = audio_output_channel_num();//AUDIO_CH_MAX;

    audio_decoder_set_handler(&dec->decoder, &pcm_dec_handler);
    audio_decoder_set_event_handler(&dec->decoder, pcm_dec_event_handler, pcm_dec_maigc++);


    f.coding_type = AUDIO_CODING_PCM;
    f.sample_rate = TCFG_IIS_INPUT_SR;
    f.channel = (dec->source_ch_num == 0) ? 2 : dec->source_ch_num;
    printf("iis dec ch %d \n", f.channel);
    err = audio_decoder_set_fmt(&dec->decoder, &f);
    if (err) {
        goto __err2;
    }

    audio_mixer_ch_open(&dec->mix_ch, &mixer);
    /* audio_mixer_ch_set_sample_rate(&dec->mix_ch, audio_output_rate(f.sample_rate)); */

    dec->src_out_sr = audio_output_rate(f.sample_rate);
    /* dec->src_out_sr = audio_output_rate(audio_mixer_get_sample_rate(&mixer)); */
    audio_mixer_ch_set_sample_rate(&dec->mix_ch, dec->src_out_sr);
    printf("s sr%d; d sr%d", f.sample_rate, dec->src_out_sr);
    pcm_dec_sync_init(dec);

    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);

    /* audio_adc_mic_start(&dec->mic_ch); */
    printf("\n\n audio decoder start \n");
    audio_decoder_set_run_max(&dec->decoder, 10);
    err = audio_decoder_start(&dec->decoder);
    if (err) {
        goto __err3;
    }
    /* dec->status = REVERB_STATUS_START; */
    /* printf("\n\n audio mic start  1 \n"); */
    return 0;
__err3:
    if (dec->src_sync) {
        audio_hw_resample_close(dec->src_sync);
        dec->src_sync = NULL;
    }
__err2:
    /* audio_decoder_close(&dec->decoder); */
__err1:
    audio_decoder_task_del_wait(&decode_task, &dec->wait);
    return err;
}


static int pcmdec_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;;
    log_i("pcmdec_wait_res_handler, event:%d;status:; \n", event);
    u8 module = ALINK_PORT_TO_MODULE(TCFG_IIS_INPUT_PORT);
    if (event == AUDIO_RES_GET) {
        err = pcm_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        pcm_dec_close(module);
    }

    return err;
}
static void pcm_dec_relaese(u8 module)
{
    struct s_pcm_dec *dec = p_alink_hdl[module]->dec;
    if (dec) {
        audio_decoder_task_del_wait(&decode_task, &dec->wait);
    }
}
static void pcm_dec_close(u8 module)
{
    struct s_pcm_dec *dec = p_alink_hdl[module]->dec;
    printf("\n--func=%s\n", __FUNCTION__);
    if (dec) {
        audio_decoder_close(&dec->decoder);
        if (dec->src_sync) {
            audio_hw_resample_close(dec->src_sync);
            dec->src_sync = NULL;
        }
        audio_mixer_ch_close(&dec->mix_ch);
    }
}

#endif
/*************************pcm dec end*********************************/
/*********************alink api**************************************/
// alink 输出数据的写入接口解

/* int audio_link0_write_stereodata(s16 *buf,int len) */
int audio_link_write_stereodata(s16 *buf, int len, u8 alink_port)
{
    u8 module = ALINK_PORT_TO_MODULE(alink_port);
    int wlen = 0;
    if (!p_alink_hdl[module]) {
        printf("alink module [%d] not open \n", module);
        return 0;
    }
    if (ch_en_bitmap[module]&BIT(0)) {
        wlen = cbuf_write(&p_alink_hdl[module]->pcm_cbuf[0], buf, len);
    }
    if (ch_en_bitmap[module]&BIT(1)) {
        wlen = cbuf_write(&p_alink_hdl[module]->pcm_cbuf[1], buf, len);
    }
    if (ch_en_bitmap[module]&BIT(2)) {
        wlen = cbuf_write(&p_alink_hdl[module]->pcm_cbuf[2], buf, len);
    }
    if (ch_en_bitmap[module]&BIT(3)) {
        wlen = cbuf_write(&p_alink_hdl[module]->pcm_cbuf[3], buf, len);
    }
    /* printf("alink write data len:%d,success%d \n",len,wlen); */
    return wlen;
}


int audio_link_write_monodata(s16 *front_ldata, s16 *front_rdata, s16 *rear_ldata, s16 *rear_rdata, int len, u8 alink_port)
{
    int wlen = 0;
    u8 module = ALINK_PORT_TO_MODULE(alink_port);
    if (!p_alink_hdl[module]) {
        printf("alink module 1 not open \n");
        return 0;
    }
    if (front_ldata && (ch_en_bitmap[module]&BIT(0))) {
        wlen = cbuf_write(&p_alink_hdl[module]->pcm_cbuf[0], front_ldata, len);
    }
    if (front_rdata && (ch_en_bitmap[module]&BIT(1))) {
        wlen = cbuf_write(&p_alink_hdl[module]->pcm_cbuf[1], front_rdata, len);
    }
    if (rear_ldata && (ch_en_bitmap[module]&BIT(2))) {
        wlen = cbuf_write(&p_alink_hdl[module]->pcm_cbuf[2], rear_ldata, len);
    }
    if (rear_rdata && (ch_en_bitmap[module]&BIT(3))) {
        wlen = cbuf_write(&p_alink_hdl[module]->pcm_cbuf[3], rear_rdata, len);
    }

    return wlen;
}

///////////////////////////

int audio_link_init(void)
{
    u8 i;
    for (i = 0; i < ALINK_MODULE_NUM; i++) {
        p_alink_hdl[i] = NULL;
    }
    return 0;
}

int audio_link_open(u8 alink_port, u8 dir)
{
    u8 i;
    int err;
    u8 module = ALINK_PORT_TO_MODULE(alink_port);
    if (p_alink_hdl[module]) {
        printf("iis module:%d is working or not close \n");
        return -1;
    }
    struct s_alink_hdl *alink_hdl = zalloc(sizeof(struct s_alink_hdl));
    if (!alink_hdl) {
        printf("iis malloc err \n");
        return -1;
    }
    p_alink_hdl[module] =  alink_hdl;
    alink_hdl->param.isr_cbfun = (alink_port < 2) ? iis0_cb : iis1_cb;
    alink_hdl->param.port = alink_port;
    /* alink_hdl->param.rate = 44100; */
    /* alink_hdl->param.soe = 1; */
    alink_hdl->param.moe = 0,
                     /* alink_hdl->param.dsp = 0;//基本模式 只支持双声道	 */
                     alink_hdl->param.frame_len = IIS_POINT_NUM;
    struct s_pcm_dec *dec = NULL;
    if (dir == ALINK_DIR_RX) { //输入暂时只支持单通道 立体声输入
        printf("iss rx mode ");
        alink_hdl->param.soe = 0;//从机
        alink_hdl->param.rate = TCFG_IIS_INPUT_SR;
        alink_hdl->param.dsp = TCFG_IIS_INPUT_CH_NUM ? 0 : 1; //基本模式 只支持双声道
        ch_en_bitmap[module] = TCFG_IIS_INPUT_DATAPORT_SEL;
#if PCM_DEC_ENABLE
        //open pcm dec
        dec = zalloc(sizeof(struct s_pcm_dec));
        if (!dec) {
            printf("iis dec malloc err\n");
            goto err;
        }
        /* cbuf_init(&dec->dec_pcm_cbuf,dec->dec_pcm_buf,4*1024); */
        dec->source_sr = alink_hdl->param.rate;
        alink_hdl->dec = dec;
        //0: 立体声；    1个通道
        //1：单声道数据  1个通道
        //2：单声道数据  2个通道
        //3：单声道数据  3个通道
        //4：单声道数据  4个通道
        // 通道数 要与 TCFG_IIS_INPUT_DATAPORT_SEL 选择bit数对应
        dec->source_ch_num = 0;

        dec->wait.priority = 1;
        dec->wait.preemption = 1;
        dec->wait.protect = 0;
        dec->wait.handler = pcmdec_wait_res_handler;
#endif
    } else {
        /* alink_hdl->param.rate = 44100; */
        alink_hdl->param.rate = TCFG_IIS_OUTPUT_SR;
        alink_hdl->param.soe = 1;//主机
        alink_hdl->param.dsp = TCFG_IIS_OUTPUT_CH_NUM ? 0 : 1; //立体声走基本模式:0; 单声道走扩展模式:1
        ch_en_bitmap[module] = TCFG_IIS_OUTPUT_DATAPORT_SEL;
    }

    for (i = 0; i < ALINK_CH_MAX; i++) {
        if (BIT(i)&ch_en_bitmap[module]) {
            printf("iis ch[%d] enable", i);
            alink_hdl->ch_idx = i;
            alink_hdl->p_data_buf[i] = malloc(CH_CBUF_DATA_LEN);
            alink_hdl->p16_dma_buf[i] = malloc(2 * IIS_POINT_NUM * 2 * 2); //双buf + 16bit + 立体声
            /* alink_hdl->p32_dma_buf[i] = malloc(2*IIS_POINT_NUM*4*2);//双buf + 32bit + 立体声  */
            if (!alink_hdl->p_data_buf[i] || !alink_hdl->p16_dma_buf[i]) {
                goto err;
            }
            cbuf_init(&alink_hdl->pcm_cbuf[i], alink_hdl->p_data_buf[i], CH_CBUF_DATA_LEN);
            if (dec) {
                /* memcpy(&dec->dec_pcm_cbuf,&alink_hdl->pcm_cbuf[i],sizeof(cbuffer_t)); */
                dec->dec_pcm_cbuf = &alink_hdl->pcm_cbuf[i];
                printf("\n\n\n-----cbuf total len:%d -------\n\n\n", dec->dec_pcm_cbuf->total_len);
            }
            alink_hdl->param.ch[i].enable = 1;
            alink_hdl->param.ch[i].dir = dir ? ALINK_DIR_RX : ALINK_DIR_TX;
            alink_hdl->param.ch[i].bit_wide = ALINK_LEN_16BIT;
            if (alink_hdl->param.dsp) {
                alink_hdl->param.ch[i].ch_md = ALINK_MD_DSP0;
            } else {
                alink_hdl->param.ch[i].ch_md = ALINK_MD_IIS;
            }

            alink_hdl->param.ch[i].dma_adr = alink_hdl->p16_dma_buf[i];
        }
    }

    if (dec) {
        err = audio_decoder_task_add_wait(&decode_task, &dec->wait);
    }
    if (iis_open(&alink_hdl->param) == 0) {
        g_printf("iis open succes\n");
    } else {
        g_printf("iis open error \n");
    }
    alink_hdl->state = ALINK_STATUS_START;
    return 0;
err:
    for (i = 0; i < ALINK_CH_MAX; i++) {
        if (alink_hdl->p_data_buf[i]) {
            free(alink_hdl->p_data_buf[i]);
        }
        if (alink_hdl->p16_dma_buf[i]) {
            free(alink_hdl->p_data_buf[i]);
        }
    }
    return -1;
}

int audio_link_close(u8 alink_port, u8 dir)
{
    u8 i;
    int err;
    u8 module = ALINK_PORT_TO_MODULE(alink_port);
    ch_en_bitmap[module] = 0;
    close(module);
    printf("\n--func=%s\n", __FUNCTION__);
    if (!p_alink_hdl[module]) {
        return 0;
    }
    if (dir == ALINK_DIR_RX) {
#if PCM_DEC_ENABLE
        pcm_dec_close(module);
        pcm_dec_relaese(module);
        free(p_alink_hdl[module]->dec);
#endif
    } else {

    }
    struct s_alink_hdl *alink_hdl = p_alink_hdl[module];
    for (i = 0; i < ALINK_CH_MAX; i++) {
        if (alink_hdl->p_data_buf[i]) {
            free(alink_hdl->p_data_buf[i]);
        }
        if (alink_hdl->p16_dma_buf[i]) {
            free(alink_hdl->p16_dma_buf[i]);
        }
    }
    p_alink_hdl[module] = NULL;
    free(alink_hdl);
    iis_module_data[module].iis_use  = 0;
    return 0;
}
void audio_link_switch_sr(u8 alink_port, u32 rate)
{
    u8 module = ALINK_PORT_TO_MODULE(alink_port);
    if (!p_alink_hdl[module]) {
        return;
    }
    ALINK_EN((module ? 1 : 0), 0);
    iis_set_sr(alink_port, rate, p_alink_hdl[module]->param.soe);              //设置采样率
    ALINK_EN((module ? 1 : 0), 1);
}
#endif
