
#ifndef _AUDIO_IIS_API_H_
#define _AUDIO_IIS_API_H_

#include "audio_config.h"
#include "asm/iis.h"
#include "system/includes.h"
#include "media/includes.h"
enum {
    ALINK_STATUS_INIT = 0,
    ALINK_STATUS_START,
    ALINK_STATUS_PAUSE,
    ALINK_STATUS_STOP,
};
enum {
    ALINK_
};

struct s_pcm_dec {
    u32 out_ch_num : 2;
    u32 source_ch_num : 2;
    u32 source_ch_mono : 2;
    u16 source_sr;
    u16 src_out_sr;
    struct audio_decoder decoder;
    struct audio_res_wait wait;
    struct audio_mixer_ch mix_ch;
    struct audio_src_handle *src_sync;
    cbuffer_t *dec_pcm_cbuf;
    s16 dec_pcm_buf[512];
    int begin_size;
    int top_size;
    int bottom_size;
    u16 audio_new_rate;
    u16 audio_max_speed;
    u16 audio_min_speed;
    u8 sync_start;
};
struct s_alink_hdl {
    // s16 *p_alink_dma[4];
    cbuffer_t pcm_cbuf[ALINK_CH_MAX];
    u8 *p_data_buf[ALINK_CH_MAX];
    s16 *p16_dma_buf[ALINK_CH_MAX];
    s32 *p32_dma_buf[ALINK_CH_MAX];
    iis_param  param;
    u8 state : 4;
    u8 ch_idx: 4;
    struct s_pcm_dec *dec;
};

int audio_link_init(void);
int audio_link_open(u8 alink_port, u8 dir);
int audio_link_close(u8 alink_port, u8 dir);
void audio_link_switch_sr(u8 alink_port, u32 rate);
int audio_link_write_stereodata(s16 *buf, int len, u8 alink_port);
int audio_link_write_monodata(s16 *front_ldata, s16 *front_rdata, s16 *rear_ldata, s16 *rear_rdata, int len, u8 alink_port);
#endif

