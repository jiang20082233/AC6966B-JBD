#ifndef __MUSIC_NEW_H__
#define __MUSIC_NEW_H__

#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "media/audio_decoder.h"
#include "music_player.h"

enum {
    MUSIC_TASK_START_BY_NORMAL = 0x0,
    MUSIC_TASK_START_BY_BREAKPOINT,
    MUSIC_TASK_START_BY_SCLUST,
    MUSIC_TASK_START_BY_NUMBER,
    MUSIC_TASK_START_BY_PATH,
};

void music_task_set_parm(u8 type, int val);
void music_player_err_deal(int err);

#endif//__MUSIC_NEW_H__
