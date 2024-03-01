/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _GST_CAM_API_H_
#define _GST_CAM_API_H_

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

struct gstConfig {
    int tuningServerEnalbe;
    int showFps;
    int autoRun;
    int testFrame;
    struct cppTestConfig cppConfig[2];
    struct isFeTestConfig ispFeConfig[2];
};

int single_pipeline_online_test(struct testConfig *config);
int dual_pipeline_online_test(struct testConfig *config);
int only_rawdump_test(struct testConfig *config);
int only_viisp_online_test(struct testConfig *config);
int only_viisp_offline_preview_test(struct testConfig *config);
int only_cpp_test(struct testConfig *config);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _ONLINE_PIPELINE_TEST_H_ */
