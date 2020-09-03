#include "smartbox/config.h"
#include "smartbox/event.h"
#include "smartbox_update.h"
#include "btstack_3th_protocol_user.h"

///>>>>>>>>>>>>>>>设备接收到上报给APP端命令的回复
void cmd_respone(void *priv, u8 OpCode, u8 status, u8 *data, u16 len)
{
    printf("cmd_respone:%x\n", OpCode);
#if RCSP_UPDATE_EN
    if (0 == JL_rcsp_update_cmd_receive_resp(priv, OpCode, status, data, len)) {
        return;
    }
#endif
}


