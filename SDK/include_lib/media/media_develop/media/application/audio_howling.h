#ifndef _AUDIO_HOWLING_API_H
#define _AUDIO_HOWLING_API_H

#include "asm/howling_api.h"


#define TRAP_MODE                       0
#define SHIFT_PITCH_MODE                1
#define SHIFT_PHASE_MODE                2
#define PEMAFROW_MODE                   3

#define HOWLING_MODE                   SHIFT_PITCH_MODE

#if 0

#if(HOWLING_MODE == TRAP_MODE)
#define REVERB_RUN_POINT_NUM		160	//固定160个点
#elif (HOWLING_MODE == SHIFT_PHASE_MODE)
#define REVERB_RUN_POINT_NUM		256	//固定256个点
#else
#define REVERB_RUN_POINT_NUM		128	//无限制
#endif
#endif
HOWLING_API_STRUCT *open_howling(void *howl_para, u16 sample_rate, u8 channel, u8 mode);
// HOWLING_API_STRUCT *open_howling(HOWLING_PARM_SET *howl_para, u16 sample_rate, u8 channel, u8 mode);
void run_howling(HOWLING_API_STRUCT *howling_hdl, short *in, short *out, int len);
void close_howling(HOWLING_API_STRUCT *holing_hdl);
void pause_howling(HOWLING_API_STRUCT *holing_hdl, u8 run_mark);

#endif

