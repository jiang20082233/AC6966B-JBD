#ifndef _DUAL_BANK_API_H_
#define _DUAL_BANK_API_H_

u32 get_dual_bank_passive_update_max_buf(void);

u32 dual_bank_passive_update_init(u16 fw_crc,u32 fw_size,u16 max_pkt_len,void *priv);
u32 dual_bank_passive_update_exit(void *priv);

u32 dual_bank_update_allow_check(u32 fw_size);
u32 dual_bank_update_burn_boot_info(int (*burn_boot_info_result_hdl)(int err));
u32 dual_bank_update_write(void *data,u16 len,int (*write_complete_cb)(void *priv));
u32 dual_bank_update_verify(void (*crc_init_hdl)(void),u16 (*crc_calc_hdl)(u16 init_crc,u8 *data,u32 len),int (*verify_result_hdl)(int calc_crc));


enum {
	
	CLEAR_APP_RUNNING_AREA = 0,		
	CLEAR_APP_UPDATE_AREA,
};

int flash_update_clr_boot_info(u8 type);
#endif
