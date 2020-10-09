// #include "includes.h"
#include "user_gpio.h"
#ifndef _CONFIG_BOARD_AC6965E_CFG_H_
#define _CONFIG_BOARD_AC6965E_CFG_H_

#ifdef CONFIG_BOARD_AC6965E
// #include "gpio.h"

#define CONFIG_SDFILE_ENABLE
#define CONFIG_FLASH_SIZE       (1024 * 1024)

//*********************************************************************************//
//                                 配置开始                                        //
//*********************************************************************************//
#define ENABLE_THIS_MOUDLE					1
#define DISABLE_THIS_MOUDLE					0

#define ENABLE								1
#define DISABLE								0

#define LINEIN_INPUT_WAY_ANALOG      0
#define LINEIN_INPUT_WAY_ADC         1
#define LINEIN_INPUT_WAY_DAC         2

#define NO_CONFIG_PORT						(-1)

//*********************************************************************************//
//                                  app 配置                                       //
//*********************************************************************************//
#define TCFG_APP_BT_EN			            1
#define TCFG_APP_MUSIC_EN			        1
#define TCFG_APP_LINEIN_EN					1
#define TCFG_APP_FM_EN					    1
#define TCFG_APP_PC_EN					    0//1
#define TCFG_APP_RTC_EN					    0
#define TCFG_APP_RECORD_EN				    1
#define TCFG_APP_SPDIF_EN                   0
//*********************************************************************************//
//                               PCM_DEBUG调试配置                                 //
//*********************************************************************************//

//#define AUDIO_PCM_DEBUG					  	//PCM串口调试，写卡通话数据

//*********************************************************************************//
//                                 UART配置                                        //
//*********************************************************************************//
#define TCFG_UART0_ENABLE					0//ENABLE_THIS_MOUDLE                     //串口打印模块使能
#define TCFG_UART0_RX_PORT					NO_CONFIG_PORT                         //串口接收脚配置（用于打印可以选择NO_CONFIG_PORT）
#define TCFG_UART0_TX_PORT  				IO_PORT_DP//IO_PORT_DP//IO_PORTA_05                            //串口发送脚配置
#define TCFG_UART0_BAUDRATE  				1000000                                //串口波特率配置

#ifdef CONFIG_DEBUG_ENABLE
#undef TCFG_UART0_ENABLE
#define TCFG_UART0_ENABLE 1
#endif

//*********************************************************************************//
//                                 IIC配置                                        //
//*********************************************************************************//
/*软件IIC设置*/
#define TCFG_SW_I2C0_CLK_PORT               IO_PORTA_09                             //软件IIC  CLK脚选择
#define TCFG_SW_I2C0_DAT_PORT               IO_PORTA_10                             //软件IIC  DAT脚选择
#define TCFG_SW_I2C0_DELAY_CNT              50                                      //IIC延时参数，影响通讯时钟频率

/*硬件IIC端口选择
  SCL         SDA
  'A': IO_PORT_DP   IO_PORT_DM
  'B': IO_PORTC_04  IO_PORTC_05
  'C': IO_PORTB_06  IO_PORTB_07
  'D': IO_PORTA_05  IO_PORTA_06
 */
#define TCFG_HW_I2C0_PORTS                  'B'
#define TCFG_HW_I2C0_CLK                    100000                                  //硬件IIC波特率

//*********************************************************************************//
//                                 硬件SPI 配置                                        //
//*********************************************************************************//
#define	TCFG_HW_SPI0_ENABLE		0//ENABLE_THIS_MOUDLE//ENABLE_THIS_MOUDLE
//A组IO:    DI: PB2     DO: PD1     CLK: PD0
//B组IO:    DI: PC3     DO: PC5     CLK: PC4
#define TCFG_HW_SPI0_PORT		'A'
#define TCFG_HW_SPI0_BAUD		4000000L
#define TCFG_HW_SPI0_MODE		SPI_MODE_BIDIR_1BIT
#define TCFG_HW_SPI0_ROLE		SPI_ROLE_MASTER

#define	TCFG_HW_SPI1_ENABLE		DISABLE_THIS_MOUDLE//ENABLE_THIS_MOUDLE
//A组IO:    DI: PB2     DO: PB1     CLK: PB0
//B组IO:    DI: PC3     DO: PC5     CLK: PC4
#define TCFG_HW_SPI1_PORT		'A'
#define TCFG_HW_SPI1_BAUD		4000000L
#define TCFG_HW_SPI1_MODE		SPI_MODE_BIDIR_1BIT
#define TCFG_HW_SPI1_ROLE		SPI_ROLE_MASTER

#define	TCFG_HW_SPI2_ENABLE		ENABLE_THIS_MOUDLE
//A组IO:    DI: PB8     DO: PB10    CLK: PB9
//B组IO:    DI: PA13    DO: DM      CLK: DP
#define TCFG_HW_SPI2_PORT		'A'
#define TCFG_HW_SPI2_BAUD		2000000L
#define TCFG_HW_SPI2_MODE		SPI_MODE_BIDIR_1BIT
#define TCFG_HW_SPI2_ROLE		SPI_ROLE_MASTER

//*********************************************************************************//
//                                 FLASH 配置                                      //
//*********************************************************************************//
#define TCFG_NORFLASH_DEV_ENABLE			DISABLE_THIS_MOUDLE //需要关闭SD0
#define TCFG_FLASH_DEV_SPI_HW_NUM			1// 1: SPI1    2: SPI2
#define TCFG_FLASH_DEV_SPI_CS_PORT	    	IO_PORTA_03


//*********************************************************************************//
//                                  充电参数配置                                   //
//*********************************************************************************//
//是否支持芯片内置充电
#define TCFG_CHARGE_ENABLE					0//ENABLE_THIS_MOUDLE
//是否支持开机充电
#define TCFG_CHARGE_POWERON_ENABLE			0//ENABLE
//是否支持拔出充电自动开机功能
#define TCFG_CHARGE_OFF_POWERON_NE			0//ENABLE

#define TCFG_CHARGE_FULL_V					CHARGE_FULL_V_4202

#define TCFG_CHARGE_FULL_MA					CHARGE_FULL_mA_10

#define TCFG_CHARGE_MA						CHARGE_mA_60


//*********************************************************************************//
//                                  SD 配置                                        //
//*********************************************************************************//
#define     SD_CMD_DECT 	0
#define     SD_CLK_DECT  	1
#define     SD_IO_DECT 		2

//A组IO: CMD:PC4    CLK:PC5    DAT0:PC3             //D组IO: CMD:PB2    CLK:PB0    DAT0:PB3
//B组IO: CMD:PB6    CLK:PB7    DAT0:PB5             //E组IO: CMD:PA4    CLK:PC5    DAT0:DM
//C组IO: CMD:PA4    CLK:PA2    DAT0:PA3             //F组IO: CMD:PB6    CLK:PB7    DAT0:PB4
#define TCFG_SD0_ENABLE						ENABLE_THIS_MOUDLE
#define TCFG_SD0_PORTS						'A'
#define TCFG_SD0_DAT_MODE					1//AC696x不支持4线模式
#define TCFG_SD0_DET_MODE					SD_CMD_DECT
#define TCFG_SD0_DET_IO 					IO_PORT_DM//当SD_DET_MODE为2时有效
#define TCFG_SD0_DET_IO_LEVEL				0//IO检查，0：低电平检测到卡。 1：高电平(外部电源)检测到卡。 2：高电平(SD卡电源)检测到卡。
#define TCFG_SD0_CLK						(3000000*2L)

#define TCFG_SD0_SD1_USE_THE_SAME_HW	    DISABLE_THIS_MOUDLE
#if TCFG_SD0_SD1_USE_THE_SAME_HW
#define TCFG_SD1_ENABLE						1
#else
#define TCFG_SD1_ENABLE						0
#endif
#define TCFG_SD1_PORTS						'F'
#define TCFG_SD1_DAT_MODE					1//AC696x不支持4线模式
#define TCFG_SD1_DET_MODE					SD_CMD_DECT
#define TCFG_SD1_DET_IO 					IO_PORT_DM//当SD_DET_MODE为2时有效
#define TCFG_SD1_DET_IO_LEVEL				0//IO检查，0：低电平检测到卡。 1：高电平(外部电源)检测到卡。 2：高电平(SD卡电源)检测到卡。
#define TCFG_SD1_CLK						(3000000*2L)

//*********************************************************************************//
//                                 USB 配置                                        //
//*********************************************************************************//
#define TCFG_PC_ENABLE						TCFG_APP_PC_EN//PC模块使能
#define TCFG_UDISK_ENABLE					ENABLE_THIS_MOUDLE//U盘模块使能
#define TCFG_OTG_USB_DEV_EN                 BIT(0)//USB0 = BIT(0)  USB1 = BIT(1)

#include "usb_std_class_def.h"
#if (defined(CONFIG_DEBUG_ENABLE) && (TCFG_UART0_TX_PORT == IO_PORT_DP))
#undef TCFG_UDISK_ENABLE
#define TCFG_UDISK_ENABLE 0
#endif

#define TCFG_USB_PORT_CHARGE            DISABLE

#define TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0       ENABLE//DISABLE

#if (!TCFG_UDISK_ENABLE)
#undef TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
#define TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 0
#endif

#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
//复用情况下，如果使用此USB口作为充电（即LDO5V_IN连接到此USB口），
//TCFG_OTG_MODE需要或上TCFG_OTG_MODE_CHARGE，用来把charge从host区
//分开；否则不需要，如果LDO5V_IN与其他IO绑定，则不能或上
#define TCFG_DM_MULTIPLEX_WITH_SD_PORT      0//0:sd0  1:sd1 //dm 参与复用的sd配置
#undef TCFG_OTG_MODE
#define TCFG_OTG_MODE                       (TCFG_OTG_MODE_HOST|TCFG_OTG_MODE_SLAVE|TCFG_OTG_MODE_CHARGE|OTG_DET_DP_ONLY)

#undef USB_DEVICE_CLASS_CONFIG
#if TCFG_SD0_SD1_USE_THE_SAME_HW //开启了双卡的可以使能读卡器存续设备
#define     USB_DEVICE_CLASS_CONFIG (MASSSTORAGE_CLASS|SPEAKER_CLASS|MIC_CLASS|HID_CLASS)
#else
#define     USB_DEVICE_CLASS_CONFIG (SPEAKER_CLASS|MIC_CLASS|HID_CLASS)
#endif

#undef TCFG_SD0_DET_MODE
#define TCFG_SD0_DET_MODE					SD_CLK_DECT
#define TCFG_USB_SD_MULTIPLEX_IO            IO_PORTC_03//IO_PORTB_03

#endif

//*********************************************************************************//
//                                 fat_FLASH 配置                                      //
//*********************************************************************************//
#define TCFG_CODE_FLASH_ENABLE				DISABLE_THIS_MOUDLE

#define FLASH_INSIDE_REC_ENABLE             0

#if  TCFG_NORFLASH_DEV_ENABLE
#define TCFG_NOR_FAT                    1//ENABLE
#define TCFG_NOR_FS                     0//ENABLE
#define TCFG_NOR_REC                    1//ENABLE
#else
#define TCFG_NOR_FAT                    0//ENABLE
#define TCFG_NOR_FS                     0//ENABLE
#define TCFG_NOR_REC                    0//ENABLE
#endif



//*********************************************************************************//
//                                 key 配置                                        //
//*********************************************************************************//
//#define KEY_NUM_MAX                        	10
//#define KEY_NUM                            	3
#define KEY_IO_NUM_MAX						6
#define KEY_AD_NUM_MAX						10
#define KEY_IR_NUM_MAX						21
#define KEY_TOUCH_NUM_MAX					6
#define KEY_RDEC_NUM_MAX                    6
#define KEY_CTMU_TOUCH_NUM_MAX				6

#define MULT_KEY_ENABLE						DISABLE 		//是否使能组合按键消息, 使能后需要配置组合按键映射表
//*********************************************************************************//
//                                 iokey 配置                                      //
//*********************************************************************************//
#define TCFG_IOKEY_ENABLE					ENABLE_THIS_MOUDLE //是否使能IO按键

#define TCFG_IOKEY_POWER_CONNECT_WAY		ONE_PORT_TO_LOW    //按键一端接低电平一端接IO
#define TCFG_IOKEY_POWER_ONE_PORT			NO_CONFIG_PORT//IO_PORTB_01        //IO按键端口

#define TCFG_IOKEY_PP_CONNECT_WAY 		    ONE_PORT_TO_LOW  //按键一端接低电平一端接IO
#define TCFG_IOKEY_PP_ONE_PORT			    IO_PORTA_03

#define TCFG_IOKEY_MODE_CONNECT_WAY 		ONE_PORT_TO_LOW  //按键一端接低电平一端接IO
#define TCFG_IOKEY_MODE_ONE_PORT			IO_PORTA_02

#define TCFG_IOKEY_PREV_CONNECT_WAY			ONE_PORT_TO_HIGH  //按键一端接低电平一端接IO
#define TCFG_IOKEY_PREV_ONE_PORT			IO_PORTA_02

