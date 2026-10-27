/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 *
 * SmartSens SC485SL linear MIPI driver.
 */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "spm_comm_cam.h"
#include "cam_sensor.h"
#include "../module/sc485sl_spm/sc485sl_spm.h"

#define SENSOR_MAGIC 0x4855L
#define SC485SL_NAME "sc485sl"
#define SC485SL_VTS_ADJUST 8
#define SC485SL_VTS_MAX 0x3ffff0
#define SC485SL_EXPOSURE_MIN 1

#define SC485SL_VTS_H 0x326d
#define SC485SL_VTS_M 0x320e
#define SC485SL_VTS_L 0x320f
#define SC485SL_EXP_HH 0x3e20
#define SC485SL_EXP_H 0x3e00
#define SC485SL_EXP_M 0x3e01
#define SC485SL_EXP_L 0x3e02
#define SC485SL_AGAIN 0x3e08
#define SC485SL_AGAIN_FINE 0x3e09
#define SC485SL_GROUP_HOLD 0x3812

#define SC485SL_AGAIN_MIN_Q8 0x100

enum {
    SC485SL_INFO_EXP_L,
    SC485SL_INFO_EXP_M,
    SC485SL_INFO_EXP_H,
    SC485SL_INFO_EXP_HH,
    SC485SL_INFO_AGAIN,
    SC485SL_INFO_AGAIN_FINE,
    SC485SL_INFO_VTS_L,
    SC485SL_INFO_VTS_M,
    SC485SL_INFO_VTS_H,
    SC485SL_INFO_REG_NUM,
};

typedef struct {
    uint32_t gain_q8;
    uint8_t coarse;
    uint8_t fine_max;
} SC485SL_GAIN_MAP_S;

static const SC485SL_GAIN_MAP_S sc485sl_again_map[] = {
    {0x0100, 0x00, 0x3f}, /* 1.000x ~ 1.969x */
    {0x0200, 0x01, 0x36}, /* 2.000x ~ 3.375x */
    {0x0369, 0x80, 0x3f}, /* 3.410x ~ 6.713x */
    {0x06d2, 0x81, 0x3f}, /* 6.820x ~ 13.427x */
    {0x0da4, 0x83, 0x3f}, /* 13.640x ~ 26.854x */
    {0x1b48, 0x87, 0x3f}, /* 27.280x ~ 53.708x */
    {0x3690, 0x8f, 0x3f}, /* 54.560x ~ 107.415x */
};

#define SC485SL_AGAIN_MAX_Q8 \
    (sc485sl_again_map[ARRAY_SIZE(sc485sl_again_map) - 1].gain_q8 * 0x3f / 0x20)

static const unsigned int sc485sl_reg_addr_byte = I2C_16BIT;
static const unsigned int sc485sl_reg_data_byte = I2C_8BIT;
static uint8_t sc485sl_vts_h_base;

static struct regval_tab sc485sl_stream_on_regs[] = {
    {0x302c, 0x00},
    {0x36e9, 0x24},
    {0x37f9, 0x24},
    {0x0100, 0x01},
};

static struct regval_tab sc485sl_stream_off_regs[] = {
    {0x0100, 0x00},
    {0x36e9, 0xa4},
    {0x37f9, 0xa4},
    {0x302c, 0x0f},
};

static struct regval_tab sc485sl_soft_reset_regs[] = {
    {0x0103, 0x01},
};

static uint8_t sc485sl_vts_h_data(uint32_t vts)
{
    return (sc485sl_vts_h_base & ~0x3f) | ((vts >> 16) & 0x3f);
}

static int sc485sl_write_register(void* handle, uint16_t regAddr, uint16_t value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_i2c_data reg_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    reg_data.addr = sensor_context->i2c_addr;
    reg_data.reg_len = sc485sl_reg_addr_byte;
    reg_data.val_len = sc485sl_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = value;
    ret = sensor_write_register(sensor_context->devId, &reg_data);
    return ret;
}

static int sc485sl_read_register(void* handle, uint16_t regAddr, uint16_t* value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_i2c_data reg_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    reg_data.addr = sensor_context->i2c_addr;
    reg_data.reg_len = sc485sl_reg_addr_byte;
    reg_data.val_len = sc485sl_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = 0;
    ret = sensor_read_register(sensor_context->devId, &reg_data);
    if (!ret) {
        *value = reg_data.tab.val;
    }

    return ret;
}

