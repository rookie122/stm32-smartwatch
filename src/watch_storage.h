/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WATCH_STORAGE_H
#define WATCH_STORAGE_H
#include "watch_model.h"
typedef enum { WATCH_STORE_OK, WATCH_STORE_MISSING, WATCH_STORE_CORRUPT, WATCH_STORE_IO } watch_store_result;
watch_store_result watch_storage_load(watch_model *m, const char *path);
watch_store_result watch_storage_save(const watch_model *m, const char *path);
#endif
