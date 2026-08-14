/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 *
 * SmartSens SC485SL linear MIPI driver.
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "spm_comm_cam.h"
#include "cam_sensor.h"
#include "../module/sc485sl_spm/sc485sl_spm.h"

#define SENSOR_MAGIC 0x4855L
#define SC485SL_NAME "sc485sl"
#define SC485SL_VTS_ADJUST 8U
#define SC485SL_VTS_MAX 0x3ffff0U
#define SC485SL_EXPOSURE_MIN 2U

#define SC485SL_VTS_H 0x320e
#define SC485SL_VTS_L 0x320f
#define SC485SL_EXP_H 0x3e00
#define SC485SL_EXP_M 0x3e01
#define SC485SL_EXP_L 0x3e02
#define SC485SL_AGAIN 0x3e08
#define SC485SL_AGAIN_FINE 0x3e09
#define SC485SL_DGAIN 0x3e06
#define SC485SL_DGAIN_FINE 0x3e07
#define SC485SL_GROUP_HOLD 0x3812

static const unsigned int sc485sl_reg_addr_byte = I2C_16BIT;
static const unsigned int sc485sl_reg_data_byte = I2C_8BIT;

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

static int sc485sl_write_register(void* handle, uint16_t reg, uint16_t val)
{
    SENSOR_CONTEXT_S* ctx = handle;
    struct cam_i2c_data data;

    SENSORS_CHECK_PARA_POINTER(ctx);
    data.addr = ctx->i2c_addr;
    data.reg_len = sc485sl_reg_addr_byte;
    data.val_len = sc485sl_reg_data_byte;
    data.tab.reg = reg;
    data.tab.val = val;
    return sensor_write_register(ctx->devId, &data);
}

static int sc485sl_read_register(void* handle, uint16_t reg, uint16_t* val)
{
    SENSOR_CONTEXT_S* ctx = handle;
    struct cam_i2c_data data;
    int ret;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(val);
    data.addr = ctx->i2c_addr;
    data.reg_len = sc485sl_reg_addr_byte;
    data.val_len = sc485sl_reg_data_byte;
    data.tab.reg = reg;
    data.tab.val = 0;
    ret = sensor_read_register(ctx->devId, &data);
    if (!ret)
        *val = data.tab.val;
    return ret;
}

static int sc485sl_write_burst_register(void* handle, struct regval_tab* table, int count)
{
    SENSOR_CONTEXT_S* ctx = handle;
    struct cam_burst_i2c_data data;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(table);
    data.addr = ctx->i2c_addr;
    data.reg_len = sc485sl_reg_addr_byte;
    data.val_len = sc485sl_reg_data_byte;
    data.tab = table;
    data.num = count;
    return sensor_write_burst_register(ctx->devId, &data);
}

static int sc485sl_sensor_write_reg(void* handle, uint32_t reg, uint32_t val)
{
    SENSOR_CONTEXT_S* ctx = handle;
    int ret;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    pthread_mutex_lock(&ctx->apiLock);
    ret = sc485sl_write_register(ctx, reg, val);
    pthread_mutex_unlock(&ctx->apiLock);
    return ret;
}

static int sc485sl_sensor_group_reg_start(void* handle)
{
    SENSOR_CONTEXT_S* ctx = handle;
    int ret;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    pthread_mutex_lock(&ctx->apiLock);
    ret = sc485sl_write_register(ctx, SC485SL_GROUP_HOLD, 0x00);
    pthread_mutex_unlock(&ctx->apiLock);
    return ret;
}

static int sc485sl_sensor_group_reg_done(void* handle)
{
    SENSOR_CONTEXT_S* ctx = handle;
    int ret;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    pthread_mutex_lock(&ctx->apiLock);
    ret = sc485sl_write_register(ctx, SC485SL_GROUP_HOLD, 0x10);
    if (!ret)
        ret = sc485sl_write_register(ctx, SC485SL_GROUP_HOLD, 0xa0);
    pthread_mutex_unlock(&ctx->apiLock);
    return ret;
}