static int sc485sl_write_burst_register(void* handle, struct regval_tab* reg_table, int reg_table_num)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(reg_table);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_table_data.addr = sensor_context->i2c_addr;
    reg_table_data.reg_len = sc485sl_reg_addr_byte;
    reg_table_data.val_len = sc485sl_reg_data_byte;
    reg_table_data.tab = reg_table;
    reg_table_data.num = reg_table_num;
    ret = sensor_write_burst_register(sensor_context->devId, &reg_table_data);
    return ret;
}

static int sc485sl_sensor_write_reg(void* snsHandle, uint32_t regAddr, uint32_t value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);
    pthread_mutex_lock(&sensor_context->apiLock);
    ret = sc485sl_write_register(sensor_context, regAddr, value);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int sc485sl_sensor_group_reg_start(void* snsHandle)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = sc485sl_write_register(sensor_context, SC485SL_GROUP_HOLD, 0x00);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int sc485sl_sensor_group_reg_done(void* snsHandle)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = sc485sl_write_register(sensor_context, SC485SL_GROUP_HOLD, 0x10);
    if (!ret)
        ret = sc485sl_write_register(sensor_context, SC485SL_GROUP_HOLD, 0xa0);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int sc485sl_sensor_get_isp_default(void* snsHandle, uint32_t u32ChanelId, uint32_t camScene,
                                           ISP_SENSOR_DEFAULT_S* pstDef)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstDef);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    memset(pstDef, 0, sizeof(ISP_SENSOR_DEFAULT_S));
    if (camScene >= CAM_ISP_SCENE_INVALID) {
        CLOG_WARNING("%s: invalid camera scene: %u", __FUNCTION__, camScene);
        pstDef->pstIspDefaultSetting = NULL;
    } else {
        pstDef->pstIspDefaultSetting = sensor_context->work_info.pstIspDefaultSettings[camScene];
    }
    
#if 0
    // Transfer isp tunning file to struct like this if you need.
    if (sensor_context->work_info.image_mode == SENSOR_LINEAR_MODE) {
        if (0 == u32ChanelId)
            pstDef->pstIspDefaultSetting = NULL;
        else if (1 == u32ChanelId)
            pstDef->pstIspDefaultSetting = NULL;
    } else {
        if (0 == u32ChanelId)
            pstDef->pstIspDefaultSetting = NULL;
        else if (0 == u32ChanelId)
            pstDef->pstIspDefaultSetting = NULL;
    }
