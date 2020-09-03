#ifndef _AUDIO_SBC_CODEC_H_
#define _AUDIO_SBC_CODEC_H_

extern void audio_sbc_enc_init(void);
extern int audio_sbc_enc_is_work(void);
extern int audio_sbc_enc_write(s16 *data, int len);
extern int audio_sbc_enc_get_rate(void);
extern int audio_sbc_enc_get_channel_num(void);
extern void bt_emitter_set_vol(u8 vol);
extern int audio_sbc_enc_reset_buf(u8 flag);

#endif