static int sc485sl_sensor_get_isp_default(void* handle, uint32_t channel, uint32_t scene,
                                           ISP_SENSOR_DEFAULT_S* def)
{
    SENSOR_CONTEXT_S* ctx = handle;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(def);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    memset(def, 0, sizeof(*def));
    if (scene < CAM_ISP_SCENE_INVALID)
        def->pstIspDefaultSetting = ctx->work_info.pstIspDefaultSettings[scene];
    (void)channel;
    return 0;
}

static int sc485sl_sensor_get_isp_black_level(void* handle, uint32_t channel,
                                               ISP_SENSOR_BLACK_LEVEL_S* black)
{
    SENSOR_CONTEXT_S* ctx = handle;
    int bit_depth;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(black);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    bit_depth = ctx->work_info.work_mode == SC485SL_SPM_2688X1520_10BIT_90FPS_4LANE ? 10 : 12;
    black->bUpdate = false;
    black->bitDepth = bit_depth;
    for (int i = 0; i < 4; ++i)
        black->sensorBlackLevel[i] = 0;
    (void)channel;
    return 0;
}

static int sc485sl_sensor_get_reg_info(void* handle, ISP_SENSOR_REGS_INFO_S* info)
{
    SENSOR_CONTEXT_S* ctx = handle;
    static const uint16_t regs[] = {
        SC485SL_EXP_L, SC485SL_EXP_M, SC485SL_EXP_H,
        SC485SL_AGAIN, SC485SL_AGAIN_FINE, SC485SL_DGAIN, SC485SL_DGAIN_FINE,
        SC485SL_VTS_L, SC485SL_VTS_H,
    };

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(info);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    pthread_mutex_lock(&ctx->apiLock);
    if (!ctx->syncInit) {
        ctx->sensorRegs[0].u8CfgDelayMax = 2;
        ctx->sensorRegs[0].u32RegNum = ARRAY_SIZE(regs);
        ctx->sensorRegs[0].stSensorComBus.s8I2cDev = ctx->twsi_no;
        for (unsigned int i = 0; i < ARRAY_SIZE(regs); ++i) {
            ctx->sensorRegs[0].astI2cData[i].bUpdate = true;
            ctx->sensorRegs[0].astI2cData[i].u8DelayFrmNum = 2;
            ctx->sensorRegs[0].astI2cData[i].u8DevAddr = ctx->i2c_addr;
            ctx->sensorRegs[0].astI2cData[i].u32AddrWidth = sc485sl_reg_addr_byte;
            ctx->sensorRegs[0].astI2cData[i].u32DataWidth = sc485sl_reg_data_byte;
            ctx->sensorRegs[0].astI2cData[i].u32RegAddr = regs[i];
        }
        ctx->syncInit = true;
    } else {
        for (unsigned int i = 0; i < ctx->sensorRegs[0].u32RegNum; ++i)
            ctx->sensorRegs[0].astI2cData[i].bUpdate =
                ctx->sensorRegs[0].astI2cData[i].u32Data != ctx->sensorRegs[1].astI2cData[i].u32Data;
    }
    memcpy(info, &ctx->sensorRegs[0], sizeof(*info));
    memcpy(&ctx->sensorRegs[1], &ctx->sensorRegs[0], sizeof(*info));
    ctx->vts[1] = ctx->vts[0];
    pthread_mutex_unlock(&ctx->apiLock);
    return 0;
}

static int sc485sl_sensor_dump_info(void* handle)
{
    SENSOR_CONTEXT_S* ctx = handle;
    uint16_t hi, lo;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    if (!sc485sl_read_register(ctx, SC485SL_VTS_H, &hi) &&
        !sc485sl_read_register(ctx, SC485SL_VTS_L, &lo))
        CLOG_INFO("sc485sl: VTS=%u", ((uint32_t)hi << 8) | lo);
    return 0;
}