#endif
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int sc485sl_sensor_get_isp_black_level(void* snsHandle, uint32_t u32ChanelId,
                                               ISP_SENSOR_BLACK_LEVEL_S* pstBlackLevel)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int i = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstBlackLevel);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    /* Don't need to update black level when iso change */
    pstBlackLevel->bUpdate = false;
    if (SENSOR_LINEAR_MODE == sensor_context->work_info.image_mode) {
        for (i = 0; i < 4; i++) {
            pstBlackLevel->sensorBlackLevel[i] = 259; /*10bit,0x40*/
            pstBlackLevel->bitDepth = 12;
        }
    }
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int sc485sl_sensor_get_reg_info(void* snsHandle, ISP_SENSOR_REGS_INFO_S* pstSensorRegsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    static const uint16_t regs[] = {
        SC485SL_EXP_L, SC485SL_EXP_M, SC485SL_EXP_H, SC485SL_EXP_HH,
        SC485SL_AGAIN, SC485SL_AGAIN_FINE,
        SC485SL_VTS_L, SC485SL_VTS_M, SC485SL_VTS_H,
    };
    uint32_t expLine;

    _Static_assert(ARRAY_SIZE(regs) == SC485SL_INFO_REG_NUM, "SC485SL register index mismatch");
    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorRegsInfo);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);
    pthread_mutex_lock(&sensor_context->apiLock);
    if (!sensor_context->syncInit) {
        sensor_context->sensorRegs[0].u8CfgDelayMax = 2;
        sensor_context->sensorRegs[0].u32RegNum = ARRAY_SIZE(regs);
        sensor_context->sensorRegs[0].stSensorComBus.s8I2cDev = sensor_context->twsi_no;
        for (unsigned int i = 0; i < ARRAY_SIZE(regs); ++i) {
            sensor_context->sensorRegs[0].astI2cData[i].bUpdate = true;
            sensor_context->sensorRegs[0].astI2cData[i].u8DelayFrmNum = 2;
            sensor_context->sensorRegs[0].astI2cData[i].u8DevAddr = sensor_context->i2c_addr;
            sensor_context->sensorRegs[0].astI2cData[i].u32AddrWidth = sc485sl_reg_addr_byte;
            sensor_context->sensorRegs[0].astI2cData[i].u32DataWidth = sc485sl_reg_data_byte;
            sensor_context->sensorRegs[0].astI2cData[i].u32RegAddr = regs[i];
        }
        if (!sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_L].u32Data &&
            !sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_M].u32Data &&
            !sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_H].u32Data &&
            !sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_HH].u32Data) {
            expLine = (uint64_t)sensor_context->work_info.exp_time[0] * 1000 /
                      sensor_context->lineTime;
            if (expLine < SC485SL_EXPOSURE_MIN)
                expLine = SC485SL_EXPOSURE_MIN;
            if (expLine > sensor_context->initVTS - SC485SL_VTS_ADJUST)
                expLine = sensor_context->initVTS - SC485SL_VTS_ADJUST;
            sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_L].u32Data = (expLine & 0x0f) << 4;
            sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_M].u32Data = (expLine >> 4) & 0xff;
            sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_H].u32Data = (expLine >> 12) & 0xff;
            sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_HH].u32Data = (expLine >> 20) & 0xff;
        }
        if (!sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_AGAIN_FINE].u32Data) {
            sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_AGAIN].u32Data = 0x00;
            sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_AGAIN_FINE].u32Data = 0x20;
        }
        sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_VTS_L].u32Data = LOW_8BITS(sensor_context->vts[0]);
        sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_VTS_M].u32Data = HIGH_8BITS(sensor_context->vts[0]);
        sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_VTS_H].u32Data = sc485sl_vts_h_data(sensor_context->vts[0]);
        sensor_context->syncInit = true;
    } else {
        for (unsigned int i = 0; i < sensor_context->sensorRegs[0].u32RegNum; ++i)
            sensor_context->sensorRegs[0].astI2cData[i].bUpdate =
                sensor_context->sensorRegs[0].astI2cData[i].u32Data != sensor_context->sensorRegs[1].astI2cData[i].u32Data;
    }
    memcpy(pstSensorRegsInfo, &sensor_context->sensorRegs[0], sizeof(*pstSensorRegsInfo));
    memcpy(&sensor_context->sensorRegs[1], &sensor_context->sensorRegs[0], sizeof(*pstSensorRegsInfo));
    sensor_context->vts[1] = sensor_context->vts[0];
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int sc485sl_sensor_dump_info(void* snsHandle)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t vts = 0, exp_time = 0;
    uint32_t again = 0, dgain = 0;
    uint16_t reg_val_h, reg_val_m, reg_val_l, reg_val_fine;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    ret = sc485sl_read_register(snsHandle, SC485SL_VTS_H, &reg_val_h);
    if (ret)
        return ret;
    ret = sc485sl_read_register(snsHandle, SC485SL_VTS_M, &reg_val_m);
    if (ret)
        return ret;
    ret = sc485sl_read_register(snsHandle, SC485SL_VTS_L, &reg_val_l);
    if (ret)
        return ret;
    vts = (((uint32_t)reg_val_h & 0x3f) << 16) |
          ((uint32_t)reg_val_m << 8) | reg_val_l;

    ret = sc485sl_read_register(snsHandle, SC485SL_EXP_HH, &reg_val_h);
    if (ret)
        return ret;
    ret = sc485sl_read_register(snsHandle, SC485SL_EXP_H, &reg_val_m);
    if (ret)
        return ret;
    ret = sc485sl_read_register(snsHandle, SC485SL_EXP_M, &reg_val_l);
    if (ret)
        return ret;
    ret = sc485sl_read_register(snsHandle, SC485SL_EXP_L, &reg_val_fine);
    if (ret)
        return ret;
    exp_time = ((uint32_t)reg_val_h << 20) |
               ((uint32_t)reg_val_m << 12) |
               ((uint32_t)reg_val_l << 4) | (reg_val_fine >> 4);

    ret = sc485sl_read_register(snsHandle, SC485SL_AGAIN, &reg_val_h);
    if (ret)
        return ret;
    again = reg_val_h;
    ret = sc485sl_read_register(snsHandle, SC485SL_AGAIN_FINE, &reg_val_fine);
    if (ret)
        return ret;
    dgain = 4096;

    pthread_mutex_lock(&sensor_context->apiLock);
    CLOG_INFO("sc485sl regs(vts=%u,exptime=%u lines,again=0x%02x/0x%02x,dgain=%u),"
              "struct(initVTS=%u,initFps=%f,vts=%u,expline=%u)",
              vts, exp_time, again, reg_val_fine, dgain, sensor_context->initVTS,
              sensor_context->initFps, sensor_context->vts[0],
              (uint32_t)((uint64_t)sensor_context->hdrIntTime[0] * 1000 /
                         sensor_context->lineTime));
    pthread_mutex_unlock(&sensor_context->apiLock);

    return ret;
}

