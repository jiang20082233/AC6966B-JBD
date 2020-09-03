
#ifndef _AUDIO_PITCH_API_H_
#define _AUDIO_PITCH_API_H_
#include "pitchshifter/pitchshifter_api.h"
#include "mono2stereo/reverb_mono2stero_api.h"
#include "media/audio_stream.h"
typedef struct _s_pitch_hdl {

    PITCHSHIFT_FUNC_API *ops;
    RMONO2STEREO_FUNC_API *mono2stereo_ops;
    u8 *databuf;
    PITCH_SHIFT_PARM parm;
    void *mono2stereo_buf;
    s16 *signal_buf;
    // s16 signal_buf[512];

    struct audio_stream_entry entry;	// 音频流入口
    int out_len;
    int process_len;
    u8 run_en;
} s_pitch_hdl;

PITCH_SHIFT_PARM *get_pitch_parm(void);
s_pitch_hdl *open_pitch(PITCH_SHIFT_PARM *param);
void close_pitch(s_pitch_hdl *picth_hdl);
void update_pict_parm(s_pitch_hdl *picth_hdl);
void pitch_run(s_pitch_hdl *picth_hdl, s16 *indata, s16 *outdata, int len, u8 ch_num);
void pause_pitch(s_pitch_hdl *pitch_hdl, u8 run_mark);
#endif

