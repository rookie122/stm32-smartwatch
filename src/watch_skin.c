/* SPDX-License-Identifier: GPL-3.0-only
 * Restyle existing screen objects while preserving upstream event callbacks.
 */
#include "watch_skin.h"
#include "watch_app.h"
#include "user_test/GUI_App/Screens/Inc/ui_HomePage.h"
#include "user_test/GUI_App/Screens/Inc/ui_MenuPage.h"
#include "user_test/GUI_App/Screens/Inc/ui_HRPage.h"
#include "user_test/GUI_App/Screens/Inc/ui_WellnessPage.h"
#include <stdio.h>
#include <time.h>
/* These upstream objects are defined in ui_HomePage.c, but absent from its header. */
extern lv_obj_t *ui_HRArc;
extern lv_obj_t *ui_MenuGamePanel,*ui_MenuGameButton,*ui_MenuGameicon,*ui_MenuGameLabel;
static lv_obj_t *goal_arc,*goal_percent,*goal_caption;
lv_obj_t *watch_home_apps;

void watch_skin_screen(lv_obj_t *screen)
{
    lv_obj_set_style_bg_color(screen,lv_color_hex(WATCH_BG),0);
    lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0);
    lv_obj_set_style_text_color(screen,lv_color_hex(WATCH_TEXT),0);
    lv_obj_set_style_text_font(screen,&lv_font_montserrat_12,0);
    lv_obj_set_style_pad_all(screen,0,0);
    lv_obj_set_style_border_width(screen,0,0);
}
void watch_skin_button(lv_obj_t *obj,int primary)
{
    lv_obj_set_style_radius(obj,10,0);lv_obj_set_style_shadow_width(obj,0,0);
    lv_obj_set_style_bg_color(obj,lv_color_hex(primary?WATCH_MINT:WATCH_CARD),0);
    lv_obj_set_style_border_width(obj,primary?0:1,0);
    lv_obj_set_style_border_color(obj,lv_color_hex(WATCH_EDGE),0);
    lv_obj_set_style_text_color(obj,lv_color_hex(primary?WATCH_BG:WATCH_TEXT),0);
    lv_obj_set_style_text_font(obj,&lv_font_montserrat_12,0);
    lv_obj_set_style_pad_all(obj,7,0);
    lv_obj_set_style_bg_color(obj,lv_color_hex(primary?0x8DECCF:0x26384B),LV_STATE_PRESSED);
}
static void place(lv_obj_t *obj,int x,int y)
{
    lv_obj_set_align(obj,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(obj,x,y);
}
static lv_obj_t *caption(lv_obj_t *parent,const char *text,int x,int y,uint32_t color,const lv_font_t *font)
{
    lv_obj_t *obj=lv_label_create(parent);lv_label_set_text(obj,text);
    place(obj,x,y);lv_obj_set_style_text_color(obj,lv_color_hex(color),0);
    lv_obj_set_style_text_font(obj,font,0);return obj;
}
static lv_obj_t *card(lv_obj_t *parent,int x,int y,int width,int height)
{
    lv_obj_t *obj=lv_obj_create(parent);place(obj,x,y);lv_obj_set_size(obj,width,height);
    lv_obj_set_style_pad_all(obj,0,0);lv_obj_set_style_border_width(obj,0,0);
    lv_obj_set_style_bg_color(obj,lv_color_hex(WATCH_CARD),0);lv_obj_set_style_radius(obj,18,0);
    lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLLABLE);return obj;
}
static void open_page(lv_event_t *event)
{
    Page_Load((Page_t *)lv_event_get_user_data(event));
}
void watch_skin_home_refresh(void)
{
    watch_model *m=watch_app_model();char text[40];
    time_t now=time(NULL);struct tm local;
    if(localtime_s(&local,&now)==0) {
        ui_TimeHourValue=(uint8_t)local.tm_hour;ui_TimeMinuteValue=(uint8_t)local.tm_min;
        snprintf(text,sizeof(text),"%02u",ui_TimeHourValue);lv_label_set_text(ui_TimeHourLabel,text);
        snprintf(text,sizeof(text),"%02u",ui_TimeMinuteValue);lv_label_set_text(ui_TimeMinuteLabel,text);
        strftime(text,sizeof(text),"%b %d",&local);lv_label_set_text(ui_DateLabel,text);
        strftime(text,sizeof(text),"%a",&local);lv_label_set_text(ui_DayLabel,text);
    }
    lv_label_set_text(ui_BatNumLabel,"85%");
    snprintf(text,sizeof(text),"%u",m->steps);lv_label_set_text(ui_StepNumLabel,text);
    unsigned percent=(unsigned)((uint64_t)m->steps*100/m->settings.goal);
    lv_arc_set_value(goal_arc,percent>100?100:(int)percent);
    snprintf(text,sizeof(text),"%u%%",percent>100?100:percent);lv_label_set_text(goal_percent,text);
    lv_obj_align_to(goal_percent,goal_arc,LV_ALIGN_CENTER,0,0);
    snprintf(text,sizeof(text),"of %u steps",m->settings.goal);lv_label_set_text(goal_caption,text);
    if(m->latest_valid) snprintf(text,sizeof(text),"%u",m->latest_bpm);else snprintf(text,sizeof(text),"--");
    lv_label_set_text(ui_HRNumLabel,text);
}
void watch_skin_home(void)
{
    watch_skin_screen(ui_HomePage);
    /* Sensor-only complications and the hardware quick-settings drawer are hidden.
     * Their original objects remain owned by the screen and are deleted with it. */
    lv_obj_t *hidden[]={ui_BatArc,ui_BaticonLabel,ui_StepiconLabel,ui_StepNumBar,
        ui_TempArc,ui_TempiconLabel,ui_TempNumLabel,ui_HumiArc,ui_HumiiconLabel,
        ui_HumiNumLabel,ui_HRArc,ui_DropDownPanel};
    for(unsigned n=0;n<sizeof(hidden)/sizeof(hidden[0]);n++) lv_obj_add_flag(hidden[n],LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *date[]={ui_DateLabel,ui_DayLabel,ui_BatNumLabel};
    for(unsigned n=0;n<3;n++) {
        lv_obj_set_style_text_font(date[n],&lv_font_montserrat_12,0);
        lv_obj_set_style_text_color(date[n],lv_color_hex(WATCH_MUTED),0);
    }
    place(ui_DateLabel,14,12);place(ui_DayLabel,75,12);
    lv_obj_align(ui_BatNumLabel,LV_ALIGN_TOP_RIGHT,-14,12);
    lv_obj_t *clock[]={ui_TimeHourLabel,ui_TimeColonLabel,ui_TimeMinuteLabel};
    for(unsigned n=0;n<3;n++) lv_obj_set_style_text_font(clock[n],&lv_font_montserrat_40,0);
    place(ui_TimeHourLabel,14,34);place(ui_TimeColonLabel,69,32);place(ui_TimeMinuteLabel,84,34);
    lv_obj_set_style_text_color(ui_TimeColonLabel,lv_color_hex(WATCH_MINT),0);
    caption(ui_HomePage,"OV / ACTIVE",155,58,WATCH_MINT,&lv_font_montserrat_12);
    lv_obj_t *activity=card(ui_HomePage,12,91,216,94);
    lv_obj_add_event_cb(activity,open_page,LV_EVENT_CLICKED,&Page_Sports);
    goal_arc=lv_arc_create(activity);lv_obj_set_size(goal_arc,70,70);place(goal_arc,10,12);
    lv_arc_set_rotation(goal_arc,270);lv_arc_set_bg_angles(goal_arc,0,360);lv_arc_set_range(goal_arc,0,100);
    lv_obj_clear_flag(goal_arc,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(goal_arc,lv_color_hex(WATCH_EDGE),LV_PART_MAIN);
    lv_obj_set_style_arc_width(goal_arc,6,LV_PART_MAIN);
    lv_obj_set_style_arc_color(goal_arc,lv_color_hex(WATCH_MINT),LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(goal_arc,6,LV_PART_INDICATOR);
    lv_obj_remove_style(goal_arc,NULL,LV_PART_KNOB);
    goal_percent=caption(activity,"0%",0,0,WATCH_MINT,&lv_font_montserrat_14);
    lv_obj_set_parent(ui_StepCnLabel,activity);place(ui_StepCnLabel,94,12);
    lv_label_set_text(ui_StepCnLabel,"DAILY STEPS");lv_obj_set_style_text_font(ui_StepCnLabel,&lv_font_montserrat_12,0);
    lv_obj_set_style_text_color(ui_StepCnLabel,lv_color_hex(WATCH_MUTED),0);
    lv_obj_set_parent(ui_StepNumLabel,activity);place(ui_StepNumLabel,92,30);
    lv_obj_set_style_text_font(ui_StepNumLabel,&lv_font_montserrat_28,0);
    lv_obj_set_style_text_color(ui_StepNumLabel,lv_color_hex(WATCH_TEXT),0);
    goal_caption=caption(activity,"of 3000 steps",94,66,WATCH_MUTED,&lv_font_montserrat_12);
    lv_obj_t *heart=card(ui_HomePage,12,195,216,48);
    lv_obj_add_event_cb(heart,open_page,LV_EVENT_CLICKED,&Page_HR);
    lv_obj_set_parent(ui_HRiconLabel,heart);place(ui_HRiconLabel,10,10);
    lv_obj_set_style_text_color(ui_HRiconLabel,lv_color_hex(WATCH_ROSE),0);
    lv_obj_set_parent(ui_HRNumLabel,heart);place(ui_HRNumLabel,47,8);
    lv_obj_set_style_text_font(ui_HRNumLabel,&lv_font_montserrat_24,0);
    caption(heart,"bpm",103,18,WATCH_MUTED,&lv_font_montserrat_12);
    caption(heart,LV_SYMBOL_RIGHT,192,17,WATCH_MUTED,&lv_font_montserrat_14);
    lv_obj_t *apps=watch_home_apps=lv_btn_create(ui_HomePage);place(apps,12,252);lv_obj_set_size(apps,216,24);
    watch_skin_button(apps,0);lv_obj_set_style_bg_opa(apps,LV_OPA_TRANSP,0);
    lv_obj_set_style_border_width(apps,0,0);
    lv_obj_t *text=caption(apps,"All apps  " LV_SYMBOL_RIGHT,0,0,WATCH_MUTED,&lv_font_montserrat_12);
    lv_obj_center(text);lv_obj_add_event_cb(apps,open_page,LV_EVENT_CLICKED,&Page_Menu);
    watch_skin_home_refresh();
}
void watch_skin_menu(void)
{
    int scroll=ui_MenuScrollY;
    watch_skin_screen(ui_MenuPage);lv_obj_set_style_pad_all(ui_MenuPage,12,0);
    caption(ui_MenuPage,"Apps",0,0,WATCH_TEXT,&lv_font_montserrat_20);
    caption(ui_MenuPage,"OV",182,6,WATCH_MINT,&lv_font_montserrat_12);
    struct {lv_obj_t *panel,*icon_button,*icon,*label;const char *title;uint32_t tint;} tiles[]={
        {ui_MenuHRPanel,ui_MenuHRButton,ui_MenuHRicon,ui_MenuHRLabel,"Heart",WATCH_ROSE},
        {ui_MenuSprPanel,ui_MenuSprButton,ui_MenuSpricon,ui_MenuSprLabel,"Activity",WATCH_MINT},
        {ui_MenuCalPanel,ui_MenuCalButton,ui_MenuCalicon,ui_MenuCalLabel,"Calendar",0xACAAFF},
        {ui_MenuTimPanel,ui_MenuTimButton,ui_MenuTimicon,ui_MenuTimLabel,"Timer",0x8EC6FF},
        {ui_MenuComPanel,ui_MenuComButton,ui_MenuComicon,ui_MenuComLabel,"Calculator",0xF3C17B},
        {ui_MenuSetPanel,ui_MenuSetButton,ui_MenuSeticon,ui_MenuSetLabel,"Device",0x8EC6FF},
        {ui_MenuEnvPanel,ui_MenuEnvButton,ui_MenuEnvicon,ui_MenuEnvLabel,"Environment",WATCH_MINT},
        {ui_MenuCPPanel,ui_MenuCPButton,ui_MenuCPicon,ui_MenuCPLabel,"Compass",0xACAAFF},
        {ui_MenuO2Panel,ui_MenuO2Button,ui_MenuO2icon,ui_MenuO2Label,"SpO2",WATCH_ROSE},
        {ui_MenuCardPanel,ui_MenuCardButton,ui_MenuCardicon,ui_MenuCardLabel,"NFC",0xF3C17B},
        {ui_MenuGamePanel,ui_MenuGameButton,ui_MenuGameicon,ui_MenuGameLabel,"Games",0xACAAFF},
        {ui_MenuAbPanel,ui_MenuAbButton,ui_MenuAbicon,ui_MenuAbLabel,"About",WATCH_MUTED}
    };
    for(unsigned n=0;n<sizeof(tiles)/sizeof(tiles[0]);n++) {
        lv_obj_t *panel=tiles[n].panel;
        place(panel,(n%2)*112,38+(n/2)*76);lv_obj_set_size(panel,104,68);
        lv_obj_set_style_pad_all(panel,8,0);lv_obj_set_style_radius(panel,14,0);
        lv_obj_set_style_bg_color(panel,lv_color_hex(WATCH_CARD),0);lv_obj_set_style_bg_opa(panel,LV_OPA_COVER,0);
        lv_obj_set_style_bg_color(panel,lv_color_hex(0x26384B),LV_STATE_PRESSED);
        place(tiles[n].icon_button,0,-4);lv_obj_set_size(tiles[n].icon_button,36,36);
        lv_obj_set_style_bg_opa(tiles[n].icon_button,LV_OPA_TRANSP,0);
        lv_obj_set_style_shadow_width(tiles[n].icon_button,0,0);
        lv_obj_clear_flag(tiles[n].icon_button,LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_style_text_color(tiles[n].icon,lv_color_hex(tiles[n].tint),0);
        lv_obj_center(tiles[n].icon);
        place(tiles[n].label,0,36);lv_obj_set_width(tiles[n].label,88);
        lv_label_set_text(tiles[n].label,tiles[n].title);
        lv_obj_set_style_text_font(tiles[n].label,&lv_font_montserrat_12,0);
        lv_obj_set_style_text_color(tiles[n].label,lv_color_hex(WATCH_TEXT),0);
    }
    lv_obj_update_layout(ui_MenuPage);
    lv_obj_scroll_to_y(ui_MenuPage,scroll,LV_ANIM_OFF);
}