static int sc485sl_sensor_get_ae_default(void* snsHandle, uint32_t u32ChanelId,
                                          ISP_SENSOR_AE_DEFAULT_S* pstSensorAeDft)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t exp_time = 0;
    uint32_t again = 0, dgain = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorAeDft);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    exp_time = sensor_context->work_info.exp_time[u32ChanelId];
    again = sensor_context->work_info.again[u32ChanelId];
    dgain = sensor_context->work_info.dgain[u32ChanelId];
    pstSensorAeDft->initSceneLuma = sensor_context->init_3a_attr.initSceneLuma[u32ChanelId];
    pstSensorAeDft->initSceneLux = sensor_context->init_3a_attr.initSceneLux[u32ChanelId];

    pstSensorAeDft->initExpTime = exp_time;
    pstSensorAeDft->initAnaGain = again;
    pstSensorAeDft->initDGain = dgain;
    pstSensorAeDft->initTGain = pstSensorAeDft->initAnaGain * pstSensorAeDft->initDGain / 0x1000;

    pstSensorAeDft->maxDelayCfg = 2;
    pstSensorAeDft->minDelayCfg = 2;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int sc485sl_sensor_get_expotime_by_fps(void* snsHandle, float f32Fps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t max_expotime = 0;
    uint32_t vts = 0;
    float minFps;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    minFps = (sensor_context->minVTS * sensor_context->maxFps) / SC485SL_VTS_MAX;
    if (!(f32Fps >= minFps && f32Fps <= sensor_context->maxFps)) {
        CLOG_ERROR("sc485sl: unsupported fps %f", f32Fps);
        pthread_mutex_unlock(&sensor_context->apiLock);
        return -EINVAL;
    }
    /* 模式 0 保持 180fps 常态（客户需求）：曝光上限钉在帧长 5.5ms，不按 AE 的
     * 30fps 规划拉伸 VTS——帧率模型不一致的拉伸会造成违约瞬态导致 AE 失稳。 */
    if (sensor_context->work_info.work_mode == SC485SL_SPM_1344X760_12BIT_180FPS_4LANE)
        vts = sensor_context->minVTS;
    else
        vts = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
    max_expotime = (uint64_t)(vts - SC485SL_VTS_ADJUST) * sensor_context->lineTime / 1000;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return max_expotime;
}

