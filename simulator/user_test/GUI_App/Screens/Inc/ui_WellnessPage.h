/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef UI_WELLNESS_H
#define UI_WELLNESS_H
#include "../../ui.h"
extern Page_t Page_Sports;
extern lv_obj_t *ui_SportsPage;
extern lv_obj_t *wellness_tabs,*wellness_mode,*wellness_add_steps;
extern lv_obj_t *wellness_goal_plus,*wellness_goal_minus,*wellness_ack,*wellness_save;
extern lv_obj_t *wellness_low,*wellness_high,*wellness_goal,*wellness_enabled;
void wellness_init(lv_obj_t **screen,unsigned tab);
void wellness_deinit(void);
void wellness_refresh(void);
#endif