#define TCFG_IOKEY_NEXT_CONNECT_WAY 		ONE_PORT_TO_HIGH  //按键一端接低电平一端接IO
#define TCFG_IOKEY_NEXT_ONE_PORT			IO_PORTA_03

//*********************************************************************************//
//                                 adkey 配置                                      //
//*********************************************************************************//
#define TCFG_ADKEY_ENABLE                   DISABLE_THIS_MOUDLE//是否使能AD按键
#define TCFG_ADKEY_LED_IO_REUSE				DISABLE_THIS_MOUDLE	//ADKEY 和 LED IO复用，led只能设置蓝灯显示
#define TCFG_ADKEY_PORT                     IO_PORTA_06         //AD按键端口(需要注意选择的IO口是否支持AD功能)
#define TCFG_ADKEY_AD_CHANNEL               AD_CH_PA6
#define TCFG_ADKEY_EXTERN_UP_ENABLE         ENABLE_THIS_MOUDLE //是否使用外部上拉

#if TCFG_ADKEY_EXTERN_UP_ENABLE
#define R_UP    220                 //22K，外部上拉阻值在此自行设置
#else
#define R_UP    100                 //10K，内部上拉默认10K
#endif

//必须从小到大填电阻，没有则同VDDIO,填0x3ffL
#define USER_003K    (30)//3k
#define USER_005K    (51)//5.1k
#define USER_011K    (110)//11K
#define USER_014K    (140)//14K
#define USER_015K    (150)
#define USER_033K    (330)
#define USER_100K    (1000)
#define USER_NULLK   (2200)

#define USER_R_KEY_01 USER_003K
#define USER_R_KEY_02 USER_005K
#define USER_R_KEY_03 USER_011K
#define USER_R_KEY_04 USER_014K
#define USER_R_KEY_05 USER_NULLK
#define USER_R_KEY_06 USER_NULLK

#define TCFG_ADKEY_AD0(x)      (0)                                 //0R
#define TCFG_ADKEY_AD1(x)      (((x) * USER_R_KEY_01)  / (USER_R_KEY_01   + R_UP))//(0x3ffL * 30   / (30   + R_UP))     //3k
#define TCFG_ADKEY_AD2(x)      (((x) * USER_R_KEY_02)  / (USER_R_KEY_02   + R_UP))     //6.2k
#define TCFG_ADKEY_AD3(x)      (((x) * USER_R_KEY_03)  / (USER_R_KEY_03   + R_UP))     //9.1k
#define TCFG_ADKEY_AD4(x)      (((x) * USER_R_KEY_04)  / (USER_R_KEY_04  + R_UP))     //15k
#define TCFG_ADKEY_AD5(x)      (((x) * USER_R_KEY_05)  / (USER_R_KEY_05  + R_UP))//(((x) * 240)  / (240  + R_UP))     //24k
#define TCFG_ADKEY_AD6(x)      (((x) * USER_R_KEY_06)  / (USER_R_KEY_06  + R_UP))     //33k
#define TCFG_ADKEY_AD7(x)      (((x) * 510)  / (510  + R_UP))     //51k
#define TCFG_ADKEY_AD8(x)      (((x) * 1000) / (1000 + R_UP))     //100k
#define TCFG_ADKEY_AD9(x)      (((x) * 2200) / (2200 + R_UP))     //220k
#define TCFG_ADKEY_VDDIO       (0x3ffL)

#define TCFG_ADKEY_VOLTAGE0(x) ((TCFG_ADKEY_AD0(x) + TCFG_ADKEY_AD1(x)) / 2)
#define TCFG_ADKEY_VOLTAGE1(x) ((TCFG_ADKEY_AD1(x) + TCFG_ADKEY_AD2(x)) / 2)
#define TCFG_ADKEY_VOLTAGE2(x) ((TCFG_ADKEY_AD2(x) + TCFG_ADKEY_AD3(x)) / 2)
#define TCFG_ADKEY_VOLTAGE3(x) ((TCFG_ADKEY_AD3(x) + TCFG_ADKEY_AD4(x)) / 2)
#define TCFG_ADKEY_VOLTAGE4(x) ((TCFG_ADKEY_AD4(x) + TCFG_ADKEY_AD5(x)) / 2)
#define TCFG_ADKEY_VOLTAGE5(x) ((TCFG_ADKEY_AD5(x) + TCFG_ADKEY_AD6(x)) / 2)
#define TCFG_ADKEY_VOLTAGE6(x) ((TCFG_ADKEY_AD6(x) + TCFG_ADKEY_AD7(x)) / 2)
#define TCFG_ADKEY_VOLTAGE7(x) ((TCFG_ADKEY_AD7(x) + TCFG_ADKEY_AD8(x)) / 2)
#define TCFG_ADKEY_VOLTAGE8(x) ((TCFG_ADKEY_AD8(x) + TCFG_ADKEY_AD9(x)) / 2)
#define TCFG_ADKEY_VOLTAGE9(x) ((TCFG_ADKEY_AD9(x) + TCFG_ADKEY_VDDIO) / 2)

#define TCFG_ADKEY_VALUE0                   0
#define TCFG_ADKEY_VALUE1                   1
#define TCFG_ADKEY_VALUE2                   2
#define TCFG_ADKEY_VALUE3                   3
#define TCFG_ADKEY_VALUE4                   4
#define TCFG_ADKEY_VALUE5                   5
#define TCFG_ADKEY_VALUE6                   6
#define TCFG_ADKEY_VALUE7                   7
#define TCFG_ADKEY_VALUE8                   8
#define TCFG_ADKEY_VALUE9                   9

//*********************************************************************************//
//                                 irkey 配置                                      //
//*********************************************************************************//
#define TCFG_IRKEY_ENABLE                   ENABLE_THIS_MOUDLE//DISABLE_THIS_MOUDLE//是否使能ir按键
#define TCFG_IRKEY_PORT                     IO_PORTB_05        //IR按键端口

//*********************************************************************************//
//                             tocuh key 配置 (不支持)                                      //
//*********************************************************************************//
//#define TCFG_TOUCH_KEY_ENABLE 				ENABLE_THIS_MOUDLE 		//是否使能触摸按键
#define TCFG_TOUCH_KEY_ENABLE 				DISABLE_THIS_MOUDLE 		//是否使能触摸按键