static int sc485sl_sensor_fps_set(void* snsHandle, float f32Fps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t lines;
    float minFps;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    minFps = (sensor_context->minVTS * sensor_context->maxFps) / SC485SL_VTS_MAX;
    if (!(f32Fps >= minFps && f32Fps <= sensor_context->maxFps)) {
        CLOG_ERROR("sc485sl: unsupported fps %f", f32Fps);
        pthread_mutex_unlock(&sensor_context->apiLock);
        return -EINVAL;
    }
    lines = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
    sensor_context->initVTS = lines;
    sensor_context->initFps = f32Fps;
    sensor_context->vts[0] = sensor_context->initVTS;
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_VTS_L].u32Data = LOW_8BITS(lines);
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_VTS_M].u32Data = HIGH_8BITS(lines);
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_VTS_H].u32Data = sc485sl_vts_h_data(lines);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int sc485sl_sensor_expotime_update(void* snsHandle, uint32_t u32ChanelId,
                                          uint32_t u32ExpoTime,
                                          ISP_SENSOR_VTS_INFO_S* pstSensorVtsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t expLine, maxExpLine, actualExpoTime, vts;
    float sensorFps;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorVtsInfo);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);
    pthread_mutex_lock(&sensor_context->apiLock);
    expLine = (uint64_t)u32ExpoTime * 1000 / sensor_context->lineTime;
    expLine = (expLine < SC485SL_EXPOSURE_MIN) ? SC485SL_EXPOSURE_MIN : expLine;
    /* 模式 0：曝光与 VTS 钳位在 180fps 帧长内，禁止拉伸（见 get_expotime_by_fps 注释）。 */
    if (sensor_context->work_info.work_mode == SC485SL_SPM_1344X760_12BIT_180FPS_4LANE) {
        maxExpLine = sensor_context->initVTS - SC485SL_VTS_ADJUST;
        expLine = (expLine > maxExpLine) ? maxExpLine : expLine;
        sensor_context->vts[0] = sensor_context->initVTS;
    } else {
        maxExpLine = SC485SL_VTS_MAX - SC485SL_VTS_ADJUST;
        expLine = (expLine > maxExpLine) ? maxExpLine : expLine;
        if (expLine > sensor_context->initVTS - SC485SL_VTS_ADJUST)
            sensor_context->vts[0] = expLine + SC485SL_VTS_ADJUST;
        else
            sensor_context->vts[0] = sensor_context->initVTS;
    }

    actualExpoTime = (uint64_t)expLine * sensor_context->lineTime / 1000;
    sensor_context->hdrIntTime[u32ChanelId] = actualExpoTime;
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_VTS_L].u32Data = LOW_8BITS(sensor_context->vts[0]);
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_VTS_M].u32Data = HIGH_8BITS(sensor_context->vts[0]);
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_VTS_H].u32Data =
        sc485sl_vts_h_data(sensor_context->vts[0]);
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_L].u32Data = (expLine & 0x0f) << 4;
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_M].u32Data = (expLine >> 4) & 0xff;
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_H].u32Data = (expLine >> 12) & 0xff;
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_EXP_HH].u32Data = (expLine >> 20) & 0xff;
    pstSensorVtsInfo->snsLineTime = sensor_context->lineTime;
    pstSensorVtsInfo->snsVts = sensor_context->vts[0];
    pstSensorVtsInfo->snsFps = sensor_context->initFps * sensor_context->initVTS / sensor_context->vts[0];
    vts = sensor_context->vts[0];
    sensorFps = pstSensorVtsInfo->snsFps;
    pthread_mutex_unlock(&sensor_context->apiLock);

    // printf("sc485sl exposure: request=%u us, applied=%u us, lines=%u, vts=%u, fps=%.2f\n",
    //        u32ExpoTime, actualExpoTime, expLine, vts, sensorFps);
    return 0;
}

