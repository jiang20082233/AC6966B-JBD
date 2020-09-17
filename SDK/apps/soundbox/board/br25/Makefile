LIBS = \
--start-group \
   $(AR_DIR)/cpu$(LIB_SUFFIX).a \
   $(AR_DIR)/btstack$(LIB_SUFFIX).a \
   $(AR_DIR)/btctrler$(LIB_SUFFIX).a \
   $(AR_DIR)/system$(LIB_SUFFIX).a \
   $(AR_DIR)/libepmotion.a \
   $(AR_DIR)/wtg_dec_lib.a \
   $(AR_DIR)/sbc_eng_lib.a \
   $(AR_DIR)/mp3tsy_dec_lib.a \
   $(AR_DIR)/wtg_dec_lib.a \
   $(AR_DIR)/flac_dec_lib.a \
   $(AR_DIR)/ape_dec_lib.a \
   $(AR_DIR)/m4a_tws_lib.a \
   $(AR_DIR)/amr_dec_lib.a \
   $(AR_DIR)/dts_dec_lib.a \
   $(AR_DIR)/mp3_decstream_lib.a \
   $(AR_DIR)/wma_decstream_lib.a \
   $(AR_DIR)/libAptFilt_pi32v2_OnChip.a \
   $(AR_DIR)/libEchoSuppress_pi32v2_OnChip.a \
   $(AR_DIR)/libNoiseSuppress_pi32v2_OnChip.a \
   $(AR_DIR)/libSplittingFilter_pi32v2_OnChip.a \
   $(AR_DIR)/libDelayEstimate_pi32v2_OnChip.a \
   $(AR_DIR)/compressor.a \
   $(AR_DIR)/crossover_coff.a \
   $(AR_DIR)/limiter.a \
   $(AR_DIR)/lib_pitchshifter.a \
   $(AR_DIR)/lib_pitch_speed.a \
   $(AR_DIR)/lib_reverb_cal.a \
   $(AR_DIR)/lib_mp2_code.a \
   $(AR_DIR)/lib_adpcm_code.a \
   $(AR_DIR)/mp3_enc_lib.a \
   $(AR_DIR)/opus_enc_lib.a \
   $(AR_DIR)/speex_enc_lib.a \
   $(AR_DIR)/lib_rmono2stereo.a \
   $(AR_DIR)/limiter_noiseGate.a \
   $(AR_DIR)/howling.a \
   $(AR_DIR)/noisegate.a \
   $(AR_DIR)/shiftPhase.a \
   $(AR_DIR)/pemafrow.a \
   $(AR_DIR)/lib_howlings_phf.a \
   $(AR_DIR)/lib_resample_cal.a \
   $(AR_DIR)/mp3_dec_lib.a \
   $(AR_DIR)/wma_dec_lib.a \
   $(AR_DIR)/wav_dec_lib.a \
   $(AR_DIR)/agreement.a \
   $(AR_DIR)/lib_sur_cal.a \
   $(AR_DIR)/lib_vbass_cal.a \
   $(AR_DIR)/loudness.a \
   $(AR_DIR)/filtparam_cal_lib.a \
   $(AR_DIR)/media_app.a \
   $(AR_DIR)/bt_hash_enc.a \
   $(AR_DIR)/rcsp_stack.a \
   $(AR_DIR)/lib_midi_dec.a \
# $(AR_DIR)/libOpcore_maskrom_pi32v2_OnChip.a \



ifeq ($(CONFIG_ANS_V2),y)
LIBS += \
   $(AR_DIR)/media_v2$(LIB_SUFFIX).a \
   #$(AR_DIR)/libNoiseSuppress_pi32v2_OnChip_v2.a \

else
LIBS += \
   $(AR_DIR)/media$(LIB_SUFFIX).a \
   #$(AR_DIR)/libNoiseSuppress_pi32v2_OnChip.a \

endif

ifeq ($(BFILTERFUN_LIB_IN_MASKROM),n)
LIBS += \
   $(AR_DIR)/bfilterfun_lib.a
endif

ifeq ($(libFFT_pi32v2_OnChip_IN_MASKROM),n)
LIBS += \
   $(AR_DIR)/libFFT_pi32v2_OnChip.a
endif

ifeq ($(OTA_UPDATE),y)
LIBS += \
   $(AR_DIR)/update$(LIB_SUFFIX).a \

endif

ifeq ($(FLASH_256K),y)
LIBS += \
   $(AR_DIR)/crypto_toolbox_Osize.a

else
LIBS += \
   $(AR_DIR)/crypto_toolbox_Ospeed.a

endif

LIBS += \
--end-group

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_demo/board_ac696x_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_demo/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_demo/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_demo/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_demo/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_demo/key_table/touch_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6969d_demo/board_ac6969d_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6969d_demo/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6969d_demo/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6969d_demo/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6969d_demo/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6969d_demo/key_table/touch_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_lighter/board_ac696x_lighter.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_lighter/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_lighter/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_lighter/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_lighter/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_lighter/key_table/touch_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws/board_ac696x_tws.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws/key_table/touch_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws_box/board_ac696x_tws_box.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws_box/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws_box/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws_box/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws_box/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_tws_box/key_table/touch_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_smartbox/board_ac696x_smartbox.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_smartbox/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_smartbox/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_smartbox/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_smartbox/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_smartbox/key_table/touch_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6082_demo/board_ac6082_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6082_demo/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6082_demo/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6082_demo/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6082_demo/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6082_demo/key_table/touch_key_table.o \

