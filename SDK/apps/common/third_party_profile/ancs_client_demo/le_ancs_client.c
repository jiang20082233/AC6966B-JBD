/*********************************************************************************************
    *   Filename        : le_server_module.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2017-01-17 11:14

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/

// *****************************************************************************
/* EXAMPLE_START(le_counter): LE Peripheral - Heartbeat Counter over GATT
 *
 * @text All newer operating systems provide GATT Client functionality.
 * The LE Counter examples demonstrates how to specify a minimal GATT Database
 * with a custom GATT Service and a custom Characteristic that sends periodic
 * notifications.
 */
// *****************************************************************************
#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"

#include "le_ancs_client.h"
#include "le_common.h"

#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_ANCS_ADV)

#include "btstack/btstack_event.h"
#define ANCS_SUBEVENT_CLIENT_CONNECTED                              0xF0
#define ANCS_SUBEVENT_CLIENT_NOTIFICATION                           0xF1
#define ANCS_SUBEVENT_CLIENT_DISCONNECTED                           0xF2
#define HCI_EVENT_ANCS_META                                0xEA
void ancs_client_init(void);
void ancs_client_register_callback(btstack_packet_handler_t callback);
const char *ancs_client_attribute_name_for_id(int id);
void ancs_set_notification_buffer(u8 *buffer, u16 buffer_size);

#define TEST_SEND_DATA_RATE          0  //测试上行发送数据
#define TEST_SEND_HANDLE_VAL         ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE
/* #define TEST_SEND_HANDLE_VAL         ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE */
#define EXT_ADV_MODE_EN              0

#define TEST_AUDIO_DATA_UPLOAD       0 //测试文件上传


#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info          printf
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

/* #define LOG_TAG_CONST       BT_BLE */
/* #define LOG_TAG             "[LE_S_DEMO]" */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
/* #define LOG_CLI_ENABLE */
/* #include "debug.h" */

//------
//ATT发送的包长,    note: 20 <=need >= MTU
#define ATT_LOCAL_PAYLOAD_SIZE    (200)                   //
//ATT缓存的buffer大小,  note: need >= 20,可修改
#define ATT_SEND_CBUF_SIZE        (512)                   //

//共配置的RAM
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));
//---------------

#if TEST_SEND_DATA_RATE
static u32 test_data_count;
static u32 server_timer_handle = 0;
static u8 test_data_start;
#endif

//ancs info buffer
#define ANCS_INFO_BUFFER_SIZE  (1024)
static u8 ancs_info_buffer[ANCS_INFO_BUFFER_SIZE];

//---------------
// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN          0x30//(160*5)

#define HOLD_LATENCY_CNT_MIN  (3)  //(0~0xffff)
#define HOLD_LATENCY_CNT_MAX  (15) //(0~0xffff)
#define HOLD_LATENCY_CNT_ALL  (0xffff)

static volatile hci_con_handle_t con_handle;

//加密设置
static const uint8_t sm_min_key_size = 7;

//连接参数更新请求设置
//是否使能参数请求更新,0--disable, 1--enable
static const uint8_t connection_update_enable = 1; ///0--disable, 1--enable
//当前请求的参数表index
static uint8_t connection_update_cnt = 0; //

//参数表
static const struct conn_update_param_t connection_param_table[] = {
    {16, 24, 10, 600},//11
    {12, 28, 10, 600},//3.7
    {8,  20, 10, 600},
    /* {12, 28, 4, 600},//3.7 */
    /* {12, 24, 30, 600},//3.05 */
};

//共可用的参数组数
#define CONN_PARAM_TABLE_CNT      (sizeof(connection_param_table)/sizeof(struct conn_update_param_t))

#if (ATT_RAM_BUFSIZE < 64)
#error "adv_data & rsp_data buffer error!!!!!!!!!!!!"
#endif

//用户可配对的，这是样机跟客户开发的app配对的秘钥
/* const u8 link_key_data[16] = {0x06, 0x77, 0x5f, 0x87, 0x91, 0x8d, 0xd4, 0x23, 0x00, 0x5d, 0xf1, 0xd8, 0xcf, 0x0c, 0x14, 0x2b}; */
/* #define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K' */
/* static const char user_tag_string[] = {EIR_TAG_STRING}; */