static int sc485sl_sensor_gain_update(void* snsHandle, uint32_t u32ChanelId,
                                      uint32_t* pAgainVal, uint32_t* pDgainVal)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    const SC485SL_GAIN_MAP_S* gain_map = &sc485sl_again_map[0];
    uint32_t AGain_Val, DGain_Val, AGain_Reg, AGain_Reg_Fine, AGain_Val_Actual;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pAgainVal);
    SENSORS_CHECK_PARA_POINTER(pDgainVal);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    AGain_Val = *pAgainVal;
    DGain_Val = *pDgainVal;
    AGain_Val = (AGain_Val < SC485SL_AGAIN_MIN_Q8) ? SC485SL_AGAIN_MIN_Q8 : AGain_Val;
    AGain_Val = (AGain_Val > SC485SL_AGAIN_MAX_Q8) ? SC485SL_AGAIN_MAX_Q8 : AGain_Val;
    for (unsigned int i = 1; i < ARRAY_SIZE(sc485sl_again_map); ++i) {
        if (AGain_Val < sc485sl_again_map[i].gain_q8)
            break;
        gain_map = &sc485sl_again_map[i];
    }
    AGain_Reg = gain_map->coarse;
    AGain_Reg_Fine = AGain_Val * 0x20 / gain_map->gain_q8;
    AGain_Reg_Fine = (AGain_Reg_Fine < 0x20) ? 0x20 : AGain_Reg_Fine;
    AGain_Reg_Fine = (AGain_Reg_Fine > gain_map->fine_max) ? gain_map->fine_max : AGain_Reg_Fine;
    AGain_Val_Actual = gain_map->gain_q8 * AGain_Reg_Fine / 0x20;

    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_AGAIN].u32Data = AGain_Reg;
    sensor_context->sensorRegs[0].astI2cData[SC485SL_INFO_AGAIN_FINE].u32Data = AGain_Reg_Fine;
    *pAgainVal = AGain_Val_Actual;
    *pDgainVal = 4096;
    pthread_mutex_unlock(&sensor_context->apiLock);

    // printf("sc485sl gain: request=%u Q8/%u Q12, applied=%u Q8/4096 Q12, reg=0x%02x/0x%02x\n",
    //        AGain_Val, DGain_Val, AGain_Val_Actual, AGain_Reg, AGain_Reg_Fine);
    (void)u32ChanelId;
    return 0;
}

static int sc485sl_get_aelib_default_settings(void* snsHandle, uint32_t u32ChanelId,
                                               AE_LIB_DEFAULT_SETTING_S** ppstAeLibDefault)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(ppstAeLibDefault);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);
    pthread_mutex_lock(&sensor_context->apiLock);
    *ppstAeLibDefault = NULL;
    pthread_mutex_unlock(&sensor_context->apiLock);
    (void)u32ChanelId;
    return 0;
}

static int sc485sl_sensor_get_awb_default(void* snsHandle, uint32_t u32ChanelId, ISP_SENSOR_AWB_DEFAULT_S* pstSensorAwbDft)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;
    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSORS_CHECK_PARA_POINTER(pstSensorAwbDft);
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId] != 0 &&
        sensor_context->init_3a_attr.initTint[u32ChanelId] != 0) {
        pstSensorAwbDft->initCorrelationCT = sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId];
        pstSensorAwbDft->initTint = sensor_context->init_3a_attr.initTint[u32ChanelId];
    }
    pthread_mutex_unlock(&sensor_context->apiLock);

    return ret;
}

static int sc485sl_get_awblib_default_settings(void* snsHandle, uint32_t u32ChanelId,
                                                AWB_LIB_DEFAULT_SETTING_S** ppstAwbLibDefault)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(ppstAwbLibDefault);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    *ppstAwbLibDefault = NULL;
    pthread_mutex_unlock(&sensor_context->apiLock);
    (void)u32ChanelId;
    return 0;
}

static int sc485sl_power_on(SENSOR_CONTEXT_S* sensor_context)
{
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 0);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 0);
    sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1800000);
    sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_DVDD, 1200000);
    sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_AVDD, 2800000);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DVDD, 1);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_AVDD, 1);
    sensor_set_mclk_rate(sensor_context->devId, 27000000);
    sensor_set_mclk_enable(sensor_context->devId, 1);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 1);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 1);
    usleep(5000);
    return 0;
}

static int sc485sl_power_off(SENSOR_CONTEXT_S* sensor_context)
{
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 0);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 0);
    sensor_set_mclk_enable(sensor_context->devId, 0);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_AVDD, 0);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DVDD, 0);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 0);
    return 0;
}

static int sc485sl_init(void** pHandle, SENSOR_CUSTOM_S snr_custom)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_sensor_info sensor_hw_info;

    SENSORS_CHECK_PARA_POINTER(pHandle);
    sensor_context = calloc(1, sizeof(*sensor_context));
    if (!sensor_context)
        return -ENOMEM;
    sensor_context->name = SC485SL_NAME;
    sensor_context->devId = snr_custom.dev_id;
    sensor_context->i2c_addr = snr_custom.i2c_addr;
    sensor_context->magic = SENSOR_MAGIC;
    pthread_mutex_init(&sensor_context->apiLock, NULL);
    sensor_hw_init(sensor_context->devId);
    sc485sl_power_on(sensor_context);
    sensor_get_hw_info(sensor_context->devId, &sensor_hw_info);
    sensor_context->twsi_no = sensor_hw_info.twsi_no;
    *pHandle = sensor_context;
    return 0;
}

