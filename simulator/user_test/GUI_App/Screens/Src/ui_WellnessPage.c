/* SPDX-License-Identifier: GPL-3.0-only */
#include "../Inc/ui_WellnessPage.h"
#include "../Inc/ui_HRPage.h"
#include "watch_app.h"
#include "watch_skin.h"
#include <stdio.h>
#include <time.h>

lv_obj_t *ui_SportsPage;
lv_obj_t *wellness_tabs,*wellness_mode,*wellness_add_steps;
lv_obj_t *wellness_goal_plus,*wellness_goal_minus,*wellness_ack,*wellness_save;
lv_obj_t *wellness_low,*wellness_high,*wellness_goal,*wellness_enabled;
static lv_obj_t *heart_value,*heart_status,*chart,*step_value,*step_status;
static lv_obj_t *progress,*history_text,*storage_text,*settings_notice;
static lv_chart_series_t *series;
static lv_timer_t *refresh_timer;
static uint32_t displayed_revision=UINT32_MAX;
static void sports_init(void) {wellness_init(&ui_SportsPage,1);}
Page_t Page_Sports={sports_init,wellness_deinit,&ui_SportsPage};

static lv_obj_t *label(lv_obj_t *parent,const char *text)
{
    lv_obj_t *obj=lv_label_create(parent);
    lv_obj_set_width(obj,LV_PCT(100)); lv_label_set_long_mode(obj,LV_LABEL_LONG_WRAP);
    lv_label_set_text(obj,text); return obj;
}
static lv_obj_t *row(lv_obj_t *parent)
{
    lv_obj_t *obj=lv_obj_create(parent);
    lv_obj_set_width(obj,LV_PCT(100)); lv_obj_set_height(obj,LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(obj,0,0); lv_obj_set_style_border_width(obj,0,0);
    lv_obj_set_style_bg_opa(obj,LV_OPA_TRANSP,0);
    lv_obj_set_flex_flow(obj,LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj,LV_FLEX_ALIGN_SPACE_BETWEEN,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLLABLE); return obj;
}
static lv_obj_t *button(lv_obj_t *parent,const char *text,lv_event_cb_t callback,void *data)
{
    lv_obj_t *obj=lv_btn_create(parent); lv_obj_set_height(obj,32); lv_obj_set_flex_grow(obj,1);
    watch_skin_button(obj,0);
    lv_obj_t *caption=lv_label_create(obj); lv_label_set_text(caption,text); lv_obj_center(caption);
    lv_obj_add_event_cb(obj,callback,LV_EVENT_CLICKED,data); return obj;
}
static void back_event(lv_event_t *e) {(void)e;Page_Back();}
static void mode_event(lv_event_t *e)
{
    watch_app_set_mode((watch_mode)lv_dropdown_get_selected(lv_event_get_target(e))); wellness_refresh();
}
static void steps_event(lv_event_t *e)
{
    watch_app_add_steps((uint32_t)(uintptr_t)lv_event_get_user_data(e)); wellness_refresh();
}
static void goal_event(lv_event_t *e)
{
    watch_settings next=watch_app_model()->settings;
    int goal=(int)next.goal+(int)(intptr_t)lv_event_get_user_data(e);
    if(goal<500) goal=500;
    if(goal>50000) goal=50000;
    next.goal=(uint32_t)goal; watch_app_configure(&next);
    lv_spinbox_set_value(wellness_goal,goal); wellness_refresh();
}
static void ack_event(lv_event_t *e) {(void)e;watch_app_ack_all();wellness_refresh();}
static void spin_event(lv_event_t *e)
{
    lv_obj_t *spin=lv_event_get_user_data(e);
    const char *name=lv_label_get_text(lv_obj_get_child(lv_event_get_target(e),0));
    if(name[0]=='+') lv_spinbox_increment(spin); else lv_spinbox_decrement(spin);
}
static void save_event(lv_event_t *e)
{
    (void)e;
    watch_settings next={(uint16_t)lv_spinbox_get_value(wellness_low),
        (uint16_t)lv_spinbox_get_value(wellness_high),(uint32_t)lv_spinbox_get_value(wellness_goal),
        (uint8_t)lv_obj_has_state(wellness_enabled,LV_STATE_CHECKED)};
    if(!watch_app_configure(&next)) {
        lv_label_set_text(settings_notice,"High must exceed low by 10."); return;
    }
    lv_label_set_text(settings_notice,watch_app_storage_status()); wellness_refresh();
}
static lv_obj_t *setting(lv_obj_t *parent,const char *name,int low,int high,int value,int digits)
{
    label(parent,name); lv_obj_t *controls=row(parent);
    lv_obj_t *spin=lv_spinbox_create(controls);
    lv_obj_set_width(spin,96); lv_obj_set_height(spin,32);
    lv_obj_set_style_pad_all(spin,7,0);lv_obj_set_style_radius(spin,10,0);
    lv_obj_set_style_bg_color(spin,lv_color_hex(WATCH_CARD),0);
    lv_obj_set_style_border_color(spin,lv_color_hex(WATCH_EDGE),0);
    lv_obj_set_style_border_width(spin,1,0);
    lv_obj_set_style_bg_color(spin,lv_color_hex(0x254E47),LV_PART_CURSOR);
    lv_obj_set_style_text_color(spin,lv_color_hex(WATCH_MINT),LV_PART_CURSOR);
    lv_spinbox_set_range(spin,low,high); lv_spinbox_set_digit_format(spin,(uint8_t)digits,0);
    lv_spinbox_set_value(spin,value); lv_spinbox_set_step(spin,1);
    button(controls,"-",spin_event,spin); button(controls,"+",spin_event,spin); return spin;
}

void wellness_refresh(void)
{
    watch_model *m=watch_app_model(); char buffer[96];
    if(m->latest_valid) snprintf(buffer,sizeof(buffer),"%u bpm",m->latest_bpm);
    else snprintf(buffer,sizeof(buffer),"-- bpm");
    lv_label_set_text(heart_value,buffer);
    const char *status=m->active_kind==WATCH_ALERT_HIGH ? "High alert - simulated" :
        m->active_kind==WATCH_ALERT_LOW ? "Low alert - simulated" :
        watch_app_mode()==WATCH_PAUSED ? "Simulation paused" : !m->latest_valid ? "Signal unavailable" :
        m->candidate_count ? "Checking consecutive samples" : "Within configured range";
    lv_label_set_text(heart_status,status);
    lv_obj_set_style_text_color(heart_status,lv_color_hex(m->active_kind?WATCH_ROSE:WATCH_MUTED),0);
    snprintf(buffer,sizeof(buffer),"%u steps",m->steps); lv_label_set_text(step_value,buffer);
    unsigned percent=(unsigned)((uint64_t)m->steps*100/m->settings.goal);
    lv_bar_set_value(progress,percent>100?100:(int)percent,LV_ANIM_OFF);
    snprintf(buffer,sizeof(buffer),"of %u  /  %u%% %s",m->settings.goal,percent,m->steps>=m->settings.goal?"done":"");
    lv_label_set_text(step_status,buffer); lv_label_set_text(storage_text,watch_app_storage_status());
    if(displayed_revision==m->revision) return;
    displayed_revision=m->revision;
    for(unsigned n=0;n<WATCH_SAMPLES;n++) {
        const watch_sample *sample=watch_model_sample_at(m,n);
        lv_chart_set_value_by_id(chart,series,n,sample && sample->valid ? sample->bpm : LV_CHART_POINT_NONE);
    }
    lv_chart_refresh(chart);
    char history[2300]; size_t used=0;
    used+=(size_t)snprintf(history+used,sizeof(history)-used,"Today %u\n%u / %u steps\n\nDaily history\n",m->day,m->steps,m->settings.goal);
    if(!m->day_count) used+=(size_t)snprintf(history+used,sizeof(history)-used,"Available after a new day.\n");
    for(size_t n=m->day_count;n>0;n--) {
        const watch_day *d=watch_model_day_at(m,n-1);
        used+=(size_t)snprintf(history+used,sizeof(history)-used,"%u: %u / %u\n",d->day,d->steps,d->goal);
    }
    used+=(size_t)snprintf(history+used,sizeof(history)-used,"\nAlert history\n");
    if(!m->alert_count) used+=(size_t)snprintf(history+used,sizeof(history)-used,"No alerts yet.\n");
    for(size_t n=m->alert_count;n>0;n--) {
        const watch_alert *a=watch_model_alert_at(m,n-1);
        time_t timestamp=(time_t)a->at;struct tm local;char time_text[32];
        if(localtime_s(&local,&timestamp)==0) strftime(time_text,sizeof(time_text),"%m-%d %H:%M:%S",&local);
        else snprintf(time_text,sizeof(time_text),"%u",a->at);
        used+=(size_t)snprintf(history+used,sizeof(history)-used,"#%u %s %u bpm\n%s | %s | %s\n",a->id,
            a->kind==WATCH_ALERT_HIGH?"HIGH":"LOW",a->bpm,time_text,
            a->acknowledged?"ACK":"Unread",a->resolved?"Closed":"Active");
    }
    lv_label_set_text(history_text,history);
}
static void refresh(lv_timer_t *timer) {(void)timer;wellness_refresh();}
void wellness_init(lv_obj_t **screen,unsigned tab)
{
    watch_settings *settings=&watch_app_model()->settings;
    *screen=lv_obj_create(NULL); lv_obj_clear_flag(*screen,LV_OBJ_FLAG_SCROLLABLE);
    watch_skin_screen(*screen);
    lv_obj_t *header=lv_obj_create(*screen);
    lv_obj_set_size(header,240,30); lv_obj_align(header,LV_ALIGN_TOP_MID,0,0);
    lv_obj_set_style_pad_all(header,0,0); lv_obj_set_style_border_width(header,0,0);
    lv_obj_set_style_bg_color(header,lv_color_hex(WATCH_BG),0);
    lv_obj_set_flex_flow(header,LV_FLEX_FLOW_ROW); lv_obj_clear_flag(header,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *back=button(header,LV_SYMBOL_LEFT,back_event,NULL);
    lv_obj_set_style_border_width(back,0,0);lv_obj_set_style_bg_opa(back,LV_OPA_TRANSP,0);
    lv_obj_t *caption=label(header,"Health  /  demo"); lv_obj_set_width(caption,185);
    lv_obj_set_style_text_color(caption,lv_color_hex(WATCH_MUTED),0);
    wellness_tabs=lv_tabview_create(*screen,LV_DIR_TOP,28);
    lv_obj_set_size(wellness_tabs,240,250); lv_obj_align(wellness_tabs,LV_ALIGN_BOTTOM_MID,0,0);
    lv_obj_set_style_bg_color(wellness_tabs,lv_color_hex(WATCH_BG),0);
    lv_obj_t *tab_buttons=lv_tabview_get_tab_btns(wellness_tabs);
    lv_obj_set_style_bg_color(tab_buttons,lv_color_hex(WATCH_BG),0);
    lv_obj_set_style_text_color(tab_buttons,lv_color_hex(WATCH_MUTED),LV_PART_ITEMS);
    lv_obj_set_style_bg_color(tab_buttons,lv_color_hex(0x193B35),LV_PART_ITEMS|LV_STATE_CHECKED);
    lv_obj_set_style_text_color(tab_buttons,lv_color_hex(WATCH_MINT),LV_PART_ITEMS|LV_STATE_CHECKED);
    lv_obj_set_style_border_width(tab_buttons,0,LV_PART_ITEMS|LV_STATE_CHECKED);
    lv_obj_set_style_radius(tab_buttons,8,LV_PART_ITEMS);
    lv_obj_t *heart=lv_tabview_add_tab(wellness_tabs,"Heart");
    lv_obj_t *activity=lv_tabview_add_tab(wellness_tabs,"Move");
    lv_obj_t *history=lv_tabview_add_tab(wellness_tabs,"Log");
    lv_obj_t *setup=lv_tabview_add_tab(wellness_tabs,"Setup");
    lv_obj_t *tabs[]={heart,activity,history,setup};
    for(unsigned n=0;n<4;n++) {
        lv_obj_set_flex_flow(tabs[n],LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(tabs[n],12,0); lv_obj_set_style_pad_row(tabs[n],6,0);
        lv_obj_set_style_text_color(tabs[n],lv_color_hex(WATCH_MUTED),0);
        lv_obj_set_style_bg_color(tabs[n],lv_color_hex(WATCH_BG),0);
    }
    heart_value=label(heart,"-- bpm"); lv_obj_set_style_text_font(heart_value,&lv_font_montserrat_36,0);
    lv_obj_set_style_text_color(heart_value,lv_color_hex(WATCH_TEXT),0);
    heart_status=label(heart,"Waiting for simulated data");
    chart=lv_chart_create(heart); lv_obj_set_size(chart,216,78);
    lv_obj_set_style_radius(chart,12,0);lv_obj_set_style_border_width(chart,0,0);
    lv_obj_set_style_bg_color(chart,lv_color_hex(WATCH_CARD),0);
    lv_obj_set_style_line_color(chart,lv_color_hex(WATCH_EDGE),0);
    lv_obj_set_style_line_width(chart,2,LV_PART_ITEMS);
    lv_obj_set_style_size(chart,0,LV_PART_INDICATOR);
    lv_chart_set_div_line_count(chart,3,0);
    lv_chart_set_type(chart,LV_CHART_TYPE_LINE); lv_chart_set_range(chart,LV_CHART_AXIS_PRIMARY_Y,20,250);
    lv_chart_set_point_count(chart,WATCH_SAMPLES);
    series=lv_chart_add_series(chart,lv_color_hex(WATCH_ROSE),LV_CHART_AXIS_PRIMARY_Y);
    wellness_mode=lv_dropdown_create(heart); lv_obj_set_size(wellness_mode,216,32);
    lv_obj_set_style_bg_color(wellness_mode,lv_color_hex(WATCH_CARD),0);
    lv_obj_set_style_text_color(wellness_mode,lv_color_hex(WATCH_TEXT),0);
    lv_obj_set_style_border_color(wellness_mode,lv_color_hex(WATCH_EDGE),0);
    lv_obj_set_style_border_width(wellness_mode,1,0);lv_obj_set_style_radius(wellness_mode,10,0);
    lv_obj_set_style_pad_all(wellness_mode,8,0);
    lv_dropdown_set_options(wellness_mode,"Normal signal\nHigh signal\nLow signal\nInvalid signal\nPause simulation");
    lv_dropdown_set_selected(wellness_mode,(uint16_t)watch_app_mode());
    lv_obj_add_event_cb(wellness_mode,mode_event,LV_EVENT_VALUE_CHANGED,NULL);
    label(heart,"120 samples, one per second.\nDemo thresholds, editable in Setup.");
    step_value=label(activity,"0 steps"); lv_obj_set_style_text_font(step_value,&lv_font_montserrat_28,0);
    lv_obj_set_style_text_color(step_value,lv_color_hex(WATCH_TEXT),0);
    progress=lv_bar_create(activity); lv_obj_set_size(progress,216,8); lv_bar_set_range(progress,0,100);
    lv_obj_set_style_bg_color(progress,lv_color_hex(WATCH_EDGE),0);
    lv_obj_set_style_bg_color(progress,lv_color_hex(WATCH_MINT),LV_PART_INDICATOR);
    step_status=label(activity,"Goal progress");
    label(activity,"Daily goal"); lv_obj_t *goal_row=row(activity);
    wellness_goal_minus=button(goal_row,"-500",goal_event,(void *)(intptr_t)-500);
    wellness_goal_plus=button(goal_row,"+500",goal_event,(void *)(intptr_t)500);
    label(activity,"Add simulated steps"); lv_obj_t *steps_row=row(activity);
    button(steps_row,"+100",steps_event,(void *)(uintptr_t)100);
    wellness_add_steps=button(steps_row,"+1000",steps_event,(void *)(uintptr_t)1000);
    label(activity,"Goal reminder is retained for today.\nNew day archives the previous day.");
    lv_obj_t *ack_row=row(history); wellness_ack=button(ack_row,"Acknowledge alerts",ack_event,NULL);
    history_text=label(history,"");
    label(setup,"Reminder settings (simulation)");
    wellness_low=setting(setup,"Low threshold / bpm",30,200,settings->low,3);
    wellness_high=setting(setup,"High threshold / bpm",40,220,settings->high,3);
    wellness_goal=setting(setup,"Daily step goal",500,50000,(int)settings->goal,5);
    wellness_enabled=lv_switch_create(setup);
    if(settings->alerts_enabled) lv_obj_add_state(wellness_enabled,LV_STATE_CHECKED);
    label(setup,"Enable threshold reminders");
    lv_obj_t *save_row=row(setup); wellness_save=button(save_row,"Apply and save",save_event,NULL);
    watch_skin_button(wellness_save,1);
    settings_notice=label(setup,"High - low must be at least 10.");
    storage_text=label(setup,watch_app_storage_status());
    displayed_revision=UINT32_MAX; lv_tabview_set_act(wellness_tabs,tab,LV_ANIM_OFF);
    wellness_refresh(); refresh_timer=lv_timer_create(refresh,250,NULL);
    ui_HRPageNumLabel=heart_value; ui_HRPageNoticeLabel=heart_status;
}
void wellness_deinit(void)
{
    if(refresh_timer) {lv_timer_del(refresh_timer);refresh_timer=NULL;}
}