static u8 adv_data_len;
static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 scan_rsp_data_len;
static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31

/* #define adv_data       &att_ram_buffer[0] */
/* #define scan_rsp_data  &att_ram_buffer[32] */

static char gap_device_name[BT_NAME_LEN_MAX] = "jl_ble_test";
static u8 gap_device_name_len = 0; //名字长度，不包含结束符
static u8 ble_work_state = 0;      //ble 状态变化
static u8 adv_ctrl_en;             //广播控制

static u8 test_read_write_buf[4];

static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;

static int app_send_user_data_check(u16 len);
static int app_send_user_data_do(void *priv, u8 *data, u16 len);
static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);

// Complete Local Name  默认的蓝牙名字

//------------------------------------------------------
//广播参数设置
static void advertisements_setup_init();
static int set_adv_enable(void *priv, u32 en);
static int get_buffer_vaild_len(void *priv);
extern const char *bt_get_local_name();
extern void clr_wdt(void);
extern void sys_auto_shut_down_disable(void);
extern void sys_auto_shut_down_enable(void);
extern u8 get_total_connect_dev(void);
//------------------------------------------------------
#if TEST_AUDIO_DATA_UPLOAD
static const u8 test_audio_data_file[1024] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9
};

#define AUDIO_ONE_PACKET_LEN  128
static void test_send_audio_data(int init_flag)
{
    static u32 send_pt = 0;
    static u32 start_flag = 0;

    if (!con_handle) {
        return;
    }

    if (init_flag) {
        log_info("audio send init\n");
        send_pt = 0;
        start_flag = 1;
    }

    if (!start_flag) {
        return;
    }

    u32 file_size = sizeof(test_audio_data_file);
    u8 *file_ptr = test_audio_data_file;

    if (send_pt >= file_size) {
        log_info("audio send Complete\n");
        start_flag = 0;
        return;
    }

    u32 send_len = file_size - send_pt;
    if (send_len > AUDIO_ONE_PACKET_LEN) {
        send_len = AUDIO_ONE_PACKET_LEN;
    }

    while (1) {
        if (app_send_user_data_check(send_len)) {
            log_info("audio send %08x\n", send_pt);
            if (app_send_user_data(ATT_CHARACTERISTIC_ae3c_01_VALUE_HANDLE, &file_ptr[send_pt], send_len, ATT_OP_AUTO_READ_CCC)) {
                log_info("audio send fail!\n");
                break;
            } else {
                send_pt += send_len;
            }
        } else {
            break;
        }
    }
}

#endif


static void send_request_connect_parameter(u8 table_index)
{
    struct conn_update_param_t *param = (void *)&connection_param_table[table_index];//static ram

    log_info("update_request:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    if (con_handle) {
        ble_op_conn_param_request(con_handle, param);
    }
}

static void check_connetion_updata_deal(void)
{
    if (connection_update_enable) {
        if (connection_update_cnt < CONN_PARAM_TABLE_CNT) {
            send_request_connect_parameter(connection_update_cnt);
        }
    }
}

static void connection_update_complete_success(u8 *packet)
{
    int con_handle, conn_interval, conn_latency, conn_timeout;

    con_handle = hci_subevent_le_connection_update_complete_get_connection_handle(packet);
    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);
}


static void set_ble_work_state(ble_state_e state)
{
    if (state != ble_work_state) {
        log_info("ble_work_st:%x->%x\n", ble_work_state, state);
        ble_work_state = state;
        if (app_ble_state_callback) {
            app_ble_state_callback((void *)channel_priv, state);
        }
    }
}

static ble_state_e get_ble_work_state(void)
{
    return ble_work_state;
}

static void app_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            log_info("Just Works Confirmed.\n");
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("Passkey display: %06u.\n", tmp32);
            break;
        }
        break;
    }
}


#if TEST_SEND_DATA_RATE
static void server_timer_handler(void)
{
    if (!con_handle) {
        test_data_count = 0;
        test_data_start = 0;
        return;
    }

    log_info("peer_rssi = %d\n", ble_vendor_get_peer_rssi(con_handle));

    if (test_data_count) {
        log_info("\n%d bytes send: %d.%02d KB/s \n", test_data_count, test_data_count / 1000, test_data_count % 1000);
        test_data_count = 0;
    }
}

