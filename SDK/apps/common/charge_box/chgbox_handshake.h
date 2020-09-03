#ifndef _CHGBOX_HANDSHAKE_H_
#define _CHGBOX_HANDSHAKE_H_

//handshake
#define CHGBOX_HANDSHAKE_IO  IO_PORTA_01

void chgbox_handshake_run_app(void);
void chgbox_handshake_init(void);
void chgbox_handshake_set_repeat(u8 times);
void chgbox_handshake_repeatedly(void);
#endif
