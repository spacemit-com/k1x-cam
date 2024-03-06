/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef __CONDITION_H__
#define __CONDITION_H__

#include <pthread.h>

struct condition {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char done;
};

void condition_init(struct condition *cond);
void condition_post(struct condition *cond);
int condition_wait(struct condition *cond);
int condition_timedwait(struct condition *cond, int timeout_ms);
void condition_deinit(struct condition *cond);

#endif /* __CONDITION_H__ */