static void server_timer_start(void)
{
    if (server_timer_handle) {
        return;
    }

    server_timer_handle  = sys_timer_add(NULL, server_timer_handler, 1000);
}

static void server_timer_stop(void)
{
    if (server_timer_handle) {
        sys_timeout_del(server_timer_handle);
        server_timer_handle = 0;
    }
}

void test_data_send_packet(void)
{
    u32 vaild_len = get_buffer_vaild_len(0);//获取发送buffer可写入的数据
    if (!test_data_start) {
        return;
    }

    if (vaild_len) {
        if (!app_send_user_data(TEST_SEND_HANDLE_VAL, (void *)&test_data_count, vaild_len, ATT_OP_AUTO_READ_CCC)) {
            test_data_count += vaild_len;
        }
    }
    clr_wdt();
}
#endif


static void can_send_now_wakeup(void)
{
    /* putchar('E'); */
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }

#if TEST_SEND_DATA_RATE
    test_data_send_packet();
#endif

#if TEST_AUDIO_DATA_UPLOAD
    test_send_audio_data(0);
#endif

}

static void ble_auto_shut_down_enable(u8 enable)
{
#if TCFG_AUTO_SHUT_DOWN_TIME
    if (enable) {
        if (get_total_connect_dev() == 0) {    //已经没有设备连接
            sys_auto_shut_down_enable();
        }
    } else {
        sys_auto_shut_down_disable();
    }
#endif
}

const char *const phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};

static void set_connection_data_length(u16 tx_octets, u16 tx_time)
{
    if (con_handle) {
        ble_op_set_data_length(con_handle, tx_octets, tx_time);
    }
}

static void set_connection_data_phy(u8 tx_phy, u8 rx_phy)
{
    if (0 == con_handle) {
        return;
    }

    u8 all_phys = 0;
    u16 phy_options = CONN_SET_PHY_OPTIONS_S8;

    ble_op_set_ext_phy(con_handle, all_phys, tx_phy, rx_phy, phy_options);
}

static void server_profile_start(u16 con_handle)
{
#if BT_FOR_APP_EN
    set_app_connect_type(TYPE_BLE);
#endif
//    ble_op_att_send_init(con_handle, att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_PAYLOAD_SIZE);
    set_ble_work_state(BLE_ST_CONNECT);
    ble_auto_shut_down_enable(0);

    /* set_connection_data_phy(CONN_SET_CODED_PHY, CONN_SET_CODED_PHY); */
}

_WEAK_
u8 ble_update_get_ready_jump_flag(void)
{
    return 0;
}

static void ancs_callback(u8 packet_type, u16 channel, u8 *packet, u16 size)
{
    const char *attribute_name;
    if (hci_event_packet_get_type(packet) != HCI_EVENT_ANCS_META) {
        printf(".>>>>>>no ance envet \n");
        return;
    }

    switch (hci_event_ancs_meta_get_subevent_code(packet)) {
    case ANCS_SUBEVENT_CLIENT_CONNECTED:
        printf("ANCS Client: Connected \n");
        server_profile_start(/*con_handle*/0);
        break;
    case ANCS_SUBEVENT_CLIENT_DISCONNECTED:
        printf("ANCS Client: Disconnected \n");
        set_ble_work_state(BLE_ST_DISCONN);

        if (!ble_update_get_ready_jump_flag()) {
            bt_ble_adv_enable(1);
        }
        connection_update_cnt = 0;
#if BT_FOR_APP_EN
        set_app_connect_type(TYPE_NULL);
#endif
        ble_auto_shut_down_enable(1);
        break;
    case ANCS_SUBEVENT_CLIENT_NOTIFICATION:
        printf("ANCS_SUBEVENT_CLIENT_NOTIFICATION \n");
        attribute_name = ancs_client_attribute_name_for_id(ancs_subevent_client_notification_get_attribute_id(packet));
        if (!attribute_name) {
            printf("ancs unknow attribute_id :%d \n", ancs_subevent_client_notification_get_attribute_id(packet));
            break;
        } else {
            u16 attribute_strlen = little_endian_read_16(packet, 7);
            u8 *attribute_str = (void *)little_endian_read_32(packet, 9);
            printf("Notification: %s - %s \n", attribute_name, attribute_str);
        }
        break;
    default:
        break;
    }

}

