/* SPDX-License-Identifier: GPL-3.0-only */
#include "watch_app.h"
#include "watch_storage.h"
#include "user_test/Func/Inc/HWDataAccess.h"
#include <direct.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

static watch_model model;
static watch_mode mode=WATCH_NORMAL;
static uint32_t ticks,saved_revision;
static char state_path[1024];
static const char *storage_status="Not saved";
static int initialized,directory_ready;
static void (*notice_listener)(int kind);
void watch_app_set_listener(void (*listener)(int kind)) {notice_listener=listener;}
static void changed(uint32_t prior_id,uint8_t prior_goal)
{
    if(notice_listener && model.next_alert_id!=prior_id) notice_listener(WATCH_NOTICE_ALERT);
    if(notice_listener && !prior_goal && model.goal_notified) notice_listener(WATCH_NOTICE_GOAL);
}

static uint32_t today(void)
{
    time_t now=time(NULL); struct tm local;
    if(localtime_s(&local,&now)!=0) return 20261004;
    return (uint32_t)((local.tm_year+1900)*10000+(local.tm_mon+1)*100+local.tm_mday);
}
static void publish_hardware(void)
{
    HWInterface.HR_meter.HrRate=model.latest_valid ? (uint8_t)model.latest_bpm : 0;
    HWInterface.HR_meter.ConnectionError=!model.latest_valid;
    HWInterface.IMU.Steps=(uint16_t)(model.steps>UINT16_MAX ? UINT16_MAX : model.steps);
    HWInterface.IMU.ConnectionError=0;
    HWInterface.Power.power_remain=85;
}
void watch_app_init(void)
{
    const char *directory=getenv("OV_WATCH_DATA_DIR");
    if(!directory || !*directory || strlen(directory)>900) directory="data";
    directory_ready=_mkdir(directory)==0 || errno==EEXIST;
    snprintf(state_path,sizeof(state_path),"%s/watch_state.bin",directory);
    watch_model_init(&model,today());
    watch_store_result result=watch_storage_load(&model,state_path);
    if(result==WATCH_STORE_OK) storage_status="Restored from disk";
    else if(result==WATCH_STORE_MISSING) storage_status="New profile";
    else if(result==WATCH_STORE_CORRUPT) {
        char backup[1100];
        snprintf(backup,sizeof(backup),"%s.invalid-%llu",state_path,(unsigned long long)time(NULL));
        if(!CopyFileA(state_path,backup,TRUE)) directory_ready=0;
        storage_status=directory_ready ? "Corrupt file backed up; defaults" : "Backup failed; saving disabled";
    } else {storage_status="Storage read failed; saving disabled";directory_ready=0;}
    if(!directory_ready && result!=WATCH_STORE_CORRUPT) storage_status="Data directory unavailable";
    watch_model_roll_day(&model,today());
    model.candidate_kind=model.candidate_count=model.recovery_count=0;
    model.revision++; saved_revision=0; initialized=1;
    publish_hardware();
    atexit(watch_app_shutdown);
}
void watch_app_tick(void)
{
    if(!initialized) return;
    watch_model_roll_day(&model,today());
    uint32_t prior_id=model.next_alert_id;
    uint8_t prior_goal=model.goal_notified;
    watch_model_simulate(&model,mode,ticks++,(uint32_t)time(NULL));
    publish_hardware();
    changed(prior_id,prior_goal);
    if(ticks%10==0 && saved_revision!=model.revision) watch_app_save();
}
watch_model *watch_app_model(void) {return &model;}
watch_mode watch_app_mode(void) {return mode;}
const char *watch_app_storage_status(void) {return storage_status;}
const char *watch_app_state_path(void) {return state_path;}
void watch_app_set_mode(watch_mode value)
{
    if(value>=WATCH_NORMAL && value<=WATCH_PAUSED) {
        mode=value; model.candidate_kind=model.candidate_count=model.recovery_count=0;
    }
}
int watch_app_save(void)
{
    if(!initialized || !directory_ready) return 0;
    if(watch_storage_save(&model,state_path)!=WATCH_STORE_OK) {
        storage_status="Save failed; changes in memory"; return 0;
    }
    storage_status="Saved to disk"; saved_revision=model.revision; return 1;
}
int watch_app_configure(const watch_settings *settings)
{
    uint8_t prior_goal=model.goal_notified;
    if(!watch_model_configure(&model,settings)) return 0;
    changed(model.next_alert_id,prior_goal);
    watch_app_save(); return 1;
}
void watch_app_add_steps(uint32_t amount)
{
    uint8_t prior_goal=model.goal_notified;
    watch_model_steps(&model,amount); publish_hardware();
    changed(model.next_alert_id,prior_goal); watch_app_save();
}
void watch_app_ack_all(void)
{
    for(size_t n=0;n<model.alert_count;n++) watch_model_ack(&model,watch_model_alert_at(&model,n)->id);
    watch_app_save();
}
void watch_app_shutdown(void)
{
    if(initialized && saved_revision!=model.revision) watch_app_save();
}
