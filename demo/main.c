/*
 * Copyright (C) 2023 ASR Micro Limited
 * All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>

#include "dual_pipeline_capture_test.h"
#include "online_pipeline_test.h"

int main(int argc, char* argv[])
{
    int caseId = 0;
    int sensorId = 0;
    int useAux = 0;

    if (argc != 4) {
        printf("usage: sdkTest [caseId] [sensorId] [useAux]\n");
        printf("[caseId]:\n\t0: single_pipeline_online_test\n\t1: dual_pipeline_online_test\n\t2: dual_pipeline_capture_test\n");
        printf("[sensorId]: for case0 and case2, select sensorId from [0,1,2]; for case1, set to 0\n");
        printf("[useAux]: for case1, 0: rear + front, 1: rear + aux; for case0 and case2, set to 0\n");
        printf("example: sdkTest 0 0 0\n");
        return -1;
    }

    caseId = atoi(argv[1]);
    sensorId = atoi(argv[2]);
    useAux = atoi(argv[3]);
    switch (caseId) {
    case 0:
        single_pipeline_online_test(sensorId);
        break;
    case 1:
        dual_pipeline_online_test(useAux);
        break;
    case 2:
        dual_pipeline_capture_test(sensorId);
        break;
    default:
        break;
    }
    return 0;
}
