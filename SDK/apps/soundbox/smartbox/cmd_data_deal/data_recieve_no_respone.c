#include "smartbox/config.h"
#include "smartbox/event.h"
#include "btstack_3th_protocol_user.h"

///>>>>>>>>>>>设备接收到APP下发不需要回复数据
#if (SMART_BOX_EN)
void data_recieve_no_respone(void *priv, u8 CMD_OpCode, u8 *data, u16 len)
{
    printf("data_recieve_no_respone %x\n", CMD_OpCode);
}
#endif//SMART_BOX_EN

