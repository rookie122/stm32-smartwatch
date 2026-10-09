/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WATCH_MODEL_H
#define WATCH_MODEL_H
#include <stddef.h>
#include <stdint.h>

#define WATCH_SAMPLES 120
#define WATCH_ALERTS 16
#define WATCH_DAYS 7
#define WATCH_DEBOUNCE 3
#define WATCH_COOLDOWN 30
#define WATCH_MAX_STEPS 200000

typedef enum { WATCH_NORMAL, WATCH_HIGH, WATCH_LOW, WATCH_INVALID, WATCH_PAUSED } watch_mode;
typedef enum { WATCH_ALERT_NONE, WATCH_ALERT_HIGH, WATCH_ALERT_LOW } watch_alert_kind;
typedef struct { uint32_t at; uint16_t bpm; uint8_t valid; } watch_sample;
typedef struct { uint32_t id, at; uint16_t bpm; uint8_t kind, acknowledged, resolved; } watch_alert;
typedef struct { uint32_t day, steps, goal; } watch_day;
typedef struct {
    uint16_t low, high;
    uint32_t goal;
    uint8_t alerts_enabled;
} watch_settings;
typedef struct {
    watch_settings settings;
    uint32_t day, steps, next_alert_id, last_alert_at, revision;
    uint16_t latest_bpm;
    uint8_t latest_valid, goal_notified, active_kind, candidate_kind;
    uint8_t candidate_count, recovery_count;
    uint16_t sample_head, sample_count;
    uint8_t alert_head, alert_count, day_head, day_count;
    watch_sample samples[WATCH_SAMPLES];
    watch_alert alerts[WATCH_ALERTS];
    watch_day days[WATCH_DAYS];
} watch_model;

void watch_model_init(watch_model *m, uint32_t day);
int watch_day_valid(uint32_t day);
int watch_settings_valid(const watch_settings *settings);
int watch_model_configure(watch_model *m, const watch_settings *settings);
void watch_model_roll_day(watch_model *m, uint32_t day);
void watch_model_sample(watch_model *m, uint16_t bpm, int valid, uint32_t at);
void watch_model_steps(watch_model *m, uint32_t increment);
int watch_model_ack(watch_model *m, uint32_t id);
const watch_sample *watch_model_sample_at(const watch_model *m, size_t index);
const watch_alert *watch_model_alert_at(const watch_model *m, size_t index);
const watch_day *watch_model_day_at(const watch_model *m, size_t index);
void watch_model_simulate(watch_model *m, watch_mode mode, uint32_t tick, uint32_t at);
int watch_model_valid(const watch_model *m);
#endif
