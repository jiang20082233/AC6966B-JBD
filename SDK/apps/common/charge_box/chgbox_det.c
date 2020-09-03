#include "gpio.h"
#include "system/event.h"
#include "app_config.h"
#include "chgbox_det.h"
#include "chgbox_box.h"
#include "chgbox_ctrl.h"
#include "system/timer.h"

#if(TCFG_CHARGE_BOX_ENABLE)

struct _io_det_hdl {
    const struct io_det_platform_data *data;
    u16 det_cnt;
};

struct _io_det_hdl hall_det = {
    .data = NULL,
    .det_cnt = 0,
};

/*------------------------------------------------------------------------------------*/
/**@brief    霍尔传感器检测回调
   @param    priv:扩展参数
   @return   无
   @note     传感器上下线处理，设置状态、发送相关事件
*/
/*------------------------------------------------------------------------------------*/
static void hall_detect_cb(void *priv)
{
    u8 io_level = gpio_read(hall_det.data->port);
    if (sys_info.status[LID_DET] == STATUS_ONLINE) {
        if (((hall_det.data->level == LOW_LEVEL) && io_level) || ((hall_det.data->level == HIGHT_LEVEL) && (!io_level))) {
            hall_det.det_cnt++;
            sys_info.life_cnt = 0;
            if (hall_det.det_cnt >= hall_det.data->offline_time) {
                hall_det.det_cnt = 0;
                sys_info.status[LID_DET] = STATUS_OFFLINE;
                app_chargebox_event_to_user(CHGBOX_EVENT_CLOSE_LID);
            }
        } else {
            hall_det.det_cnt = 0;//去抖
        }
    } else {
        if (((hall_det.data->level == LOW_LEVEL) && (!io_level)) || ((hall_det.data->level == HIGHT_LEVEL) && io_level)) {
            hall_det.det_cnt++;
            sys_info.life_cnt = 0;
            if (hall_det.det_cnt >= hall_det.data->online_time) {
                hall_det.det_cnt = 0;
                sys_info.status[LID_DET] = STATUS_ONLINE;
                app_chargebox_event_to_user(CHGBOX_EVENT_OPEN_LID);
            }
        } else {
            hall_det.det_cnt = 0;//去抖
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    霍尔传感器检测初始化
   @param    data:io相关参数,io选择，上下线检测次数
   @return   无
   @note     初始化相关io,传感器初始状态,注册检测函数到timer
*/
/*------------------------------------------------------------------------------------*/
void hall_det_init(const struct io_det_platform_data *data)
{
    u8 io_level;
    hall_det.data = data;
    sys_info.status[LID_DET] = STATUS_OFFLINE;
    gpio_set_direction(hall_det.data->port, 1);
    gpio_set_die(hall_det.data->port, 1);
    gpio_set_pull_down(hall_det.data->port, 0);
    gpio_set_pull_up(hall_det.data->port, 0);
    sys_s_hi_timer_add(NULL, hall_detect_cb, 10);  //10ms
    delay(100);
    io_level = gpio_read(hall_det.data->port);
    if (((hall_det.data->level == LOW_LEVEL) && (!io_level)) || ((hall_det.data->level == HIGHT_LEVEL) && io_level)) {
        sys_info.status[LID_DET] = STATUS_ONLINE;
    }
}

#endif
