// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)
#ifndef _LE_CLIENT_DEMO_H
#define _LE_CLIENT_DEMO_H

#include <stdint.h>

enum {
    CLI_CREAT_BY_ADDRESS = 0,//指定地址创建连接
    CLI_CREAT_BY_NAME,//指定设备名称创建连接
    CLI_CREAT_BY_TAG,//匹配厂家标识创建连接
};

typedef enum {
    CLI_EVENT_MATCH_DEV = 1,
    CLI_EVENT_CONNECTED,
    CLI_EVENT_DISCONNECT,
    CLI_EVENT_MATCH_UUID,
    CLI_EVENT_SEARCH_PROFILE_COMPLETE,
} le_client_event_e;


typedef struct {
    u16 services_uuid16;
    u16 characteristic_uuid16;
    u8  services_uuid128[16];
    u8  characteristic_uuid128[16];
    u16 opt_type;
} target_uuid_t;

//搜索操作记录的 handle
#define OPT_HANDLE_MAX   16
typedef struct {
    target_uuid_t *search_uuid;
    u16 value_handle;
} opt_handle_t;


#define CLIENT_MATCH_CONN_MAX    3
typedef struct {
    u8 create_conn_mode;
    u8 bonding_flag;//连接过后会绑定，默认快连，不搜索设备
    u8 compare_data_len;
    const u8 *compare_data;//若是地址内容,由高到低位
} client_match_cfg_t;


typedef struct {
    const client_match_cfg_t *match_dev_cfg[CLIENT_MATCH_CONN_MAX];
    u8 security_en;
    u8 search_uuid_cnt; // <= OPT_HANDLE_MAX
    const target_uuid_t *search_uuid_table;
    void (*report_data_callback)(att_data_report_t *data_report, target_uuid_t *search_uuid);
    void (*event_callback)(le_client_event_e event, u8 *packet, int size);
} client_conn_cfg_t;

void client_clear_bonding_info(void);

#endif
