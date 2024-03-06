#ifndef __TUNING_SERVE_H
#define __TUNING_SERVE_H

#include "spm_cam_tuning_assistant.h"

enum tuning_objs_index {
    TUNING_OBJS_ISP0 = 0,
    TUNING_OBJS_ISP1,
    TUNING_OBJS_CPP0,
    TUNING_OBJS_CPP1,
    TUNING_OBJS_MAX,
};

struct tuning_objs_config {
    char objs_is_enabled[TUNING_OBJS_MAX];
    char objs_rawdump_is_enabled[TUNING_OBJS_MAX];
    int32_t (*StartDumpRaw)(TUNING_MODULE_TYPE_E type, uint32_t groupId, uint32_t frameCount, TUNING_BUFFER_S *frames);
    int32_t (*EndDumpRaw)(TUNING_MODULE_TYPE_E type, uint32_t groupId);
};

int tuning_server_init(struct tuning_objs_config cfg);
void tuning_server_deinit();
#endif