/* 触摸按键计数参考时钟选择, 频率越高, 精度越高
** 可选参数:
	1.TOUCH_KEY_OSC_CLK,
    2.TOUCH_KEY_MUX_IN_CLK,  //外部输入, ,一般不用, 保留
    3.TOUCH_KEY_PLL_192M_CLK,
    4.TOUCH_KEY_PLL_240M_CLK,
*/
#define TCFG_TOUCH_KEY_CLK 					TOUCH_KEY_PLL_192M_CLK 	//触摸按键时钟配置
#define TCFG_TOUCH_KEY_CHANGE_GAIN 			4 	//变化放大倍数, 一般固定
#define TCFG_TOUCH_KEY_PRESS_CFG 			-100//触摸按下灵敏度, 类型:s16, 数值越大, 灵敏度越高
#define TCFG_TOUCH_KEY_RELEASE_CFG0 		-50 //触摸释放灵敏度0, 类型:s16, 数值越大, 灵敏度越高
#define TCFG_TOUCH_KEY_RELEASE_CFG1 		-80 //触摸释放灵敏度1, 类型:s16, 数值越大, 灵敏度越高

//key0配置
#define TCFG_TOUCH_KEY0_PORT 				IO_PORTB_06  //触摸按键IO配置
#define TCFG_TOUCH_KEY0_VALUE 				1 		 	 //触摸按键key0 按键值

//key1配置
#define TCFG_TOUCH_KEY1_PORT 				IO_PORTB_07  //触摸按键key1 IO配置
#define TCFG_TOUCH_KEY1_VALUE 				2 		 	 //触摸按键key1按键值

//*********************************************************************************//
//                            ctmu tocuh key 配置 (不支持)                                     //
//*********************************************************************************//
#define TCFG_CTMU_TOUCH_KEY_ENABLE              DISABLE_THIS_MOUDLE             //是否使能CTMU触摸按键
//key0配置
#define TCFG_CTMU_TOUCH_KEY0_PORT 				IO_PORTB_06  //触摸按键key0 IO配置
#define TCFG_CTMU_TOUCH_KEY0_VALUE 				0 		 	 //触摸按键key0 按键值

//key1配置
#define TCFG_CTMU_TOUCH_KEY1_PORT 				IO_PORTB_07  //触摸按键key1 IO配置
#define TCFG_CTMU_TOUCH_KEY1_VALUE 				1 		 	 //触摸按键key1 按键值

//*********************************************************************************//
//                                 rdec_key 配置                                      //
//*********************************************************************************//
#define TCFG_RDEC_KEY_ENABLE					DISABLE_THIS_MOUDLE //是否使能RDEC按键
//RDEC0配置
#define TCFG_RDEC0_ECODE1_PORT					IO_PORTA_03
#define TCFG_RDEC0_ECODE2_PORT					IO_PORTA_04
#define TCFG_RDEC0_KEY0_VALUE 				 	0
#define TCFG_RDEC0_KEY1_VALUE 				 	1

//RDEC1配置
#define TCFG_RDEC1_ECODE1_PORT					IO_PORTB_02
#define TCFG_RDEC1_ECODE2_PORT					IO_PORTB_03
#define TCFG_RDEC1_KEY0_VALUE 				 	2
#define TCFG_RDEC1_KEY1_VALUE 				 	3

//RDEC2配置
#define TCFG_RDEC2_ECODE1_PORT					IO_PORTB_09
#define TCFG_RDEC2_ECODE2_PORT					IO_PORTB_08
#define TCFG_RDEC2_KEY0_VALUE 				 	4
#define TCFG_RDEC2_KEY1_VALUE 				 	5

//*********************************************************************************//
//                                 Audio配置                                       //
//*********************************************************************************//
#define TCFG_AUDIO_ADC_ENABLE				ENABLE_THIS_MOUDLE
//MIC只有一个声道，固定选择右声道
#define TCFG_AUDIO_ADC_MIC_CHA				LADC_CH_MIC_L
/*MIC LDO电流档位设置：
    0:0.625ua    1:1.25ua    2:1.875ua    3:2.5ua*/
#define TCFG_AUDIO_ADC_LDO_SEL				2

// LADC通道
#define TCFG_AUDIO_ADC_LINE_CHA0			LADC_LINE1_MASK
#define TCFG_AUDIO_ADC_LINE_CHA1			LADC_CH_LINE0_L

#define TCFG_AUDIO_DAC_ENABLE				ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_DAC_LDO_SEL				1
/*
DACVDD电压设置(要根据具体的硬件接法来确定):
    DACVDD_LDO_1_20V        DACVDD_LDO_1_30V        DACVDD_LDO_2_35V        DACVDD_LDO_2_50V
    DACVDD_LDO_2_65V        DACVDD_LDO_2_80V        DACVDD_LDO_2_95V        DACVDD_LDO_3_10V*/
#define TCFG_AUDIO_DAC_LDO_VOLT				DACVDD_LDO_2_90V
/*预留接口，未使用*/
#define TCFG_AUDIO_DAC_PA_PORT				NO_CONFIG_PORT
/*
DAC硬件上的连接方式,可选的配置：
    DAC_OUTPUT_MONO_L               左声道
    DAC_OUTPUT_MONO_R               右声道
    DAC_OUTPUT_LR                   立体声
    DAC_OUTPUT_MONO_LR_DIFF         单声道差分输出
*/
#define TCFG_AUDIO_DAC_CONNECT_MODE   DAC_OUTPUT_LR// DAC_OUTPUT_MONO_L

/*
解码后音频的输出方式:
    AUDIO_OUTPUT_ORIG_CH            按原始声道输出
    AUDIO_OUTPUT_STEREO             按立体声
    AUDIO_OUTPUT_L_CH               只输出原始声道的左声道
    AUDIO_OUTPUT_R_CH               只输出原始声道的右声道
    AUDIO_OUTPUT_MONO_LR_CH         输出左右合成的单声道
 */

#define AUDIO_OUTPUT_MODE          AUDIO_OUTPUT_MONO_LR_CH

#define AUDIO_OUTPUT_WAY_DAC        0
#define AUDIO_OUTPUT_WAY_IIS        1
#define AUDIO_OUTPUT_WAY_FM         2
#define AUDIO_OUTPUT_WAY_HDMI       3
#define AUDIO_OUTPUT_WAY_SPDIF      4
#define AUDIO_OUTPUT_WAY_BT      	5	// bt emitter
#define AUDIO_OUTPUT_WAY_DONGLE		7
#define AUDIO_OUTPUT_WAY            AUDIO_OUTPUT_WAY_DAC
#define LINEIN_INPUT_WAY            LINEIN_INPUT_WAY_ANALOG

#define AUDIO_OUTPUT_AUTOMUTE       ENABLE
/*
 *系统音量类型选择
 *软件数字音量是指纯软件对声音进行运算后得到的
 *硬件数字音量是指dac内部数字模块对声音进行运算后输出
 */