static int sc485sl_deinit(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    sensor_context->magic = 0;
    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag) {
        sc485sl_write_burst_register(sensor_context, sc485sl_stream_off_regs, ARRAY_SIZE(sc485sl_stream_off_regs));
                sensor_context->stream_on_flag = 0;
    }
    sc485sl_power_off(sensor_context);
    sensor_hw_exit(sensor_context->devId);
    pthread_mutex_unlock(&sensor_context->apiLock);

    pthread_mutex_destroy(&sensor_context->apiLock);
    if (sensor_context) {
        memset(sensor_context, 0, sizeof(SENSOR_CONTEXT_S));
    free(sensor_context);
        sensor_context = NULL;
    }
    return 0;
}

static int sc485sl_global_config(void* handle, SENSOR_WORK_INFO_S* work_info)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;
    uint16_t reg_val = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(work_info);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag == 1) {
        CLOG_ERROR("%s: sensor global config must be done before stream on", __FUNCTION__);
        ret = -EPERM;
        goto out;
    }
    memcpy(&sensor_context->work_info, work_info, sizeof(SENSOR_WORK_INFO_S));
    memset(&sensor_context->init_3a_attr, 0, sizeof(SENSOR_INIT_ATTR_S));

    sensor_context->initVTS = sensor_context->work_info.vts;
    sensor_context->initFps = sensor_context->work_info.f32maxFps;
    sensor_context->minVTS = sensor_context->work_info.vts;
    sensor_context->maxFps = sensor_context->work_info.f32maxFps;
    sensor_context->lineTime = sensor_context->work_info.linetime;
    sensor_context->vts[0] = sensor_context->initVTS;
    sensor_context->vts[1] = sensor_context->vts[0];
    memset(sensor_context->hdrIntTime, 0, sizeof(sensor_context->hdrIntTime));
    memset(sensor_context->sensorRegs, 0, 2 * sizeof(ISP_SENSOR_REGS_INFO_S));
    sensor_context->syncInit = 0;

    ret = sc485sl_write_burst_register(handle, sc485sl_soft_reset_regs,
                                       ARRAY_SIZE(sc485sl_soft_reset_regs));
    if (ret) {
        CLOG_ERROR("sensor soft reset fail");
        goto out;
    }
    usleep(5000);

    ret = sc485sl_write_burst_register(handle, sensor_context->work_info.setting_table,
                                       sensor_context->work_info.setting_table_size);
    if (ret) {
        CLOG_ERROR("sensor write reg fail");
        goto out;
    }

    /* Preserve non-VTS bits of SC485SL register 0x326d. */
    ret = sc485sl_read_register(handle, SC485SL_VTS_H, &reg_val);
    if (ret) {
        CLOG_ERROR("sensor read VTS high register fail");
        goto out;
    }
    sc485sl_vts_h_base = reg_val & ~0x3f;

    /* ISP 的寄存器同步机制只在值于 AE 更新间变化时才写硬件，首次批量写不会落地；
     * VTS 恒定不变的模式（如 30fps/VTS=4800）必须在这里直接写硬件，否则 sensor
     * 按初始化表值出流（实测停留 800 行 = 180fps，AE 按 30fps 规划会严重违约）。 */
    ret = sc485sl_write_register(handle, SC485SL_VTS_L, LOW_8BITS(work_info->vts));
    if (!ret)
        ret = sc485sl_write_register(handle, SC485SL_VTS_M, HIGH_8BITS(work_info->vts));
    if (!ret)
        ret = sc485sl_write_register(handle, SC485SL_VTS_H,
                                     (sc485sl_vts_h_base & ~0x3f) | ((work_info->vts >> 16) & 0x3f));
    if (ret) {
        CLOG_ERROR("sensor write VTS(%u) fail", work_info->vts);
        goto out;
    }

out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int sc485sl_set_param(void* handle, const SENSOR_INIT_ATTR_S* init_attr)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    memcpy(&sensor_context->init_3a_attr, init_attr, sizeof(SENSOR_INIT_ATTR_S));

    return 0;
}

