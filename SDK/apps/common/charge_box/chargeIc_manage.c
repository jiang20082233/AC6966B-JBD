#include "chargeIc_manage.h"
#include "device/device.h"
#include "app_config.h"
#include "app_main.h"
#include "user_cfg.h"


#if(TCFG_CHARGE_BOX_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHG_IC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


static const struct chargeIc_platform_data *platform_data;
CHARGE_IC_INTERFACE *chargeIc_hdl = NULL;
CHARGE_IC_INFO  __chargeIc_info = {.iic_delay = 30};
#define chargeIc_info (&__chargeIc_info)

#if TCFG_CHARGE_IC_USER_IIC_TYPE
#define iic_init(iic)                       hw_iic_init(iic)
#define iic_uninit(iic)                     hw_iic_uninit(iic)
#define iic_start(iic)                      hw_iic_start(iic)
#define iic_stop(iic)                       hw_iic_stop(iic)
#define iic_tx_byte(iic, byte)              hw_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               hw_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         hw_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        hw_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    hw_iic_suspend(iic)
#define iic_resume(iic)                     hw_iic_resume(iic)
#else
#define iic_init(iic)                       soft_iic_init(iic)
#define iic_uninit(iic)                     soft_iic_uninit(iic)
#define iic_start(iic)                      soft_iic_start(iic)
#define iic_stop(iic)                       soft_iic_stop(iic)
#define iic_tx_byte(iic, byte)              soft_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               soft_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         soft_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        soft_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    soft_iic_suspend(iic)
#define iic_resume(iic)                     soft_iic_resume(iic)
#endif

/*------------------------------------------------------------------------------------*/
/**@brief    充电ic检测
   @param    priv:      扩展参数
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
static void chargeIc_detect(void *priv)
{
    chargeIc_hdl->detect();
}

/*------------------------------------------------------------------------------------*/
/**@brief    iic写命令
   @param    r_chip_id:       芯片ID
             register_address:寄存器地址
             function_command:命令
   @return   是否成功
   @note
*/
/*------------------------------------------------------------------------------------*/
u8 chargeIc_command(u8 w_chip_id, u8 register_address, u8 function_command)
{
    u8 ret = 1;
    iic_start(chargeIc_info->iic_hdl);
    if (0 == iic_tx_byte(chargeIc_info->iic_hdl, w_chip_id)) {
        ret = 0;
        log_e("\n chargeIc iic wr err 0\n");
        goto __gcend;
    }

    delay(chargeIc_info->iic_delay);

    if (0 == iic_tx_byte(chargeIc_info->iic_hdl, register_address)) {
        ret = 0;
        log_e("\n chargeIc iic wr err 1\n");
        goto __gcend;
    }

    delay(chargeIc_info->iic_delay);

    if (0 == iic_tx_byte(chargeIc_info->iic_hdl, function_command)) {
        ret = 0;
        log_e("\n chargeIc iic wr err 2\n");
        goto __gcend;
    }

__gcend:
    iic_stop(chargeIc_info->iic_hdl);

    return ret;
}

/*------------------------------------------------------------------------------------*/
/**@brief    iic获取数据
   @param    r_chip_id:       芯片ID
             register_address:寄存器地址
             buf:             数据存放buf
             data_len:        读取数据长度
   @return   获取到的数据的长度
   @note
*/
/*------------------------------------------------------------------------------------*/
u8 chargeIc_get_ndata(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len)
{
    u8 read_len = 0;

    iic_start(chargeIc_info->iic_hdl);
    if (0 == iic_tx_byte(chargeIc_info->iic_hdl, r_chip_id - 1)) {
        log_e("\n chargeIc iic rd err 0\n");
        read_len = 0;
        goto __gdend;
    }


    delay(chargeIc_info->iic_delay);
    if (0 == iic_tx_byte(chargeIc_info->iic_hdl, register_address)) {
        log_e("\n chargeIc iic rd err 1\n");
        read_len = 0;
        goto __gdend;
    }

    iic_start(chargeIc_info->iic_hdl);
    if (0 == iic_tx_byte(chargeIc_info->iic_hdl, r_chip_id)) {
        log_e("\n chargeIc iic rd err 2\n");
        read_len = 0;
        goto __gdend;
    }

    delay(chargeIc_info->iic_delay);

    for (; data_len > 1; data_len--) {
        *buf++ = iic_rx_byte(chargeIc_info->iic_hdl, 1);
        read_len ++;
    }

    *buf = iic_rx_byte(chargeIc_info->iic_hdl, 0);
    read_len ++;

__gdend:

    iic_stop(chargeIc_info->iic_hdl);
    delay(chargeIc_info->iic_delay);

    return read_len;
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电升压开关
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chargeIc_boost_ctrl(u8 en)
{
    if (chargeIc_hdl) {
        chargeIc_hdl->boost_ctrl(en);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    左耳充电开关
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chargeIc_vol_ctrl(u8 en)
{
    if (chargeIc_hdl) {
        chargeIc_hdl->vol_ctrl(en);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    右耳充电开关
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chargeIc_vor_ctrl(u8 en)
{
    if (chargeIc_hdl) {
        chargeIc_hdl->vor_ctrl(en);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    获取仓电源电压
   @param    无
   @return   电源电压
   @note
*/
/*------------------------------------------------------------------------------------*/
u16 chargeIc_get_vbat(void)
{
    if (chargeIc_hdl) {
        return chargeIc_hdl->get_vbat();
    }
    return 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电IC初始化
   @param    _data:扩展参数,是否使用iic，充电IC的名字字符串
   @return   retval:初始化状态
   @note     根据参数选择是否初始化iic模块，初始对应的充电IC,注册det到timer
*/
/*------------------------------------------------------------------------------------*/
int chargeIc_init(const void *_data)
{
    int retval = 0;
    platform_data = (const struct chargeIc_platform_data *)_data;
    chargeIc_info->iic_hdl = platform_data->iic;

    if (chargeIc_info->iic_hdl) {
        retval = iic_init(chargeIc_info->iic_hdl);
        if (retval < 0) {
            log_e("\n  open iic for chargeIc err\n");
            return retval;
        } else {
            log_info("\n iic open succ\n");
        }
    }

    retval = -EINVAL;
    list_for_each_chargeIc(chargeIc_hdl) {
        if (!memcmp(chargeIc_hdl->logo, platform_data->chargeIc_name, strlen(platform_data->chargeIc_name))) {
            retval = 0;
            break;
        }
    }

    if (retval < 0) {
        log_e(">>>chargeIc_hdl logo err\n");
        return retval;
    }

    if (chargeIc_hdl->init()) {
        log_e(">>>>chargeIc_Int ERROR\n");
    } else {
        log_e(">>>>chargeIc_Int SUCC\n");
        chargeIc_info->init_flag  = 1;
        sys_timer_add(NULL, chargeIc_detect, 10); //10ms,iic通信的可以设置为50ms，注意ic->det 里各种计数时间
    }
    return 0;
}

#endif