static int sc485sl_sensor_get_ae_default(void* handle, uint32_t channel, ISP_SENSOR_AE_DEFAULT_S* def)
{
    SENSOR_CONTEXT_S* ctx = handle;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(def);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    def->initSceneLuma = ctx->init_3a_attr.initSceneLuma[channel];
    def->initSceneLux = ctx->init_3a_attr.initSceneLux[channel];
    def->initExpTime = ctx->work_info.exp_time[channel];
    def->initAnaGain = ctx->work_info.again[channel];
    def->initDGain = ctx->work_info.dgain[channel];
    def->initTGain = def->initAnaGain * def->initDGain / 0x1000;
    def->maxDelayCfg = 2;
    def->minDelayCfg = 2;
    return 0;
}

static int sc485sl_sensor_get_expotime_by_fps(void* handle, float fps)
{
    SENSOR_CONTEXT_S* ctx = handle;
    uint32_t vts;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    if (fps > ctx->maxFps || fps <= 0)
        return -EINVAL;
    vts = ctx->minVTS * ctx->maxFps / fps;
    if (vts > SC485SL_VTS_MAX)
        return -EINVAL;
    return (vts - SC485SL_VTS_ADJUST) * ctx->lineTime / 1000;
}

static int sc485sl_sensor_fps_set(void* handle, float fps)
{
    SENSOR_CONTEXT_S* ctx = handle;
    uint32_t vts;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    if (fps > ctx->maxFps || fps <= 0)
        return -EINVAL;
    vts = ctx->minVTS * ctx->maxFps / fps;
    if (vts > SC485SL_VTS_MAX)
        return -EINVAL;
    pthread_mutex_lock(&ctx->apiLock);
    ctx->initVTS = vts;
    ctx->initFps = fps;
    ctx->vts[0] = vts;
    ctx->sensorRegs[0].astI2cData[7].u32Data = LOW_8BITS(vts);
    ctx->sensorRegs[0].astI2cData[8].u32Data = HIGH_8BITS(vts);
    pthread_mutex_unlock(&ctx->apiLock);
    return 0;
}

static int sc485sl_sensor_expotime_update(void* handle, uint32_t channel, uint32_t exposure,
                                           ISP_SENSOR_VTS_INFO_S* vts_info)
{
    SENSOR_CONTEXT_S* ctx = handle;
    uint32_t lines;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(vts_info);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    pthread_mutex_lock(&ctx->apiLock);
    lines = exposure * 1000 / ctx->lineTime;
    if (lines < SC485SL_EXPOSURE_MIN)
        lines = SC485SL_EXPOSURE_MIN;
    if (lines > SC485SL_VTS_MAX - SC485SL_VTS_ADJUST)
        lines = SC485SL_VTS_MAX - SC485SL_VTS_ADJUST;
    if (lines + SC485SL_VTS_ADJUST > ctx->vts[0])
        ctx->vts[0] = lines + SC485SL_VTS_ADJUST;
    ctx->hdrIntTime[channel] = lines * ctx->lineTime / 1000;
    ctx->sensorRegs[0].astI2cData[0].u32Data = (lines << 4) & 0xf0;
    ctx->sensorRegs[0].astI2cData[1].u32Data = (lines >> 4) & 0xff;
    ctx->sensorRegs[0].astI2cData[2].u32Data = (lines >> 12) & 0x0f;
    ctx->sensorRegs[0].astI2cData[7].u32Data = LOW_8BITS(ctx->vts[0]);
    ctx->sensorRegs[0].astI2cData[8].u32Data = HIGH_8BITS(ctx->vts[0]);
    vts_info->snsLineTime = ctx->lineTime;
    vts_info->snsVts = ctx->vts[0];
    vts_info->snsFps = ctx->initFps * ctx->initVTS / ctx->vts[0];
    pthread_mutex_unlock(&ctx->apiLock);
    return 0;
}

static void sc485sl_again_to_reg(uint32_t gain_q8, uint8_t* coarse, uint8_t* fine, uint32_t* actual_q8)
{
    static const struct {
        uint8_t coarse;
        uint32_t base_q8;
        uint32_t step_q8;
    } stages[] = {
        {0x00, 256, 8},
        {0x01, 512, 16},
        {0x80, 873, 27},
        {0x81, 1746, 55},
        {0x83, 3492, 109},
        {0x87, 6984, 218},
        {0x8f, 13968, 436},
    };
    unsigned int stage = 0;
    unsigned int step;

    if (gain_q8 < 256)
        gain_q8 = 256;
    while (stage + 1 < ARRAY_SIZE(stages) && gain_q8 >= stages[stage + 1].base_q8)
        ++stage;
    step = (gain_q8 - stages[stage].base_q8 + stages[stage].step_q8 / 2) / stages[stage].step_q8;
    if (step > 31)
        step = 31;
    *coarse = stages[stage].coarse;
    *fine = 0x20 + step;
    *actual_q8 = stages[stage].base_q8 + step * stages[stage].step_q8;
}

