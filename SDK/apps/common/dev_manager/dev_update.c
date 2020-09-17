#include "dev_update.h"
#include "dev_manager.h"
#include "update/update.h"
#include "update/update_loader_download.h"
#include "app_config.h"

#if defined(CONFIG_SD_UPDATE_ENABLE) || defined(CONFIG_USB_UPDATE_ENABLE)
#define DEV_UPDATE_EN		1
#else
#define DEV_UPDATE_EN		0
#endif

extern bool uart_update_send_update_ready(char *file_path);
extern bool get_uart_update_sta(void);
extern void storage_update_loader_download_init_with_file_hdl(
    int  type,
    char *update_path,
    void *fd,
    void (*cb)(void *priv, int type, u8 cmd),
    void *cb_priv,
    u8 task_en
);

static char update_path[48] = {0};
extern const char updata_file_name[];

struct __update_dev_reg {
    char *logo;
    int type;
    union {
        UPDATA_SD sd;
    } u;
};


#if TCFG_SD0_ENABLE
static const struct __update_dev_reg sd0_update = {
    .logo = "sd0",
    .type = SD0_UPDATA,
    .u.sd.control_type = SD_CONTROLLER_0,
#if (TCFG_SD0_PORTS=='A')
    .u.sd.control_io = SD0_IO_A,
#elif (TCFG_SD0_PORTS=='B')
    .u.sd.control_io = SD0_IO_B,
#elif (TCFG_SD0_PORTS=='C')
    .u.sd.control_io = SD0_IO_C,
#elif (TCFG_SD0_PORTS=='D')
    .u.sd.control_io = SD0_IO_D,
#elif (TCFG_SD0_PORTS=='E')
    .u.sd.control_io = SD0_IO_E,
#elif (TCFG_SD0_PORTS=='F')
    .u.sd.control_io = SD0_IO_F,
#endif
    .u.sd.power = 1,
};
#endif//TCFG_SD0_ENABLE

#if TCFG_SD1_ENABLE
static const struct __update_dev_reg sd1_update = {
    .logo = "sd1",
    .type = SD1_UPDATA,
    .u.sd.control_type = SD_CONTROLLER_1,
#if (TCFG_SD1_PORTS=='A')
    .u.sd.control_io = SD1_IO_A,
#else
    .u.sd.control_io = SD1_IO_B,
#endif
    .u.sd.power = 1,

};
#endif//TCFG_SD1_ENABLE

#if TCFG_UDISK_ENABLE
static const struct __update_dev_reg udisk_update = {
    .logo = "udisk0",
    .type = USB_UPDATA,
};
#endif//TCFG_UDISK_ENABLE


static const struct __update_dev_reg *update_dev_list[] = {
#if TCFG_UDISK_ENABLE
    &udisk_update,
#endif//TCFG_UDISK_ENABLE
#if TCFG_SD0_ENABLE
    &sd0_update,
#endif//
#if TCFG_SD1_ENABLE
    &sd1_update,
#endif//TCFG_SD1_ENABLE
};

static void dev_update_callback(void *priv, int type, u8 cmd)
{
    struct __update_dev_reg *parm = (struct __update_dev_reg *)priv;
    if (cmd == UPDATE_LOADER_OK) {
        update_mode_api(type);
    } else {
        printf("update fail, cpu reset!!!\n");
        cpu_reset();
    }
}

void *dev_update_get_parm(int type)
{
    struct __update_dev_reg *parm = NULL;
    for (int i = 0; i < ARRAY_SIZE(update_dev_list); i++) {
        if (update_dev_list[i]->type == type) {
            parm = (struct __update_dev_reg *)update_dev_list[i];
        }
    }
    if (parm == NULL) {
        return NULL;
    }
    return (void *)&parm->u.sd;
}


void dev_update_check(char *logo)
{
    if (update_success_boot_check() == true) {
        return ;
    }
    struct __dev *dev = dev_manager_find_spec(logo, 0);
    if (dev) {
#if DEV_UPDATE_EN
        //<查找设备升级配置
        struct __update_dev_reg *parm = NULL;
        for (int i = 0; i < ARRAY_SIZE(update_dev_list); i++) {
            if (0 == strcmp(update_dev_list[i]->logo, logo)) {
                parm = (struct __update_dev_reg *)update_dev_list[i];
            }
        }
        if (parm == NULL) {
            printf("dev update without parm err!!!\n");
            return ;
        }
        //<尝试按照路径打开升级文件
        char *updata_file = (char *)updata_file_name;
        if (*updata_file == '/') {
            updata_file ++;
        }
        memset(update_path, 0, sizeof(update_path));
        sprintf(update_path, "%s%s", dev_manager_get_root_path(dev), updata_file);
        printf("update_path: %s\n", update_path);
        FILE *fd = fopen(update_path, "r");
        if (!fd) {
            ///没有升级文件， 继续跑其他解码相关的流程
            printf("open update file err!!!\n");
            return ;
        }
#if(USER_UART_UPDATE_ENABLE) && (UART_UPDATE_ROLE == UART_UPDATE_MASTER)
        uart_update_send_update_ready(update_path);
        while (get_uart_update_sta()) {
            os_time_dly(10);
        }
#else
        ///进行升级
        storage_update_loader_download_init_with_file_hdl(
            parm->type,
            update_path,
            (void *)fd,
            dev_update_callback,
            (void *)parm,
            0);
#endif// USER_UART_UPDATE_ENABLE
#endif//DEV_UPDATE_EN
    }
}



