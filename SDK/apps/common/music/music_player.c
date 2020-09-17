#include "music/music_player.h"
#include "key_event_deal.h"
#include "app_config.h"
#include "audio_enc.h"

#define LOG_TAG_CONST       APP_MUSIC
#define LOG_TAG             "[APP_MUSIC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if (TCFG_APP_MUSIC_EN)

///music player总控制句柄
struct __music_player {
    struct __dev 			*dev;//当前播放设备节点
    struct vfscan			*fsn;//设备扫描句柄
    FILE		 			*file;//当前播放文件句柄
    void 					*priv;//music回调函数，私有参数
    struct __player_parm 	parm;//回调及参数配置
};

static struct __music_player *music_player = NULL;
#define __this 	music_player

static u8 cycle_mode = FCYCLE_ALL;///循环播放模式
static volatile u16 magic_cnt = 0;

///播放参数，文件扫描时用，文件后缀等
static const char scan_parm[] = "-t"
#if (TCFG_DEC_MP3_ENABLE)
                                "MP1MP2MP3"
#endif
#if (TCFG_DEC_WMA_ENABLE)
                                "WMA"
#endif
#if ( TCFG_DEC_WAV_ENABLE || TCFG_DEC_DTS_ENABLE)
                                "WAVDTS"
#endif
#if (TCFG_DEC_FLAC_ENABLE)
                                "FLA"
#endif
#if (TCFG_DEC_APE_ENABLE)
                                "APE"
#endif
#if (TCFG_DEC_M4A_ENABLE)
                                "M4AAAC"
#endif
#if (TCFG_DEC_M4A_ENABLE || TCFG_DEC_ALAC_ENABLE)
                                "MP4"
#endif
#if (TCFG_DEC_AMR_ENABLE)
                                "AMR"
#endif
#if (TCFG_DEC_DECRYPT_ENABLE)
                                "SMP"
#endif
#if (TCFG_DEC_MIDI_ENABLE)
                                "MID"
#endif
                                " -sn -r"
#if (TCFG_RECORD_FOLDER_DEV_ENABLE)
                                " -m"
                                REC_FOLDER_NAME
#endif
                                ;

