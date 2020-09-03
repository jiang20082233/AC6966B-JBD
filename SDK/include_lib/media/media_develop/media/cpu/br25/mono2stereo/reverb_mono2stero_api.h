#ifndef REVERB_MONO2STEREO_APH_H
#define REVERB_MONO2STEREO_APH_H


typedef struct _RMONO2STEREO_FUNC_API_ {
    unsigned int (*need_buf)();
    void(*open)(void *ptr, int panval, int fir_type);      //中途改变参数，可以调init
    void(*run)(void *ptr, short *indata, short *outdata, int len);    //len是多少个byte
} RMONO2STEREO_FUNC_API;

extern RMONO2STEREO_FUNC_API *get_rm2s_func_api();

#endif // !REVERB_MONO2STEREO_APH_H
