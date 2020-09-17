#ifndef _AUDIO_RECORDER_MIX_H_
#define _AUDIO_RECORDER_MIX_H_

#include "system/includes.h"
#include "media/includes.h"


struct recorder_mix_stream {
    u8						  source;
    struct audio_stream_entry entry;
};

u32 recorder_mix_sco_data_write(u8 *data, u16 len);
void recorder_mix_stream_resume(void);
void recorder_mix_init(struct audio_mixer *mixer, s16 *mix_buf, u16 buf_size);
int recorder_mix_start(void);
void recorder_mix_stop(void);
int recorder_mix_get_status(void);
void recorder_mix_bt_status(u8 status);
void recorder_mix_call_status_change(u8 active);

#endif//_AUDIO_RECORDER_MIX_H_

