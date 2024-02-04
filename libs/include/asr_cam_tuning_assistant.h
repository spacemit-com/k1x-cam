/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _ASR_CAM_TUNING_ASSISTANT_H_
#define _ASR_CAM_TUNING_ASSISTANT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

/* ------------------------------------------------------------------------ */

typedef void *ASR_TUNING_ASSISTANT_HANDLE;
typedef enum TUNING_MODULE_TYPE {
    TUNING_MODULE_TYPE_ISP,
    TUNING_MODULE_TYPE_CPP,
    TUNING_MODULE_TYPE_ALGO,
    TUNING_MODULE_TYPE_MAX
} TUNING_MODULE_TYPE_E;

typedef struct TUNING_BUFFER {
    uint32_t frameId;
    uint32_t length;
    void *virAddr;
} TUNING_BUFFER_S, *TUNING_BUFFER_S_PTR;

typedef struct TUNING_MODULE_OBJECT {
    TUNING_MODULE_TYPE_E type;
    void *moduleHandle;
    uint32_t groupId;
    uint8_t dumpRaw;
    char name[32];
    int (*loadSettingFile)(void *handle, const char *filename);
    int (*saveSettingFile)(void *handle, const char *filename);
    int (*saveFilterSettingFile)(void *handle, const char *filtername, const char *filename);
    int (*readFirmware) (void *handle, const char *filter, const char *param, int32_t row, int32_t colum, int32_t *pVal);
    int (*writeFirmware) (void *handle, const char *filter, const char *param, int32_t row, int32_t colum, int32_t val);
} TUNING_MODULE_OBJECT_S, *TUNING_MODULE_OBJECT_S_PTR;

typedef struct ASR_TUNING_ASSISTANT_TRIGGER {
    uint32_t (*GetModuleCount)();
    int32_t (*GetModules)(uint32_t moduleCount, TUNING_MODULE_OBJECT_S *modules);
    int32_t (*StartDumpRaw)(TUNING_MODULE_TYPE_E type, uint32_t groupId, uint32_t frameCount, TUNING_BUFFER_S *frames);
    int32_t (*EndDumpRaw)(TUNING_MODULE_TYPE_E type, uint32_t groupId);
} ASR_TUNING_ASSISTANT_TRIGGER_S, *ASR_TUNING_ASSISTANT_TRIGGER_S_PTR;

/* ------------------------------------------------------------------------ */

ASR_TUNING_ASSISTANT_HANDLE ASR_TuningAssistant_Create(const ASR_TUNING_ASSISTANT_TRIGGER_S *trigger);

bool ASR_TuningAssistant_Destroy(ASR_TUNING_ASSISTANT_HANDLE handle);

/* ------------------------------------------------------------------------ */

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _ASR_CAM_TUNING_ASSISTANT_H_ */