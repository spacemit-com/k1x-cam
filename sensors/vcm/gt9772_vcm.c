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

#include "cam_vcm.h"
//#include "../sensor/cam_sensor.h"
#include "i2c_common.h"

#define VCM_MAGIC 0x977205B5
#define VCM_NAME  "gt9772"

static int i2c_fd;

static const unsigned int sensor_reg_addr_byte = I2C_8BIT; /*byte width of the sensor register address*/
static const unsigned int sensor_reg_data_byte = I2C_8BIT; /*byte width of sensor register data*/
static int gt9772_vcm_move(void* vcmHandle, int position);

static struct regval_tab stream_on_regs[] = {
    /* selection mode on */
    {0xec, 0xa3},
    /* DLC and MCLK[1:0] setting */
    {0xA1, 0x0D},
    /* T_SRC[4:0] setting */
    {0xf2, 0x00},
    /* selection mode off */
    {0xdc, 0x51},
};

/*******************************************************************/
static int gt9772_write_register(void* handle, uint16_t regAddr, uint16_t value)
{
    VCM_CONTEXT_S* vcm_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    vcm_context = (VCM_CONTEXT_S*)handle;

#if 0
    struct cam_i2c_data reg_data;
    reg_data.addr = vcm_context->i2c_addr;
    reg_data.reg_len = sensor_reg_addr_byte;
    reg_data.val_len = sensor_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = value;
    ret = sensor_write_register(vcm_context->devId, &reg_data);
#else
    struct cmd_i2c_data data;
    data.addr = vcm_context->i2c_addr;
    data.reg_len = sensor_reg_addr_byte;
    data.val_len = sensor_reg_data_byte;
    data.reg = regAddr;
    data.val = value;
    ret = i2_ctrl_write(i2c_fd, &data);
    if (ret < 0) {
        CLOG_ERROR("write register error regAddr = %x value %x\n", regAddr, value);
        return -1;
    }
#endif
    // printf("gt9772_write_register regAddr = %x value %x\n",regAddr, value);

    return ret;
}

static int gt9772_read_register(void* handle, uint16_t regAddr, uint16_t* value)
{
    VCM_CONTEXT_S* vcm_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    vcm_context = (VCM_CONTEXT_S*)handle;

#if 0
    struct cam_i2c_data reg_data;
    reg_data.addr = vcm_context->i2c_addr;
    reg_data.reg_len = sensor_reg_addr_byte;
    reg_data.val_len = I2C_16BIT;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = 0;
    ret = sensor_read_register(vcm_context->devId, &reg_data);
    if (!ret) {
        *value = reg_data.tab.val;
    }
#else
    struct cmd_i2c_data data;
    data.addr = vcm_context->i2c_addr;
    data.reg_len = sensor_reg_addr_byte;
    data.val_len = I2C_16BIT;
    data.reg = regAddr;
    data.val = 0;
    ret = i2_ctrl_read(i2c_fd, &data);
    if (ret < 0) {
        CLOG_ERROR("read register error regAddr = %x value %x\n", regAddr, *value);
        return -1;
    } else {
        *value = data.val;
    }
#endif
    // printf(" gt9772_read_register regAddr %d  value  %d\n", data.reg, data.val);
    return ret;
}

/*******************************************************************/
/*isp sensor function*/

#define GT9772_DATA_SHIFT 0x4
static int gt9777c_write_burst_register(void* handle, struct regval_tab* reg_table, int reg_table_num)
{
    VCM_CONTEXT_S* vcm_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(reg_table);
    vcm_context = (VCM_CONTEXT_S*)handle;

#if 0
    struct cam_burst_i2c_data reg_table_data;
    reg_table_data.addr = vcm_context->i2c_addr;
    reg_table_data.reg_len = sensor_reg_addr_byte;
    reg_table_data.val_len = sensor_reg_data_byte;
    reg_table_data.tab = reg_table;
    reg_table_data.num = reg_table_num;
    ret = sensor_write_burst_register(vcm_context->devId, &reg_table_data);
#else
    {
        int i;
        for (i = 0; i < reg_table_num; i++) {
            gt9772_write_register(handle, reg_table[i].reg, reg_table[i].val);
        }
    }
#endif

    return ret;
}

