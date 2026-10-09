/* SPDX-License-Identifier: GPL-3.0-only
 * Calls real LVGL control events in the application; no Windows UI automation.
 */
#include "watch_ui_probe.h"
#include "watch_app.h"
#include "watch_skin.h"
#include "watch_storage.h"
#include "user_test/GUI_App/Screens/Inc/ui_WellnessPage.h"
#include "user_test/GUI_App/Screens/Inc/ui_HRPage.h"
#include "user_test/GUI_App/Screens/Inc/ui_HomePage.h"
#include "user_test/GUI_App/Screens/Inc/ui_MenuPage.h"
#include "lv_drivers/sdl/sdl.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"UI_FAIL line %d: %s\n",__LINE__,#x);return 1;}} while(0)
static unsigned passed;
static void done(const char *name) {printf("PASS %s\n",name);passed++;}
static void pump(void)
{
    for(unsigned n=0;n<5;n++) {lv_timer_handler();SDL_Delay(5);}
    lv_refr_now(NULL);
}
static void settle_draw_cache(void)
{
    for(unsigned n=0;n<50;n++) {lv_timer_handler();SDL_Delay(5);}
    /* Renderer scratch buffers are intentionally retained by LVGL; exclude them. */
    lv_mem_buf_free_all();
}
static int capture(const char *directory,const char *name,unsigned tab)
{
    char path[1200];snprintf(path,sizeof(path),"%s/%s.bmp",directory,name);
    lv_tabview_set_act(wellness_tabs,tab,LV_ANIM_OFF);
    lv_obj_scroll_to_y(lv_obj_get_child(lv_tabview_get_content(wellness_tabs),(int32_t)tab),0,LV_ANIM_OFF);
    pump();
    return sdl_save_frame(path);
}
static int capture_screen(const char *directory,const char *name)
{
    char path[1200];snprintf(path,sizeof(path),"%s/%s.bmp",directory,name);
    pump();return sdl_save_frame(path);
}
int watch_ui_probe(const char *directory)
{
    watch_model *m=watch_app_model();
    CHECK(m->settings.goal==3000 && m->alert_count==0);
    Page_Back();CHECK(Page_Get_NowPage()==&Page_Menu);pump();
    CHECK(lv_obj_get_scroll_y(ui_MenuPage)==0);
    CHECK(capture_screen(directory,"menu")==0);
    lv_event_send(ui_MenuHRPanel,LV_EVENT_CLICKED,NULL);
    CHECK(Page_Get_NowPage()==&Page_HR);done("menu_opens_health_application");
    for(unsigned n=0;n<120;n++) watch_model_sample(m,(uint16_t)(72+n%11),1,1000+n);
    lv_dropdown_set_selected(wellness_mode,WATCH_HIGH);
    lv_event_send(wellness_mode,LV_EVENT_VALUE_CHANGED,NULL);
    CHECK(watch_app_mode()==WATCH_HIGH);
    for(unsigned n=0;n<3;n++) watch_app_tick();
    CHECK(m->active_kind==WATCH_ALERT_HIGH && m->alert_count==1 && ui_notification_count==1);
    wellness_refresh();CHECK(strstr(lv_label_get_text(ui_HRPageNoticeLabel),"High alert"));
    done("signal_control_triggers_debounced_visible_alert");
    lv_dropdown_set_selected(wellness_mode,WATCH_PAUSED);
    lv_event_send(wellness_mode,LV_EVENT_VALUE_CHANGED,NULL);
    uint32_t revision=m->revision;watch_app_tick();CHECK(m->revision==revision);
    done("pause_control_stops_sample_generation");
    ui_notifications_dismiss();CHECK(capture(directory,"heart",0)==0);
    uint32_t goal=m->settings.goal;
    lv_event_send(wellness_goal_plus,LV_EVENT_CLICKED,NULL);CHECK(m->settings.goal==goal+500);
    lv_event_send(wellness_goal_minus,LV_EVENT_CLICKED,NULL);CHECK(m->settings.goal==goal);
    for(unsigned n=0;n<3;n++) lv_event_send(wellness_add_steps,LV_EVENT_CLICKED,NULL);
    CHECK(m->steps>=m->settings.goal && m->goal_notified && ui_notification_count==2);
    lv_event_send(wellness_add_steps,LV_EVENT_CLICKED,NULL);CHECK(ui_notification_count==2);
    done("step_buttons_goal_progress_and_one_daily_notification");
    ui_notifications_dismiss();CHECK(capture(directory,"activity",1)==0);
    lv_spinbox_set_value(wellness_low,130);lv_spinbox_set_value(wellness_high,120);
    lv_event_send(wellness_save,LV_EVENT_CLICKED,NULL);CHECK(m->settings.low==50);
    done("settings_control_rejects_invalid_thresholds");
    lv_spinbox_set_value(wellness_low,55);lv_spinbox_set_value(wellness_high,125);
    lv_spinbox_set_value(wellness_goal,3500);lv_event_send(wellness_save,LV_EVENT_CLICKED,NULL);
    CHECK(m->settings.low==55 && m->settings.high==125 && m->settings.goal==3500);
    done("settings_controls_apply_changes");
    lv_event_send(wellness_ack,LV_EVENT_CLICKED,NULL);
    CHECK(watch_model_alert_at(m,0)->acknowledged);done("history_acknowledgement_control");
    CHECK(capture(directory,"history",2)==0);CHECK(capture(directory,"settings",3)==0);
    watch_model restored;watch_model_init(&restored,m->day);
    CHECK(watch_storage_load(&restored,watch_app_state_path())==WATCH_STORE_OK);
    CHECK(restored.steps==m->steps && restored.settings.goal==3500 && restored.sample_count==m->sample_count);
    CHECK(watch_model_alert_at(&restored,0)->acknowledged);
    done("ui_actions_are_restored_from_actual_state_file");
    Page_Back();CHECK(Page_Get_NowPage()==&Page_Menu);
    lv_event_send(ui_MenuSprPanel,LV_EVENT_CLICKED,NULL);
    CHECK(Page_Get_NowPage()==&Page_Sports && lv_tabview_get_tab_act(wellness_tabs)==1);
    done("sports_menu_opens_activity_tab");
    Page_Back_Bottom();pump();
    CHECK(capture_screen(directory,"home")==0);
    lv_event_send(lv_obj_get_parent(ui_HRNumLabel),LV_EVENT_CLICKED,NULL);
    CHECK(Page_Get_NowPage()==&Page_HR);Page_Back();
    done("home_heart_card_opens_heart_page");
    lv_event_send(lv_obj_get_parent(ui_StepNumLabel),LV_EVENT_CLICKED,NULL);
    CHECK(Page_Get_NowPage()==&Page_Sports);Page_Back();
    done("home_activity_card_opens_activity_page");
    lv_event_send(watch_home_apps,LV_EVENT_CLICKED,NULL);
    CHECK(Page_Get_NowPage()==&Page_Menu);Page_Back();
    done("home_apps_button_opens_menu");
    settle_draw_cache();
    lv_mem_monitor_t before,after;lv_mem_monitor(&before);
    unsigned screen_count=lv_disp_get_default()->screen_cnt;
    for(unsigned n=0;n<40;n++) {
        Page_Load(n%2?&Page_HR:&Page_Sports);pump();Page_Back();pump();
        CHECK(Page_Get_NowPage()==&Page_Home);
        CHECK(lv_disp_get_default()->screen_cnt==screen_count);
        if(n%10==9) {
            lv_mem_monitor(&after);
            printf("UI_MEMORY_CYCLE %u free=%u screens=%u\n",n+1,after.free_size,lv_disp_get_default()->screen_cnt);
        }
    }
    settle_draw_cache();
    lv_mem_monitor(&after);
    printf("UI_MEMORY before_free=%u after_free=%u before_used=%u after_used=%u before_free_count=%u after_free_count=%u\n",before.free_size,after.free_size,before.used_cnt,after.used_cnt,before.free_cnt,after.free_cnt);
    /* TLSF free payload varies with fragmentation and allocation fit slack.
     * Compare live block ownership at the same page, not identical free bytes.
     * Free bytes and free block counts remain in the log for inspection. */
    CHECK(after.used_cnt==before.used_cnt);
    done("forty_page_roundtrips_keep_live_allocation_and_screen_counts");
    CHECK(lv_mem_test()==LV_RES_OK);done("lvgl_heap_integrity");
    printf("UI_CASES_PASSED %u\n",passed);return 0;
}

int watch_restore_probe(void)
{
    watch_model *m=watch_app_model();
    CHECK(m->settings.low==55 && m->settings.high==125 && m->settings.goal==3500);
    done("new_process_restores_settings");
    CHECK(m->steps>=4000 && m->goal_notified);done("new_process_restores_steps_and_goal_latch");
    CHECK(m->sample_count==120 && watch_model_sample_at(m,119)->bpm==135);
    done("new_process_restores_trend");
    CHECK(m->alert_count==1 && watch_model_alert_at(m,0)->acknowledged);
    done("new_process_restores_acknowledged_alert");
    CHECK(ui_notification_count==0);done("startup_does_not_repeat_saved_notifications");
    printf("RESTORE_CASES_PASSED %u\n",passed);return 0;
}
