/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "spm_comm_cam.h"
#include "cam_sensor.h"

#ifdef __OHOS__
#define LOG_DOMAIN 0xD002B00
#define LOG_TAG "CamSensor"
#include "hilog/log.h"
#endif

#define SENSOR_MAGIC 0x885810B5
#define OV8858_NAME "ov8858"
static const unsigned int ov8858_reg_addr_byte = I2C_16BIT;
static const unsigned int ov8858_reg_data_byte = I2C_8BIT;

static struct regval_tab stream_on_regs[] = {
    {0x0100, 0x01},
};

static struct regval_tab stream_off_regs[] = {
    {0x0100, 0x00},
};

static struct regval_tab color_bar_regs[] = {
    {0x5e00, 0x80},
};

#define OV8858_VTS_ADJUST     (4)
#define OV8858_VTS_LINES_MAX  (0x7fff)
#define OV8858_EXPO_LINES_MIN (4)
#define OV8858_AGAIN_Q8_ONE_X  (256U)
#define OV8858_AGAIN_REG_ONE_X (0x0080U)
#define OV8858_AGAIN_REG_MIN   (0x0080U)
#define OV8858_AGAIN_REG_MAX   (0x07ffU)

#define OV8858_VTS_ADDR_H (0x380E)
#define OV8858_VTS_ADDR_L (0x380F)
#define OV8858_EXPO_H     (0x3500)
#define OV8858_EXPO_M     (0x3501)
#define OV8858_EXPO_L     (0x3502)
#define OV8858_AGAIN_H    (0x3508)
#define OV8858_AGAIN_L    (0x3509)
#define OV8858_GROUP_ACCESS (0x3208)
#define OV8858_GROUP_MANUAL_CTRL (0x320B)

#define OV8858_REG_CHIP_ID (0x300a)
#define OV8858_REG_REVISION (0x302a)
#define OV8858_CHIP_ID      (0x008858)

/* reg info order: expo[3], again[2], vts[2] */
typedef enum {
    OV8858_INFO_EXP_L = 0,
    OV8858_INFO_EXP_M,
    OV8858_INFO_EXP_H,
    OV8858_INFO_AGAIN_L,
    OV8858_INFO_AGAIN_H,
    OV8858_INFO_VTS_L,
    OV8858_INFO_VTS_H,
    OV8858_INFO_MAX
} OV8858_SENSOR_INFO_E;

static int ov8858_write_register(void* handle, uint16_t regAddr, uint16_t value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_i2c_data reg_data;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_data.addr = sensor_context->i2c_addr;
    reg_data.reg_len = ov8858_reg_addr_byte;
    reg_data.val_len = ov8858_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = value;
    return sensor_write_register(sensor_context->devId, &reg_data);
}

static int ov8858_read_register(void* handle, uint16_t regAddr, uint16_t* value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_i2c_data reg_data;
    int ret;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(value);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_data.addr = sensor_context->i2c_addr;
    reg_data.reg_len = ov8858_reg_addr_byte;
    reg_data.val_len = ov8858_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = 0;
    ret = sensor_read_register(sensor_context->devId, &reg_data);
    if (!ret)
        *value = reg_data.tab.val;

    return ret;
}

static int ov8858_write_burst_register(void* handle, struct regval_tab* reg_table, int reg_table_num)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;
    int i;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(reg_table);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_table_data.addr = sensor_context->i2c_addr;
    reg_table_data.reg_len = ov8858_reg_addr_byte;
    reg_table_data.val_len = ov8858_reg_data_byte;
    reg_table_data.tab = reg_table;
    reg_table_data.num = reg_table_num;
    ret = sensor_write_burst_register(sensor_context->devId, &reg_table_data);
    if (ret == 0) {
        for (i = 0; i < reg_table_num; i++) {
            if (reg_table[i].reg == 0xffff)
                break;
        }
    }
    return ret;
}