static int sc485sl_sensor_gain_update(void* handle, uint32_t channel, uint32_t* again, uint32_t* dgain)
{
    SENSOR_CONTEXT_S* ctx = handle;
    uint8_t coarse, fine;
    uint32_t actual;
    uint32_t dgain_q12, integer, fractional;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(again);
    SENSORS_CHECK_PARA_POINTER(dgain);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    sc485sl_again_to_reg(*again, &coarse, &fine, &actual);
    dgain_q12 = *dgain < 0x1000 ? 0x1000 : *dgain;
    integer = dgain_q12 / 0x1000;
    if (integer > 8)
        integer = 8;
    fractional = (dgain_q12 * 128 + (integer * 0x1000) / 2) / (integer * 0x1000);
    if (fractional < 0x80)
        fractional = 0x80;
    if (fractional > 0xff)
        fractional = 0xff;
    pthread_mutex_lock(&ctx->apiLock);
    ctx->sensorRegs[0].astI2cData[3].u32Data = coarse;
    ctx->sensorRegs[0].astI2cData[4].u32Data = fine;
    ctx->sensorRegs[0].astI2cData[5].u32Data = integer - 1;
    ctx->sensorRegs[0].astI2cData[6].u32Data = fractional;
    pthread_mutex_unlock(&ctx->apiLock);
    *again = actual;
    *dgain = integer * fractional * 0x1000 / 128;
    (void)channel;
    return 0;
}

static int sc485sl_get_aelib_default_settings(void* handle, uint32_t channel,
                                               AE_LIB_DEFAULT_SETTING_S** setting)
{
    SENSORS_CHECK_PARA_POINTER(setting);
    *setting = NULL;
    (void)handle;
    (void)channel;
    return 0;
}

static int sc485sl_sensor_get_awb_default(void* handle, uint32_t channel, ISP_SENSOR_AWB_DEFAULT_S* def)
{
    SENSOR_CONTEXT_S* ctx = handle;
    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(def);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    def->initCorrelationCT = ctx->init_3a_attr.initCorrelationCT[channel];
    def->initTint = ctx->init_3a_attr.initTint[channel];
    return 0;
}

static int sc485sl_get_awblib_default_settings(void* handle, uint32_t channel,
                                               AWB_LIB_DEFAULT_SETTING_S** setting)
{
    SENSORS_CHECK_PARA_POINTER(setting);
    *setting = NULL;
    (void)handle;
    (void)channel;
    return 0;
}

static int sc485sl_power_on(SENSOR_CONTEXT_S* ctx)
{
    sensor_set_gpio_enable(ctx->devId, SENSOR_GPIO_PWDN, 0);
    sensor_set_gpio_enable(ctx->devId, SENSOR_GPIO_RST, 0);
    sensor_set_power_voltage(ctx->devId, SENSOR_REGULATOR_DOVDD, 1800000);
    sensor_set_power_voltage(ctx->devId, SENSOR_REGULATOR_DVDD, 1200000);
    sensor_set_power_voltage(ctx->devId, SENSOR_REGULATOR_AVDD, 2800000);
    sensor_set_power_on(ctx->devId, SENSOR_REGULATOR_DOVDD, 1);
    sensor_set_power_on(ctx->devId, SENSOR_REGULATOR_DVDD, 1);
    sensor_set_power_on(ctx->devId, SENSOR_REGULATOR_AVDD, 1);
    sensor_set_mclk_rate(ctx->devId, 27000000);
    sensor_set_mclk_enable(ctx->devId, 1);
    sensor_set_gpio_enable(ctx->devId, SENSOR_GPIO_PWDN, 1);
    sensor_set_gpio_enable(ctx->devId, SENSOR_GPIO_RST, 1);
    usleep(5000);
    return 0;
}