#define VOL_TYPE_DIGITAL		0	//软件数字音量
#define VOL_TYPE_ANALOG			1	//硬件模拟音量
#define VOL_TYPE_AD				2	//联合音量(模拟数字混合调节)
#define VOL_TYPE_DIGITAL_HW		3  	//硬件数字音量
#define SYS_VOL_TYPE            VOL_TYPE_ANALOG
/*
 *通话的时候使用数字音量
 *0：通话使用和SYS_VOL_TYPE一样的音量调节类型
 *1：通话使用数字音量调节，更加平滑
 */
#define TCFG_CALL_USE_DIGITAL_VOLUME		0

// 使能改宏，提示音音量使用music音量
#define APP_AUDIO_STATE_WTONE_BY_MUSIC      (1)//TONE_MODE_DEFAULE_VOLUME 不为0 APP_AUDIO_STATE_WTONE_BY_MUSIC宏失效
// 0:提示音不使用默认音量； 1:默认提示音音量值
#define TONE_MODE_DEFAULE_VOLUME            (20)
//*********************************************************************************//
//                                  充电仓配置  (不支持)                                   //
//*********************************************************************************//
#define TCFG_CHARGESTORE_ENABLE				DISABLE_THIS_MOUDLE       //是否支持智能充点仓
#define TCFG_TEST_BOX_ENABLE			    0
#define TCFG_CHARGESTORE_PORT				IO_PORTA_02               //耳机和充点仓通讯的IO口
#define TCFG_CHARGESTORE_UART_ID			IRQ_UART1_IDX             //通讯使用的串口号

#ifdef AUDIO_PCM_DEBUG
#ifdef	TCFG_TEST_BOX_ENABLE
#undef 	TCFG_TEST_BOX_ENABLE
#define TCFG_TEST_BOX_ENABLE				0		//因为使用PCM使用到了串口1
#endif
#endif/*AUDIO_PCM_DEBUG*/

//*********************************************************************************//
//                                  LED 配置                                       //
//******************************************************************************
#if TCFG_ADKEY_LED_IO_REUSE
//打开ADKEY和LED IO复用功能，LED使用ADKEY_IO
#define TCFG_PWMLED_ENABLE					ENABLE_THIS_MOUDLE			//是否支持PMW LED推灯模块
#define TCFG_PWMLED_IOMODE					LED_ONE_IO_MODE				//LED模式，单IO还是两个IO推灯
#define TCFG_PWMLED_PIN						TCFG_ADKEY_PORT						//LED使用的IO口

#else

#define TCFG_PWMLED_ENABLE					DISABLE_THIS_MOUDLE			//是否支持PMW LED推灯模块
#define TCFG_PWMLED_IOMODE					LED_ONE_IO_MODE				//LED模式，单IO还是两个IO推灯
#define TCFG_PWMLED_PIN						IO_PORTB_00					//LED使用的IO口

#endif
//*********************************************************************************//
//                                  UI 配置                                        //
//*********************************************************************************//
#define TCFG_UI_ENABLE 						DISABLE_THIS_MOUDLE 	//UI总开关
#define CONFIG_UI_STYLE                     STYLE_JL_LED7
#define TCFG_UI_LED7_ENABLE 			 	ENABLE_THIS_MOUDLE 	//UI使用LED7显示
// #define TCFG_UI_LCD_SEG3X9_ENABLE 		ENABLE_THIS_MOUDLE 	//UI使用LCD段码屏显示
// #define TCFG_LCD_ST7735S_ENABLE	        ENABLE_THIS_MOUDLE
// #define TCFG_LCD_ST7789VW_ENABLE	        ENABLE_THIS_MOUDLE
#define TCFG_SPI_LCD_ENABLE                 DISABLE_THIS_MOUDLE //spi lcd开关
#define TCFG_TFT_LCD_DEV_SPI_HW_NUM			 1// 1: SPI1    2: SPI2 配置lcd选择的spi口

//*********************************************************************************//
//                                  时钟配置                                       //
//*********************************************************************************//
#define TCFG_CLOCK_SYS_SRC					SYS_CLOCK_INPUT_PLL_BT_OSC   //系统时钟源选择
#define TCFG_CLOCK_SYS_HZ					24000000                     //系统时钟设置
#define TCFG_CLOCK_OSC_HZ					24000000                     //外界晶振频率设置
#define TCFG_CLOCK_MODE                     CLOCK_MODE_ADAPTIVE

//*********************************************************************************//
//                                  低功耗配置                                     //
//*********************************************************************************//
#define TCFG_LOWPOWER_POWER_SEL				PWR_LDO15                    //电源模式设置，可选DCDC和LDO
#define TCFG_LOWPOWER_BTOSC_DISABLE			0                            //低功耗模式下BTOSC是否保持
#define TCFG_LOWPOWER_LOWPOWER_SEL			0//SLEEP_EN                     //SNIFF状态下芯片是否进入powerdown
/*强VDDIO等级配置,可选：
    VDDIOM_VOL_20V    VDDIOM_VOL_22V    VDDIOM_VOL_24V    VDDIOM_VOL_26V
    VDDIOM_VOL_30V    VDDIOM_VOL_30V    VDDIOM_VOL_32V    VDDIOM_VOL_36V*/
#define TCFG_LOWPOWER_VDDIOM_LEVEL			VDDIOM_VOL_30V    //VDDIO 设置的值要和vbat的压差要大于300mv左右，否则会出现DAC杂音
/*弱VDDIO等级配置，可选：
    VDDIOW_VOL_21V    VDDIOW_VOL_24V    VDDIOW_VOL_28V    VDDIOW_VOL_32V*/
#define TCFG_LOWPOWER_VDDIOW_LEVEL			VDDIOW_VOL_28V               //弱VDDIO等级配置

//*********************************************************************************//
//                                  EQ配置                                         //
//*********************************************************************************//
//EQ配置，使用在线EQ时，EQ文件和EQ模式无效。有EQ文件时，默认不用EQ模式切换功能
#define TCFG_EQ_ENABLE                      1     //支持EQ功能
#define TCFG_EQ_ONLINE_ENABLE               0     //支持在线EQ调试
#define TCFG_BT_MUSIC_EQ_ENABLE             1      //支持蓝牙音乐EQ
#define TCFG_PHONE_EQ_ENABLE                1      //支持通话近端EQ
#define TCFG_MUSIC_MODE_EQ_ENABLE           1     //支持音乐模式EQ
#define TCFG_LINEIN_MODE_EQ_ENABLE          1     //支持linein近端EQ
#define TCFG_FM_MODE_EQ_ENABLE              0     //支持fm模式EQ
#define TCFG_SPDIF_MODE_EQ_ENABLE           0     //支持SPDIF模式EQ
#define TCFG_PC_MODE_EQ_ENABLE              1     //支持pc模式EQ
#define TCFG_AUDIO_OUT_EQ_ENABLE			0 	  //mix_out后高低音EQ

