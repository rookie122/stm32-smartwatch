/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WATCH_APP_H
#define WATCH_APP_H
#include "watch_model.h"
void watch_app_init(void);
void watch_app_tick(void);
void watch_app_shutdown(void);
enum { WATCH_NOTICE_GOAL=1, WATCH_NOTICE_ALERT=2 };
void watch_app_set_listener(void (*listener)(int kind));
watch_model *watch_app_model(void);
watch_mode watch_app_mode(void);
void watch_app_set_mode(watch_mode mode);
int watch_app_configure(const watch_settings *settings);
void watch_app_add_steps(uint32_t amount);
void watch_app_ack_all(void);
int watch_app_save(void);
const char *watch_app_storage_status(void);
const char *watch_app_state_path(void);
#endif
