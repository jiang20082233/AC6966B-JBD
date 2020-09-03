#ifndef _CHGBOX_DET_H_
#define _CHGBOX_DET_H_

#define LOW_LEVEL     0
#define HIGHT_LEVEL   1

struct io_det_platform_data {
    u32 port;
    u16 level;
    u16 online_time;
    u16 offline_time;
};

struct ad_det_platform_data {
    u32 port;
    u16 ad_ch;
    u16 online_time;
    u16 offline_time;
};


extern const struct io_det_platform_data hall_det_data;
void hall_det_init(const struct io_det_platform_data *data);


// extern const struct io_det_platform_data usb_in_det_data;
// void usb_in_det_init(const struct io_det_platform_data *data);
#endif