static int ov8858_sensor_write_reg(void* snsHandle, uint32_t regAddr, uint32_t value)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = ov8858_write_register(snsHandle, regAddr, value);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int ov8858_sensor_group_reg_start(void* snsHandle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = ov8858_write_register(snsHandle, OV8858_GROUP_ACCESS, 0x00);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int ov8858_sensor_group_reg_done(void* snsHandle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    /*
     * Databook 2.11.2 Mode 1 (quick manual launch): "Manual launch is enabled by
     * setting the register 0x320B to 0", then 0x3208 = 0xE0 launches group 0.
     * 0x320B defaults to 0x01 (auto switch), so it must be cleared first.
     */
    ret = ov8858_write_register(snsHandle, OV8858_GROUP_MANUAL_CTRL, 0x00);
    if (ret == 0)
        ret = ov8858_write_register(snsHandle, OV8858_GROUP_ACCESS, 0x10);
    if (ret == 0)
        ret = ov8858_write_register(snsHandle, OV8858_GROUP_ACCESS, 0xe0);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int ov8858_sensor_get_isp_default(void* snsHandle, uint32_t u32ChanelId, uint32_t camScene,
                                         ISP_SENSOR_DEFAULT_S* pstDef)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstDef);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    memset(pstDef, 0, sizeof(ISP_SENSOR_DEFAULT_S));
    if (camScene < CAM_ISP_SCENE_INVALID)
        pstDef->pstIspDefaultSetting = sensor_context->work_info.pstIspDefaultSettings[camScene];
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int ov8858_sensor_get_isp_black_level(void* snsHandle, uint32_t u32ChanelId,
                                             ISP_SENSOR_BLACK_LEVEL_S* pstBlackLevel)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int i;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstBlackLevel);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    pstBlackLevel->bUpdate = false;
    for (i = 0; i < 4; i++)
        pstBlackLevel->sensorBlackLevel[i] = 256;
    pstBlackLevel->bitDepth = 10;
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int ov8858_sensor_get_reg_info(void* snsHandle, ISP_SENSOR_REGS_INFO_S* pstSensorRegsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t i;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorRegsInfo);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (false == sensor_context->syncInit) {
        sensor_context->sensorRegs[0].u8CfgDelayMax = 1;
        sensor_context->sensorRegs[0].u32RegNum = OV8858_INFO_MAX;
        sensor_context->sensorRegs[0].stSensorComBus.s8I2cDev = sensor_context->twsi_no;
        for (i = 0; i < sensor_context->sensorRegs[0].u32RegNum; i++) {
            sensor_context->sensorRegs[0].astI2cData[i].bUpdate = true;
            sensor_context->sensorRegs[0].astI2cData[i].u8DevAddr = sensor_context->i2c_addr;
            sensor_context->sensorRegs[0].astI2cData[i].u32AddrWidth = ov8858_reg_addr_byte;
            sensor_context->sensorRegs[0].astI2cData[i].u32DataWidth = ov8858_reg_data_byte;
        }
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_EXP_L].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_EXP_L].u32RegAddr = OV8858_EXPO_L;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_EXP_M].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_EXP_M].u32RegAddr = OV8858_EXPO_M;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_EXP_H].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_EXP_H].u32RegAddr = OV8858_EXPO_H;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_AGAIN_L].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_AGAIN_L].u32RegAddr = OV8858_AGAIN_L;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_AGAIN_H].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_AGAIN_H].u32RegAddr = OV8858_AGAIN_H;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_VTS_L].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_VTS_L].u32RegAddr = OV8858_VTS_ADDR_L;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_VTS_H].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_VTS_H].u32RegAddr = OV8858_VTS_ADDR_H;
        sensor_context->syncInit = true;
    } else {
        for (i = 0; i < sensor_context->sensorRegs[0].u32RegNum; i++) {
            sensor_context->sensorRegs[0].astI2cData[i].bUpdate =
                (sensor_context->sensorRegs[0].astI2cData[i].u32Data != sensor_context->sensorRegs[1].astI2cData[i].u32Data);
        }
    }

    memcpy(pstSensorRegsInfo, &sensor_context->sensorRegs[0], sizeof(ISP_SENSOR_REGS_INFO_S));
    memcpy(&sensor_context->sensorRegs[1], &sensor_context->sensorRegs[0], sizeof(ISP_SENSOR_REGS_INFO_S));
    sensor_context->vts[1] = sensor_context->vts[0];
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int ov8858_sensor_dump_info(void* snsHandle)
{
    return 0;
}

static int ov8858_sensor_get_ae_default(void* snsHandle, uint32_t u32ChanelId, ISP_SENSOR_AE_DEFAULT_S* pstSensorAeDft)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorAeDft);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    pstSensorAeDft->initSceneLuma = sensor_context->init_3a_attr.initSceneLuma[u32ChanelId];
    pstSensorAeDft->initSceneLux = sensor_context->init_3a_attr.initSceneLux[u32ChanelId];
    pstSensorAeDft->initExpTime = sensor_context->work_info.exp_time[u32ChanelId];
    pstSensorAeDft->initAnaGain = sensor_context->work_info.again[u32ChanelId];
    pstSensorAeDft->initDGain = sensor_context->work_info.dgain[u32ChanelId];
    pstSensorAeDft->initTGain = pstSensorAeDft->initAnaGain * pstSensorAeDft->initDGain / 0x1000;
    pstSensorAeDft->maxDelayCfg = 1;
    pstSensorAeDft->minDelayCfg = 1;
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int ov8858_sensor_get_expotime_by_fps(void* snsHandle, float f32Fps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t vts;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (f32Fps <= 0.0f || f32Fps > sensor_context->maxFps) {
        pthread_mutex_unlock(&sensor_context->apiLock);
        return -EINVAL;
    }
    vts = (uint32_t)((sensor_context->minVTS * sensor_context->maxFps) / f32Fps);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return (vts - OV8858_VTS_ADJUST) * sensor_context->lineTime / 1000;
}