#define TCFG_USE_EQ_FILE                    0    //离线eq使用配置文件还是默认系数表 1：使用文件  0 使用默认系数表
#if TCFG_EQ_ONLINE_ENABLE
#if (TCFG_USE_EQ_FILE == 0)
#undef TCFG_USE_EQ_FILE
#define TCFG_USE_EQ_FILE                    1    //开在线调试时，打开使用离线配置文件宏定义
#endif
#if TCFG_AUDIO_OUT_EQ_ENABLE
#undef TCFG_AUDIO_OUT_EQ_ENABLE
#define TCFG_AUDIO_OUT_EQ_ENABLE            0    //开在线调试时，关闭高低音
#endif
#endif
#define TCFG_DRC_ENABLE						0 	  //DRC
#define TCFG_BT_MUSIC_DRC_ENABLE            0     //支持蓝牙音乐DRC
#define TCFG_MUSIC_MODE_DRC_ENABLE          0     //支持音乐模式DRC
#define TCFG_LINEIN_MODE_DRC_ENABLE         0     //支持LINEIN模式DRC
#define TCFG_FM_MODE_DRC_ENABLE             0     //支持FM模式DRC
#define TCFG_SPDIF_MODE_DRC_ENABLE          0     //支持SPDIF模式DRC
#define TCFG_PC_MODE_DRC_ENABLE             0     //支持PC模式DRC
#define TCFG_AUDIO_OUT_DRC_ENABLE			0 	  //mix_out后drc

#define EQ_SECTION_MAX                      10    //eq段数
// ONLINE CCONFIG
// 如果调试串口是DP DM,使用eq调试串口时，需关闭usb宏
#define TCFG_ONLINE_ENABLE                  (TCFG_EQ_ONLINE_ENABLE)    //是否支持EQ在线调试功能
#define TCFG_ONLINE_TX_PORT					IO_PORT_DP                 //EQ调试TX口选择
#define TCFG_ONLINE_RX_PORT					IO_PORT_DM                 //EQ调试RX口选择




//*********************************************************************************//
//                                  mic effect 配置                                //
//*********************************************************************************//
#define TCFG_MIC_EFFECT_ENABLE       ENABLE
#define TCFG_MIC_EFFECT_DEBUG        0//调试打印
#define TCFG_MIC_EFFECT_ONLINE_ENABLE  0//混响音效在线调试使能
#if ((TCFG_ONLINE_ENABLE == 0) && TCFG_MIC_EFFECT_ONLINE_ENABLE)
#undef TCFG_ONLINE_ENABLE
#define TCFG_ONLINE_ENABLE 1
#endif

#define MIC_EFFECT_REVERB             0
#define MIC_EFFECT_ECHO               1
// #define TCFG_MIC_EFFECT_SEL           MIC_EFFECT_REVERB
#define TCFG_MIC_EFFECT_SEL           MIC_EFFECT_ECHO

#if TCFG_MIC_EFFECT_ENABLE
#define TCFG_CALLING_EN_REVERB        ENABLE
#else
#define TCFG_CALLING_EN_REVERB        DISABLE
#endif

#if TCFG_MIC_EFFECT_ENABLE
#undef MIC_AUDIO_RATE
#define     MIC_AUDIO_RATE              16000
#endif


#define TCFG_REVERB_SAMPLERATE_DEFUAL (44100)


#define TCFG_LOUDSPEAKER_ENABLE            DISABLE //不能与TCFG_MIC_EFFECT_ENABLE同时打开

//*********************************************************************************//
//                                  g-sensor配置                                   //
//*********************************************************************************//
#define TCFG_GSENSOR_ENABLE                       0     //gSensor使能
#define TCFG_DA230_EN                             0
#define TCFG_SC7A20_EN                            0
#define TCFG_STK8321_EN                           0
#define TCFG_GSENOR_USER_IIC_TYPE                 0     //0:软件IIC  1:硬件IIC

//*********************************************************************************//
//                                  系统配置                                         //
//*********************************************************************************//
#define TCFG_AUTO_SHUT_DOWN_TIME		    0   //没有蓝牙连接自动关机时间
#define TCFG_SYS_LVD_EN						1   //电量检测使能
#define TCFG_POWER_ON_NEED_KEY				0	  //是否需要按按键开机配置
#define TWFG_APP_POWERON_IGNORE_DEV         4000//上电忽略挂载设备，0时不忽略，非0则n毫秒忽略

#define TCFG_PREVENT_TASK_FILL				1	// 防止task占满cpu

//*********************************************************************************//
//                                  蓝牙配置                                       //
//*********************************************************************************//
#define TCFG_USER_TWS_ENABLE                1   //tws功能使能
#define TCFG_USER_BLE_ENABLE                0   //BLE功能使能
#define TCFG_USER_BT_CLASSIC_ENABLE         1   //经典蓝牙功能使能
#define TCFG_BT_SUPPORT_AAC                 0   //AAC格式支持
#define TCFG_USER_EMITTER_ENABLE            0   //(暂不支持)emitter功能使能
#define TCFG_BT_SNIFF_ENABLE                0   //bt sniff 功能使能

#define USER_SUPPORT_PROFILE_SPP    0
#define USER_SUPPORT_PROFILE_HFP    1
#define USER_SUPPORT_PROFILE_A2DP   1
#define USER_SUPPORT_PROFILE_AVCTP  1
#define USER_SUPPORT_PROFILE_HID    1
#define USER_SUPPORT_PROFILE_PNP    1
#define USER_SUPPORT_PROFILE_PBAP   0



#if TCFG_USER_TWS_ENABLE
#define TCFG_BD_NUM						    1   //连接设备个数配置
#define TCFG_AUTO_STOP_PAGE_SCAN_TIME       0   //配置一拖二第一台连接后自动关闭PAGE SCAN的时间(单位分钟)
#define TCFG_USER_ESCO_SLAVE_MUTE           1   //对箱通话slave出声音
#else
#define TCFG_BD_NUM						    1   //连接设备个数配置
#define TCFG_AUTO_STOP_PAGE_SCAN_TIME       0 //配置一拖二第一台连接后自动关闭PAGE SCAN的时间(单位分钟)
#define TCFG_USER_ESCO_SLAVE_MUTE           0   //对箱通话slave出声音
#endif

#define BT_INBAND_RINGTONE                  0   //是否播放手机自带来电铃声
#define BT_PHONE_NUMBER                     0   //是否播放来电报号
#define BT_SYNC_PHONE_RING                  1   //是否TWS同步播放来电铃声
#define BT_SUPPORT_DISPLAY_BAT              1   //是否使能电量显示
#define BT_SUPPORT_MUSIC_VOL_SYNC           0   //是否使能音量同步

