/* SPDX-License-Identifier: GPL-3.0-only
 * Replaces the upstream single-value heart page with the wellness application.
 */
#include "../Inc/ui_WellnessPage.h"
#include "../Inc/ui_HRPage.h"
lv_obj_t *ui_HRPage;
lv_obj_t *ui_HRPageNumLabel,*ui_HRPageUnitLabel,*ui_HRPaggiconLabel,*ui_HRPageNoticeLabel;
Page_t Page_HR={ui_HRPage_screen_init,ui_HRPage_screen_deinit,&ui_HRPage};
void ui_HRPage_screen_init(void) {wellness_init(&ui_HRPage,0);}
void ui_HRPage_screen_deinit(void) {wellness_deinit();}