static int sc485sl_stream_on(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;
    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);
    pthread_mutex_lock(&sensor_context->apiLock);
    ret = sensor_mipi_clock_set(sensor_context->devId, sensor_context->work_info.mipi_clock);
    if (!ret)
        ret = sc485sl_write_burst_register(sensor_context, sc485sl_stream_on_regs, ARRAY_SIZE(sc485sl_stream_on_regs));
    if (!ret)
        sensor_context->stream_on_flag = 1;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int sc485sl_stream_off(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;
    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);
    pthread_mutex_lock(&sensor_context->apiLock);
    ret = sc485sl_write_burst_register(sensor_context, sc485sl_stream_off_regs, ARRAY_SIZE(sc485sl_stream_off_regs));
    sensor_context->stream_on_flag = 0;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int sc485sl_get_ops(void* handle, ISP_SENSOR_REGISTER_S* pSensorFuncOps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(pSensorFuncOps);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);
    pthread_mutex_lock(&sensor_context->apiLock);
    pSensorFuncOps->snsHandle = handle;
    pSensorFuncOps->stSensorFunc.pfn_sensor_write_reg = sc485sl_sensor_write_reg;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_default = sc485sl_sensor_get_isp_default;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_black_level = sc485sl_sensor_get_isp_black_level;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_reg_info = sc485sl_sensor_get_reg_info;
    pSensorFuncOps->stSensorFunc.pfn_sensor_dump_info = sc485sl_sensor_dump_info;
    pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_start = sc485sl_sensor_group_reg_start;
    pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_done = sc485sl_sensor_group_reg_done;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_ae_default = sc485sl_sensor_get_ae_default;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_fps_set = sc485sl_sensor_fps_set;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_expotime_by_fps = sc485sl_sensor_get_expotime_by_fps;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_expotime_update = sc485sl_sensor_expotime_update;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_gain_update = sc485sl_sensor_gain_update;
    pSensorFuncOps->stSensorAeFunc.pfn_get_aelib_default_settings = sc485sl_get_aelib_default_settings;
    pSensorFuncOps->stSensorAwbFunc.pfn_sensor_get_awb_default = sc485sl_sensor_get_awb_default;
    pSensorFuncOps->stSensorAwbFunc.pfn_get_awblib_default_settings = sc485sl_get_awblib_default_settings;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int sc485sl_detect_sensor(void* handle, SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct regval_tab* vendor_id_table;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(vendor_id);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSORS_CHECK_PARA_POINTER(vendor_id->id_table);
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);
    vendor_id_table = calloc(vendor_id->id_table_size, sizeof(*vendor_id_table));
    if (!vendor_id_table)
        return -ENOMEM;
    for (unsigned int i = 0; i < vendor_id->id_table_size; ++i)
        vendor_id_table[i].reg = vendor_id->id_table[i].reg;
    reg_table_data.addr = sensor_context->i2c_addr;
    reg_table_data.reg_len = sc485sl_reg_addr_byte;
    reg_table_data.val_len = sc485sl_reg_data_byte;
    reg_table_data.tab = vendor_id_table;
    reg_table_data.num = vendor_id->id_table_size;
    ret = sensor_read_burst_register(sensor_context->devId, &reg_table_data);
    if (!ret) {
        for (unsigned int i = 0; i < vendor_id->id_table_size; ++i) {
            if (vendor_id_table[i].val != vendor_id->id_table[i].val) {
                ret = -ENODEV;
                break;
            }
        }
    }
    free(vendor_id_table);
    return ret;
}

SENSOR_OBJ_S sc485slObj = {
    .name = SC485SL_NAME,
    .pfnInit = sc485sl_init,
    .pfnDeinit = sc485sl_deinit,
    .pfnGloablConfig = sc485sl_global_config,
    .pfnSetParam = sc485sl_set_param,
    .pfnStreamOn = sc485sl_stream_on,
    .pfnStreamOff = sc485sl_stream_off,
    .pfnGetSensorOps = sc485sl_get_ops,
    .pfnDetectSns = sc485sl_detect_sensor,
    .pfnWriteReg = sc485sl_write_register,
    .pfnReadReg = sc485sl_read_register,
};