#define TCFG_BLUETOOTH_BACK_MODE			0	//不支持后台模式

#if(TCFG_BLUETOOTH_BACK_MODE)
#error "ont support background mode!!!!"
#endif

#if (TCFG_USER_TWS_ENABLE && TCFG_BLUETOOTH_BACK_MODE) && (TCFG_BT_SNIFF_ENABLE==0) && defined(CONFIG_LOCAL_TWS_ENABLE)
#define TCFG_DEC2TWS_ENABLE					0
#define TCFG_PCM_ENC2TWS_ENABLE				0
#define TCFG_TONE2TWS_ENABLE				0
#else
#define TCFG_DEC2TWS_ENABLE					0
#define TCFG_PCM_ENC2TWS_ENABLE				0
#define TCFG_TONE2TWS_ENABLE				0
#endif


//*********************************************************************************//
//                                  REC 配置                                       //
//*********************************************************************************//
#define RECORDER_MIX_EN						DISABLE//混合录音使能
#define TCFG_RECORD_FOLDER_DEV_ENABLE       DISABLE//ENABLE//音乐播放录音区分使能


//*********************************************************************************//
//                                  linein配置                                     //
//*********************************************************************************//
#define TCFG_LINEIN_ENABLE					TCFG_APP_LINEIN_EN	// linein使能
// #define TCFG_LINEIN_LADC_IDX				0					// linein使用的ladc通道，对应ladc_list
#if (RECORDER_MIX_EN)
#define TCFG_LINEIN_LR_CH					AUDIO_LIN0L_CH//AUDIO_LIN0_LR
#else
#define TCFG_LINEIN_LR_CH					AUDIO_LIN0R_CH//AUDIO_LIN0_LR
#endif/*RECORDER_MIX_EN*/
#define TCFG_LINEIN_CHECK_PORT				NO_CONFIG_PORT			// linein检测IO
#define TCFG_LINEIN_PORT_UP_ENABLE        	1					// 检测IO上拉使能
#define TCFG_LINEIN_PORT_DOWN_ENABLE       	0					// 检测IO下拉使能
#define TCFG_LINEIN_AD_CHANNEL             	NO_CONFIG_PORT		// 检测IO是否使用AD检测
#define TCFG_LINEIN_VOLTAGE                	0					// AD检测时的阀值
#if(TCFG_MIC_EFFECT_ENABLE)
#define TCFG_LINEIN_INPUT_WAY               LINEIN_INPUT_WAY_ANALOG
#else
#if (RECORDER_MIX_EN)
#define TCFG_LINEIN_INPUT_WAY               LINEIN_INPUT_WAY_ADC//LINEIN_INPUT_WAY_ANALOG
#else
#define TCFG_LINEIN_INPUT_WAY               LINEIN_INPUT_WAY_ANALOG
#endif/*RECORDER_MIX_EN*/
#endif
#define TCFG_LINEIN_MULTIPLEX_WITH_FM		DISABLE 				// linein 脚与 FM 脚复用
#define TCFG_LINEIN_MULTIPLEX_WITH_SD		DISABLE 				// linein 检测与 SD cmd 复用
#define TCFG_LINEIN_SD_PORT		            0// 0:sd0 1:sd1     //选择复用的sd

//*********************************************************************************//
//                                  music 配置                                     //
//*********************************************************************************//
#define TCFG_DEC_G729_ENABLE                ENABLE
#define TCFG_DEC_MP3_ENABLE					ENABLE
#define TCFG_DEC_WMA_ENABLE					DISABLE
#define TCFG_DEC_WAV_ENABLE					ENABLE
#define TCFG_DEC_FLAC_ENABLE				DISABLE
#define TCFG_DEC_APE_ENABLE					DISABLE
#define TCFG_DEC_M4A_ENABLE					DISABLE
#define TCFG_DEC_ALAC_ENABLE				DISABLE
#define TCFG_DEC_AMR_ENABLE					DISABLE
#define TCFG_DEC_DTS_ENABLE					DISABLE
#define TCFG_DEC_MIDI_ENABLE                DISABLE
#define TCFG_DEC_G726_ENABLE                DISABLE
#define TCFG_DEC_MTY_ENABLE					DISABLE


#define TCFG_DEC_ID3_V1_ENABLE				DISABLE
#define TCFG_DEC_ID3_V2_ENABLE				DISABLE
#define TCFG_DEC_DECRYPT_ENABLE				DISABLE
#define TCFG_DEC_DECRYPT_KEY				(0x12345678)

////<变速变调
#define TCFG_SPEED_PITCH_ENABLE             DISABLE//
//*********************************************************************************//
//                                  fm 配置                                     //
//*********************************************************************************//
#define TCFG_FM_ENABLE							TCFG_APP_FM_EN // fm 使能
#define TCFG_FM_INSIDE_ENABLE					ENABLE
#define TCFG_FM_RDA5807_ENABLE					DISABLE
#define TCFG_FM_BK1080_ENABLE					DISABLE
#define TCFG_FM_QN8035_ENABLE					DISABLE

#define TCFG_FMIN_LADC_IDX				1				// linein使用的ladc通道，对应ladc_list
#define TCFG_FMIN_LR_CH					AUDIO_LIN1_LR
#define TCFG_FM_INPUT_WAY               LINEIN_INPUT_WAY_ANALOG

//*********************************************************************************//
//                                  fm emitter 配置 (不支持)                                    //
//*********************************************************************************//
#define TCFG_APP_FM_EMITTER_EN                  DISABLE_THIS_MOUDLE
#define TCFG_FM_EMITTER_INSIDE_ENABLE			DISABLE
#define TCFG_FM_EMITTER_AC3433_ENABLE			DISABLE
#define TCFG_FM_EMITTER_QN8007_ENABLE			DISABLE
#define TCFG_FM_EMITTER_QN8027_ENABLE			DISABLE

//*********************************************************************************//
//                                  rtc 配置(不支持)                               //
//*********************************************************************************//
#define TCFG_RTC_ENABLE						TCFG_APP_RTC_EN

#if TCFG_RTC_ENABLE
#define TCFG_USE_FAKE_RTC                   ENABLE
#define rtc_dev_ops rtc_simulate_ops
#endif