static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type)
{
    u32 ret = APP_BLE_NO_ERROR;

    if (!con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (!att_get_ccc_config(handle + 1)) {
        log_info("fail,no write ccc!!!,%04x\n", handle + 1);
        return APP_BLE_NO_WRITE_CCC;
    }

    ret = ble_op_att_send_data(handle, data, len, handle_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }

    if (ret) {
        log_info("app_send_fail:%d !!!!!!\n", ret);
    }
    return ret;
}

//------------------------------------------------------
static int make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = adv_data;
    const u8 _adv_data[] = {
        // Flags general discoverable, BR/EDR not supported
        0x02, 0x01, 0x06,
        // Name
        0x05, 0x09, 'A', 'N', 'C', 'S',
        // Service Solicitation, 128-bit UUIDs - ANCS (little endian)
        0x11, 0x15, 0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4, 0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79
    };

#if 0
    /* offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x18, 1); */
    /* offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x1A, 1); */
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    printf(">>>>>>>>test \n");
    printf_buf(buf, offset);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, 0xAF30, 2);

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
#endif
    offset = sizeof(_adv_data);
    memcpy(adv_data, _adv_data, offset);
    adv_data_len = offset;
    ble_op_set_adv_data(offset, buf);
    return 0;
}

static int make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = scan_rsp_data;

#if RCSP_BTMATE_EN
    u8  tag_len = sizeof(user_tag_string);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, (void *)user_tag_string, tag_len);
#endif

    u8 name_len = gap_device_name_len;
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len > vaild_len) {
        name_len = vaild_len;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    scan_rsp_data_len = offset;
    ble_op_set_rsp_data(offset, buf);
    return 0;
}

//广播参数设置
static void advertisements_setup_init()
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    int   ret = 0;

    ble_op_set_adv_param(ADV_INTERVAL_MIN, adv_type, adv_channel);

    ret |= make_set_adv_data();
    ret |= make_set_rsp_data();

    if (ret) {
        puts("advertisements_setup_init fail !!!!!!\n");
        return;
    }

}

#define PASSKEY_ENTER_ENABLE      0 //输入passkey使能，可修改passkey
//重设passkey回调函数，在这里可以重新设置passkey
//passkey为6个数字组成，十万位、万位。。。。个位 各表示一个数字 高位不够为0
static void reset_passkey_cb(u32 *key)
{
#if 1
    u32 newkey = rand32();//获取随机数

    newkey &= 0xfffff;
    if (newkey > 999999) {
        newkey = newkey - 999999; //不能大于999999
    }
    *key = newkey; //小于或等于六位数
    printf("set new_key= %06u\n", *key);
#else
    *key = 123456; //for debug
#endif
}

void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
{
    //setup SM: Display only
    sm_init();
    sm_set_io_capabilities(io_type);
    sm_set_authentication_requirements(auth_req);
    sm_set_encryption_key_size_range(min_key_size, 16);
    sm_set_request_security(security_en);
    sm_event_callback_set(app_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        reset_PK_cb_register(reset_passkey_cb);
    }
}


void ble_profile_init(void)
{
    printf("ble profile init\n");
    le_device_db_init();

    ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_BONDING, 7, TCFG_BLE_SECURITY_EN);

    /* setup ATT server */
//    att_server_init(profile_data, att_read_callback, att_write_callback);
    att_server_init(profile_data, NULL, NULL);
    att_server_register_packet_handler(app_packet_handler);
    /* gatt_client_register_packet_handler(packet_cbk); */

    // register for HCI events
    hci_event_callback_set(app_packet_handler);
    /* ble_l2cap_register_packet_handler(packet_cbk); */
    /* sm_event_packet_handler_register(packet_cbk); */
    le_l2cap_register_packet_handler(app_packet_handler);
    //printf(">>>>>func:%s line:%d \n",__func__,__LINE__);

    //setup GATT client
    gatt_client_init();

    //setup ANCS clent
    ancs_client_init();
    ancs_set_notification_buffer(ancs_info_buffer, sizeof(ancs_info_buffer));
    ancs_client_register_callback(&ancs_callback);

}

#if EXT_ADV_MODE_EN


