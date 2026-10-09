/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WATCH_SKIN_H
#define WATCH_SKIN_H
#include "lvgl/lvgl.h"
#define WATCH_BG 0x0A0F16
#define WATCH_CARD 0x151F2C
#define WATCH_EDGE 0x263345
#define WATCH_TEXT 0xF1F5F9
#define WATCH_MUTED 0x94A3B8
#define WATCH_MINT 0x64E3BD
#define WATCH_ROSE 0xFF8196
extern lv_obj_t *watch_home_apps;
void watch_skin_screen(lv_obj_t *screen);
void watch_skin_button(lv_obj_t *button,int primary);
void watch_skin_home(void);
void watch_skin_home_refresh(void);
void watch_skin_menu(void);
#endif