static int gt9772_vcm_stream_on(void* handle)
{
    VCM_CONTEXT_S* vcm_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    vcm_context = (VCM_CONTEXT_S*)handle;
    VCM_CHECK_HANDLE_IS_ERR(vcm_context);

    // pthread_mutex_lock(&vcm_context->apiLock);
#if 0
    uint32_t i = 0;
    for (i = 0; i < sensor_context->sensorRegs[0].u32RegNum; i++) {
        ov13b10_write_register(handle, sensor_context->sensorRegs[0].astI2cData[i].u32RegAddr,
                               sensor_context->sensorRegs[0].astI2cData[i].u32Data);
    }
#else
    ret = gt9777c_write_burst_register(handle, stream_on_regs, ARRAY_SIZE(stream_on_regs));
#endif
    // vcm_context->stream_on_flag = 1;
    // pthread_mutex_unlock(&vcm_context->apiLock);
    return ret;
}

static int gt9772_vcm_init(void** pHandle)
{
    VCM_CONTEXT_S* vcm_context = NULL;

    SENSORS_CHECK_PARA_POINTER(pHandle);

    vcm_context = (VCM_CONTEXT_S*)calloc(1, sizeof(VCM_CONTEXT_S));
    if (NULL == vcm_context) {
        CLOG_ERROR("%s: vcm_context malloc memory failed!", __FUNCTION__);
        return -ENOMEM;
    }
    vcm_context->name = VCM_NAME;
    vcm_context->magic = VCM_MAGIC;
    vcm_context->i2c_addr = 0x0c;
    vcm_context->twsi_no = 0;
    vcm_context->current_position = 0;

    i2c_fd = i2_ctrl_open(vcm_context->twsi_no);
    if (i2c_fd < 0) {
        CLOG_ERROR("open i2c devce %d fail  ret %d\n", vcm_context->twsi_no, i2c_fd);
        return -1;
    }
    gt9772_vcm_stream_on(vcm_context);
    *pHandle = vcm_context;
    return 0;
}

static int gt9772_vcm_deinit(void* handle)
{
    VCM_CONTEXT_S* vcm_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    vcm_context = (VCM_CONTEXT_S*)handle;
    VCM_CHECK_HANDLE_IS_ERR(vcm_context);

    for (int i = ((vcm_context->current_position) / 50); i > 0; i--) {
        gt9772_vcm_move(vcm_context, i * 50);
        usleep(2000);
    }
    vcm_context->magic = 0;
    free(vcm_context);
    vcm_context = NULL;

    i2_ctrl_close(i2c_fd);
    return 0;
}

static int gt9772_vcm_move(void* vcmHandle, int position)
{
    int ret = 0;
    VCM_CONTEXT_S* vcm_context = NULL;

    SENSORS_CHECK_PARA_POINTER(vcmHandle);
    vcm_context = (VCM_CONTEXT_S*)vcmHandle;
    VCM_CHECK_HANDLE_IS_ERR(vcm_context);

    if (vcm_context->current_position == position) {
        CLOG_DEBUG("same position %d\n", position);
        return ret;
    }
    vcm_context->current_position = position;
    if (position > 1023 || position < 0) {
        CLOG_ERROR("wrong position %d\n", position);
        return -1;
    }

    uint8_t val_buf[2];

    position = (position << 4) | 0x0f;
    val_buf[0] = (position >> 8) & 0x3f;
    val_buf[1] = position & 0xff;

    gt9772_write_register(vcmHandle, val_buf[0], val_buf[1]);
    // printf("gt9772 vcm move to position %d val 0x%x, 0x%x\n", position, val_buf[0], val_buf[1]);

    return ret;
}

static int gt9772_vcm_get_ops(void* handle, ISP_AF_MOTOR_REGISTER_S* pVcmFuncOps)
{
    VCM_CONTEXT_S* vcm_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(pVcmFuncOps);
    vcm_context = (VCM_CONTEXT_S*)handle;
    VCM_CHECK_HANDLE_IS_ERR(vcm_context);

    pVcmFuncOps->vcmHandle = handle;
    pVcmFuncOps->pfn_af_motor_move = gt9772_vcm_move;

    return 0;
}

VCM_OBJ_S gt9772VcmObj = {
    .name = VCM_NAME,
    .pfnInit = gt9772_vcm_init,
    .pfnDeinit = gt9772_vcm_deinit,
    .pfnGetVcmOps = gt9772_vcm_get_ops,
};
