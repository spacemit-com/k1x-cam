/*
 * Copyright (C) 2021 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef __I2C_COMMON_H__
#define __I2C_COMMON_H__

#include <stdint.h>

#include "cam_sensor_uapi.h"
#if defined(__cplusplus)
extern "C" {
#endif

/*
enum i2c_ctrl_len {
    I2C_8BIT = 1,
    I2C_16BIT = 2,
    I2C_24BIT = 3,
    I2C_32BIT = 4,
};
*/
struct cmd_i2c_data {
    enum sensor_i2c_len reg_len;
    enum sensor_i2c_len val_len;
    uint8_t addr; /* 7 bit i2c address*/
    uint32_t reg;
    uint32_t val;
};

int i2_ctrl_open(int twsi_no);
int i2_ctrl_close(int i2c_ctrl_fp);
int i2_ctrl_write(int i2c_ctrl_fp, struct cmd_i2c_data* i2c_data);
int i2_ctrl_read(int i2c_ctrl_fp, struct cmd_i2c_data* i2c_data);

#if defined(__cplusplus)
}
#endif

#endif /* __I2C_COMMON_H__ */
