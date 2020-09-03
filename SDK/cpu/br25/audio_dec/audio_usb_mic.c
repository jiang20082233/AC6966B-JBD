#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "device/uac_stream.h"
#include "audio_enc.h"
#include "app_main.h"
#include "user_cfg_id.h"

/*usb mic的数据是否经过AEC,包括里面的ANS模块*/
#define USB_MIC_AEC_EN				0
#if USB_MIC_AEC_EN
#include "aec_user.h"
#endif/*USB_MIC_AEC_EN*/

#define PCM_ENC2USB_OUTBUF_LEN		(5 * 1024)

#define USB_MIC_BUF_NUM        3
#define USB_MIC_CH_NUM         1
#define USB_MIC_IRQ_POINTS     256
#define USB_MIC_BUFS_SIZE      (USB_MIC_CH_NUM * USB_MIC_BUF_NUM * USB_MIC_IRQ_POINTS)

#define USB_MIC_STOP  0x00
#define USB_MIC_START 0x01

extern struct audio_adc_hdl adc_hdl;

struct _usb_mic_hdl {
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch    mic_ch;
    struct audio_adc_ch linein_ch;
    s16 *adc_buf;//[USB_MIC_BUFS_SIZE];
    enum enc_source source;

    cbuffer_t output_cbuf;
    u8 *output_buf;//[PCM_ENC2USB_OUTBUF_LEN];
    u8 rec_tx_channels;
    u8 mic_data_ok;/*mic数据等到一定长度再开始发送*/
    u8 status;
};

static struct _usb_mic_hdl *usb_mic_hdl = NULL;
static int usb_audio_mic_sync(u32 data_size)
{
#if 0
    int change_point = 0;

    if (data_size > __this->rec_top_size) {
        change_point = -1;
    } else if (data_size < __this->rec_bottom_size) {
        change_point = 1;
    }

    if (change_point) {
        struct audio_pcm_src src;
        src.resample = 0;
        src.ratio_i = (1024) * 2;
        src.ratio_o = (1024 + change_point) * 2;
        src.convert = 1;
        dev_ioctl(__this->rec_dev, AUDIOC_PCM_RATE_CTL, (u32)&src);
    }
#endif

    return 0;
}

static int usb_audio_mic_tx_handler(int event, void *data, int len)
{
    if (usb_mic_hdl == NULL) {
        return 0;
    }
    if (usb_mic_hdl->status == USB_MIC_STOP) {
        return 0;
    }

    int i = 0;
    int r_len = 0;
    u8 ch = 0;
    u8 double_read = 0;

    /* int rlen = 0; */

    if (usb_mic_hdl->mic_data_ok == 0) {
        if (usb_mic_hdl->output_cbuf.data_len > (PCM_ENC2USB_OUTBUF_LEN / 4)) {
            usb_mic_hdl->mic_data_ok = 1;
        }
        //y_printf("mic_tx NULL\n");
        memset(data, 0, len);
        return len;
    }

    /* usb_audio_mic_sync(size); */
    if (usb_mic_hdl->rec_tx_channels == 2) {
        len = cbuf_read(&usb_mic_hdl->output_cbuf, data, len / 2);
        s16 *tx_pcm = (s16 *)data;
        int cnt = len / 2;
        for (cnt = len / 2; cnt >= 2;) {
            tx_pcm[cnt - 1] = tx_pcm[cnt / 2 - 1];
            tx_pcm[cnt - 2] = tx_pcm[cnt / 2 - 1];
            cnt -= 2;
        }
        len *= 2;
    } else {
        len = cbuf_read(&usb_mic_hdl->output_cbuf, data, len);
    }
    return len;
}


