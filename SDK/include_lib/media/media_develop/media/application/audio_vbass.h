
#ifndef _AUDIO_VBASS_API_H_
#define _AUDIO_VBASS_API_H_
#include "vbass/vbass_api.h"

typedef struct _vbass_open_parm {
    u16 sr; //输入音频采样率
    u8 channel;//输入音频声道数
} vbass_open_parm;

typedef struct _vbass_update_parm {
    int bass_f;//外放的低音截止频率
    int level;//增强强度
} vbass_update_parm;


typedef struct _vbass_hdl {
    VBASS_FUNC_API *ops;
    void *work_buf;
    OS_MUTEX mutex;

    /*     u32 total_len; */
    /* u32 output_len; */

    vbass_open_parm o_parm;
    vbass_update_parm u_parm;
    int process_len;
    struct audio_stream_entry entry;	// 音频流入口

} vbass_hdl;
/*----------------------------------------------------------------------------*/
/**@brief   audio_vbass_open  虚拟低音打开
   @param    *_parm: 始化参数，详见结构体vbass_open_parm
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
vbass_hdl *audio_vbass_open(vbass_open_parm *_parm);

/*----------------------------------------------------------------------------*/
/**@brief   audio_vbass_parm_update 虚拟低音参数更新
   @param    cmd:命令
   @param    *_parm:参数指针,NULL则使用默认德参数，否则传入自定义参数
   @return   0：成功  -1: 失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vbass_parm_update(vbass_hdl *_hdl, u32 cmd, vbass_update_parm *_parm);

/*----------------------------------------------------------------------------*/
/**@brief    audio_vbass_close 虚拟低音关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vbass_close(vbass_hdl *_hdl);
#endif

