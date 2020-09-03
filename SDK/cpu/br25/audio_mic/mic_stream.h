#ifndef __MIC_STREAM_H__
#define __MIC_STREAM_H__

#include "system/includes.h"

enum {
    MIC_EFFECT_CONFIG_REVERB = 0x0,
    MIC_EFFECT_CONFIG_ECHO,
    MIC_EFFECT_CONFIG_PITCH,
    MIC_EFFECT_CONFIG_NOISEGATE,
    MIC_EFFECT_CONFIG_DODGE,
    MIC_EFFECT_CONFIG_DVOL,
    MIC_EFFECT_CONFIG_HOWLING,
    MIC_EFFECT_CONFIG_FILT,
    MIC_EFFECT_CONFIG_EQ,
};


struct __mic_stream_io {
    void *priv;
    u32(*func)(void *priv, void *in, void *out, u32 inlen, u32 outlen);
};

struct __mic_stream_parm {
    u16 sample_rate;
    u16 point_unit;
    u16	dac_delay;
};

typedef struct __mic_stream mic_stream;

u32 mic_stream_adc_write(u8 *data, u32 len);
mic_stream *mic_stream_creat(struct __mic_stream_parm *parm);
void mic_stream_destroy(mic_stream **hdl);
bool mic_stream_start(mic_stream  *stream);
void mic_stream_set_output(struct __mic_stream  *stream, void *priv, u32(*func)(void *priv, void *in, void *out, u32 inlen, u32 outlen));

#endif// __MIC_STREAM_H__