#define EXT_ADV_NAME                    'J', 'L', '_', 'E', 'X', 'T', '_', 'A', 'D', 'V'
/* #define EXT_ADV_NAME                    "JL_EXT_ADV" */
#define BYTE_LEN(x...)                  sizeof((u8 []) {x})
#define EXT_ADV_DATA                    \
    0x02, 0x01, 0x06, \
    0x03, 0x02, 0xF0, 0xFF, \
    BYTE_LEN(EXT_ADV_NAME) + 1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, EXT_ADV_NAME

struct ext_advertising_param {
    u8 Advertising_Handle;
    u16 Advertising_Event_Properties;
    u8 Primary_Advertising_Interval_Min[3];
    u8 Primary_Advertising_Interval_Max[3];
    u8 Primary_Advertising_Channel_Map;
    u8 Own_Address_Type;
    u8 Peer_Address_Type;
    u8 Peer_Address[6];
    u8 Advertising_Filter_Policy;
    u8 Advertising_Tx_Power;
    u8 Primary_Advertising_PHY;
    u8 Secondary_Advertising_Max_Skip;
    u8 Secondary_Advertising_PHY;
    u8 Advertising_SID;
    u8 Scan_Request_Notification_Enable;
} _GNU_PACKED_;

struct ext_advertising_data  {
    u8 Advertising_Handle;
    u8 Operation;
    u8 Fragment_Preference;
    u8 Advertising_Data_Length;
    u8 Advertising_Data[BYTE_LEN(EXT_ADV_DATA)];
} _GNU_PACKED_;

struct ext_advertising_enable {
    u8  Enable;
    u8  Number_of_Sets;
    u8  Advertising_Handle;
    u16 Duration;
    u8  Max_Extended_Advertising_Events;
} _GNU_PACKED_;

const struct ext_advertising_param ext_adv_param = {
    .Advertising_Handle = 0,
    .Advertising_Event_Properties = 1,
    .Primary_Advertising_Interval_Min = {30, 0, 0},
    .Primary_Advertising_Interval_Max = {30, 0, 0},
    .Primary_Advertising_Channel_Map = 7,
    .Primary_Advertising_PHY = ADV_SET_1M_PHY,
    .Secondary_Advertising_PHY = ADV_SET_1M_PHY,
};

const struct ext_advertising_data ext_adv_data = {
    .Advertising_Handle = 0,
    .Operation = 3,
    .Fragment_Preference = 0,
    .Advertising_Data_Length = BYTE_LEN(EXT_ADV_DATA),
    .Advertising_Data = EXT_ADV_DATA,
};

const struct ext_advertising_enable ext_adv_enable = {
    .Enable = 1,
    .Number_of_Sets = 1,
    .Advertising_Handle = 0,
    .Duration = 0,
    .Max_Extended_Advertising_Events = 0,
};

const struct ext_advertising_enable ext_adv_disable = {
    .Enable = 0,
    .Number_of_Sets = 1,
    .Advertising_Handle = 0,
    .Duration = 0,
    .Max_Extended_Advertising_Events = 0,
};

#endif /* EXT_ADV_MODE_EN */

static int set_adv_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    if (!adv_ctrl_en) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (en) {
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state =  get_ble_work_state();
    switch (cur_state) {
    case BLE_ST_ADV:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
        break;
    default:
        return APP_BLE_OPERATION_ERROR;
        break;
    }

    if (cur_state == next_state) {
        return APP_BLE_NO_ERROR;
    }
    log_info("adv_en:%d\n", en);
    set_ble_work_state(next_state);

#if EXT_ADV_MODE_EN
    if (en) {
        ble_op_set_ext_adv_param(&ext_adv_param, sizeof(ext_adv_param));

        log_info_hexdump(&ext_adv_data, sizeof(ext_adv_data));
        ble_op_set_ext_adv_data(&ext_adv_data, sizeof(ext_adv_data));

        ble_op_set_ext_adv_enable(&ext_adv_enable, sizeof(ext_adv_enable));
    } else {
        ble_op_set_ext_adv_enable(&ext_adv_disable, sizeof(ext_adv_disable));
    }
#else
    if (en) {
        advertisements_setup_init();
    }
    ble_op_adv_enable(en);
#endif /* EXT_ADV_MODE_EN */

    return APP_BLE_NO_ERROR;
}