static int ov8858_sensor_fps_set(void* snsHandle, float f32Fps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t lines;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (f32Fps <= 0.0f || f32Fps > sensor_context->maxFps) {
        pthread_mutex_unlock(&sensor_context->apiLock);
        return -EINVAL;
    }
    lines = (uint32_t)((sensor_context->minVTS * sensor_context->maxFps) / f32Fps);
    sensor_context->initVTS = lines;
    sensor_context->initFps = f32Fps;
    sensor_context->vts[0] = lines;
    sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_VTS_L].u32Data = LOW_8BITS(lines);
    sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_VTS_H].u32Data = HIGH_8BITS(lines);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int ov8858_sensor_expotime_update(void* snsHandle, uint32_t u32ChanelId, uint32_t u32ExpoTime,
                                         ISP_SENSOR_VTS_INFO_S* pstSensorVtsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t expLine;
    uint32_t reg_expo;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorVtsInfo);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    expLine = (uint32_t)((uint64_t)u32ExpoTime * 1000U / sensor_context->lineTime);
    expLine = (expLine < OV8858_EXPO_LINES_MIN) ? OV8858_EXPO_LINES_MIN : expLine;
    expLine = (expLine > (OV8858_VTS_LINES_MAX - OV8858_VTS_ADJUST)) ?
              (OV8858_VTS_LINES_MAX - OV8858_VTS_ADJUST) : expLine;
    sensor_context->hdrIntTime[u32ChanelId] = expLine * sensor_context->lineTime / 1000;
    if (expLine > (sensor_context->initVTS - OV8858_VTS_ADJUST))
        sensor_context->vts[0] = expLine + OV8858_VTS_ADJUST;
    else
        sensor_context->vts[0] = sensor_context->initVTS;

    sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_VTS_L].u32Data = LOW_8BITS(sensor_context->vts[0]);
    sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_VTS_H].u32Data = HIGH_8BITS(sensor_context->vts[0]);
    reg_expo = expLine << 4;
    sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_EXP_L].u32Data = reg_expo & 0xFF;
    sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_EXP_M].u32Data = (reg_expo >> 8) & 0xFF;
    sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_EXP_H].u32Data = (reg_expo >> 16) & 0x0F;

    pstSensorVtsInfo->snsLineTime = sensor_context->lineTime;
    pstSensorVtsInfo->snsVts = sensor_context->vts[0];
    pstSensorVtsInfo->snsFps = sensor_context->initFps * sensor_context->initVTS / sensor_context->vts[0];
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int ov8858_sensor_gain_update(void* snsHandle, uint32_t u32ChanelId, uint32_t* pAgainVal, uint32_t* pDgainVal)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t again_reg;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pAgainVal);
    SENSORS_CHECK_PARA_POINTER(pDgainVal);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    /* AE gain uses Q8 (256 = 1x), while OV8858 gain registers use Q7 (0x80 = 1x). */
    again_reg = (uint32_t)((uint64_t)(*pAgainVal) * OV8858_AGAIN_REG_ONE_X / OV8858_AGAIN_Q8_ONE_X);
    if (again_reg < OV8858_AGAIN_REG_MIN)
        again_reg = OV8858_AGAIN_REG_MIN;
    else if (again_reg > OV8858_AGAIN_REG_MAX)
        again_reg = OV8858_AGAIN_REG_MAX;
    sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_AGAIN_L].u32Data = again_reg & 0xFF;
    sensor_context->sensorRegs[0].astI2cData[OV8858_INFO_AGAIN_H].u32Data = (again_reg >> 8) & 0x07;
    *pAgainVal = again_reg * OV8858_AGAIN_Q8_ONE_X / OV8858_AGAIN_REG_ONE_X;
    *pDgainVal = 4096;
#ifdef __OHOS__
    HILOG_INFO(LOG_CORE,
                "ov8858 pAgainVal=%{public}u again_reg=0x%{public}03x",
                *pAgainVal, again_reg);
#else
    CLOG_INFO("ov8858 pAgainVal=%u again_reg=0x%03x", *pAgainVal, again_reg);
