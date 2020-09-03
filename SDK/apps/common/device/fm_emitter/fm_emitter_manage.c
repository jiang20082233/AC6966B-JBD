

#include "app_config.h"
#include "circular_buf.h"
#include "system/includes.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "ac3433/ac3433.h"
#include "qn8007/qn8007.h"
#include "qn8027/qn8027.h"
#include "fm_inside/fm_emitter_inside.h"
#include "audio_config.h"
#include "audio_digital_vol.h"

#if TCFG_APP_FM_EMITTER_EN

static FM_EMITTER_INTERFACE *fm_emitter_hdl = NULL;

#define FM_TX_BUF_LEN 	(2048*2)
static cbuffer_t *fm_emitter_cbuf = NULL;
static u8 *fm_emitter_cbuf_p = NULL;
static OS_SEM fm_emitter_cbuf_sem;
static u8 door_flag = 1; // 1:up 0:down
static dvol_handler *fmtx_dvol = NULL;

int fm_emitter_cbuf_write(u8 *data, u32 len)
{
    if (fm_emitter_cbuf == NULL) {
        return 0;
    }
    /* while (cbuf_write(fm_emitter_cbuf, data, len) == 0) { */
    /* os_sem_set(&fm_emitter_cbuf_sem, 0); */
    /* os_sem_pend(&fm_emitter_cbuf_sem, 0); */
    /* } */
    return cbuf_write(fm_emitter_cbuf, data, len);
}

extern u8 bt_phone_dec_is_running();
extern void audio_aec_ref_start(u8 en);
void fm_emitter_cbuf_read(u8 *data, u32 len)
{
    if (fm_emitter_cbuf == NULL) {
        memset(data, 0x00, len);
        return;
    }
    int rlen = cbuf_get_data_len(fm_emitter_cbuf);
    if (door_flag && (rlen < FM_TX_BUF_LEN / 2)) {
        memset(data, 0x00, len);
        return;
    }
    if (bt_phone_dec_is_running()) {
        audio_aec_ref_start(1);
    }
    door_flag = 0;
    if (rlen >= len) {
        rlen = cbuf_read(fm_emitter_cbuf, data, len);
    } else {
        printf("fm end %d %d\n", len, rlen);
        rlen = cbuf_read(fm_emitter_cbuf, data, rlen);
        memset(data + rlen, 0x00, len - rlen);
        door_flag = 1;
    }

    /* putchar('A'+(ret == len)); */
    os_sem_set(&fm_emitter_cbuf_sem, 0);
    os_sem_post(&fm_emitter_cbuf_sem);
}


void fm_emitter_data_read(u8 *data, u32 len)
{
    fm_emitter_cbuf_read(data, len);
    audio_digital_vol_run(fm_dvol, data, len);
}

void fm_emitter_cbuf_init()
{
    if (fm_emitter_cbuf_p) {
        printf("fm_emitter_cbuf is inited\n");
        return;
    }
    fm_emitter_cbuf_p = malloc(FM_TX_BUF_LEN + sizeof(cbuffer_t));
    fm_emitter_cbuf = fm_emitter_cbuf_p;
    cbuf_init(fm_emitter_cbuf, fm_emitter_cbuf_p + sizeof(cbuffer_t), FM_TX_BUF_LEN);
    os_sem_create(&fm_emitter_cbuf_sem, 0);
}

int fm_emitter_cbuf_len(void)
{
    return FM_TX_BUF_LEN;
}

int fm_emitter_cbuf_data_len(void)
{
    if (fm_emitter_cbuf == NULL) {
        return 0;
    }
    return cbuf_get_data_len(fm_emitter_cbuf);
}

void fm_emitter_cbuf_uninit()
{
    printf("fm_emitter_cbuf_uninit\n");
    if (fm_emitter_cbuf_p) {
        free(fm_emitter_cbuf_p);
        fm_emitter_cbuf_p = NULL;
        return;
    }
}

/*************************************************
 *
 *      Fmtx fre save vm
 *
 *************************************************/
static int fm_emitter_manage_timer = 0;
static u8 fm_emitter_manage_cnt = 0;
static u16 cur_fmtx_freq = 0;

static void fm_emitter_manage_save_fre(void)
{
    printf("fm_emitter_manage_save_fre %d\n", cur_fmtx_freq);
    if (fm_emitter_hdl) {
        u8 tbuf[2];
        tbuf[0] = (cur_fmtx_freq >> 8) & 0xFF;
        tbuf[1] = cur_fmtx_freq & 0xFF;
        int ret = syscfg_write(VM_FM_EMITTER_FREQ, tbuf, 2);
        if (ret != 2) {
            printf("fm last fre write err!\n");
        }
    } else {
        printf("%s %d no hdl\n", __func__, __LINE__);
    }
}

