#include "lyrics_api.h"
#include "ui/lyrics.h"
#include "system/fs/fs.h"
#include "app_config.h"

#if (TCFG_LRC_LYRICS_ENABLE)


u8 lrc_info_buf[2324] __attribute__((aligned(4)));
volatile u8 lrc_analysis_flag = 0;

static LRC_FILE_IO lrc_file_io = {
    .seek = fseek,
    .read = fread,
};


/*----------------------------------------------------------------------------*/
/**@brief 配置翻屏的速度
   @param lrc_len--当前歌词长度，time_gap-与下一条歌词的间隔，
   @param *roll_speed - 翻屏的速度 500ms unit
   @return
   @note
 */
/*----------------------------------------------------------------------------*/
void lrc_roll_speed_ctrl(u8 lrc_len, u32 time_gap, u8 *roll_speed)
{
    ///翻页滚动速度控制
    ///printf("speed = %d,%d",lrc_len,time_gap);
    ///DVcTxt1_11 显示长度为32 bytes
    if (lrc_len > (LRC_DISPLAY_TEXT_LEN * 2)) {
        *roll_speed = ((time_gap + 2) / 3) << 1;
    } else if (lrc_len > LRC_DISPLAY_TEXT_LEN) {
        *roll_speed = time_gap;
    } else {
        *roll_speed = 250; ///never load new page
    }

}

/*----------------------------------------------------------------------------*/
/**@brief  清空显示区域
   @param
   @return
   @note
 */
/*----------------------------------------------------------------------------*/
void clr_lrc_disp_buff(void)
{
    /* lcd_clear_area_rect(4, 8, 0, 128); */
}


/*----------------------------------------------------------------------------*/
/**@brief 歌词模块初始化
   @param
   @return 0--成功，非0 失败
   @note
 */
/*----------------------------------------------------------------------------*/
int lrc_init(void)
{
    LRC_CFG t_lrc_cfg;
    t_lrc_cfg.once_read_len = ONCE_READ_LENGTH;
    t_lrc_cfg.once_disp_len = ONCE_DIS_LENGTH;
    t_lrc_cfg.label_temp_buf_len = LABEL_TEMP_BUF_LEN;
    t_lrc_cfg.roll_speed_ctrl_cb = lrc_roll_speed_ctrl;
    t_lrc_cfg.clr_lrc_disp_cb = NULL;//clr_lrc_disp_buff;
    /* t_lrc_cfg.lrc_text_id = LRC_DISPLAY_TEXT_ID; */
    t_lrc_cfg.read_next_lrc_flag = 0;
    t_lrc_cfg.enable_save_lable_to_flash = LRC_ENABLE_SAVE_LABEL_TO_FLASH;

    u32 need_buf_size = LRC_SIZEOF_ALIN(sizeof(LRC_INFO), 4)
                        + LRC_SIZEOF_ALIN(sizeof(LABEL_INFO), 4)
                        + LRC_SIZEOF_ALIN(sizeof(SORTING_INFO), 4)
                        + LRC_SIZEOF_ALIN(t_lrc_cfg.once_disp_len, 4)
                        + LRC_SIZEOF_ALIN(t_lrc_cfg.label_temp_buf_len, 4)
                        + LRC_SIZEOF_ALIN(t_lrc_cfg.once_read_len, 4);
    printf("---lrc need_buf_size=%d---\n", need_buf_size);
    /* ASSERT(sizeof(lrc_info_buf) >= need_buf_size); */

    memset(lrc_info_buf, 0, sizeof(lrc_info_buf));


    return lrc_param_init(&t_lrc_cfg, lrc_info_buf);
}

/*----------------------------------------------------------------------------*/
/**@brief 歌词模块退出
   @param
   @return
   @note

 */
/*----------------------------------------------------------------------------*/
void lrc_exit(void)
{
    lrc_destroy();
}

/*----------------------------------------------------------------------------*/
/**@brief 搜索歌词，解析
   @param
   @return 0--成功，非0 失败
   @note
 */
/*----------------------------------------------------------------------------*/
int lrc_find(FILE *file, FILE **newFile, void *ext_name)
{
    u32 find_lrc_flag;

    find_lrc_flag = fget_file_byname_indir(file, newFile, ext_name);

    if (find_lrc_flag) {
        puts("\ncan't find lrc------\n");
        return -1;
    }

    return 0;
}

void lrc_set_analysis_flag(u8 flag)
{
    lrc_analysis_flag = flag;
}

bool lrc_show_api(int text_id, u16 dbtime_s, u8 btime_100ms)
{
    if (lrc_analysis_flag == 1) {
        return lrc_show(text_id, dbtime_s, btime_100ms);
    } else {
        return false;
    }
}

bool lrc_get_api(u16 dbtime_s, u8 btime_100ms)
{
    if (lrc_analysis_flag == 1) {
        return lrc_get(dbtime_s, btime_100ms);
    } else {
        return false;
    }
}

bool lrc_analysis_api(FILE *file, FILE **newFile)
{
    void *ext_name = "LRC";
    LRC_FILE_IO *p_lrc_file_io = &lrc_file_io;

    if (*newFile) {
        fclose(*newFile);
        *newFile = NULL;
    }

    if (0 == lrc_find(file, newFile, ext_name)) {
        if (false == lrc_analysis(*newFile, p_lrc_file_io)) {
            printf("lrc analazy err \n");
            return false;
        } else {
            printf("lrc analazy succ \n");
            return true;
        }
    } else {
        printf("lrc_find err\n");
        return false;
    }
}

#endif //TCFG_LRC_LYRICS_ENABLE