static int sc485sl_power_off(SENSOR_CONTEXT_S* ctx)
{
    sensor_set_gpio_enable(ctx->devId, SENSOR_GPIO_RST, 0);
    sensor_set_gpio_enable(ctx->devId, SENSOR_GPIO_PWDN, 0);
    sensor_set_mclk_enable(ctx->devId, 0);
    sensor_set_power_on(ctx->devId, SENSOR_REGULATOR_AVDD, 0);
    sensor_set_power_on(ctx->devId, SENSOR_REGULATOR_DVDD, 0);
    sensor_set_power_on(ctx->devId, SENSOR_REGULATOR_DOVDD, 0);
    return 0;
}

static int sc485sl_init(void** handle, SENSOR_CUSTOM_S custom)
{
    SENSOR_CONTEXT_S* ctx;
    struct cam_sensor_info info;

    SENSORS_CHECK_PARA_POINTER(handle);
    ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        return -ENOMEM;
    ctx->name = SC485SL_NAME;
    ctx->devId = custom.dev_id;
    ctx->i2c_addr = custom.i2c_addr;
    ctx->magic = SENSOR_MAGIC;
    pthread_mutex_init(&ctx->apiLock, NULL);
    sensor_hw_init(ctx->devId);
    sc485sl_power_on(ctx);
    sensor_get_hw_info(ctx->devId, &info);
    ctx->twsi_no = info.twsi_no;
    *handle = ctx;
    return 0;
}

static int sc485sl_deinit(void* handle)
{
    SENSOR_CONTEXT_S* ctx = handle;
    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    pthread_mutex_lock(&ctx->apiLock);
    if (ctx->stream_on_flag)
        sc485sl_write_burst_register(ctx, sc485sl_stream_off_regs, ARRAY_SIZE(sc485sl_stream_off_regs));
    sc485sl_power_off(ctx);
    sensor_hw_exit(ctx->devId);
    pthread_mutex_unlock(&ctx->apiLock);
    pthread_mutex_destroy(&ctx->apiLock);
    ctx->magic = 0;
    free(ctx);
    return 0;
}

static int sc485sl_global_config(void* handle, SENSOR_WORK_INFO_S* work)
{
    SENSOR_CONTEXT_S* ctx = handle;
    int ret;
    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(work);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    pthread_mutex_lock(&ctx->apiLock);
    if (ctx->stream_on_flag) {
        pthread_mutex_unlock(&ctx->apiLock);
        return -EPERM;
    }
    memcpy(&ctx->work_info, work, sizeof(*work));
    memset(&ctx->init_3a_attr, 0, sizeof(ctx->init_3a_attr));
    ctx->initVTS = work->vts;
    ctx->minVTS = work->vts;
    ctx->maxFps = work->f32maxFps;
    ctx->initFps = work->f32maxFps;
    ctx->lineTime = work->linetime;
    ctx->vts[0] = work->vts;
    ctx->vts[1] = work->vts;
    ctx->syncInit = false;
    memset(ctx->sensorRegs, 0, sizeof(ctx->sensorRegs));
    ret = sc485sl_write_burst_register(ctx, sc485sl_soft_reset_regs, ARRAY_SIZE(sc485sl_soft_reset_regs));
    if (!ret)
        usleep(1000);
    if (!ret)
        ret = sc485sl_write_burst_register(ctx, work->setting_table, work->setting_table_size);
    pthread_mutex_unlock(&ctx->apiLock);
    return ret;
}

static int sc485sl_set_param(void* handle, const SENSOR_INIT_ATTR_S* attr)
{
    SENSOR_CONTEXT_S* ctx = handle;
    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(attr);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    memcpy(&ctx->init_3a_attr, attr, sizeof(*attr));
    return 0;
}