//*----------------------------------------------------------------------------*/
/**@brief    music_player释放接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void music_player_destroy(void)
{
    if (__this) {
        music_player_stop(1);
        free(__this);
        __this = NULL;
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player创建接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
bool music_player_creat(void *priv, struct __player_parm *parm)
{
    __this = zalloc(sizeof(struct __music_player));
    if (__this == NULL) {
        return false;
    }

    __this->priv = priv;
    memcpy(&__this->parm, parm, sizeof(struct __player_parm));
    ASSERT(__this->parm.cb, "music_player parm error!!\n");
    return true;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player播放结束处理
   @param	 parm：结束参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_end_deal(int parm)
{
    int err = 0;
    u8 event = (u8)parm;
    u8 read_err = (u8)(parm >> 8);
    u16 magic = (u16)(parm >> 16);
    switch (event) {
    ///文件播放结束, 包括正常播放结束和读文件错误导致的结束, 如拔掉设备产生的错误结束
    case AUDIO_PLAY_EVENT_END:
        log_i("AUDIO_PLAY_EVENT_END\n");
        if (read_err) {
            log_e("read err, magic err = %d, %d\n", magic, magic_cnt);
            if (magic == magic_cnt - 1) {
                err = MUSIC_PLAYER_ERR_DEV_READ;///设备读错误
            } else {
                err = MUSIC_PLAYER_ERR_NULL;///序号已经对不上了， 不处理
            }

        } else {
            ///正常结束，自动下一曲
            err = music_player_play_auto_next();
        }
        break;
    ///解码器产生的错误, 文件损坏等
    case AUDIO_PLAY_EVENT_ERR:
        log_i("AUDIO_PLAY_EVENT_ERR\n");
        err = music_player_play_auto_next();///文件播放过程出现的错误， 自动下一曲
        break;
    default:
        break;
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player播放结束事件回调
   @param
   @return
   @note	非api层接口
*/
/*----------------------------------------------------------------------------*/
static void music_player_end_event_callback(void *priv, int argc, int *argv)
{
    u8 event = (u8)argv[0];
    u8 read_err = (u8)argv[1];
    u16 magic = (u16)priv;
    log_i("music_player_end_event_callback\n");
    int parm = event | (read_err << 8) | (magic << 16);
    if (__this->parm.cb && __this->parm.cb->end) {
        __this->parm.cb->end(__this->priv, parm);
    }

}
//*----------------------------------------------------------------------------*/
/**@brief    music_player解码器启动接口
   @param
			 file：
			 	文件句柄
			 dbp：
			 	断点信息
   @return   music_player 错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_decode_start(FILE *file, struct audio_dec_breakpoint *dbp)
{
    if (file) {
        ///get file short name
        u8 file_name[12 + 1] = {0}; //8.3+\0
        fget_name(music_player_get_file_hdl(), file_name, sizeof(file_name));
        log_i("\n");
        log_i("file name: %s\n", file_name);
        log_i("\n");
    }
    int ret;
    ret = file_dec_create((void *)magic_cnt, music_player_end_event_callback);
    if (ret) {
        return MUSIC_PLAYER_ERR_NO_RAM;
    }
    magic_cnt ++;
    ret = file_dec_open(file, dbp);
    if (ret) {
        return MUSIC_PLAYER_ERR_DECODE_FAIL;
    }

    if (__this->parm.cb && __this->parm.cb->start) {
        __this->parm.cb->start(__this->priv, 0);
    }

    return MUSIC_PLAYER_SUCC;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备断点信息
   @param
			 bp：
			 	断点缓存，外部调用提供
			 flag：
			 	1：需要获取歌曲断点信息及文件信息， 0：只获取文件信息
   @return   成功与否
   @note
*/
/*----------------------------------------------------------------------------*/
bool music_player_get_playing_breakpoint(struct __breakpoint *bp, u8 flag)
{
    if (__this == NULL || bp == NULL) {
        return false;
    }
    if (dev_manager_online_check(__this->dev, 1)) {
        if (file_dec_is_play() == true || file_dec_is_pause() == true) {
            if (__this->file) {
                if (flag) {
                    ///获取断点解码信息
                    int ret = file_dec_get_breakpoint(&bp->dbp);
                    if (ret) {
                        ///获取断点解码信息错误
                        log_e("file_dec_get_breakpoint err !!\n");
                    }
                }
                ///获取断点文件信息
                struct vfs_attr attr = {0};
                fget_attrs(__this->file, &attr);
                bp->sclust = attr.sclust;
                bp->fsize = attr.fsize;
                log_i("get playing breakpoint ok\n");
                return true;
            }
        }
    }
    return false;

}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备文件总数
   @param
   @return   文件总数
   @note
