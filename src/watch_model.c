/* SPDX-License-Identifier: GPL-3.0-only */
#include "watch_model.h"
#include <string.h>

int watch_day_valid(uint32_t day)
{
    static const unsigned lengths[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    unsigned year = day / 10000, month = day / 100 % 100, date = day % 100;
    if(year < 2000 || year > 2099 || month < 1 || month > 12) return 0;
    unsigned limit = lengths[month - 1] + (month == 2 && year % 4 == 0);
    return date >= 1 && date <= limit;
}

int watch_settings_valid(const watch_settings *s)
{
    return s && s->low >= 30 && s->low <= 200 && s->high >= 40 && s->high <= 220
        && s->high >= s->low + 10 && s->goal >= 500 && s->goal <= 50000
        && s->alerts_enabled <= 1;
}

void watch_model_init(watch_model *m, uint32_t day)
{
    memset(m, 0, sizeof(*m));
    m->settings = (watch_settings){50, 120, 3000, 1};
    m->day = watch_day_valid(day) ? day : 20261004;
    m->next_alert_id = 1;
}

const watch_sample *watch_model_sample_at(const watch_model *m, size_t i)
{
    return i < m->sample_count ? &m->samples[(m->sample_head + i) % WATCH_SAMPLES] : NULL;
}
const watch_alert *watch_model_alert_at(const watch_model *m, size_t i)
{
    return i < m->alert_count ? &m->alerts[(m->alert_head + i) % WATCH_ALERTS] : NULL;
}
const watch_day *watch_model_day_at(const watch_model *m, size_t i)
{
    return i < m->day_count ? &m->days[(m->day_head + i) % WATCH_DAYS] : NULL;
}

static void resolve_alert(watch_model *m)
{
    if(m->active_kind && m->alert_count) {
        size_t i = (m->alert_head + m->alert_count - 1) % WATCH_ALERTS;
        m->alerts[i].resolved = 1;
    }
    m->active_kind = WATCH_ALERT_NONE;
    m->recovery_count = 0;
}

int watch_model_configure(watch_model *m, const watch_settings *s)
{
    if(!watch_settings_valid(s)) return 0;
    if(m->settings.low==s->low && m->settings.high==s->high && m->settings.goal==s->goal
        && m->settings.alerts_enabled==s->alerts_enabled) return 1;
    int reminder_changed=m->settings.low!=s->low || m->settings.high!=s->high
        || m->settings.alerts_enabled!=s->alerts_enabled;
    m->settings = *s;
    if(reminder_changed) {
        resolve_alert(m);
        m->candidate_kind = m->candidate_count = 0;
    }
    if(m->steps >= s->goal) m->goal_notified = 1;
    m->revision++;
    return 1;
}

void watch_model_roll_day(watch_model *m, uint32_t day)
{
    /* A backwards clock change must not award another daily goal or archive twice. */
    if(!watch_day_valid(day) || day <= m->day) return;
    size_t i = (m->day_head + m->day_count) % WATCH_DAYS;
    if(m->day_count == WATCH_DAYS) m->day_head = (m->day_head + 1) % WATCH_DAYS;
    else m->day_count++;
    m->days[i] = (watch_day){m->day, m->steps, m->settings.goal};
    m->day = day;
    m->steps = 0;
    m->goal_notified = 0;
    m->revision++;
}

void watch_model_steps(watch_model *m, uint32_t increment)
{
    uint32_t room = WATCH_MAX_STEPS - m->steps;
    m->steps += increment > room ? room : increment;
    if(m->steps >= m->settings.goal) m->goal_notified = 1;
    if(increment) m->revision++;
}

static void start_alert(watch_model *m, uint8_t kind, uint16_t bpm, uint32_t at)
{
    if(m->next_alert_id == UINT32_MAX) return;
    resolve_alert(m);
    size_t i = (m->alert_head + m->alert_count) % WATCH_ALERTS;
    if(m->alert_count == WATCH_ALERTS) m->alert_head = (m->alert_head + 1) % WATCH_ALERTS;
    else m->alert_count++;
    m->alerts[i] = (watch_alert){m->next_alert_id++, at, bpm, kind, 0, 0};
    m->active_kind = kind;
    m->last_alert_at = at;
    m->candidate_kind = m->candidate_count = 0;
}

void watch_model_sample(watch_model *m, uint16_t bpm, int valid, uint32_t at)
{
    valid = valid && bpm >= 20 && bpm <= 250;
    size_t i = (m->sample_head + m->sample_count) % WATCH_SAMPLES;
    if(m->sample_count == WATCH_SAMPLES) m->sample_head = (m->sample_head + 1) % WATCH_SAMPLES;
    else m->sample_count++;
    m->samples[i] = (watch_sample){at, bpm, (uint8_t)valid};
    m->latest_bpm = bpm;
    m->latest_valid = (uint8_t)valid;
    m->revision++;
    if(!valid || !m->settings.alerts_enabled) {
        m->candidate_kind = m->candidate_count = m->recovery_count = 0;
        return;
    }
    uint8_t kind = bpm > m->settings.high ? WATCH_ALERT_HIGH :
        (bpm < m->settings.low ? WATCH_ALERT_LOW : WATCH_ALERT_NONE);
    if(m->active_kind) {
        int recovered = bpm >= m->settings.low + 3 && bpm <= m->settings.high - 3;
        if(recovered) {
            if(++m->recovery_count >= WATCH_DEBOUNCE) resolve_alert(m);
        } else m->recovery_count = 0;
    }
    if(!kind || kind == m->active_kind) {
        m->candidate_kind = m->candidate_count = 0;
        return;
    }
    if(kind != m->candidate_kind) {
        m->candidate_kind = kind;
        m->candidate_count = 1;
    } else if(m->candidate_count < WATCH_DEBOUNCE) m->candidate_count++;
    if(m->candidate_count >= WATCH_DEBOUNCE) {
        /* Opposite active alarms can change immediately; a recovered alarm has a cooldown. */
        int cooldown_over = m->next_alert_id == 1 || (at >= m->last_alert_at && at - m->last_alert_at >= WATCH_COOLDOWN);
        if(m->active_kind || cooldown_over) start_alert(m, kind, bpm, at);
    }
}

int watch_model_ack(watch_model *m, uint32_t id)
{
    for(size_t n = 0; n < m->alert_count; n++) {
        watch_alert *a = &m->alerts[(m->alert_head + n) % WATCH_ALERTS];
        if(a->id == id) {
            if(!a->acknowledged) { a->acknowledged = 1; m->revision++; }
            return 1;
        }
    }
    return 0;
}

void watch_model_simulate(watch_model *m, watch_mode mode, uint32_t tick, uint32_t at)
{
    if(mode == WATCH_PAUSED) return;
    uint16_t bpm = (uint16_t)(72 + (tick % 20 <= 10 ? tick % 20 : 20 - tick % 20));
    if(mode == WATCH_HIGH) bpm = (uint16_t)(m->settings.high + 15);
    if(mode == WATCH_LOW) bpm = (uint16_t)(m->settings.low - 10);
    if(mode == WATCH_INVALID) bpm = 0;
    watch_model_sample(m, bpm, mode != WATCH_INVALID, at);
    watch_model_steps(m, 2 + tick % 5);
}

int watch_model_valid(const watch_model *m)
{
    if(!watch_settings_valid(&m->settings) || !watch_day_valid(m->day) || m->steps > WATCH_MAX_STEPS
        || !m->next_alert_id || m->latest_valid > 1 || m->goal_notified > 1
        || m->active_kind > WATCH_ALERT_LOW || m->candidate_kind > WATCH_ALERT_LOW
        || m->candidate_count > WATCH_DEBOUNCE || m->recovery_count >= WATCH_DEBOUNCE
        || m->sample_head >= WATCH_SAMPLES || m->sample_count > WATCH_SAMPLES
        || m->alert_head >= WATCH_ALERTS || m->alert_count > WATCH_ALERTS
        || m->day_head >= WATCH_DAYS || m->day_count > WATCH_DAYS) return 0;
    if(m->latest_valid && (m->latest_bpm < 20 || m->latest_bpm > 250)) return 0;
    uint32_t prior_id = 0, prior_day = 0;
    for(size_t n = 0; n < m->sample_count; n++) {
        const watch_sample *s = watch_model_sample_at(m,n);
        if(s->valid > 1 || (s->valid && (s->bpm < 20 || s->bpm > 250))) return 0;
    }
    for(size_t n = 0; n < m->alert_count; n++) {
        const watch_alert *a = watch_model_alert_at(m,n);
        if(a->id <= prior_id || a->id >= m->next_alert_id || a->kind < 1 || a->kind > 2
            || a->bpm < 20 || a->bpm > 250 || a->acknowledged > 1 || a->resolved > 1) return 0;
        prior_id = a->id;
    }
    if(m->active_kind && (!m->alert_count || watch_model_alert_at(m,m->alert_count-1)->resolved
        || watch_model_alert_at(m,m->alert_count-1)->kind != m->active_kind)) return 0;
    for(size_t n = 0; n < m->day_count; n++) {
        const watch_day *d = watch_model_day_at(m,n);
        if(!watch_day_valid(d->day) || d->day <= prior_day || d->day >= m->day
            || d->steps > WATCH_MAX_STEPS || d->goal < 500 || d->goal > 50000) return 0;
        prior_day = d->day;
    }
    return 1;
}
