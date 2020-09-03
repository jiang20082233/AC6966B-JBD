#include "app_config.h"
#include "system/includes.h"
#include "printf.h"
#if TCFG_PC_ENABLE
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"
#include "usb/device/uac_audio.h"
#include "uac_stream.h"
#include "audio_config.h"
#include "usb_config.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"



static volatile u8 speaker_stream_is_open = 0;
struct uac_speaker_handle {
    cbuffer_t cbuf;
    void *buffer;
    void (*rx_handler)(int, void *, int);
};
#define UAC_BUFFER_SIZE     (8 * 1024)

static struct uac_speaker_handle *uac_speaker = NULL;

#if USB_MALLOC_ENABLE

#else
static struct uac_speaker_handle uac_speaker_handle SEC(.uac_var);
static u8 uac_rx_buffer[UAC_BUFFER_SIZE] ALIGNED(4) SEC(.uac_rx);
#endif
u32 uac_speaker_stream_length()
{
    return UAC_BUFFER_SIZE;
}
u32 uac_speaker_stream_size()
{
    if (!speaker_stream_is_open) {
        return 0;
    }

    if (uac_speaker) {
        return cbuf_get_data_size(&uac_speaker->cbuf);
    }

    return 0;
}

void uac_speaker_stream_buf_clear(void)
{
    if (speaker_stream_is_open) {
        cbuf_clear(&uac_speaker->cbuf);
    }
}

void set_uac_speaker_rx_handler(void *priv, void (*rx_handler)(int, void *, int))
{
    if (uac_speaker) {
        uac_speaker->rx_handler = rx_handler;
    }
}

void uac_speaker_stream_write(const u8 *obuf, u32 len)
{
    if (speaker_stream_is_open) {
        //write dac
        int wlen = cbuf_write(&uac_speaker->cbuf, (void *)obuf, len);
        if (wlen != len) {
            putchar('W');
        }
        if (uac_speaker->rx_handler) {
            uac_speaker->rx_handler(0, (void *)obuf, len);
        }
    }
}

int uac_speaker_read(void *priv, void *data, u32 len)
{
    int r_len;
    int err = 0;

    local_irq_disable();
    if (!speaker_stream_is_open) {
        local_irq_enable();
        return 0;
    }

    r_len = cbuf_get_data_size(&uac_speaker->cbuf);
    if (r_len) {
        r_len = r_len > len ? len : r_len;
        r_len = cbuf_read(&uac_speaker->cbuf, data, r_len);
        if (!r_len) {
            putchar('U');
        }
    }
    local_irq_enable();

    return r_len;
}

void uac_speaker_stream_open(u32 samplerate, u32 ch)
{
    if (speaker_stream_is_open) {
        return;
    }
    log_info("%s", __func__);

    if (!uac_speaker) {
#if USB_MALLOC_ENABLE

        uac_speaker = zalloc(sizeof(struct uac_speaker_handle));
        if (!uac_speaker) {
            return;
        }

        uac_speaker->buffer = malloc(UAC_BUFFER_SIZE);
        if (!uac_speaker->buffer) {
            free(uac_speaker);
            uac_speaker = NULL;
            goto __err;
        }


#else

        uac_speaker = &uac_speaker_handle;

        memset(uac_speaker, 0, sizeof(struct uac_speaker_handle));

        uac_speaker->buffer = uac_rx_buffer;
#endif
    }


    uac_speaker->rx_handler = NULL;

    cbuf_init(&uac_speaker->cbuf, uac_speaker->buffer, UAC_BUFFER_SIZE);
    speaker_stream_is_open = 1;
    struct sys_event event;
    event.type = SYS_DEVICE_EVENT;
    event.arg = (void *)DEVICE_EVENT_FROM_UAC;
    event.u.dev.event = USB_AUDIO_PLAY_OPEN;
    event.u.dev.value = (int)((ch << 24) | samplerate);

    sys_event_notify(&event);

    return;

__err:
    return;
}

void uac_speaker_stream_close()
{
    if (speaker_stream_is_open == 0) {
        return;
    }

    log_info("%s", __func__);
    speaker_stream_is_open = 0;

    if (uac_speaker) {
#if USB_MALLOC_ENABLE
        if (uac_speaker->buffer) {
            free(uac_speaker->buffer);
        }
        free(uac_speaker);
#endif
        uac_speaker = NULL;
    }
    struct sys_event event;
    event.type = SYS_DEVICE_EVENT;
    event.arg = (void *)DEVICE_EVENT_FROM_UAC;
    event.u.dev.event = USB_AUDIO_PLAY_CLOSE;
    event.u.dev.value = (int)0;

    sys_event_notify(&event);
}

int __attribute__((weak)) uac_vol_switch(int vol)
{

    u16 valsum = vol * 32 / 100;

    if (valsum > 31) {
        valsum = 31;
    }
    return valsum;
}

