/* SPDX-License-Identifier: GPL-3.0-only */
#include "watch_model.h"
#include "watch_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}} while(0)
static unsigned passed;
static void done(const char *name) {printf("PASS %s\n",name);passed++;}
static void feed(watch_model *m,uint16_t value,unsigned count,uint32_t at)
{for(unsigned n=0;n<count;n++) watch_model_sample(m,value,1,at+n);}

int main(int argc,char **argv)
{
    watch_model m,next;
    if(argc==3 && strcmp(argv[1],"--reject")==0) {
        watch_model_init(&m,20261004);m.steps=123;
        CHECK(watch_storage_load(&m,argv[2])==WATCH_STORE_CORRUPT);
        CHECK(m.steps==123 && m.settings.goal==3000);
        done("malformed_file_rejected_without_partial_load");return 0;
    }
    watch_model_init(&m,20261004);
    for(unsigned n=0;n<130;n++) watch_model_sample(&m,80,1,1000+n);
    CHECK(m.sample_count==120 && watch_model_sample_at(&m,0)->at==1010);
    CHECK(watch_model_sample_at(&m,119)->at==1129 && watch_model_sample_at(&m,120)==NULL);
    CHECK(m.alert_count==0 && watch_model_valid(&m));done("trend_ring_eviction");

    watch_model_init(&m,20261004);feed(&m,135,2,100);
    CHECK(m.alert_count==0);feed(&m,80,1,102);feed(&m,135,2,103);
    CHECK(m.alert_count==0);feed(&m,135,1,105);
    CHECK(m.alert_count==1 && m.active_kind==WATCH_ALERT_HIGH);done("consecutive_alert_debounce");
    feed(&m,135,20,106);CHECK(m.alert_count==1);done("sustained_alert_not_duplicated");
    CHECK(!watch_model_ack(&m,999));CHECK(watch_model_ack(&m,1));
    CHECK(watch_model_ack(&m,1) && watch_model_alert_at(&m,0)->acknowledged);
    CHECK(m.active_kind==WATCH_ALERT_HIGH);done("ack_does_not_clear_active_condition");
    feed(&m,119,3,130);CHECK(m.active_kind==WATCH_ALERT_HIGH);
    feed(&m,117,3,133);CHECK(!m.active_kind && watch_model_alert_at(&m,0)->resolved);
    done("hysteresis_recovery");

    watch_model_init(&m,20261004);feed(&m,135,3,100);feed(&m,80,3,103);
    feed(&m,135,3,106);CHECK(m.alert_count==1);feed(&m,135,1,132);
    CHECK(m.alert_count==2);done("recovered_alert_cooldown");
    watch_model_sample(&m,0,0,134);CHECK(!m.latest_valid && m.active_kind==WATCH_ALERT_HIGH);
    feed(&m,80,2,135);watch_model_sample(&m,600,1,137);feed(&m,80,2,138);
    CHECK(m.active_kind==WATCH_ALERT_HIGH);feed(&m,80,1,140);CHECK(!m.active_kind);
    done("invalid_signal_preserves_condition_resets_streak");

    watch_model_init(&m,20261004);feed(&m,135,3,100);feed(&m,40,3,103);
    CHECK(m.alert_count==2 && m.active_kind==WATCH_ALERT_LOW);
    CHECK(watch_model_alert_at(&m,0)->resolved && watch_model_valid(&m));done("high_to_low_transition");
    watch_settings config=m.settings;config.alerts_enabled=0;CHECK(watch_model_configure(&m,&config));
    feed(&m,135,4,200);CHECK(!m.active_kind && m.alert_count==2);done("disabled_reminders");
    config.high=config.low+5;CHECK(!watch_model_configure(&m,&config));
    CHECK(m.settings.high==120);config=m.settings;config.goal=499;
    CHECK(!watch_model_configure(&m,&config));
    watch_model_init(&m,20261004);feed(&m,135,3,100);
    config=m.settings;config.goal=3500;CHECK(watch_model_configure(&m,&config));
    CHECK(m.active_kind==WATCH_ALERT_HIGH && !watch_model_alert_at(&m,0)->resolved);
    done("settings_validation_and_independent_step_goal");

    watch_model_init(&m,20261004);watch_model_steps(&m,2999);CHECK(!m.goal_notified);
    watch_model_steps(&m,1);CHECK(m.goal_notified);watch_model_steps(&m,1000);
    config=m.settings;config.goal=5000;CHECK(watch_model_configure(&m,&config));
    CHECK(m.goal_notified);watch_model_steps(&m,UINT32_MAX);CHECK(m.steps==WATCH_MAX_STEPS);
    done("goal_latch_and_step_overflow");
    watch_model_roll_day(&m,20261005);CHECK(!m.steps && !m.goal_notified && m.day_count==1);
    CHECK(watch_model_day_at(&m,0)->steps==WATCH_MAX_STEPS);
    watch_model_roll_day(&m,20261004);CHECK(m.day==20261005 && m.day_count==1);
    CHECK(watch_day_valid(20240229) && !watch_day_valid(20250229) && !watch_day_valid(20261301));
    done("day_rollover_and_backwards_clock");
    for(uint32_t d=20261006;d<=20261015;d++) {watch_model_steps(&m,100);watch_model_roll_day(&m,d);}
    CHECK(m.day_count==7 && watch_model_day_at(&m,0)->day==20261008 && watch_model_valid(&m));
    done("daily_history_eviction");

    watch_model_init(&m,20261004);
    for(unsigned n=0;n<20;n++) {feed(&m,135,3,100+n*60);feed(&m,80,3,103+n*60);}
    CHECK(m.alert_count==16 && watch_model_alert_at(&m,0)->id==5 && watch_model_valid(&m));
    done("alert_history_eviction");
    unsigned samples=m.sample_count;uint32_t steps=m.steps;
    watch_model_simulate(&m,WATCH_PAUSED,0,1400);CHECK(m.sample_count==samples && m.steps==steps);
    watch_model_simulate(&m,WATCH_INVALID,0,1400);CHECK(!m.latest_valid);
    done("simulation_pause_and_invalid_mode");

    watch_model_steps(&m,3100);watch_model_ack(&m,20);watch_model_roll_day(&m,20261005);
    for(unsigned n=0;n<140;n++) watch_model_sample(&m,82,1,1500+n);
    CHECK(watch_storage_save(&m,"state.bin")==WATCH_STORE_OK);
    watch_model_init(&next,20261004);CHECK(watch_storage_load(&next,"state.bin")==WATCH_STORE_OK);
    CHECK(next.day==m.day && next.steps==m.steps && next.sample_count==120 && next.alert_count==16);
    CHECK(next.day_count==1 && watch_model_alert_at(&next,15)->acknowledged);
    for(size_t n=0;n<120;n++) CHECK(watch_model_sample_at(&next,n)->at==watch_model_sample_at(&m,n)->at);
    done("persistent_ring_roundtrip");
    FILE *orphan=fopen("state.bin.tmp","wb");CHECK(orphan);fputs("interrupted write",orphan);fclose(orphan);
    CHECK(watch_storage_load(&next,"state.bin")==WATCH_STORE_OK);
    m.steps=456;CHECK(watch_storage_save(&m,"state.bin")==WATCH_STORE_OK);
    CHECK(watch_storage_load(&next,"state.bin")==WATCH_STORE_OK && next.steps==456);
    done("orphan_temp_ignored_and_atomic_replacement");
    CHECK(watch_storage_save(&m,"missing-directory/state.bin")==WATCH_STORE_IO);
    CHECK(watch_storage_load(&next,"state.bin")==WATCH_STORE_OK && next.steps==456);
    CHECK(watch_storage_load(&next,"not-created.bin")==WATCH_STORE_MISSING);
    done("storage_failure_keeps_committed_state");
    printf("CORE_CASES_PASSED %u\n",passed);return 0;
}