static int sc485sl_stream_on(void* handle)
{
    SENSOR_CONTEXT_S* ctx = handle;
    int ret;
    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    pthread_mutex_lock(&ctx->apiLock);
    ret = sensor_mipi_clock_set(ctx->devId, ctx->work_info.mipi_clock);
    if (!ret)
        ret = sc485sl_write_burst_register(ctx, sc485sl_stream_on_regs, ARRAY_SIZE(sc485sl_stream_on_regs));
    if (!ret)
        ctx->stream_on_flag = 1;
    pthread_mutex_unlock(&ctx->apiLock);
    return ret;
}

static int sc485sl_stream_off(void* handle)
{
    SENSOR_CONTEXT_S* ctx = handle;
    int ret;
    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    pthread_mutex_lock(&ctx->apiLock);
    ret = sc485sl_write_burst_register(ctx, sc485sl_stream_off_regs, ARRAY_SIZE(sc485sl_stream_off_regs));
    ctx->stream_on_flag = 0;
    pthread_mutex_unlock(&ctx->apiLock);
    return ret;
}

static int sc485sl_get_ops(void* handle, ISP_SENSOR_REGISTER_S* ops)
{
    SENSOR_CONTEXT_S* ctx = handle;
    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(ops);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    ops->snsHandle = ctx;
    ops->stSensorFunc.pfn_sensor_write_reg = sc485sl_sensor_write_reg;
    ops->stSensorFunc.pfn_sensor_get_isp_default = sc485sl_sensor_get_isp_default;
    ops->stSensorFunc.pfn_sensor_get_isp_black_level = sc485sl_sensor_get_isp_black_level;
    ops->stSensorFunc.pfn_sensor_get_reg_info = sc485sl_sensor_get_reg_info;
    ops->stSensorFunc.pfn_sensor_dump_info = sc485sl_sensor_dump_info;
    ops->stSensorFunc.pfn_sensor_group_regs_start = sc485sl_sensor_group_reg_start;
    ops->stSensorFunc.pfn_sensor_group_regs_done = sc485sl_sensor_group_reg_done;
    ops->stSensorAeFunc.pfn_sensor_get_ae_default = sc485sl_sensor_get_ae_default;
    ops->stSensorAeFunc.pfn_sensor_fps_set = sc485sl_sensor_fps_set;
    ops->stSensorAeFunc.pfn_sensor_get_expotime_by_fps = sc485sl_sensor_get_expotime_by_fps;
    ops->stSensorAeFunc.pfn_sensor_expotime_update = sc485sl_sensor_expotime_update;
    ops->stSensorAeFunc.pfn_sensor_gain_update = sc485sl_sensor_gain_update;
    ops->stSensorAeFunc.pfn_get_aelib_default_settings = sc485sl_get_aelib_default_settings;
    ops->stSensorAwbFunc.pfn_sensor_get_awb_default = sc485sl_sensor_get_awb_default;
    ops->stSensorAwbFunc.pfn_get_awblib_default_settings = sc485sl_get_awblib_default_settings;
    return 0;
}

static int sc485sl_detect_sensor(void* handle, SENSOR_VENDOR_ID_S* vendor)
{
    SENSOR_CONTEXT_S* ctx = handle;
    struct regval_tab* readback;
    struct cam_burst_i2c_data data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(ctx);
    SENSORS_CHECK_PARA_POINTER(vendor);
    SENSORS_CHECK_PARA_POINTER(vendor->id_table);
    SENSOR_CHECK_HANDLE_IS_ERR(ctx);
    readback = calloc(vendor->id_table_size, sizeof(*readback));
    if (!readback)
        return -ENOMEM;
    for (unsigned int i = 0; i < vendor->id_table_size; ++i)
        readback[i].reg = vendor->id_table[i].reg;
    data.addr = ctx->i2c_addr;
    data.reg_len = sc485sl_reg_addr_byte;
    data.val_len = sc485sl_reg_data_byte;
    data.tab = readback;
    data.num = vendor->id_table_size;
    ret = sensor_read_burst_register(ctx->devId, &data);
    if (!ret) {
        for (unsigned int i = 0; i < vendor->id_table_size; ++i) {
            if (readback[i].val != vendor->id_table[i].val) {
                ret = -ENODEV;
                break;
            }
        }
    }
    free(readback);
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
