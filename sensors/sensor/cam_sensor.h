/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _CAM_SENSOR_H_
#define _CAM_SENSOR_H_

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "cam_sensor_uapi.h"
#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef struct SENSOR_CONTEXT {
    const char* name;
    int32_t devId;
    uint8_t i2c_addr;
    uint8_t twsi_no;
    SENSOR_WORK_INFO_S work_info;
    SENSOR_INIT_ATTR_S init_3a_attr;
    int stream_on_flag;
    int syncInit;
    uint32_t magic;
    pthread_mutex_t apiLock;

    ISP_SENSOR_REGS_INFO_S sensorRegs[2]; /* [0]: Sensor regs of cur-frame; [1]: Sensor regs of pre-frame ; */
    uint32_t vts[2];                      /* [0]: vts of cur-frame; [1]: vts of pre-frame */
    uint32_t hdrIntTime[4];
    uint32_t initVTS;
    uint32_t minVTS;
    float initFps;
    float maxFps;
    int lineTime;         /*unit ns */
    uint32_t maxShortExp; /*Max short exposure */
} SENSOR_CONTEXT_S;

int sensor_hw_init(int sns_id);
int sensor_hw_exit(int sns_id);
int sensor_hw_unreset(int sns_id);
int sensor_hw_reset(int sns_id);
int sensor_write_register(int sns_id, struct cam_i2c_data* data);
int sensor_read_register(int sns_id, struct cam_i2c_data* data);
int sensor_write_burst_register(int sns_id, struct cam_burst_i2c_data* data);
int sensor_read_burst_register(int sns_id, struct cam_burst_i2c_data* data);
int sensor_get_hw_info(int sns_id, struct cam_sensor_info* info);
int sensor_mipi_clock_set(unsigned int sns_id, unsigned int mipi_clock);
int sensor_set_power_voltage(int sns_id, uint8_t regulator_id, uint32_t voltage);
int sensor_set_power_on(int sns_id, uint8_t regulator_id, uint8_t on);
int sensor_set_gpio_enable(int sns_id, uint8_t gpio_id, uint8_t enable);
int sensor_set_mclk_rate(int sns_id, uint32_t clk_rate);
int sensor_set_mclk_enable(int sns_id, uint32_t clk_enable);

#define SENSOR_CHECK_HANDLE_IS_ERR(ptr)                                                       \
    do {                                                                                      \
        if ((ptr)->magic != SENSOR_MAGIC) {                                                   \
            CLOG_ERROR("%s: sensor context magic 0x%08x is err", __FUNCTION__, (ptr)->magic); \
            return -ENXIO;                                                                    \
        }                                                                                     \
    } while (0)

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif