#ifndef UI_TEXT_H
#define UI_TEXT_H

#include "ui/ui_core.h"
#include "ui/control.h"
#include "font/font_all.h"


struct ui_text {
    struct element elm;
    struct ui_text_attrs attrs;
    void *timer;
    char _str[4];
    char _format[7];
    u8 str_num: 4;
    u8 css_num: 4;
    u16 css[2];
    int attr_color;
    int attr_highlight_color;
    // const struct ui_text_info *info;
    const struct element_event_handler *handler;
};



void ui_text_enable();
void *new_ui_text(const void *_info, struct element *parent);
int ui_text_show_index_by_id(int id, int index);
int ui_text_set_index(struct ui_text *text, int index);
int ui_text_set_str(struct ui_text *text, const char *format, const char *str, int strlen, u32 flags);
int ui_text_set_str_by_id(int id, const char *format, const char *str);
int ui_text_set_text_by_id(int id, const char *str, int strlen, u32 flags);
int ui_text_set_textw_by_id(int id, const char *str, int strlen, int endian, u32 flags);
int ui_text_set_textu_by_id(int id, const char *str, int strlen, u32 flags);
void ui_text_set_text_attrs(struct ui_text *text, const char *str, int strlen, u8 encode, u8 endian, u32 flags);
void text_release(struct ui_text *text);

#endif
