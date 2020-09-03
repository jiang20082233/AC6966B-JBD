#ifndef __IIS_H__
#define __IIS_H__

#include "generic/typedef.h"

#define ALINK_MODULE_NUM         1                      //ALINK模块个数
#define ALINK_CH_MAX             4                      //ALINK每个模块的通道数

#define ALINK_PORT_TO_MODULE(port)      ((port < 2) ? 0 : 1)

#define ALINK_SEL(idx, reg)             (((JL_ALNK_TypeDef    *)(((u8 *)JL_ALNK0) + idx*(JL_ALNK1_BASE - JL_ALNK0_BASE)))->reg)
//#define ALINK_SEL(idx, reg)             ((JL_ALNK0 + idx)->reg)

#define ALINK_DSPE(idx, x)              SFR(ALINK_SEL(idx, CON0), 6,  1, x)
#define ALINK_SOE(idx, x)               SFR(ALINK_SEL(idx, CON0), 7,  1, x)
#define ALINK_MOE(idx, x)               SFR(ALINK_SEL(idx, CON0), 8,  1, x)
#define F32_EN(idx, x)                  SFR(ALINK_SEL(idx, CON0), 9,  1, x)
#define SCLKINV(idx, x)                 SFR(ALINK_SEL(idx, CON0), 10, 1, x)
#define ALINK_EN(idx, x)                SFR(ALINK_SEL(idx, CON0), 11, 1, x)
#define ALINK_CPND(idx, x)              SFR(ALINK_SEL(idx, CON2), 0, 4, x)
#define ALINK_MSRC(idx, x)              SFR(ALINK_SEL(idx, CON3), 0, 2, x)
#define ALINK_MDIV(idx, x)              SFR(ALINK_SEL(idx, CON3), 2, 3, x)
#define ALINK_LRDIV(idx, x)             SFR(ALINK_SEL(idx, CON3), 5, 3, x)

#define ALINK_SET_ADR(idx, ch, adr)     switch(ch){\
                                           case 0:ALINK_SEL(idx, ADR0) = adr;break;\
                                           case 1:ALINK_SEL(idx, ADR1) = adr;break;\
                                           case 2:ALINK_SEL(idx, ADR2) = adr;break;\
                                           case 3:ALINK_SEL(idx, ADR3) = adr;break;\
                                        }

#define ALINK_SET_LEN(idx, len)        (ALINK_SEL(idx, LEN) = len)
#define ALINK_SET_MD(idx, ch, md)      SFR(ALINK_SEL(idx, CON1), 0, 2, md);

#define PLL_ALINK0_SEL(x)              SFR(JL_CLOCK->CLK_CON2, 6,  2, x)     //0:192M   1:160M     2:FM_CKO75M   3:1
#define PLL_ALINK1_SEL(x)              SFR(JL_CLOCK->CLK_CON2, 10, 2, x)     //0:192M   1:160M     2:FM_CKO75M   3:1
#define PLL_ALINK0_DIV(x)              SFR(JL_CLOCK->CLK_CON2, 8,  2, x)     //0:div17(选择192M必须选择此分频11.2896M)  1:div13(选择160M必须选择此分频12.288M)  2:fm/8(选择FM_CKO)   3:fm/4
#define PLL_ALINK1_DIV(x)              SFR(JL_CLOCK->CLK_CON2, 12, 2, x)

#define PLL_ALINK_SEL(idx, x)          ((!idx)? PLL_ALINK0_SEL(x): PLL_ALINK1_SEL(x))
#define PLL_ALINK_DIV(idx, x)          ((!idx)? PLL_ALINK0_DIV(x): PLL_ALINK1_DIV(x))
//port
typedef enum {
    ALINK0_PORTA,            // MCLK:PA2 SCLK:PA3  LRCK:PA4 CH0:PA0 CH1:PA1 CH2:PA5 CH3:PA6
    ALINK0_PORTB,            //MCLK:PC2 SCLK:PA5  LRCK:PA6 CH0:PA0 CH1:PA2 CH2:PA3 CH3:PA4
} ALINK_PORT;

typedef enum {
    ALINK_CH0 = BIT(0),
    ALINK_CH1 = BIT(1),
    ALINK_CH2 = BIT(2),
    ALINK_CH3 = BIT(3),
} ALINK_CHANNEL;

//ch_dir
typedef enum {
    ALINK_DIR_TX	= 0u,
    ALINK_DIR_RX		,
} ALINK_DIR;

//ch_bitwide
typedef enum {
    ALINK_LEN_16BIT = 0u,
    ALINK_LEN_24BIT		,
} ALINK_BIT_WIDE;


//ch_mode
typedef enum {
    ALINK_MD_NONE	= 0u,
    ALINK_MD_IIS		,
    ALINK_MD_IIS_LALIGN	,
    ALINK_MD_IIS_RALIGN	,
} ALINK_MODE_IIS;
typedef enum { // ALINK_DSPE(1)
    ALINK_MD_DSP0   = 1u,
    ALINK_MD_DSP1		,
} ALINK_MODE;

typedef void (*iis_isr_cbfun)(ALINK_CHANNEL ch, void *buf, u32 len);

/**
 * @brief ut初始化函数的返回结构体，含各函数指针，供外部使用
 */

typedef struct {
    u8  enable;                  	///< alink通道使能
    u8  dir;                    	///< alink方向选择
    u8  bit_wide;               	///< alink数据位宽选择
    u8  ch_md;                  	///< alink模式选择
    s16 *dma_adr;     				///< alink dma地址
} iis_channel;
typedef struct {
    iis_isr_cbfun   isr_cbfun; 		///< alink中断的回调函数句柄，不用回调函数则写入NULL，如无中断，句柄无效
    ALINK_PORT      port;         	///< alink端口选择
    u8  soe;                        ///< alink是否使能sclk和lrck
    u8  moe;                        ///< alink是否使能mclk
    u8  dsp;                        ///< alink选择扩展模式
    u32 rate;                       ///< alink采样率
    iis_channel ch[ALINK_CH_MAX];	///< alink通道参数
    u32 frame_len;                  ///< alink每次中断的数据长度
} iis_param;
#if 0
typedef struct {
    iis_isr_cbfun   isr_cbfun;                            ///< alink中断的回调函数句柄，不用回调函数则写入NULL，如无中断，句柄无效
    ALINK_PORT      port;                                 ///< alink端口选择
    ALINK_DIR       dir;                                  ///< alink方向选择
    ALINK_BIT_WIDE  bit_wide;                             ///< alink数据位宽选择
    u8  ch_mask;                                          ///< alink通道选择
    u8  ch_md;                                            ///< alink模式选择
    u32 rate;                                             ///< alink采样率
    u8  soe;                                              ///< alink是否使能sclk和lrck
    u8  moe;                                              ///< alink是否使能mclk
    s16 *dma_adr[ALINK_CH_MAX];                           ///< alink dma地址
    u32 frame_len;                                        ///< alink每次中断的数据长度
} iis_param;

#endif
#endif //IIS_H

