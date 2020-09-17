#include "dev_reg.h"
#include "app_config.h"

#if (TCFG_BS_DEV_PATH_EN)
#define SD0_BS_STORAGE_PATH		 					 	"storage/bs_sd0"
#define SD0_BS_ROOT_PATH				 			 	"storage/bs_sd0/C/"
#define SD1_BS_STORAGE_PATH		 					 	"storage/bs_sd1"
#define SD1_BS_ROOT_PATH				 			 	"storage/bs_sd1/C/"
#define UDISK0_BS_STORAGE_PATH		 				 	"storage/bs_udisk0"
#define UDISK0_BS_ROOT_PATH				 			 	"storage/bs_udisk0/C/"
#else
#define SD0_BS_STORAGE_PATH							 	NULL
#define SD0_BS_ROOT_PATH				 			 	NULL
#define SD1_BS_STORAGE_PATH		 					 	NULL
#define SD1_BS_ROOT_PATH				 			 	NULL
#define UDISK0_BS_STORAGE_PATH		 				 	NULL
#define UDISK0_BS_ROOT_PATH				 			 	NULL
#endif

//*----------------------------------------------------------------------------*/
/**@brief    设备配置表
   @param	 具体配置项请看struct __dev_reg结构体描述
   @return
   @note	 注意：
   			 例如logo逻辑盘符sd0_rec/sd1_rec/udisk_rec是做录音文件夹区分使用的,
			 在定义新的设备逻辑盘发的时候， 注意避开"_rec"后缀作为设备逻辑盘符
*/
/*----------------------------------------------------------------------------*/


const struct __dev_reg dev_reg[] = {
    //内置flash
    {
        /*logo*/			SDFILE_DEV,
        /*name*/			NULL,
        /*storage_path*/	SDFILE_MOUNT_PATH,
        /*bs_storage_path*/	NULL,
        /*root_path*/		SDFILE_RES_ROOT_PATH,
        /*bs_root_path*/	NULL,
        /*fs_type*/			"sdfile"
    },
    //内置录音
    {
        /*logo*/			"rec_sdfile",
        /*name*/			NULL,
        /*storage_path*/	"mnt/rec_sdfile",
        /*bs_storage_path*/	NULL,
        /*root_path*/		"mnt/rec_sdfile/C/",
        /*bs_root_path*/	NULL,
        /*fs_type*/			"rec_sdfile"
    },
    //sd0
    {
        /*logo*/			"sd0",
        /*name*/			"sd0",
        /*storage_path*/	"storage/sd0",
        /*bs_storage_path*/	SD0_BS_STORAGE_PATH,
        /*root_path*/		"storage/sd0/C/",
        /*bs_root_path*/	SD0_BS_ROOT_PATH,
        /*fs_type*/			"fat"
    },
    //sd1
    {
        /*logo*/			"sd1",
        /*name*/			"sd1",
        /*storage_path*/	"storage/sd1",
        /*bs_storage_path*/	SD1_BS_STORAGE_PATH,
        /*root_path*/		"storage/sd1/C/",
        /*bs_root_path*/	SD1_BS_ROOT_PATH,
        /*fs_type*/			"fat"
    },
    //u盘
    {
        /*logo*/			"udisk0",
        /*name*/			"udisk0",
        /*storage_path*/	"storage/udisk0",
        /*bs_storage_path*/	UDISK0_BS_STORAGE_PATH,
        /*root_path*/		"storage/udisk0/C/",
        /*bs_root_path*/	UDISK0_BS_ROOT_PATH,
        /*fs_type*/			"fat"
    },
    //sd0录音文件夹分区
    {
        /*logo*/			"sd0_rec",
        /*name*/			"sd0",
        /*storage_path*/	"storage/sd0",
        /*bs_storage_path*/	NULL,
        /*root_path*/		"storage/sd0/C/"REC_FOLDER_NAME,
        /*bs_root_path*/	NULL,
        /*fs_type*/			"fat"
    },
    //sd1录音文件夹分区
    {
        /*logo*/			"sd1_rec",
        /*name*/			"sd1",
        /*storage_path*/	"storage/sd1",
        /*bs_storage_path*/	NULL,
        /*root_path*/		"storage/sd1/C/"REC_FOLDER_NAME,
        /*bs_root_path*/	NULL,
        /*fs_type*/			"fat"
    },
    //u盘录音文件夹分区
    {
        /*logo*/			"udisk0_rec",
        /*name*/			"udisk0",
        /*storage_path*/	"storage/udisk0",
        /*bs_storage_path*/	NULL,
        /*root_path*/		"storage/udisk0/C/"REC_FOLDER_NAME,
        /*bs_root_path*/	NULL,
        /*fs_type*/			"fat"
    },
    //外挂fat分区
    {
        /*logo*/			"fat_nor",
        /*name*/			"fat_nor",
        /*storage_path*/	"storage/fat_nor",
        /*bs_storage_path*/	NULL,
        /*root_path*/		"storage/fat_nor/C/",
        /*bs_root_path*/	NULL,
        /*fs_type*/			"fat"
    },
    //外挂flash资源分区
    {
        /*logo*/			"res_nor",
        /*name*/			"res_nor",
        /*storage_path*/	"storage/res_nor",
        /*bs_storage_path*/	NULL,
        /*root_path*/		"storage/res_nor/C/",
        /*bs_root_path*/	NULL,
        /*fs_type*/			"nor_sdfile"
    },
    ///外挂录音分区
    {
        /*logo*/			"rec_nor",
        /*name*/			"rec_nor",
        /*storage_path*/	"storage/rec_nor",
        /*bs_storage_path*/	NULL,
        /*root_path*/		"storage/rec_nor/C/",
        /*bs_root_path*/	NULL,
        /*fs_type*/			"nor_fs"
    },
    {
        /*logo*/			"nor_ui",
        /*name*/			"nor_ui",
        /*storage_path*/	"storage/nor_ui",
        /*bs_storage_path*/	NULL,
        /*root_path*/		"storage/nor_ui/C/",
        /*bs_root_path*/	NULL,
        /*fs_type*/			"nor_sdfile"
    },
    // 虚拟U盘
    {
        /*logo*/			"vir_udisk0",
        /*name*/			"vir_udisk0",
        /*storage_path*/	"storage/vir_udisk0",
        /*bs_storage_path*/	NULL,
        /*root_path*/		"storage/vir_udisk0/C/",
        /*bs_root_path*/	NULL,
        /*fs_type*/			"fat"
    },
    //<新加设备参数请在reg end前添加!!
    //<reg end
    {
        /*logo*/			NULL,
        /*name*/			NULL,
        /*storage_path*/	NULL,
        /*bs_storage_path*/	NULL,
        /*root_path*/		NULL,
        /*bs_root_path*/	NULL,
        /*fs_type*/			NULL
    },
};