*/
/*----------------------------------------------------------------------------*/
u16 music_player_get_file_total(void)
{
    if (__this && __this->fsn) {
        return __this->fsn->file_number;
    }
    return 0;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放文件序号
   @param
   @return   当前文件序号
   @note
*/
/*----------------------------------------------------------------------------*/
u16 music_player_get_file_cur(void)
{
    if (__this && __this->fsn) {
        return __this->fsn->file_counter;
    }
    return 0;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放文件所在文件夹的文件总数
   @param
   @return   文件总数
   @note
*/
/*----------------------------------------------------------------------------*/
u16 music_player_get_fileindir_number(void)
{
    if (__this && __this->fsn) {
        return __this->fsn->fileTotalInDir;
    }
    return 0;

}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放文件所在文件夹
   @param
   @return   当前文件夹序号
   @note
*/
/*----------------------------------------------------------------------------*/
u16 music_player_get_dir_cur(void)
{
    if (__this && __this->fsn) {
        return __this->fsn->musicdir_counter;
    }
    return 0;

}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取文件夹总数
   @param
   @return   文件夹总数
   @note
*/
/*----------------------------------------------------------------------------*/
u16 music_player_get_dir_total(void)
{
    if (__this && __this->fsn) {
        return __this->fsn->dir_totalnumber;
    }
    return 0;

}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取文件句柄
   @param
   @return   文件句柄
   @note	 需要注意文件句柄的生命周期
*/
/*----------------------------------------------------------------------------*/
FILE *music_player_get_file_hdl(void)
{
    if (__this && __this->file) {
        return __this->file;
    }
    return NULL;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取文件簇号
   @param
   @return   文件簇号, -1:无效
   @note
*/
/*----------------------------------------------------------------------------*/
u32 music_player_get_file_sclust(void)
{
    if (__this && __this->file) {

        struct vfs_attr tmp_attr = {0};
        fget_attrs(__this->file, &tmp_attr);
        return tmp_attr.sclust;
    }
    return (u32) - 1;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备盘符
   @param
   @return   设备盘符
   @note
*/
/*----------------------------------------------------------------------------*/
char *music_player_get_dev_cur(void)
{
    if (__this) {
        return dev_manager_get_logo(__this->dev);
    }
    return NULL;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备下一个设备
   @param
   @return   设备盘符
   @note
*/
/*----------------------------------------------------------------------------*/
char *music_player_get_dev_next(void)
{
    if (__this) {
        return dev_manager_get_logo(dev_manager_find_next(__this->dev, 1));
    }
    return NULL;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备上一个设备
   @param
   @return   设备盘符
   @note
*/
/*----------------------------------------------------------------------------*/
char *music_player_get_dev_prev(void)
{
    if (__this) {
        return dev_manager_get_logo(dev_manager_find_prev(__this->dev, 1));
    }
    return NULL;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放状态
   @param
   @return   返回值如：
  				 FILE_DEC_STATUS_STOP,//解码停止
  				 FILE_DEC_STATUS_PLAY,//正在解码
  				 FILE_DEC_STATUS_PAUSE,//解码暂停
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_get_play_status(void)
{
    return file_dec_get_status();
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放歌曲时间
   @param
   @return   当前播放时间
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_get_dec_cur_time(void)
{
    if (__this) {
        return file_dec_get_cur_time();
    }
    return 0;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放歌曲总时间
   @param
   @return   当前播放总时间
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_get_dec_total_time(void)
{
    if (__this) {
        return file_dec_get_total_time();
    }
    return 0;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放循环模式
   @param
   @return   当前播放循环模式
   @note
*/
/*----------------------------------------------------------------------------*/
u8 music_player_get_repeat_mode(void)
{
    return cycle_mode;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放对应的music设备
   @param
   @return   设备盘符
   @note	 播放录音区分时，可以通过该接口判断当前播放的音乐设备是什么以便做录音区分判断
*/
/*----------------------------------------------------------------------------*/
char *music_player_get_cur_music_dev(void)
{
    if (__this) {
        char music_dev_logo[16] = {0};
        char *logo = dev_manager_get_logo(__this->dev);
        if (logo) {
            char *str = strstr(logo, "_rec");
            if (str) {
                ///录音设备,切换到音乐设备播放
                strncpy(music_dev_logo, logo, strlen(logo) - strlen(str));
                logo = dev_manager_get_logo(dev_manager_find_spec(music_dev_logo, 1));
            }
        }
        return logo;
    }
    return NULL;
}

const char *music_player_get_phy_dev(int *len)
{
    if (__this) {
        char *logo = dev_manager_get_logo(__this->dev);
        if (logo) {
            char *str = strstr(logo, "_rec");
            if (str) {
                ///录音设备,切换到音乐设备播放
                if (len) {
                    *len =  strlen(logo) - strlen(str);
                }
            } else {
                if (len) {
                    *len =  strlen(logo);
                }
            }
            return logo;
        }
    }
    if (len) {
        *len =  0;
    }
    return NULL;
}



//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前录音区分播放状态
   @param
   @return   true：录音文件夹播放, false：非录音文件夹播放
   @note	 播放录音区分时，可以通过该接口判断当前播放的是录音文件夹还是非录音文件夹
*/
/*----------------------------------------------------------------------------*/
bool music_player_get_record_play_status(void)
{
    if (__this) {
        char *logo = dev_manager_get_logo(__this->dev);
        if (logo) {
            char *str = strstr(logo, "_rec");
            if (str) {
                return true;
            }
        }
    }
    return false;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player播放/暂停
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_pp(void)
{
    if (__this) {
        file_dec_pp();
    }
    return MUSIC_PLAYER_ERR_NULL;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player解码停止
   @param
			 fsn_release：
				1：释放扫盘句柄
				0：不释放扫盘句柄
   @return
   @note	 如果释放了扫盘句柄，需要重新扫盘，否则播放失败
*/
/*----------------------------------------------------------------------------*/
void music_player_stop(u8 fsn_release)
{
    if (__this == NULL) {
        return ;
    }
    ///停止解码
    file_dec_close();
    if (__this->file) {
        fclose(__this->file);
        __this->file = NULL;
    }
    if (fsn_release && __this->fsn) {
        ///根据播放情景， 通过设定flag决定是否需要释放fscan， 释放后需要重新扫盘!!!
        dev_manager_scan_disk_release(__this->fsn);
        __this->fsn = NULL;
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player快进
   @param	 step：快进步进
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void music_player_ff(int step)
{
    if (__this) {
        file_dec_FF(step);
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player快退
   @param	 step：快退步进
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void music_player_fr(int step)
{
    if (__this) {
        file_dec_FR(step);
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player设置播放循环模式
   @param	 mode：循环模式
				FCYCLE_ALL
				FCYCLE_ONE
				FCYCLE_FOLDER
				FCYCLE_RANDOM
   @return  循环模式
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_set_repeat_mode(u8 mode)
{
    if (__this) {
        if (mode >= FCYCLE_MAX) {
            return -1;
        }
        if (__this->fsn) {
            __this->fsn->cycle_mode = mode;
            log_i("cycle_mode = %d\n", mode);
            return mode;
        }
    }
    return -1;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player切换循环模式
   @param
   @return   循环模式
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_change_repeat_mode(void)
{
    if (__this) {
        cycle_mode++;
        if (cycle_mode >= FCYCLE_MAX) {
            cycle_mode = FCYCLE_ALL;
        }
        return  music_player_set_repeat_mode(cycle_mode);
    }
    return -1;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player删除当前播放文件,并播放下一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_delete_playing_file(void)
{
    if (__this && __this->file) {
        ///获取当前播放文件序号， 文件删除之后， 播放下一曲
        int err = 0;
        int cur_file = music_player_get_file_cur();
        char *cur_dev = music_player_get_dev_cur();
        file_dec_close();
        err = fdelete(__this->file);
        if (err) {
            log_info("[%s, %d] fail!!, replay cur file\n", __FUNCTION__, __LINE__);
        } else {
            log_info("[%s, %d] ok, play next file\n", __FUNCTION__, __LINE__);
        }
        __this->file = NULL;
        return music_player_play_by_number(cur_dev, cur_file);
    }
    return MUSIC_PLAYER_ERR_NULL;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player播放上一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_prev(void)
{
    ///close player first
    music_player_stop(0);
    ///check dev, 检查设备是否有掉线
    if (dev_manager_online_check(__this->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    ///不需要重新找设备、扫盘
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_PREV_FILE, 0);///选择上一曲
    if (__this->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player播放下一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_next(void)
{
    ///close player first
    music_player_stop(0);
    ///check dev, 检查设备是否有掉线
    if (dev_manager_online_check(__this->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    ///不需要重新找设备、扫盘
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_NEXT_FILE, 0);///选择下一曲
    if (__this->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player播放第一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_first_file(char *logo)
{
    if (logo == NULL) {
        music_player_stop(0);
        if (dev_manager_online_check(__this->dev, 1) == 0) {
            puts(">>>>>>> dev error 0\n");
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        ///没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(1);
        __this->dev = dev_manager_find_spec(logo, 1);
        if (__this->dev == NULL) {
            puts(">>>>>>> dev error 1\n");
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        __this->fsn = dev_manager_scan_disk(__this->dev, NULL, scan_parm, cycle_mode, __this->parm.cb->fsn_break);
    }
    if (__this->fsn == NULL) {
            puts(">>>>>>> dev error 2\n");
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_FIRST_FILE, 0);
    if (__this->file == NULL) {
            puts(">>>>>>> dev error 3\n");
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
            puts(">>>>>>> dev error 4\n");
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player播放最后一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_last_file(char *logo)
{
    if (logo == NULL) {
        music_player_stop(0);
        if (dev_manager_online_check(__this->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        ///没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(1);
        __this->dev = dev_manager_find_spec(logo, 1);
        if (__this->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        __this->fsn = dev_manager_scan_disk(__this->dev, NULL, scan_parm, cycle_mode, __this->parm.cb->fsn_break);
    }
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_LAST_FILE, 0);
    if (__this->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player自动播放下一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_auto_next(void)
{
    ///close player first
    music_player_stop(0);
    ///get dev, 检查设备是否有掉线
    if (dev_manager_online_check(__this->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    ///不需要重新找设备、扫盘
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_AUTO_FILE, 0);///选择自动下一曲
    if (__this->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player上一个文件夹
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_folder_prev(void)
{
    ///close player first
    music_player_stop(0);
    ///get dev, 检查设备是否有掉线
    if (dev_manager_online_check(__this->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    ///不需要重新找设备、扫盘
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_PREV_FOLDER_FILE, 0);///选择播放下一个文件夹
    if (__this->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player下一个文件夹
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_folder_next(void)
{
    ///close player first
    music_player_stop(0);
    ///get dev, 检查设备是否有掉线
    if (dev_manager_online_check(__this->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    ///不需要重新找设备、扫盘
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_NEXT_FOLDER_FILE, 0);///选择播放上一个文件夹
    if (__this->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player上一个设备
   @param	 bp：断点信息
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_devcie_prev(struct __breakpoint *bp)
{
    ///close player first
    music_player_stop(1);
    ///get dev
    __this->dev = dev_manager_find_prev(__this->dev, 1);
    if (__this->dev == NULL) {
        return MUSIC_PLAYER_ERR_DEV_NOFOUND;
    }
    ///get fscan
    __this->fsn = dev_manager_scan_disk(__this->dev, NULL, (const char *)scan_parm, cycle_mode, __this->parm.cb->fsn_break);
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    int err = 0;
    if (bp) {
        __this->file = file_manager_select(__this->fsn, FSEL_BY_SCLUST, bp->sclust);//根据文件簇号查找断点文件
        if (__this->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        err = music_player_decode_start(__this->file, &(bp->dbp));
    } else {
        /* __this->file = file_manager_select(__this->fsn, FSEL_LAST_FILE, 0); */
        __this->file = file_manager_select(__this->fsn, FSEL_FIRST_FILE, 0);
        if (__this->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        err = music_player_decode_start(__this->file, 0);
    }
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player下一个设备
   @param	 bp：断点信息
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_devcie_next(struct __breakpoint *bp)
{
    ///close player first
    music_player_stop(1);
    ///get dev
    __this->dev = dev_manager_find_next(__this->dev, 1);
    if (__this->dev == NULL) {
        return MUSIC_PLAYER_ERR_DEV_NOFOUND;
    }
    ///get fscan
    __this->fsn = dev_manager_scan_disk(__this->dev, NULL, scan_parm, cycle_mode, __this->parm.cb->fsn_break);
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    int err = 0;
    if (bp) {
        __this->file = file_manager_select(__this->fsn, FSEL_BY_SCLUST, bp->sclust);//根据文件簇号查找断点文件
        if (__this->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        err = music_player_decode_start(__this->file, &(bp->dbp));
    } else {
        __this->file = file_manager_select(__this->fsn, FSEL_FIRST_FILE, 0);//选择第一个文件播放
        if (__this->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        err = music_player_decode_start(__this->file, 0);
    }
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player断点播放指定设备
   @param
   			 logo：逻辑盘符，如：sd0/sd1/udisk0
   			 bp：断点信息
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_breakpoint(char *logo, struct __breakpoint *bp)
{
    if (bp == NULL) {
        return MUSIC_PLAYER_ERR_PARM;
    }
    if (logo == NULL) {
        music_player_stop(0);
        if (dev_manager_online_check(__this->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        ///没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(1);
        __this->dev = dev_manager_find_spec(logo, 1);
        if (__this->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        __this->fsn = dev_manager_scan_disk(__this->dev, NULL, scan_parm, cycle_mode, __this->parm.cb->fsn_break);
    }
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_BY_SCLUST, bp->sclust);//根据文件簇号查找断点文件
    if (__this->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, &(bp->dbp));
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player序号播放指定设备
   @param
   			 logo：逻辑盘符，如：sd0/sd1/udisk0
   			 number：指定播放序号
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_number(char *logo, u32 number)
{
    if (logo == NULL) {
        music_player_stop(0);
        if (dev_manager_online_check(__this->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        ///没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(1);
        __this->dev = dev_manager_find_spec(logo, 1);
        if (__this->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        __this->fsn = dev_manager_scan_disk(__this->dev, NULL, scan_parm, cycle_mode, __this->parm.cb->fsn_break);
    }

    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_BY_NUMBER, number);
    if (__this->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player簇号播放指定设备
   @param
   			 logo：逻辑盘符，如：sd0/sd1/udisk0
   			 sclust：指定播放簇号
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_sclust(char *logo, u32 sclust)
{
    char *cur_logo = dev_manager_get_logo(__this->dev);
    if (logo == NULL || (cur_logo && (0 == strcmp(logo, cur_logo)))) {
        music_player_stop(0);
        if (dev_manager_online_check(__this->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        ///没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(1);
        __this->dev = dev_manager_find_spec(logo, 1);
        if (__this->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        __this->fsn = dev_manager_scan_disk(__this->dev, NULL, scan_parm, cycle_mode, __this->parm.cb->fsn_break);
    }
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_BY_SCLUST, sclust);//根据簇号查找文件
    if (__this->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player路径播放指定设备
   @param
   			 logo：逻辑盘符，如：sd0/sd1/udisk0, 设置为NULL，为默认当前播放设备
   			 path：指定播放路径
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_path(char *logo, const char *path)
{
    if (path == NULL) {
        return MUSIC_PLAYER_ERR_POINT;
    }
    if (logo == NULL) {
        music_player_stop(0);
        if (dev_manager_online_check(__this->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        ///没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(1);
        __this->dev = dev_manager_find_spec(logo, 1);
        if (__this->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        __this->fsn = dev_manager_scan_disk(__this->dev, NULL, scan_parm, cycle_mode, __this->parm.cb->fsn_break);
    }
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    __this->file = file_manager_select(__this->fsn, FSEL_BY_PATH, (int)path);//根据簇号查找文件
    if (__this->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    ///start decoder
    int err = music_player_decode_start(__this->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player录音区分切换播放
   @param
   			 logo：逻辑盘符，如：sd0/sd1/udisk0, 设置为NULL，为默认当前播放设备
   			 bp：断点信息
   @return   播放错误码
   @note	 通过指定设备盘符，接口内部通过解析盘符是否"_rec"
   			 来确定是切换到录音播放设备还是非录音播放设备
*/
/*----------------------------------------------------------------------------*/
int music_player_play_record_folder(char *logo, struct __breakpoint *bp)
{
    int err = MUSIC_PLAYER_ERR_NULL;
#if (TCFG_RECORD_FOLDER_DEV_ENABLE)
    char rec_dev_logo[16] = {0};
    char music_dev_logo[16] = {0};
    music_player_stop(1);
    if (logo == NULL) {
        if (dev_manager_online_check(__this->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        logo = dev_manager_get_logo(__this->dev);
        if (logo == NULL) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
    }

    ///判断是否是录音设备
    char *str = strstr(logo, "_rec");
    if (str == NULL) {
        ///是非录音设备，切换到录音设备播放
        sprintf(rec_dev_logo, "%s%s", logo, "_rec");
        __this->dev = dev_manager_find_spec(rec_dev_logo, 1);
        logo = rec_dev_logo;
    } else {
        ///录音设备,切换到音乐设备播放
        strncpy(music_dev_logo, logo, strlen(logo) - strlen(str));
        log_i("music_dev_logo = %s, logo = %s, str = %s, len = %d\n", music_dev_logo, logo, str, strlen(logo) - strlen(str));
        __this->dev = dev_manager_find_spec(music_dev_logo, 1);
        logo = music_dev_logo;
    }
    if (__this->dev == NULL) {
        return MUSIC_PLAYER_ERR_DEV_NOFOUND;
    }

    ///需要扫盘
    __this->fsn = dev_manager_scan_disk(__this->dev, NULL, scan_parm, cycle_mode, __this->parm.cb->fsn_break);
    if (__this->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    ///get file
    if (bp) {
        __this->file = file_manager_select(__this->fsn, FSEL_BY_SCLUST, bp->sclust);//播放录音文件夹第一个文件
        if (__this->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        ///start decoder
        err = music_player_decode_start(__this->file, &bp->dbp);
    } else {
        __this->file = file_manager_select(__this->fsn, FSEL_FIRST_FILE, 0);//播放录音文件夹第一个文件
        if (__this->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        ///start decoder
        err = music_player_decode_start(__this->file, 0);//录音文件夹不支持断点播放
    }
    if (err == MUSIC_PLAYER_SUCC) {
        ///选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(__this->dev);
        log_i("[%s %d]  %s devcie play ok\n", __FUNCTION__, __LINE__, logo);
    }
#endif//TCFG_RECORD_FOLDER_DEV_ENABLE
    return err;
}


#endif//TCFG_APP_MUSIC_EN



