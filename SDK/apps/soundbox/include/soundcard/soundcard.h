#ifndef __SOUNDCARD_H__
#define __SOUNDCARD_H__

#include "generic/typedef.h"
#include "app_config.h"

enum {
	ECHO_KTV_MODE,
	ECHO_ELECTRIC_MODE,
	ECHO_PITCH_MODE,
	ECHO_MAGIC_MODE,
	ECHO_BOOM_MODE,
	ECHO_MIC_PRIORITY_MODE,
	ECHO_DODGE_MODE,
};	

void soundcard_key_event_deal(u8 key_event);
void soundcard_user_msg_deal(int msg, int argc, int *argv);
void soundcard_bt_connect_status_event(u8 event);

#endif//__SOUNDCARD_H__