#endif
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int ov8858_get_aelib_default_settings(void* snsHandle, uint32_t u32ChanelId,
                                             AE_LIB_DEFAULT_SETTING_S** ppstAeLibDefault)
{
    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(ppstAeLibDefault);
    *ppstAeLibDefault = NULL;
    return 0;
}

static int ov8858_sensor_get_awb_default(void* snsHandle, uint32_t u32ChanelId,
                                         ISP_SENSOR_AWB_DEFAULT_S* pstSensorAwbDft)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorAwbDft);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId] != 0 &&
        sensor_context->init_3a_attr.initTint[u32ChanelId] != 0) {
        pstSensorAwbDft->initCorrelationCT = sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId];
        pstSensorAwbDft->initTint = sensor_context->init_3a_attr.initTint[u32ChanelId];
    }
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int ov8858_get_awblib_default_settings(void* snsHandle, uint32_t u32ChanelId,
                                              AWB_LIB_DEFAULT_SETTING_S** ppstAwbLibDefault)
{
    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(ppstAwbLibDefault);
    *ppstAwbLibDefault = NULL;
    return 0;
}

static int ov8858_power_on(SENSOR_CONTEXT_S* sensor_context)
{
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(sensor_context);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 0);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 0);

    sensor_set_mclk_enable(sensor_context->devId, 1);
    sensor_set_mclk_rate(sensor_context->devId, 24000000);

    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_DVDDEN, 1);




    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 1);
    usleep(10000);
    // sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 1);
    // usleep(8192);


    CLOG_INFO("finish power on %d", ret);
    return ret;
}

static int ov8858_init(void** pHandle, SENSOR_CUSTOM_S snr_custom)
{
    SENSOR_CONTEXT_S* sensor_context;
    struct cam_sensor_info sensor_hw_info;

    SENSORS_CHECK_PARA_POINTER(pHandle);
    sensor_context = (SENSOR_CONTEXT_S*)calloc(1, sizeof(SENSOR_CONTEXT_S));
    if (!sensor_context) {
        CLOG_ERROR("%s: sensor_context malloc memory failed!", __FUNCTION__);
        return -ENOMEM;
    }

    sensor_context->name = OV8858_NAME;
    sensor_context->devId = snr_custom.dev_id;
    sensor_context->i2c_addr = snr_custom.i2c_addr;
    sensor_context->magic = SENSOR_MAGIC;
    pthread_mutex_init(&sensor_context->apiLock, NULL);
    sensor_hw_init(sensor_context->devId);
    //ov8858_power_on(sensor_context);
    sensor_hw_unreset(sensor_context->devId);
    sensor_get_hw_info(sensor_context->devId, &sensor_hw_info);
    sensor_context->twsi_no = sensor_hw_info.twsi_no;

    *pHandle = sensor_context;
    return 0;
}


static int ov8858_power_off(SENSOR_CONTEXT_S* sensor_context)
{
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(sensor_context);
    sensor_set_mclk_enable(sensor_context->devId, 0);

    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 1);


    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 1);

    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_DVDDEN, 0);

    CLOG_INFO("finish power off %d", ret);
    return ret;
}

static int ov8858_deinit(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    sensor_context->magic = 0;
    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag == 1) {
        ov8858_write_burst_register(handle, stream_off_regs, ARRAY_SIZE(stream_off_regs));
        sensor_context->stream_on_flag = 0;
    }
    ov8858_power_off(sensor_context);
//     sensor_hw_reset(sensor_context->devId);
    sensor_hw_exit(sensor_context->devId);
    pthread_mutex_unlock(&sensor_context->apiLock);

    pthread_mutex_destroy(&sensor_context->apiLock);
    memset(sensor_context, 0, sizeof(SENSOR_CONTEXT_S));
    free(sensor_context);
    return 0;
}

static int ov8858_global_config(void* handle, SENSOR_WORK_INFO_S* work_info)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(work_info);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag == 1) {
        ret = -EPERM;
        goto out;
    }
    memcpy(&sensor_context->work_info, work_info, sizeof(SENSOR_WORK_INFO_S));
    memset(&sensor_context->init_3a_attr, 0x00, sizeof(SENSOR_INIT_ATTR_S));

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

    ret = ov8858_write_burst_register(handle, sensor_context->work_info.setting_table,
                                      sensor_context->work_info.setting_table_size);
    if (ret == 0 && work_info->test_pattern_mode == CC_SENSOR_TEST_PATTERN_COLOR_BARS)
        ret = ov8858_write_burst_register(handle, color_bar_regs, ARRAY_SIZE(color_bar_regs));

