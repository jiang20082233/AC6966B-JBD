#include "soundcard/notice.h"
#include "tone_player.h"


#if SOUNDCARD_ENABLE
static const char *electric_notice_tab[] = {
    SDFILE_RES_ROOT_PATH"tone/AA.*",
    SDFILE_RES_ROOT_PATH"tone/SA.*",
    SDFILE_RES_ROOT_PATH"tone/BB.*",
    //SDFILE_RES_ROOT_PATH"tone/SB.*",
    SDFILE_RES_ROOT_PATH"tone/CC.*",
    SDFILE_RES_ROOT_PATH"tone/SC.*",
    SDFILE_RES_ROOT_PATH"tone/DD.*",
    SDFILE_RES_ROOT_PATH"tone/SD.*",
    SDFILE_RES_ROOT_PATH"tone/EE.*",
    //SDFILE_RES_ROOT_PATH"tone/SE.*",
    SDFILE_RES_ROOT_PATH"tone/FF.*",
    SDFILE_RES_ROOT_PATH"tone/SF.*",
    SDFILE_RES_ROOT_PATH"tone/GG.*",
    SDFILE_RES_ROOT_PATH"tone/SG.*",
};

static const char *noise_tab[] = {
    SDFILE_RES_ROOT_PATH"tone/huan_hu.*",
    SDFILE_RES_ROOT_PATH"tone/gan_ga.*",
    SDFILE_RES_ROOT_PATH"tone/qiang.*",
    SDFILE_RES_ROOT_PATH"tone/bi_shi.*",
    SDFILE_RES_ROOT_PATH"tone/kaichang.*",
    SDFILE_RES_ROOT_PATH"tone/FeiWen.*",
    SDFILE_RES_ROOT_PATH"tone/xiao.*",
    SDFILE_RES_ROOT_PATH"tone/zhangshen.*",
    SDFILE_RES_ROOT_PATH"tone/QiuFenXiang.*",
    SDFILE_RES_ROOT_PATH"tone/memeda.*",
    SDFILE_RES_ROOT_PATH"tone/zeilala.*",
    SDFILE_RES_ROOT_PATH"tone/feicheng.*",
};



void soundcard_make_notice_electric(u8 mode)
{
    if (mode >= (sizeof(electric_notice_tab) / sizeof(electric_notice_tab[0]))) {
        return ;
    }
    tone_play_by_path(electric_notice_tab[mode], 0);
}

void soundcard_make_some_noise(u8 id)
{
    if (id >= (sizeof(noise_tab) / sizeof(noise_tab[0]))) {
        return ;
    }
    tone_play_by_path(noise_tab[id], 0);
}
#endif//SOUNDCARD_ENABLE

