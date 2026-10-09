/* SPDX-License-Identifier: GPL-3.0-only */
#include "watch_storage.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

#define STORE_LIMIT 2048
typedef struct { uint8_t bytes[STORE_LIMIT]; size_t offset, length; int failed; } stream;
static void put(stream *s, uint32_t v, unsigned width)
{
    if(s->offset + width > STORE_LIMIT) {s->failed = 1; return;}
    for(unsigned n=0; n<width; n++) s->bytes[s->offset++] = (uint8_t)(v >> (n*8));
}
static uint32_t get(stream *s, unsigned width)
{
    if(s->offset + width > s->length) {s->failed = 1; return 0;}
    uint32_t v=0;
    for(unsigned n=0; n<width; n++) v |= (uint32_t)s->bytes[s->offset++] << (n*8);
    return v;
}
static uint32_t crc32(const uint8_t *p, size_t count)
{
    uint32_t crc = UINT32_MAX;
    for(size_t i=0; i<count; i++) {
        crc ^= p[i];
        for(unsigned n=0; n<8; n++) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
    }
    return ~crc;
}

watch_store_result watch_storage_save(const watch_model *m, const char *path)
{
    if(!watch_model_valid(m) || !path || strlen(path) > 900) return WATCH_STORE_CORRUPT;
    stream s = {{0},0,0,0};
    put(&s,0x3157564f,4); /* OVW1, explicit little-endian format; no native struct dumps. */
    put(&s,1,2);
    put(&s,m->settings.low,2); put(&s,m->settings.high,2); put(&s,m->settings.goal,4);
    put(&s,m->settings.alerts_enabled,1);
    put(&s,m->day,4); put(&s,m->steps,4); put(&s,m->next_alert_id,4); put(&s,m->last_alert_at,4);
    put(&s,m->latest_bpm,2); put(&s,m->latest_valid,1); put(&s,m->goal_notified,1);
    put(&s,m->active_kind,1); put(&s,m->candidate_kind,1);
    put(&s,m->candidate_count,1); put(&s,m->recovery_count,1);
    put(&s,m->sample_count,2); put(&s,m->alert_count,1); put(&s,m->day_count,1);
    for(size_t n=0; n<m->sample_count; n++) {
        const watch_sample *v=watch_model_sample_at(m,n);
        put(&s,v->at,4); put(&s,v->bpm,2); put(&s,v->valid,1);
    }
    for(size_t n=0; n<m->alert_count; n++) {
        const watch_alert *v=watch_model_alert_at(m,n);
        put(&s,v->id,4); put(&s,v->at,4); put(&s,v->bpm,2);
        put(&s,v->kind,1); put(&s,v->acknowledged,1); put(&s,v->resolved,1);
    }
    for(size_t n=0; n<m->day_count; n++) {
        const watch_day *v=watch_model_day_at(m,n);
        put(&s,v->day,4); put(&s,v->steps,4); put(&s,v->goal,4);
    }
    uint32_t checksum=crc32(s.bytes,s.offset);
    put(&s,checksum,4);
    if(s.failed) return WATCH_STORE_CORRUPT;
    char temp[1024];
    snprintf(temp,sizeof(temp),"%s.tmp",path);
    FILE *file=fopen(temp,"wb");
    if(!file) return WATCH_STORE_IO;
    int ok=fwrite(s.bytes,1,s.offset,file)==s.offset && fflush(file)==0;
#ifdef _WIN32
    if(ok) ok=_commit(_fileno(file))==0;
#endif
    if(fclose(file)!=0) ok=0;
    if(!ok) {remove(temp); return WATCH_STORE_IO;}
#ifdef _WIN32
    if(!MoveFileExA(temp,path,MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        remove(temp); return WATCH_STORE_IO;
    }
#else
    if(rename(temp,path)!=0) {remove(temp); return WATCH_STORE_IO;}
#endif
    return WATCH_STORE_OK;
}

watch_store_result watch_storage_load(watch_model *m, const char *path)
{
    FILE *file=fopen(path,"rb");
    if(!file) return errno==ENOENT ? WATCH_STORE_MISSING : WATCH_STORE_IO;
    stream s={{0},0,0,0};
    s.length=fread(s.bytes,1,STORE_LIMIT,file);
    int extra=fgetc(file), failed=ferror(file);
    fclose(file);
    if(failed) return WATCH_STORE_IO;
    if(extra!=EOF || s.length<46) return WATCH_STORE_CORRUPT;
    s.offset=s.length-4;
    uint32_t stored=get(&s,4);
    if(stored!=crc32(s.bytes,s.length-4)) return WATCH_STORE_CORRUPT;
    s.length-=4; s.offset=0;
    if(get(&s,4)!=0x3157564f || get(&s,2)!=1) return WATCH_STORE_CORRUPT;
    watch_model next;
    memset(&next,0,sizeof(next));
    next.settings.low=(uint16_t)get(&s,2); next.settings.high=(uint16_t)get(&s,2);
    next.settings.goal=get(&s,4); next.settings.alerts_enabled=(uint8_t)get(&s,1);
    next.day=get(&s,4); next.steps=get(&s,4); next.next_alert_id=get(&s,4); next.last_alert_at=get(&s,4);
    next.latest_bpm=(uint16_t)get(&s,2); next.latest_valid=(uint8_t)get(&s,1);
    next.goal_notified=(uint8_t)get(&s,1); next.active_kind=(uint8_t)get(&s,1);
    next.candidate_kind=(uint8_t)get(&s,1); next.candidate_count=(uint8_t)get(&s,1);
    next.recovery_count=(uint8_t)get(&s,1); next.sample_count=(uint16_t)get(&s,2);
    next.alert_count=(uint8_t)get(&s,1); next.day_count=(uint8_t)get(&s,1);
    if(next.sample_count>WATCH_SAMPLES || next.alert_count>WATCH_ALERTS || next.day_count>WATCH_DAYS)
        return WATCH_STORE_CORRUPT;
    for(size_t n=0;n<next.sample_count;n++) {
        next.samples[n].at=get(&s,4); next.samples[n].bpm=(uint16_t)get(&s,2);
        next.samples[n].valid=(uint8_t)get(&s,1);
    }
    for(size_t n=0;n<next.alert_count;n++) {
        watch_alert *v=&next.alerts[n];
        v->id=get(&s,4); v->at=get(&s,4); v->bpm=(uint16_t)get(&s,2);
        v->kind=(uint8_t)get(&s,1); v->acknowledged=(uint8_t)get(&s,1); v->resolved=(uint8_t)get(&s,1);
    }
    for(size_t n=0;n<next.day_count;n++) {
        next.days[n].day=get(&s,4); next.days[n].steps=get(&s,4); next.days[n].goal=get(&s,4);
    }
    if(s.failed || s.offset!=s.length || !watch_model_valid(&next)) return WATCH_STORE_CORRUPT;
    next.revision=m->revision+1;
    *m=next;
    return WATCH_STORE_OK;
}