out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int ov8858_set_param(void* handle, const SENSOR_INIT_ATTR_S* init_attr)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(init_attr);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    memcpy(&sensor_context->init_3a_attr, init_attr, sizeof(SENSOR_INIT_ATTR_S));
    return 0;
}

static int ov8858_stream_on(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t i;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = sensor_mipi_clock_set(sensor_context->devId, sensor_context->work_info.mipi_clock);
    if (ret)
        goto out;
    for (i = 0; i < sensor_context->sensorRegs[0].u32RegNum; i++)
        ov8858_write_register(handle, sensor_context->sensorRegs[0].astI2cData[i].u32RegAddr,
                              sensor_context->sensorRegs[0].astI2cData[i].u32Data);
    ret = ov8858_write_burst_register(handle, stream_on_regs, ARRAY_SIZE(stream_on_regs));
    sensor_context->stream_on_flag = 1;
out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int ov8858_stream_off(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = ov8858_write_burst_register(handle, stream_off_regs, ARRAY_SIZE(stream_off_regs));
    sensor_context->stream_on_flag = 0;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int ov8858_get_ops(void* handle, ISP_SENSOR_REGISTER_S* pSensorFuncOps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(pSensorFuncOps);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    pSensorFuncOps->snsHandle = handle;
    pSensorFuncOps->stSensorFunc.pfn_sensor_write_reg = ov8858_sensor_write_reg;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_default = ov8858_sensor_get_isp_default;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_black_level = ov8858_sensor_get_isp_black_level;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_reg_info = ov8858_sensor_get_reg_info;
    pSensorFuncOps->stSensorFunc.pfn_sensor_dump_info = ov8858_sensor_dump_info;
    pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_start = ov8858_sensor_group_reg_start;
    pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_done = ov8858_sensor_group_reg_done;

    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_ae_default = ov8858_sensor_get_ae_default;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_fps_set = ov8858_sensor_fps_set;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_expotime_by_fps = ov8858_sensor_get_expotime_by_fps;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_expotime_update = ov8858_sensor_expotime_update;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_gain_update = ov8858_sensor_gain_update;
    pSensorFuncOps->stSensorAeFunc.pfn_get_aelib_default_settings = ov8858_get_aelib_default_settings;

    pSensorFuncOps->stSensorAwbFunc.pfn_sensor_get_awb_default = ov8858_sensor_get_awb_default;
    pSensorFuncOps->stSensorAwbFunc.pfn_get_awblib_default_settings = ov8858_get_awblib_default_settings;
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int ov8858_detect_sensor(void* handle, SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct regval_tab* vendor_id_table = NULL;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;
    int i;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(vendor_id);
    SENSORS_CHECK_PARA_POINTER(vendor_id->id_table);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    vendor_id_table = (struct regval_tab*)calloc(vendor_id->id_table_size, sizeof(struct regval_tab));
    if (!vendor_id_table) {
        pthread_mutex_unlock(&sensor_context->apiLock);
        return -ENOMEM;
    }
    for (i = 0; i < vendor_id->id_table_size; i++) {
        vendor_id_table[i].reg = vendor_id->id_table[i].reg;
        vendor_id_table[i].val = 0;
    }

    reg_table_data.addr = sensor_context->i2c_addr;
    reg_table_data.reg_len = ov8858_reg_addr_byte;
    reg_table_data.val_len = ov8858_reg_data_byte;
    reg_table_data.tab = vendor_id_table;
    reg_table_data.num = vendor_id->id_table_size;
    ret = sensor_read_burst_register(sensor_context->devId, &reg_table_data);
    if (ret == 0) {
        for (i = 0; i < vendor_id->id_table_size; i++) {
            if ((vendor_id_table[i].reg != vendor_id->id_table[i].reg) ||
                (vendor_id_table[i].val != vendor_id->id_table[i].val)) {
                ret = -1;
                break;
            }
        }
    }

    pthread_mutex_unlock(&sensor_context->apiLock);
    free(vendor_id_table);
    return ret;
}

SENSOR_OBJ_S ov8858Obj = {
    .name = OV8858_NAME,
    .pfnInit = ov8858_init,
    .pfnDeinit = ov8858_deinit,
    .pfnGloablConfig = ov8858_global_config,
    .pfnSetParam = ov8858_set_param,
    .pfnStreamOn = ov8858_stream_on,
    .pfnStreamOff = ov8858_stream_off,
    .pfnGetSensorOps = ov8858_get_ops,
    .pfnDetectSns = ov8858_detect_sensor,
    .pfnWriteReg = ov8858_write_register,
    .pfnReadReg = ov8858_read_register,
};
