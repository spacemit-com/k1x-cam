/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _CAM_LOG_H_
#define _CAM_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#ifdef __ANDROID__
#include "utils/Log.h"

#undef LOG_TAG
#define LOG_TAG "cam module"

/*
#ifdef LOG_NDDEBUG
#undef LOG_NDDEBUG
#endif
#define LOG_NDDEBUG 0 // Define to enable LOGD
#ifdef LOG_NDEBUG
#undef LOG_NDEBUG
#endif
#define LOG_NDEBUG  0 // Define to enable LOGV
*/

#if 0
#define CLOGE(fmt, ...) ALOGE("%s(%d):" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define CLOGW(fmt, ...) ALOGW("%s(%d):" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define CLOGI(fmt, ...) ALOGI("%s(%d):" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define CLOGD(fmt, ...) ALOGD("%s(%d):" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define CLOGV(fmt, ...) ALOGV("%s(%d):" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define CLOGE(fmt, ...) ALOGE("CAM_Error: " fmt, ##__VA_ARGS__)
#define CLOGW(fmt, ...) ALOGW("CAM_Warning: " fmt, ##__VA_ARGS__)
#define CLOGI(fmt, ...) ALOGI("CAM_Info: " fmt, ##__VA_ARGS__)
#define CLOGD(fmt, ...) ALOGD("CAM_Debug: " fmt, ##__VA_ARGS__)
#define CLOGV(fmt, ...) ALOGV("CAM_Verbose: " fmt, ##__VA_ARGS__)
#endif

#else /* __Linux__ */
#include <stdio.h>

#define KMAG "\x1B[35m"
#define KRED "\x1B[31m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KWHT "\x1B[37m"

#define CLOGE(fmt, ...) printf(KRED "E: %s(%d): " #fmt KWHT "\n", __func__, __LINE__, ##__VA_ARGS__)
#define CLOGW(fmt, ...) printf(KMAG "W: %s(%d): " #fmt KWHT "\n", __func__, __LINE__, ##__VA_ARGS__)
#define CLOGI(fmt, ...) printf(KYEL "I: %s(%d): " #fmt KWHT "\n", __func__, __LINE__, ##__VA_ARGS__)
#define CLOGD(fmt, ...) printf(KBLU "D: %s(%d): " #fmt KWHT "\n", __func__, __LINE__, ##__VA_ARGS__)
#define CLOGV(fmt, ...) printf(KWHT "V: " fmt "\n", ##__VA_ARGS__)

#endif /* __ANDROID__ */

/* --- change this macro to change the logging level ---*/
#define CAM_LOG_LEVEL 2

#define CAM_LOG_ERROR   0
#define CAM_LOG_WARNING 1
#define CAM_LOG_INFO    2
#define CAM_LOG_DEBUG   3
#define CAM_LOG_VERBOSE 4

#if (CAM_LOG_LEVEL >= CAM_LOG_ERROR)
#define CLOG_ERROR(fmt, ...) CLOGE(fmt, ##__VA_ARGS__)
#else
#define CLOG_ERROR(fmt, ...) \
    do {                     \
    } while (0)
#endif /* CLOG_ERROR */

#if (CAM_LOG_LEVEL >= CAM_LOG_WARNING)
#define CLOG_WARNING(fmt, ...) CLOGW(fmt, ##__VA_ARGS__)
#else
#define CLOG_WARNING(fmt, ...) \
    do {                       \
    } while (0)
#endif /* CLOG_WARNING */

#if (CAM_LOG_LEVEL >= CAM_LOG_INFO)
#define CLOG_INFO(fmt, ...) CLOGI(fmt, ##__VA_ARGS__)
#else
#define CLOG_INFO(fmt, ...) \
    do {                    \
    } while (0)
#endif /* CLOG_INFO */

#if (CAM_LOG_LEVEL >= CAM_LOG_DEBUG)
#define CLOG_DEBUG(fmt, ...) CLOGD(fmt, ##__VA_ARGS__)
#else
#define CLOG_DEBUG(fmt, ...) \
    do {                     \
    } while (0)
#endif /* CLOG_DEBUG */

#if (CAM_LOG_LEVEL >= CAM_LOG_VERBOSE)
#define CLOG_VERBOSE(fmt, ...) CLOGV(fmt, ##__VA_ARGS__)
#else
#define CLOG_VERBOSE(fmt, ...) \
    do {                       \
    } while (0)
#endif /* CLOG_VERBOSE */

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CAM_LOG_H_ */
