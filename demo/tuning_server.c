#include <string.h>
#include <stdio.h>

#include "asr_cam_tuning_assistant.h"
#include "tuning_server.h"

struct tuning_obj_set {
    TUNING_MODULE_OBJECT_S objs[TUNING_OBJS_MAX];
    struct tuning_objs_config config;
    int enabled_cnt;
};

static struct tuning_obj_set tuning_set;
static ASR_TUNING_ASSISTANT_HANDLE tuningHandle;

void _init_tuning_objs(struct tuning_objs_config cfg)
{
    int i;

    memset(&tuning_set, 0, sizeof(tuning_set));

    for (i = TUNING_OBJS_ISP0; i <= TUNING_OBJS_CPP1; i++) {
        if (cfg.objs_is_enabled[i]) {
            if (i >= TUNING_OBJS_ISP0 && i <= TUNING_OBJS_ISP1) {
                tuning_set.objs[i].type = TUNING_MODULE_TYPE_ISP;
                tuning_set.objs[i].groupId = i - TUNING_OBJS_ISP0;
                if (cfg.objs_rawdump_is_enabled[i])
                    tuning_set.objs[i].dumpRaw = 1;
                else
                    tuning_set.objs[i].dumpRaw = 0;
                snprintf(tuning_set.objs[i].name, sizeof(tuning_set.objs[i].name), "fe_pipe%d", tuning_set.objs[i].groupId);
            } else if (i >= TUNING_OBJS_CPP0 && i <= TUNING_OBJS_CPP1) {
                tuning_set.objs[i].type = TUNING_MODULE_TYPE_CPP;
                tuning_set.objs[i].groupId = i - TUNING_OBJS_CPP0;
                tuning_set.objs[i].dumpRaw = 0;
                snprintf(tuning_set.objs[i].name, sizeof(tuning_set.objs[i].name), "cpp%d", tuning_set.objs[i].groupId);
            }
            tuning_set.enabled_cnt++;
            tuning_set.config.objs_is_enabled[i] = 1;
        }
    }
}

static uint32_t ispGetModuleCount()
{
    return tuning_set.enabled_cnt;
}

static int32_t ispGetModules(uint32_t moduleCount, TUNING_MODULE_OBJECT_S *modules)
{
    uint32_t i, j;

    if (!modules)
        return 0;

    for (i = 0, j = 0; i < moduleCount && j < TUNING_OBJS_MAX; i++) {
        while (tuning_set.config.objs_is_enabled[j] != 1 && j < TUNING_OBJS_MAX)
            j++;
        if (j == TUNING_OBJS_MAX)
            break;
        modules[i] = tuning_set.objs[j];
        j++;
    }
    return i;
}

int tuning_server_init(struct tuning_objs_config cfg) {
    ASR_TUNING_ASSISTANT_TRIGGER_S trigger = {0};

    _init_tuning_objs(cfg);

    trigger.StartDumpRaw = cfg.StartDumpRaw;
    trigger.EndDumpRaw = cfg.EndDumpRaw;
    trigger.GetModuleCount = ispGetModuleCount;
    trigger.GetModules = ispGetModules;
    tuningHandle = ASR_TuningAssistant_Create(&trigger);
    if (!tuningHandle) {
        printf("ASR_TuningAssistant_Create failed\n");
        return -1;
    }
    return 0;
}

void tuning_server_deinit() {
    if (tuningHandle) {
        ASR_TuningAssistant_Destroy(tuningHandle);
        tuningHandle = NULL;
    }
}