int usb_audio_mic_write(void *data, u16 len)
{
    int wlen = len;
    if (usb_mic_hdl) {
        wlen = cbuf_write(&usb_mic_hdl->output_cbuf, data, len);
#if 1
        static u32 usb_mic_data_max = 0;
        if (usb_mic_data_max < usb_mic_hdl->output_cbuf.data_len) {
            usb_mic_data_max = usb_mic_hdl->output_cbuf.data_len;
            y_printf("usb_mic_max:%d", usb_mic_data_max);
        }
#endif
        if (wlen != len) {
            r_printf("usb_mic write full:%d-%d\n", wlen, len);
        }
    }
    return wlen;
}

static void adc_output_to_cbuf(void *priv, s16 *data, int len)
{
    if (usb_mic_hdl == NULL) {
        return ;
    }
    if (usb_mic_hdl->status == USB_MIC_STOP) {
        return ;
    }

    switch (usb_mic_hdl->source) {
    case ENCODE_SOURCE_MIC:
    case ENCODE_SOURCE_LINE0_LR:
    case ENCODE_SOURCE_LINE1_LR:
    case ENCODE_SOURCE_LINE2_LR: {
#if USB_MIC_AEC_EN
        audio_aec_inbuf(data, len);
#else
        int wlen = cbuf_write(&usb_mic_hdl->output_cbuf, data, len);
        if (wlen != len) {
            printf("wlen %d len %d\n", wlen, len);
        }
#endif
    }
    break;
    default:
        break;
    }
}

