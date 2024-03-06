/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include "cam_sensor.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int snsdev_fd[CAM_SNS_MAX_DEV_NUM] = {};

int sensor_hw_init(int sns_id)
{
    char device_name[30];

    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }

    if (snsdev_fd[sns_id] > 0) {
        CLOG_INFO("%s%d opened already", CAM_SENSOR_DEV_NAME, sns_id);
    } else {
        sprintf(device_name, "%s%d", CAM_SENSOR_DEV_NAME, sns_id);
        CLOG_INFO("open device %s", device_name);

        if ((snsdev_fd[sns_id] = open(device_name, O_RDWR | O_SYNC)) < 0) {
            CLOG_ERROR("open %s fail: %s", device_name, strerror(errno));
            return -EPERM;
        }
    }

    return 0;
}

int sensor_hw_exit(int sns_id)
{
    int ret = 0;

    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }

    if (snsdev_fd[sns_id] > 0)
        close(snsdev_fd[sns_id]);
    snsdev_fd[sns_id] = 0;

    return ret;
}

int sensor_hw_unreset(int sns_id)
{
    int ret = 0;
    sns_rst_source_t sns_src;

    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }

    sns_src = sns_id;
    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_UNRESET, &sns_src);
    if (ret) {
        CLOG_ERROR("sensor%d unreset failed: %s\n", sns_id, strerror(errno));
    }

    return ret;
}

int sensor_hw_reset(int sns_id)
{
    int ret = 0;
    sns_rst_source_t sns_src;

    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }

    sns_src = sns_id;
    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_RESET, &sns_src);
    if (ret) {
        CLOG_ERROR("sensor%d reset failed: %s\n", sns_id, strerror(errno));
    }

    return ret;
}
int sensor_set_power_voltage(int sns_id, uint8_t regulator_id, uint32_t voltage)
{
    int ret = 0;
    struct cam_sensor_power sns_power;

    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }

    memset(&sns_power, 0, sizeof(sns_power));
    sns_power.regulator_id = regulator_id;
    sns_power.voltage = voltage;
    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_SET_POWER_VOLTAGE, &sns_power);
    if (ret) {
        CLOG_ERROR("sensor%d set power[%d] voltage[%d] failed: %s\n", sns_id, regulator_id, voltage, strerror(errno));
    }

    return ret;
}

int sensor_set_power_on(int sns_id, uint8_t regulator_id, uint8_t on)
{
    int ret = 0;
    struct cam_sensor_power sns_power;

    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }

    memset(&sns_power, 0, sizeof(sns_power));
    sns_power.regulator_id = regulator_id;
    sns_power.on = on;
    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_SET_POWER_ON, &sns_power);
    if (ret) {
        CLOG_ERROR("sensor%d set power[%d] on[%d] failed: %s\n", sns_id, regulator_id, on, strerror(errno));
    }

    return ret;
}

int sensor_set_gpio_enable(int sns_id, uint8_t gpio_id, uint8_t enable)
{
    int ret = 0;
    struct cam_sensor_gpio sns_gpio;

    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }

    memset(&sns_gpio, 0, sizeof(sns_gpio));
    sns_gpio.gpio_id = gpio_id;
    sns_gpio.enable = enable;
    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_SET_GPIO_ENABLE, &sns_gpio);
    if (ret) {
        CLOG_ERROR("sensor%d set gpio[%d] enable[%d] failed: %s\n", sns_id, gpio_id, enable, strerror(errno));
    }

    return ret;
}

int sensor_set_mclk_rate(int sns_id, uint32_t clk_rate)
{
    int ret = 0;
    uint32_t mclk_rate = 0;

    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }
    mclk_rate = clk_rate;

    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_SET_MCLK_RATE, &mclk_rate);
    if (ret) {
        CLOG_ERROR("sensor%d set mclk rate [%d] failed: %s\n", sns_id, mclk_rate, strerror(errno));
    }

    return ret;
}

int sensor_set_mclk_enable(int sns_id, uint32_t clk_enable)
{
    int ret = 0;
    uint32_t mclk_enable = 0;

    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }
    mclk_enable = clk_enable;

    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_SET_MCLK_ENABLE, &mclk_enable);
    if (ret) {
        CLOG_ERROR("sensor%d set mclk enable [%d] failed: %s\n", sns_id, mclk_enable, strerror(errno));
    }

    return ret;
}

int sensor_write_register(int sns_id, struct cam_i2c_data* data)
{
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(data);
    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }

    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_I2C_WRITE, data);
    if (ret) {
        CLOG_ERROR("sensor%d write register failed: %s\n", sns_id, strerror(errno));
    }

    return ret;
}

int sensor_read_register(int sns_id, struct cam_i2c_data* data)
{
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(data);
    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }

    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_I2C_READ, data);
    if (ret) {
        CLOG_ERROR("sensor%d read register failed: %s\n", sns_id, strerror(errno));
    }

    return ret;
}

int sensor_write_burst_register(int sns_id, struct cam_burst_i2c_data* data)
{
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(data);
    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }

    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_I2C_BURST_WRITE, data);
    if (ret) {
        CLOG_ERROR("sensor%d burst write register failed: %s\n", sns_id, strerror(errno));
    }

    return ret;
}

int sensor_read_burst_register(int sns_id, struct cam_burst_i2c_data* data)
{
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(data);
    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }

    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_I2C_BURST_READ, data);
    if (ret) {
        CLOG_INFO("sensor%d burst read register failed: %s\n", sns_id, strerror(errno));
    }

    return ret;
}

int sensor_mipi_clock_set(unsigned int sns_id, unsigned int mipi_clock)
{
    int ret = 0;
    sns_mipi_clock_t sns_mipi_clock;

    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }
    if (mipi_clock < 80) {
        CLOG_ERROR("mipi clock %d is invalid!", mipi_clock);
        return -EINVAL;
    }

    sns_mipi_clock = mipi_clock;
    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_SET_MIPI_CLOCK, &sns_mipi_clock);
    if (ret) {
        CLOG_ERROR("sensor%d mipi clock set failed: %s\n", sns_id, strerror(errno));
    }

    return ret;
}

int sensor_get_hw_info(int sns_id, struct cam_sensor_info* info)
{
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(info);
    if (sns_id >= CAM_SNS_MAX_DEV_NUM) {
        CLOG_ERROR("sns_id %d is invalid, max sensor number %d\n", sns_id, CAM_SNS_MAX_DEV_NUM);
        return -EINVAL;
    }
    if (snsdev_fd[sns_id] == 0) {
        CLOG_ERROR("sensor%d not opened, please init sensor first", sns_id);
        return -EPERM;
    }

    ret = ioctl(snsdev_fd[sns_id], CAM_SENSOR_GET_INFO, info);
    if (ret) {
        CLOG_ERROR("sensor%d get hw info failed: %s\n", sns_id, strerror(errno));
    }

    return ret;
}