static int ble_disconnect(void *priv)
{
    if (con_handle) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            log_info(">>>ble send disconnect\n");
            set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_op_disconnect(con_handle);
        } else {
            log_info(">>>ble wait disconnect...\n");
        }
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}


static int get_buffer_vaild_len(void *priv)
{
    u32 vaild_len = 0;
    ble_op_att_get_remain(&vaild_len);
    return vaild_len;
}

static int app_send_user_data_do(void *priv, u8 *data, u16 len)
{
#if 0
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-le_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    return app_send_user_data(ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
#else
    return 0;
#endif
}

static int app_send_user_data_check(u16 len)
{
    u32 buf_space = get_buffer_vaild_len(0);
    if (len <= buf_space) {
        return 1;
    }
    return 0;
}


static int regiest_wakeup_send(void *priv, void *cbk)
{
    ble_resume_send_wakeup = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_recieve_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_recieve_callback = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_state_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_ble_state_callback = cbk;
    return APP_BLE_NO_ERROR;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


u8 *ble_get_scan_rsp_ptr(u16 *len)
{
    if (len) {
        *len = scan_rsp_data_len;
    }
    return scan_rsp_data;
}

u8 *ble_get_adv_data_ptr(u16 *len)
{
    if (len) {
        *len = adv_data_len;
    }
    return adv_data;
}

u8 *ble_get_gatt_profile_data(u16 *len)
{
    *len = sizeof(profile_data);
    return (u8 *)profile_data;
}


void bt_ble_adv_enable(u8 enable)
{
    set_adv_enable(0, enable);
}

u16 bt_ble_is_connected(void)
{
    return con_handle;
}

void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    if (en) {
        adv_ctrl_en = 1;
        bt_ble_adv_enable(1);
    } else {
        if (con_handle) {
            adv_ctrl_en = 0;
            ble_disconnect(NULL);
        } else {
            bt_ble_adv_enable(0);
            adv_ctrl_en = 0;
        }
    }
}

static const char ble_ext_name[] = "(BLE)";

void bt_ble_init(void)
{
    log_info("***** ancs client demo  ble_init******\n");
    char *name_p;
    u8 ext_name_len = sizeof(ble_ext_name) - 1;

    name_p = bt_get_local_name();
    gap_device_name_len = strlen(name_p);
    if (gap_device_name_len > BT_NAME_LEN_MAX - ext_name_len) {
        gap_device_name_len = BT_NAME_LEN_MAX - ext_name_len;
    }

    //增加后缀，区分名字
    memcpy(gap_device_name, name_p, gap_device_name_len);
    memcpy(&gap_device_name[gap_device_name_len], "(BLE)", ext_name_len);
    gap_device_name_len += ext_name_len;

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);

    set_ble_work_state(BLE_ST_INIT_OK);
    ble_module_enable(1);

#if TEST_SEND_DATA_RATE
    server_timer_start();
#endif
}

void bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");

    ble_module_enable(0);

#if TEST_SEND_DATA_RATE
    server_timer_stop();
#endif

}


void ble_app_disconnect(void)
{
    ble_disconnect(NULL);
}

#if RCSP_BTMATE_EN
static int rcsp_send_user_data_do(void *priv, u8 *data, u16 len)
{
    log_info("rcsp_tx:%x\n", len);
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-dma_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    return app_send_user_data(ATT_CHARACTERISTIC_ae02_02_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}
#endif

#if RCSP_BTMATE_EN
static const struct ble_server_operation_t mi_ble_operation = {
    .adv_enable = set_adv_enable,
    .disconnect = ble_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .send_data = (void *)rcsp_send_user_data_do,
    .regist_wakeup_send = regiest_wakeup_send,
    .regist_recieve_cbk = regiest_recieve_cbk,
    .regist_state_cbk = regiest_state_cbk,
};
#else
static const struct ble_server_operation_t mi_ble_operation = {
    .adv_enable = set_adv_enable,
    .disconnect = ble_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .send_data = (void *)app_send_user_data_do,
    .regist_wakeup_send = regiest_wakeup_send,
    .regist_recieve_cbk = regiest_recieve_cbk,
    .regist_state_cbk = regiest_state_cbk,
};
#endif

void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&mi_ble_operation;
}

void ble_server_send_test_key_num(u8 key_num)
{
    ;
}

#endif