int usb_audio_mic_open(void *_info)
{
    if (usb_mic_hdl) {
        return 0;
    }
    usb_mic_hdl = zalloc(sizeof(*usb_mic_hdl));
    if (!usb_mic_hdl) {
        return -EFAULT;
    }
    usb_mic_hdl->status = USB_MIC_STOP;
    usb_mic_hdl->adc_buf = zalloc(USB_MIC_BUFS_SIZE * 2);
    if (!usb_mic_hdl->adc_buf) {
        printf("usb_mic_hdl->adc_buf NULL\n");
        if (usb_mic_hdl) {
            free(usb_mic_hdl);
            usb_mic_hdl = NULL;
        }
        return -EFAULT;
    }
    usb_mic_hdl->output_buf = zalloc(PCM_ENC2USB_OUTBUF_LEN);
    if (!usb_mic_hdl->output_buf) {
        printf("usb_mic_hdl->output_buf NULL\n");
        if (usb_mic_hdl->adc_buf) {
            free(usb_mic_hdl->adc_buf);
        }
        if (usb_mic_hdl) {
            free(usb_mic_hdl);
            usb_mic_hdl = NULL;
        }
        return -EFAULT;
    }

    u32 sample_rate = (u32)_info & 0xFFFFFF;
    usb_mic_hdl->rec_tx_channels = (u32)_info >> 24;
    usb_mic_hdl->source = ENCODE_SOURCE_MIC;
    printf("usb mic sr:%d ch:%d\n", sample_rate, usb_mic_hdl->rec_tx_channels);

#if USB_MIC_AEC_EN
    sample_rate = 16000;
    printf("usb mic sr[aec]:%d\n", sample_rate);
    audio_aec_init(sample_rate);
#endif

    cbuf_init(&usb_mic_hdl->output_cbuf, usb_mic_hdl->output_buf, PCM_ENC2USB_OUTBUF_LEN);
#if 0
    audio_adc_mic_open(&usb_mic_hdl->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
    audio_adc_mic_set_sample_rate(&usb_mic_hdl->mic_ch, sample_rate);
    audio_adc_mic_set_gain(&usb_mic_hdl->mic_ch, app_var.usb_mic_gain);
    audio_adc_mic_set_buffs(&usb_mic_hdl->mic_ch, usb_mic_hdl->adc_buf, USB_MIC_IRQ_POINTS * 2, USB_MIC_BUF_NUM);
    usb_mic_hdl->adc_output.handler = adc_output_to_cbuf;
    audio_adc_add_output_handler(&adc_hdl, &usb_mic_hdl->adc_output);
    audio_adc_mic_start(&usb_mic_hdl->mic_ch);
#else
    audio_mic_open(&usb_mic_hdl->mic_ch, sample_rate, app_var.usb_mic_gain);
    usb_mic_hdl->adc_output.handler = adc_output_to_cbuf;
    audio_mic_add_output(&usb_mic_hdl->adc_output);
    audio_mic_start(&usb_mic_hdl->mic_ch);
#endif
    set_uac_mic_tx_handler(NULL, usb_audio_mic_tx_handler);

    usb_mic_hdl->status = USB_MIC_START;
    /* __this->rec_begin = 0; */
    return 0;

    return -EFAULT;
}



/*
 *************************************************************
 *
 *	usb mic gain save
 *
 *************************************************************
 */
static int usb_mic_gain_save_timer;
static u8  usb_mic_gain_save_cnt;
static void usb_audio_mic_gain_save_do(void *priv)
{
    //printf(" usb_audio_mic_gain_save_do %d\n", usb_mic_gain_save_cnt);
    local_irq_disable();
    if (++usb_mic_gain_save_cnt >= 5) {
        sys_hi_timer_del(usb_mic_gain_save_timer);
        usb_mic_gain_save_timer = 0;
        usb_mic_gain_save_cnt = 0;
        local_irq_enable();
        printf("USB_GAIN_SAVE\n");
        syscfg_write(VM_USB_MIC_GAIN, &app_var.usb_mic_gain, 1);
        return;
    }
    local_irq_enable();
}

static void usb_audio_mic_gain_change(u8 gain)
{
    local_irq_disable();
    app_var.usb_mic_gain = gain;
    usb_mic_gain_save_cnt = 0;
    if (usb_mic_gain_save_timer == 0) {
        usb_mic_gain_save_timer = sys_hi_timer_add(NULL, usb_audio_mic_gain_save_do, 1000);
    }
    local_irq_enable();
}

int usb_audio_mic_get_gain(void)
{
    return app_var.usb_mic_gain;
}

void audio_adc_set_mic_gain(struct audio_adc_hdl *adc, u8 gain);
void usb_audio_mic_set_gain(int gain)
{
    if (usb_mic_hdl == NULL) {
        r_printf("usb_mic_hdl NULL gain");
        return;
    }
    audio_adc_mic_set_gain(&usb_mic_hdl->mic_ch, gain);
    audio_adc_set_mic_gain(&adc_hdl, gain);
    usb_audio_mic_gain_change(gain);
}
int usb_audio_mic_close(void *arg)
{
    if (usb_mic_hdl == NULL) {
        r_printf("usb_mic_hdl NULL close");
        return 0;
    }
    printf("usb_mic_hdl->status %x\n", usb_mic_hdl->status);
    if (usb_mic_hdl && usb_mic_hdl->status == USB_MIC_START) {
        printf("usb_audio_mic_close in\n");
        usb_mic_hdl->status = USB_MIC_STOP;
#if USB_MIC_AEC_EN
        audio_aec_close();
#endif
#if 0
        audio_adc_mic_close(&usb_mic_hdl->mic_ch);
        audio_adc_del_output_handler(&adc_hdl, &usb_mic_hdl->adc_output);
#else
        audio_mic_close(&usb_mic_hdl->mic_ch, &usb_mic_hdl->adc_output);
#endif
        cbuf_clear(&usb_mic_hdl->output_cbuf);
        if (usb_mic_hdl) {
            if (usb_mic_hdl->adc_buf) {
                free(usb_mic_hdl->adc_buf);
                usb_mic_hdl->adc_buf = NULL;
            }

            if (usb_mic_hdl->output_buf) {
                free(usb_mic_hdl->output_buf);
                usb_mic_hdl->output_buf = NULL;
            }
            free(usb_mic_hdl);
            usb_mic_hdl = NULL;
        }
    }
    printf("usb_audio_mic_close out\n");

    return 0;
}
