#include "font/font_all.h"
#include "font/language_list.h"
#include "app_config.h"

#if TCFG_UI_ENABLE
#if TCFG_SPI_LCD_ENABLE

extern void platform_putchar(struct font_info *info, u8 *pixel, u16 width, u16 height, u16 x, u16 y);//字库输出显示函数
/* 语言字库配置 */

#define LANGUAGE  BIT(Chinese_Simplified)|\
	              BIT(English)|\
	              BIT(Chinese_Traditional)

struct font_info font_info_table[] = {

#if LANGUAGE&BIT(Chinese_Simplified)
    //gb2312
    /*
    {
        .language_id = Chinese_Simplified,
        .flags = FONT_SHOW_PIXEL | FONT_SHOW_MULTI_LINE,
        .pixel.file.name = (char *)"mnt/sdfile/res/F_GB2312.PIX",
        .ascpixel.file.name = (char *)"mnt/sdfile/res/F_ASCII.PIX",
        .tabfile.name = (char *)"mnt/sdfile/res/F_GB2312.TAB",
        .isgb2312 = true,
        .bigendian = false,
        .putchar = platform_putchar,
    },
    */
    //gbk
    {
        .language_id = Chinese_Simplified,
        .flags = FONT_SHOW_PIXEL | FONT_SHOW_MULTI_LINE,
        .pixel.file.name = (char *)"mnt/sdfile/res/F_GBK.PIX",
        .ascpixel.file.name = (char *)"mnt/sdfile/res/F_ASCII.PIX",
        .tabfile.name = (char *)"mnt/sdfile/res/F_GBK.TAB",
        .isgb2312 = false,
        .bigendian = false,
        .putchar = platform_putchar,
    },
#endif

#if LANGUAGE&BIT(Chinese_Traditional)
    {
        .language_id = Chinese_Traditional,
        .flags = FONT_SHOW_PIXEL | FONT_SHOW_MULTI_LINE,
        .pixel.file.name = (char *)"mnt/sdfile/res/F_GBK.PIX",
        .ascpixel.file.name = (char *)"mnt/sdfile/res/F_ASCII.PIX",
        .tabfile.name = (char *)"mnt/sdfile/res/F_GBK.TAB",
        .isgb2312 = false,
        .bigendian = false,
        .putchar = platform_putchar,
    },
#endif

#if LANGUAGE&BIT(English)
    {
        .language_id = English,
        .flags = FONT_SHOW_PIXEL | FONT_SHOW_MULTI_LINE,
        .pixel.file.name = (char *)"mnt/sdfile/res/F_GBK.PIX",
        .ascpixel.file.name = (char *)"mnt/sdfile/res/F_ASCII.PIX",
        .tabfile.name = (char *)"mnt/sdfile/res/F_GBK.TAB",
        .isgb2312 = false,
        .bigendian = false,
        .putchar = platform_putchar,
    },
#endif
    {
        .language_id = 0,//不能删
    },

};




#endif
#endif