//*********************************************************************************//
//                                  SPDIF & ARC 配置(不支持)                                     //
//*********************************************************************************//
#define TCFG_SPDIF_ENABLE                       TCFG_APP_SPDIF_EN
#define TCFG_SPDIF_OUTPUT_ENABLE                ENABLE
#define TCFG_HDMI_ARC_ENABLE                    ENABLE
#define TCFG_HDMI_CEC_PORT                      IO_PORTA_02
//*********************************************************************************//
//                                  IIS 配置                                     //
//*********************************************************************************//
#define TCFG_IIS_ENABLE                       DISABLE_THIS_MOUDLE
#define TCFG_IIS_OUTPUT_EN                    ENABLE //
#define TCFG_IIS_OUTPUT_PORT                  ALINK0_PORTA
#define TCFG_IIS_OUTPUT_CH_NUM                1 //0:mono,1:stereo
#define TCFG_IIS_OUTPUT_SR                    44100
#define TCFG_IIS_OUTPUT_DATAPORT_SEL          (BIT(0)|BIT(1))

#define TCFG_IIS_INPUT_EN                    DISABLE
#define TCFG_IIS_INPUT_PORT                  ALINK0_PORTA
#define TCFG_IIS_INPUT_CH_NUM                1 //0:mono,1:stereo
#define TCFG_IIS_INPUT_SR                    44100
#define TCFG_IIS_INPUT_DATAPORT_SEL          (BIT(0))
//*********************************************************************************//
//                                  fat 文件系统配置                                       //
//*********************************************************************************//
#define CONFIG_FATFS_ENABLE					ENABLE





//*********************************************************************************//
//                                  encoder 配置                                   //
//*********************************************************************************//
#define TCFG_ENC_CVSD_ENABLE                ENABLE
#define TCFG_ENC_MSBC_ENABLE                ENABLE
#define TCFG_ENC_MP3_ENABLE                 DISABLE
#define TCFG_ENC_ADPCM_ENABLE               DISABLE
#define TCFG_ENC_SBC_ENABLE                 DISABLE
#define TCFG_ENC_OPUS_ENABLE                DISABLE
#define TCFG_ENC_SPEEX_ENABLE               DISABLE

//*********************************************************************************//
//ali ai profile
#define DUEROS_DMA_EN              0  //not surport
#define TRANS_DATA_EN              0  //not surport
#define	ANCS_CLIENT_EN			   0

#if (DUEROS_DMA_EN || TRANS_DATA_EN || ANCS_CLIENT_EN)
#define BT_FOR_APP_EN			   1
#else
#define BT_FOR_APP_EN			   0
#endif

//*********************************************************************************//
//                                 电源切换配置                                    //
//*********************************************************************************//

#define CONFIG_PHONE_CALL_USE_LDO15	    1

//*********************************************************************************//
//                                人声消除使能
//*********************************************************************************//
#define AUDIO_VOCAL_REMOVE_EN       0

//*********************************************************************************//
//                                 编译警告                                         //
//*********************************************************************************//
#if ((ANCS_CLIENT_EN || TRANS_DATA_EN || ((TCFG_ONLINE_TX_PORT == IO_PORT_DP) && TCFG_ONLINE_ENABLE)) && (TCFG_PC_ENABLE || TCFG_UDISK_ENABLE || TCFG_SD0_PORTS == 'E'))
#error "eq online adjust enable, plaease close usb marco  and sdcard port not e!!!"
#endif// ((TRANS_DATA_EN || TCFG_ONLINE_ENABLE) && (TCFG_PC_ENABLE || TCFG_UDISK_ENABLE))

#if TCFG_UI_ENABLE
#if ((TCFG_SPI_LCD_ENABLE &&  TCFG_CODE_FLASH_ENABLE) && (TCFG_FLASH_DEV_SPI_HW_NUM == TCFG_TFT_LCD_DEV_SPI_HW_NUM))
#error "flash spi port == lcd spi port, please close one !!!"
#endif//((TCFG_SPI_LCD_ENABLE &&  TCFG_CODE_FLASH_ENABLE) && (TCFG_FLASH_DEV_SPI_HW_NUM == TCFG_TFT_LCD_DEV_SPI_HW_NUM))
#endif//TCFG_UI_ENABLE

#if((TRANS_DATA_EN + DUEROS_DMA_EN + ANCS_CLIENT_EN) > 1)
#error "they can not enable at the same time,just select one!!!"
#endif//(TRANS_DATA_EN && DUEROS_DMA_EN)

#if (TCFG_DEC2TWS_ENABLE && (TCFG_APP_RECORD_EN || TCFG_APP_RTC_EN ||TCFG_DRC_ENABLE))
#error "对箱支持音源转发，请关闭录音等功能 !!!"
#endif// (TCFG_DEC2TWS_ENABLE && (TCFG_APP_RECORD_EN || TCFG_APP_RTC_EN ||TCFG_DRC_ENABLE))

#if (TCFG_MIC_EFFECT_ENABLE && (TCFG_DEC_APE_ENABLE || TCFG_DEC_FLAC_ENABLE || TCFG_DEC_DTS_ENABLE))
#error "无损格式+混响暂时不支持同时打开 !!!"
#endif//(TCFG_MIC_EFFECT_ENABLE && (TCFG_DEC_APE_ENABLE || TCFG_DEC_FLAC_ENABLE || TCFG_DEC_DTS_ENABLE))


#if ((TCFG_NORFLASH_DEV_ENABLE || TCFG_NOR_FS_ENABLE) &&  TCFG_UI_ENABLE)
#error "引脚复用问题，使用norflash需要关闭UI ！！！"
#endif


#if ((TCFG_APP_RECORD_EN) && (TCFG_USER_TWS_ENABLE))
// #error "TWS 暂不支持录音功能"
#endif

#include "app_config.h"
#if ((TCFG_SD0_ENABLE) && (TCFG_SD0_PORTS == 'D') && ((RECORDER_MIX_EN) || (TCFG_SD0_DET_MODE == SD_CMD_DECT)))
/*
 1.如果有FM模式下录音的功能，即FM和SD同时工作的情况，那么SD IO就不能用PB0,PB2,PB3这组口。
 2.如果FM模式没有录音的功能，即FM和SD不会有同时工作的情况，那么 SD IO可以使用PB0,PB2,PB3这组口，
   但SD就不能使用CMD检测。要用CLK或IO检测，这样就要有硬件上的支持，如3.3K电阻或者牺牲另一个引脚做IO检测*
 */
#error "SD IO使用D组口 引脚干扰FM的问题 ！！！"
#endif





///<<<<所有宏定义不要在编译警告后面定义！！！！！！！！！！！！！！！！！！！！！！！！！！
///<<<<所有宏定义不要在编译警告后面定义！！！！！！！！！！！！！！！！！！！！！！！！！！


//*********************************************************************************//
//                                 配置结束                                         //
//*********************************************************************************//


#endif //CONFIG_BOARD_AC693X_DEMO
#endif //CONFIG_BOARD_AC693X_DEMO_CFG_H
