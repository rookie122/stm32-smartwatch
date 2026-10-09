// LVGL VERSION: 8.2.0

#include "ui.h"
#include "ui_helpers.h"
#include "./Screens/Inc/ui_HomePage.h"
#include "../Func/Inc/pubsub.h"
#include "watch_app.h"
#include "watch_skin.h"

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP !=0
    #error "LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings"
#endif

///////////////////// CallBack functions ////////////////////
static lv_obj_t *notice;
static lv_timer_t *notice_timer;
unsigned ui_notification_count;
void ui_notifications_dismiss(void)
{
    if(notice_timer) {lv_timer_del(notice_timer);notice_timer=NULL;}
    if(notice) {lv_obj_del(notice);notice=NULL;}
}
static void dismiss_notice(lv_timer_t *timer)
{
    (void)timer;
    ui_notifications_dismiss();
}
static void model_notice(int kind)
{
    ui_notifications_dismiss();
    ui_notification_count++;
    notice=lv_obj_create(lv_layer_top());
    lv_obj_set_size(notice,220,52);lv_obj_align(notice,LV_ALIGN_TOP_MID,0,4);
    lv_obj_clear_flag(notice,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(notice,8,0);
    lv_obj_set_style_bg_color(notice,lv_palette_darken(kind==WATCH_NOTICE_GOAL?LV_PALETTE_GREEN:LV_PALETTE_RED,3),0);
    lv_obj_t *text=lv_label_create(notice);lv_obj_set_width(text,200);
    lv_obj_set_style_text_font(text,&lv_font_montserrat_14,0);
    lv_label_set_text(text,kind==WATCH_NOTICE_GOAL?"Daily step goal reached":"Configured threshold reached");
    notice_timer=lv_timer_create(dismiss_notice,3000,NULL);
}

void SDL_KeyBoard_Subscriber(PubSub_Message_t message)
{
    printf("change screen\r\n");
    Page_Back();
}

/////////////////////// Timer //////////////////////
static void main_timer(lv_timer_t * timer)
{
    (void)timer;
    watch_app_tick();
}

/////////////////////// ui_initialize //////////////////////
void ui_init(void)
{
    Publisher_init(&SDL_KeyBoard_Publisher);
    Publisher_subscribe(&SDL_KeyBoard_Publisher, SDL_KeyBoard_Subscriber);

    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_color_hex(WATCH_MINT), lv_color_hex(WATCH_ROSE),
                                               true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    Pages_init();
    watch_app_set_listener(model_notice);
    //timer
    lv_timer_t * ui_MainTimer = lv_timer_create(main_timer, 1000,  NULL);
}
