
#ifndef MIDI_DEC_API_h__
#define MIDI_DEC_API_h__


#define  test_or32_midi_n     0
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct _MIDI_CONFIG_PARM_ {

    unsigned int spi_pos;
    unsigned char sample_rate;
    short player_t;

    int(*fread)(void *file, void *buf, u32 len);
    int(*fseek)(void *file, u32 offset, int seek_mode);
} MIDI_CONFIG_PARM;



#define  VOL_Norm_Bit                12

#define  CTRL_CHANNEL_NUM             16

typedef struct _EX_CH_VOL_PARM_ {
    unsigned short  cc_vol[CTRL_CHANNEL_NUM];                 //16个通道的音量      <=>8192等于原音量
} EX_CH_VOL_PARM;


typedef  struct _EX_INFO_STRUCT_ {
    void *priv;
    u32(*mark_trigger)(void *priv, u8 *val, u8 len);
} EX_INFO_STRUCT;


typedef  struct _EX_TmDIV_STRUCT_ {
    void *priv;
    u32(*timeDiv_trigger)(void *priv);
} EX_TmDIV_STRUCT;

typedef  struct _EX_BeatTrig_STRUCT_ {
    void *priv;
    u32(*beat_trigger)(void *priv, u8 val1, u8 val2);
} EX_BeatTrig_STRUCT;

typedef  struct _EX_MELODY_STRUCT_ {
    void *priv;
    u32(*melody_trigger)(void *priv, u8 key, u8 vel);
} EX_MELODY_STRUCT;

#define  CMD_MODE4_PLAY_END          0x09

enum {
    CMD_MIDI_SEEK_BACK_N = 0x00,
    CMD_MIDI_SET_CHN_PROG,
    CMD_MIDI_CTRL_TEMPO,
    CMD_MIDI_GOON,
    CMD_MIDI_CTRL_MODE,
    CMD_MIDI_SET_SWITCH,
    CMD_MIDI_SET_EX_VOL,
    CMD_INIT_CONFIG
};

enum {
    //CMD_MIDI_CTRL_MODE_0        =0X00,	//默认模式
    CMD_MIDI_CTRL_MODE_1 = 0X01,			// one key one note
    CMD_MIDI_CTRL_MODE_2,
    CMD_MIDI_CTRL_MODE_W2S                   //外部音源
};



typedef  struct _MIDI_PLAY_CTRL_MODE_ {
    u8 mode;
} MIDI_PLAY_CTRL_MODE;

typedef  struct _MIDI_PLAY_CTRL_TEMPO_ {
    u16 tempo_val;
    u16 decay_val;             //1024
    u32 mute_threshold;
} MIDI_PLAY_CTRL_TEMPO;



typedef struct _MIDI_CHNNEL_CTRL_STRUCT_ {
    u8 chn;
} MIDI_CHNNEL_CTRL_STRUCT;

typedef struct _MIDI_PROG_CTRL_STRUCT_ {
    u8 prog;
    u8 replace_mode;         //replace_mode==1，就是 替换所有通道； 否则替换主通道
    u16 ex_vol;              //1024是跟原来一样大， 变化为原来的(ex_vol/1024)倍
} MIDI_PROG_CTRL_STRUCT;

typedef struct _MIDI_SEEK_BACK_STRUCT_ {
    s8 seek_back_n;
} MIDI_SEEK_BACK_STRUCT;


#define MAX_WORD   10

typedef struct _MIDI_W2S_STRUCT_ {
    unsigned int word_cnt;                           //多少个字
    unsigned int data_pos[MAX_WORD + 1];               //数据起始位置
    unsigned int data_len[MAX_WORD + 1];               //数据长度
    unsigned short *rec_data;
    char key_diff;
} MIDI_W2S_STRUCT;

enum {
    MARK_ENABLE = 0x0001,                  //mark回调的使能
    MELODY_ENABLE = 0x0002,                //melody回调的使能
    TIM_DIV_ENABLE = 0x0004,               //小节回调的使能
    MUTE_ENABLE = 0x0008,                  //mute住解码的使能
    SAVE_DIV_ENBALE = 0x0010,               //小节保存的使能
    EX_VOL_ENABLE = 0x0020,                 //外部音量控制使能
    SET_PROG_ENABLE = 0x0040,               //主轨道设置成固定乐器使能
    MELODY_PLAY_ENABLE = 0x0080,             //主轨道播放使能
    BEAT_TRIG_ENABLE = 0x0100                //每拍回调的使能

};


typedef struct _MIDI_INIT_STRUCT_ {
    MIDI_CONFIG_PARM init_info;               //初始化参数
    MIDI_PLAY_CTRL_MODE mode_info;            //控制模式
    MIDI_PLAY_CTRL_TEMPO  tempo_info;         //节奏参数
    EX_CH_VOL_PARM  vol_info;                 //外部音量控制
    MIDI_PROG_CTRL_STRUCT prog_info;          //主轨道乐器参数
    MIDI_CHNNEL_CTRL_STRUCT  mainTrack_info;  //主轨道设置参数
    EX_INFO_STRUCT  mark_info;                //mark回调函数
    EX_MELODY_STRUCT moledy_info;             //melody回调函数
    EX_TmDIV_STRUCT  tmDiv_info;              //小节回调参数
    EX_BeatTrig_STRUCT beat_info;             //每拍回调参数
#if 1
    MIDI_W2S_STRUCT    w2s_info;
#endif
    u32   switch_info;              //初始化一些使能位，默认为0
} MIDI_INIT_STRUCT;


#if 0

{
    audio_decoder_ops *test_pos = get_midi_ops();

    MIDI_CONFIG_PARM midi_t_parm;
    midi_t_parm.sample_rate = 5;
    midi_t_parm.spi_pos = midi_spi_memory;
    midi_t_parm.player_t = 8;


    buflen = test_pos->need_dcbuf_size();
    bufptr = malloc(buflen);
    test_pos->open(bufptr, &my_dec_io0, 0);

    test_pos->dec_confing(bufptr, 0, &midi_t_parm);        //调用接口跟其它解码一致，但是需要调用这个函数

    if (!test_pos->format_check(bufptr)) {
        while (!test_pos->run(bufptr, 0)) {

            if (0) {
                EX_CH_VOL_PARM ex_vol;
                for (test_ci = 0; test_ci < CTRL_CHANNEL_NUM; test_ci++) {
                    ex_vol.cc_vol[test_ci] = 4096;
                }

                test_pos->dec_confing(bufptr, CMD_CTRL_VOL_ON, &ex_vol);           //音量控制开
                test_pos->dec_confing(bufptr, CMD_CTRL_VOL_OFF, &ex_vol);          //音量控制关


            }

        }
    }
}


#endif

#endif // MIDI_DEC_API_h__


