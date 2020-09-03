#ifndef _BT_EMITTER_H
#define _BT_EMITTER_H


extern void bt_search_device(void);
extern void emitter_search_noname(u8 status, u8 *addr, u8 *name);
extern u8 bt_emitter_stu_set(u8 on);
extern u8 bt_emitter_stu_get(void);
extern int bt_emitter_mic_open(void);
extern void bt_emitter_mic_close(void);
extern void emitter_media_source(u8 source, u8 en);
extern u8 emitter_search_result(char *name, u8 name_len, u8 *addr, u32 dev_class, char rssi);
extern void emitter_search_stop();
extern void emitter_media_source(u8 source, u8 en);


#endif
