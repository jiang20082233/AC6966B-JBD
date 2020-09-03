
#ifndef _AUDIO_LOUDNESS_API_H_
#define _AUDIO_LOUDNESS_API_H_
#include "equalloudness/loudness_api.h"
// #include "application/audio_config.h"
#include "math.h"
#include "stdlib.h"

typedef struct _equalloudness_open_parm {
    u16 sr;               //采样率
    u8 channel;           //通道数
    u8 threadhold_vol;    //触发等响度软件数字音量阈值

    int (*alpha_cb)(float *alpha, u8 *volume, u8 threadhold_vol);//该函数根据，系统软件的数字音量,参数返回 alpha值
} equalloudness_open_parm;

typedef struct _equalloudness_update_parm {
    u8 threadhold_vol;    //触发等响度软件数字音量阈值
} equalloudness_update_parm;


typedef struct _equalloudness_hdl {
    void *work_buf;
    OS_MUTEX mutex;

    /*输出的偏移*/
    u32 out_buf_size;
    s16 *out_buf;
    u32 out_points;
    u32 out_total;

    /*输入的缓冲*/
    void *buf;
    cbuffer_t cbuf;
    u8 init_ok;

    equalloudness_open_parm o_parm;

    int process_len;
    struct audio_stream_entry entry;	// 音频流入口

} equal_loudness_hdl;

/*----------------------------------------------------------------------------*/
/**@brief    audio_equal_loudness_open, 等响度 打开
   @param    *_parm: 等响度初始化参数，详见结构体equalloudness_open_parm
   @return   等响度句柄
   @note
*/
/*----------------------------------------------------------------------------*/
equal_loudness_hdl *audio_equal_loudness_open(equalloudness_open_parm *_parm);
/*----------------------------------------------------------------------------*/
/**@brief   audio_equal_loudness_parm_update 等响度 参数更新
   @param    cmd:命令
   @param    *_parm:参数指针
   @return   0：成功  -1: 失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_equal_loudness_parm_update(equal_loudness_hdl *_hdl, u32 cmd, equalloudness_update_parm *_parm);
/*----------------------------------------------------------------------------*/
/**@brief   audio_equal_loudness_close 等响度关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/

int audio_equal_loudness_close(equal_loudness_hdl *_hdl);
#endif

