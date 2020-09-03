
#ifndef _AUDIO_REVERB_API_H_
#define _AUDIO_REVERB_API_H_
#include "pitchshifter/pitchshifter_api.h"
#include "mono2stereo/reverb_mono2stero_api.h"
// #include "reverb/reverb_api.h"
#include "application/audio_echo_reverb.h"
// #include "asm/howling_api.h"

#include "audio_config.h"


// REVERBN_API_STRUCT *open_reverb(REVERBN_PARM_SET *reverb_seting, u16 sample_rate);
// void  close_reverb(REVERB_API_STRUCT *reverb_api_obj);
// void update_reverb_parm(REVERB_API_STRUCT *reverb_api_obj, REVERB_PARM_SET *reverb_seting);

// HOWLING_API_STRUCT *open_howling(HOWLING_PARM_SET *howl_para, u16 sample_rate, u8 channel);
// void close_howling(HOWLING_API_STRUCT *holing_hdl);

void start_reverb_mic2dac(struct audio_fmt *fmt);
int reverb_if_working(void);
void stop_reverb_mic2dac(void);
void set_mic_gain_up(u8 value);
void set_mic_gain_down(u8 value);
void set_reverb_deepval_up(u16 value);
void set_reverb_deepval_down(u16 value);
void reset_reverb_src_out(u16 s_rate);
void reverb_pause(void);
void reverb_resume(void);
void set_reverb_decayval_up(u16 value);
void set_reverb_decayval_down(u16 value);
#endif

