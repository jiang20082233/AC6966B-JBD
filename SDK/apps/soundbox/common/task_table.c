#include "system/includes.h"
#include "app_config.h"


/*任务列表 */
const struct task_info task_info_table[] = {
    {"app_core",            1,     1024,  1024  },
    {"sys_event",           6,     256,   0    },
    {"btctrler",            4,     512,   384  },
    {"tws",                 5,     512,   128  },
    {"btstack",             3,     768,   256  },
#if (TCFG_USER_TWS_ENABLE && TCFG_REVERB_ENABLE)
    {"audio_dec",           3,     768 + 128,   128  },
#else
    {"audio_dec",           3,     768 + 32,   128  },
#endif
#if (TCFG_PREVENT_TASK_FILL)
    {"prevent",             1,     128,   0		},
#endif
    {"dev_mg",           	3,     512,   512  },
    {"audio_enc",           3,     512,   128  },
    {"usb_msd",           	1,     512,   128   },
    {"aec",					2,	   768,   128	},
    {"aec_dbg",				3,	   128,   128	},
    {"update",				1,	   512,   0		},
#if(USER_UART_UPDATE_ENABLE)
    {"uart_update",	        1,	   256,   128	},
#endif
    {"systimer",		    6,	   128,   0		},
    {"usb_audio",           5,     256,   256  },
    {"plnk_dbg",            5,     256,   256  },
    {"adc_linein",          2,     768,   128  },
    {"enc_write",           1,     768,   0 	},
    /* {"src_write",           1,     768,   256 	}, */
    {"fm_task",             3,     512,   128  },
#if (RCSP_BTMATE_EN || RCSP_ADV_EN)
    {"rcsp_task",			4,	   768,   128	},
#endif
#if TCFG_SPI_LCD_ENABLE
    {"ui",           	    2,     768,   256  },
#else
    {"ui",                  3,     384,   128  },
#endif

#if(TCFG_CHARGE_BOX_ENABLE)
    {"chgbox_n",            6,     512,   128  },
#endif
#if (SMART_BOX_EN)
    {"smartbox",            4,     768,   128  },
#endif//SMART_BOX_EN
#if RCSP_FILE_OPT
    {"file_bs",				1,	   768,	  64  },
#endif

    {"mic_stream",          5,     768,   128  },
    {0, 0},
};


