#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 *  板级配置选择
 */


#define CONFIG_BOARD_AC6966B
// #define CONFIG_BOARD_AC6965E
// #define CONFIG_BOARD_AC696X_DEMO
// #define CONFIG_BOARD_AC6969D_DEMO
// #define CONFIG_BOARD_AC696X_LIGHTER
// #define CONFIG_BOARD_AC696X_TWS_BOX
// #define CONFIG_BOARD_AC696X_TWS
// #define CONFIG_BOARD_AC696X_SMARTBOX
// #define CONFIG_BOARD_AC6082_DEMO

#include "board_ac6966b/board_ac6966b_cfg.h"
#include "board_ac6965e/board_ac6965e_cfg.h"
#include "board_ac696x_demo/board_ac696x_demo_cfg.h"
#include "board_ac6969d_demo/board_ac6969d_demo_cfg.h"
#include "board_ac696x_lighter/board_ac696x_lighter_cfg.h"
#include "board_ac696x_tws_box/board_ac696x_tws_box.h"   //转发对箱
#include "board_ac696x_tws/board_ac696x_tws.h"   //纯对箱
#include "board_ac696x_smartbox/board_ac696x_smartbox.h"   // AI
#include "board_ac6082_demo/board_ac6082_demo_cfg.h"

#include "user_config.h"
#include "board_ac6966b/user_ac6966b_cfg.h"
#include "board_ac6965e/user_ac6965e_cfg.h"
#define  DUT_AUDIO_DAC_LDO_VOLT   							DACVDD_LDO_2_90V

#endif
