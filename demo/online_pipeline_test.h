/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _ONLINE_PIPELINE_TEST_H_
#define _ONLINE_PIPELINE_TEST_H_

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

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