int uac_get_spk_vol()
{
#if USB_DEVICE_CLASS_CONFIG & AUDIO_CLASS
    int max_vol = get_max_sys_vol();
    int vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    if (vol * 100 / max_vol < 100) {
        return vol * 100 / max_vol;
    } else {
        return 99;
    }
#endif
    return 0;
}
static u32 mic_stream_is_open;
void uac_mute_volume(u32 type, u32 l_vol, u32 r_vol)
{
    struct sys_event event;
    event.type = SYS_DEVICE_EVENT;
    event.arg = (void *)DEVICE_EVENT_FROM_UAC;

    /* int valsum = 0; */
    /* static int lastvol = -1; */
    int l_valsum, r_valsum;
    static u32 last_spk_l_vol = (u32) - 1, last_spk_r_vol = (u32) - 1;
    static u32 last_mic_vol = (u32) - 1;

    switch (type) {
    case MIC_FEATURE_UNIT_ID: //MIC
        if (mic_stream_is_open == 0) {
            return ;
        }
        if (l_vol == last_mic_vol) {
            return;
        }
        last_mic_vol = l_vol;
        l_vol /= 2;
        l_vol += 6;
        event.u.dev.event = USB_AUDIO_SET_MIC_VOL;
        break;
    case SPK_FEATURE_UNIT_ID: //SPK
        if (speaker_stream_is_open == 0) {
            return;
        }
        if (l_vol == last_spk_l_vol && r_vol == last_spk_r_vol) {
            return;
        }
        last_spk_l_vol = l_vol;
        last_spk_r_vol = r_vol;
        event.u.dev.event = USB_AUDIO_SET_PLAY_VOL;
        break;
    default:
        break;
    }
    /* valsum = uac_vol_switch(vol); */
    /* if (lastvol == valsum) { */
    /* return; */
    /* } */
    /* lastvol = valsum; */

    /* event.u.dev.value = (int)valsum; */
    l_valsum = uac_vol_switch(l_vol);
    r_valsum = uac_vol_switch(r_vol);

    event.u.dev.value = (int)(r_valsum << 16 | l_valsum);
    sys_event_notify(&event);
}


static int (*mic_tx_handler)(int, void *, int) SEC(.uac_rx);
int uac_mic_stream_read(u8 *buf, u32 len)
{
    if (mic_stream_is_open == 0) {
        return 0;
    }
#if 0//48K 1ksin
    const s16 sin_48k[] = {
        0, 2139, 4240, 6270, 8192, 9974, 11585, 12998,
        14189, 15137, 15826, 16244, 16384, 16244, 15826, 15137,
        14189, 12998, 11585, 9974, 8192, 6270, 4240, 2139,
        0, -2139, -4240, -6270, -8192, -9974, -11585, -12998,
        -14189, -15137, -15826, -16244, -16384, -16244, -15826, -15137,
        -14189, -12998, -11585, -9974, -8192, -6270, -4240, -2139
    };
    u16 *l_ch = (u16 *)buf;
    u16 *r_ch = (u16 *)buf;
    r_ch++;
    for (int i = 0; i < len / 4; i++) {
        *l_ch = sin_48k[i];
        *r_ch = sin_48k[i];
        l_ch += 2;
        r_ch += 2;
    }
    /* memcpy(buf, sin_48k, len); */
#else
    if (mic_tx_handler) {
        return mic_tx_handler(0, buf, len);
    }
    return 0;
#endif
    return 0;
}

void set_uac_mic_tx_handler(void *priv, int (*tx_handler)(int, void *, int))
{
    mic_tx_handler = tx_handler;
}

u32 uac_mic_stream_open(u32 samplerate, u32 frame_len, u32 ch)
{
    if (mic_stream_is_open) {
        return 0;
    }

    mic_tx_handler = NULL;
    log_info("%s", __func__);

    struct sys_event event;
    event.type = SYS_DEVICE_EVENT;
    event.arg = (void *)DEVICE_EVENT_FROM_UAC;
    event.u.dev.event = USB_AUDIO_MIC_OPEN;
    event.u.dev.value = (int)((ch << 24) | samplerate);
    mic_stream_is_open = 1;
    sys_event_notify(&event);
    return 0;
}

void uac_mic_stream_close()
{
    if (mic_stream_is_open == 0) {
        return ;
    }
    log_info("%s", __func__);
    struct sys_event event;
    event.type = SYS_DEVICE_EVENT;
    event.arg = (void *)DEVICE_EVENT_FROM_UAC;
    event.u.dev.event = USB_AUDIO_MIC_CLOSE;
    event.u.dev.value = (int)0;
    mic_stream_is_open = 0;
    sys_event_notify(&event);
}
#endif

