/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#include "condition.h"

#include <time.h>

void condition_init(struct condition *cond)
{
    pthread_condattr_t condattr;

    cond->done = 0;
    pthread_condattr_init(&condattr);
    pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
    pthread_cond_init(&cond->cond, &condattr);
    pthread_condattr_destroy(&condattr);
    pthread_mutex_init(&cond->lock, NULL);
}

void condition_post(struct condition *cond)
{
    pthread_mutex_lock(&cond->lock);
    cond->done = 1;
    pthread_cond_broadcast(&cond->cond);
    pthread_mutex_unlock(&cond->lock);
}

int condition_wait(struct condition *cond)
{
    int rc = 0;
    pthread_mutex_lock(&cond->lock);
    if (cond->done == 0) {
        rc = pthread_cond_wait(&cond->cond, &cond->lock);
    }
    cond->done = 0;
    pthread_mutex_unlock(&cond->lock);
    return rc;
}

int condition_timedwait(struct condition *cond, int timeout_ms)
{
    int rc = 0;
    struct timespec ts;
    unsigned long long nsec;

    if (timeout_ms <= 0)
        return condition_wait(cond);

    clock_gettime(CLOCK_MONOTONIC, &ts);
    nsec = timeout_ms * 1000000LL + ts.tv_nsec;
    ts.tv_sec = ts.tv_sec + nsec / 1000000000LL;
    ts.tv_nsec = nsec % 1000000000LL;

    pthread_mutex_lock(&cond->lock);
    if (cond->done == 0) {
        rc = pthread_cond_timedwait(&cond->cond, &cond->lock, &ts);
    }
    cond->done = 0;
    pthread_mutex_unlock(&cond->lock);

    return rc;
}

void condition_deinit(struct condition *cond)
{
    pthread_mutex_destroy(&cond->lock);
    pthread_cond_destroy(&cond->cond);
    cond->done = 0;
}
