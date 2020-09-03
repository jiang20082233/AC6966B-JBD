#include "smartbox/config.h"
#include "smartbox/event.h"
#include "btstack_3th_protocol_user.h"

///>>>>>>>>>>>>>收到APP发来不需要响应的命令处理
#if (SMART_BOX_EN)
void cmd_recieve_no_respone(void *priv, u8 OpCode, u8 *data, u16 len)
{
    printf("cmd_recieve_no_respone, %x\n", OpCode);
}
#endif//SMART_BOX_EN