static void fm_emitter_manage_fre_save_do(void *priv)
{
    //printf("fm_emitter_manage_fre_save_do %d\n", fm_emitter_manage_cnt);
    local_irq_disable();
    if (++fm_emitter_manage_cnt >= 5) {
        sys_hi_timer_del(fm_emitter_manage_timer);
        fm_emitter_manage_timer = 0;
        fm_emitter_manage_cnt = 0;
        local_irq_enable();
        fm_emitter_manage_save_fre();
        return;
    }
    local_irq_enable();
}

static void fm_emitter_manage_fre_change(void)
{
    local_irq_disable();
    fm_emitter_manage_cnt = 0;
    if (fm_emitter_manage_timer == 0) {
        fm_emitter_manage_timer = sys_hi_timer_add(NULL, fm_emitter_manage_fre_save_do, 1000);
    }
    local_irq_enable();
}


void fm_emitter_manage_init(u16 fre)
{
    printf("fm_emitter_manage_init \n");
    int found = 0;
    u8 tbuf[2];
    u8 vol;
    int ret = 0;
    list_for_each_fm_emitter(fm_emitter_hdl) {
        printf("fm_emitter_hdl %x\n", fm_emitter_hdl);
        if (!memcmp(fm_emitter_hdl->name, "fm_emitter_inside", strlen(fm_emitter_hdl->name))) {
            printf("fm fine dev %s \n", fm_emitter_hdl->name);
            found = 1;
            break;
        }
    }

    if (found) {
        if (!fre) {
            ret = syscfg_read(VM_FM_EMITTER_FREQ, tbuf, 2);
            if (ret == 2) {
                fre = (tbuf[0] << 8) | tbuf[1];
                if (fre < 875) {
                    fre = 875;
                } else if (fre > 1080) {
                    fre = 1080;
                }
                printf("fm last fre: %d\n", fre);
            } else {
                fre = 875;
                printf("fm last fre read err!\n");
            }
        }
        fm_emitter_hdl->init(fre);
        cur_fmtx_freq = fre;
        fm_emitter_cbuf_init();
        fm_emitter_hdl->data_cb(fm_emitter_data_read);

        vol = 0;
        if (fmtx_dvol == NULL) {
            fmtx_dvol = audio_digital_vol_open(vol, FM_EMITTER_MAX_VOL, 4);
        }
        tbuf[0] = (fre >> 8) & 0xFF;
        tbuf[1] = fre & 0xFF;
        ret = syscfg_write(VM_FM_EMITTER_FREQ, tbuf, 2);
        if (ret != 2) {
            printf("fm last fre init write err!\n");
        }
    }
    fm_emitter_manage_fre_change();
}


void fm_emitter_manage_start(void)
{
    if (fm_emitter_hdl) {
        fm_emitter_hdl->start();
    }
}

void fm_emitter_manage_stop(void)
{
    if (fm_emitter_hdl) {
        fm_emitter_hdl->stop();
    }
}

u16 fm_emitter_manage_get_fre()
{
    return cur_fmtx_freq;
}



void fm_emitter_manage_set_fre(u16 fre)
{
    if (fm_emitter_hdl) {
        if (fre < 875) {
            fre = 875;
        } else if (fre > 1080) {
            fre = 1080;
        }
        fm_emitter_hdl->set_fre(fre);
        cur_fmtx_freq = fre;
        fm_emitter_manage_fre_change();
    } else {
        printf("%s %d no hdl\n", __func__, __LINE__);
    }
}


void fm_emitter_manage_set_power(u8 power, u16 fre)
{
    if (fm_emitter_hdl) {
        fm_emitter_hdl->set_power(power, fre);
    }
}

void fm_emitter_manage_set_vol(u8 vol)
{
    audio_digital_vol_set(vol);
}

void fm_emitter_manage_vol_up(void)
{
    u8 vol = audio_digital_vol_get();
    if (vol <  FM_EMITTER_MAX_VOL) {
        vol ++;
    }
    audio_digital_vol_set(vol);
}

void fm_emitter_manage_vol_down(void)
{
    u8 vol = audio_digital_vol_get();
    if (vol > 0) {
        vol --;
    }
    audio_digital_vol_set(vol);
}

u8 fm_emitter_manage_get_vol(void)
{
    return audio_digital_vol_get();
}


#endif
