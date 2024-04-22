/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "cam_log.h"

#include "cjson.h"
#include "sstr.h"

int getTestConfig(struct testConfig *config, char *jsonfile)
{
    int ret = 0;
    int num = 0, i, idx = 0;
    struct cjson *root = NULL, *item = NULL, *child = NULL, *grandc = NULL;
    char dftFile[64] = "/tmp/sdktest.json";
    char name[16] = "\0";

    if (!jsonfile) {
        CLOG_INFO("using default jsonfile %s", dftFile);
        jsonfile = dftFile;
    }
    if (access(jsonfile, F_OK | R_OK)) {
        CLOG_ERROR("can not access jsonfile %s", jsonfile);
        return -1;
    }

    root = cjson_new_file(jsonfile);
    if (!root) {
        CLOG_ERROR("can not get json object from %s", jsonfile);
        return -1;
    }

    item = cjson_get_object(root, "tuning_server_enable");
    if (!item) {
        CLOG_ERROR("get tuning_server_enable failed");
        ret = -1;
        goto out;
    }
    config->tuningServerEnalbe = cjson_get_int(item);

    item = cjson_get_object(root, "show_fps");
    if (!item) {
        CLOG_ERROR("get show_fps failed");
        ret = -1;
        goto out;
    }
    config->showFps = cjson_get_int(item);

    item = cjson_get_object(root, "auto_run");
    if (!item) {
        CLOG_ERROR("get auto_run failed");
        ret = -1;
        goto out;
    }
    config->autoRun = cjson_get_int(item);

    item = cjson_get_object(root, "test_frame");
    if (!item) {
        CLOG_INFO("get test_frame failed, set test_frame: 500, dump_one_frame:250 as default");
        config->testFrame = 500;
        config->dumpFrame = 250;
    } else {
        config->testFrame = cjson_get_int(item);
        config->testFrame = (config->testFrame < 6) ? 6 : config->testFrame;

        item = cjson_get_object(root, "dump_one_frame");
        if (!item) {
            CLOG_INFO("get dump_one_frame failed, set to %d", config->testFrame / 2);
            config->dumpFrame = config->testFrame / 2;
        } else {
            config->dumpFrame = cjson_get_int(item);
            config->dumpFrame = (config->dumpFrame > config->testFrame - 1) ? config->testFrame - 1 : config->dumpFrame;
        }
    }
    CLOG_INFO("set test_frame: %d, dump_one_frame: %d", config->testFrame, config->dumpFrame);

    item = cjson_get_object(root, "cpp_node");
    if (!item) {
        CLOG_ERROR("get cpp_node failed");
        ret = -1;
        goto out;
    }
    num = cjson_get_len(item);
    CLOG_INFO("cpp node num: %d", num);
    for (i = 0; i < num; i++) {
        child = cjson_get_array(item, i);
        if (!child) {
            CLOG_ERROR("get cpp_node array %d failed", i);
            ret = -1;
            goto out;
        }
        grandc = cjson_get_object(child, "name");
        if (!grandc) {
            CLOG_ERROR("get cpp%d name failed", i);
            ret = -1;
            goto out;
        }
        snprintf(name, sizeof(name), "%s", cjson_get_str(grandc));
        if (!strcmp(name, "cpp0")) {
            idx = 0;
        } else if (!strcmp(name, "cpp1"))
            idx = 1;
        else {
            CLOG_ERROR("invalid cpp name, only cpp0 or cpp1 valid");
            ret = -1;
            goto out;
        }

        grandc = cjson_get_object(child, "enable");
        if (!grandc) {
            CLOG_ERROR("get cpp%d enable failed", i);
            ret = -1;
            goto out;
        }
        config->cppConfig[idx].enable = cjson_get_int(grandc);

        if (!config->cppConfig[idx].enable)
            continue;

        grandc = cjson_get_object(child, "format");
        if (!grandc) {
            CLOG_ERROR("get cpp%d format failed", i);
            ret = -1;
            goto out;
        }
        snprintf(config->cppConfig[idx].format,
                sizeof(config->cppConfig[idx].format),
                "%s",
                cjson_get_str(grandc));

        grandc = cjson_get_object(child, "src_from_file");
        if (!grandc) {
            CLOG_ERROR("get cpp%d src_from_file failed", i);
            ret = -1;
            goto out;
        }
        config->cppConfig[idx].srcFromFile = cjson_get_int(grandc);

        if (!config->cppConfig[idx].srcFromFile)
            continue;

        grandc = cjson_get_object(child, "src_path");
        if (!grandc) {
            CLOG_WARNING("get cpp%d src_path failed", i);
            ret = -1;
            goto out;
        } else {
            snprintf(config->cppConfig[idx].srcFile,
                    sizeof(config->cppConfig[idx].srcFile),
                    "%s",
                    cjson_get_str(grandc));
        }

        grandc = cjson_get_object(child, "size_width");
        if (!grandc) {
            CLOG_WARNING("get cpp%d size_width failed", i);
            ret = -1;
            goto out;
        } else {
            config->cppConfig[idx].width = cjson_get_int(grandc);
        }

        grandc = cjson_get_object(child, "size_height");
        if (!grandc) {
            CLOG_WARNING("get cpp%d size_height failed", i);
            ret = -1;
            goto out;
        } else {
            config->cppConfig[idx].height = cjson_get_int(grandc);
        }
    }

    item = cjson_get_object(root, "isp_node");
    if (!item) {
        CLOG_ERROR("get isp_node failed");
        ret = -1;
        goto out;
    }
    num = cjson_get_len(item);
    CLOG_INFO("isp node num: %d", num);
    for (i = 0; i < num; i++) {
        child = cjson_get_array(item, i);
        if (!child) {
            CLOG_ERROR("get isp_node array %d failed", i);
            ret = -1;
            goto out;
        }
        grandc = cjson_get_object(child, "name");
        if (!grandc) {
            CLOG_ERROR("get isp%d name failed", i);
            ret = -1;
            goto out;
        }
        snprintf(name, sizeof(name), "%s", cjson_get_str(grandc));
        if (!strcmp(name, "isp0")) {
            idx = 0;
        } else if (!strcmp(name, "isp1"))
            idx = 1;
        else {
            CLOG_ERROR("invalid isp name, only isp0 or isp1 valid");
            ret = -1;
            goto out;
        }

        grandc = cjson_get_object(child, "enable");
        if (!grandc) {
            CLOG_ERROR("get %s enable failed", name);
            ret = -1;
            goto out;
        }
        config->ispFeConfig[idx].enable = cjson_get_int(grandc);

        if (!config->ispFeConfig[idx].enable)
            continue;

        grandc = cjson_get_object(child, "work_mode");
        if (!grandc) {
            CLOG_WARNING("get isp%d work_mode failed", idx);
            ret = -1;
            goto out;
        }
        if (!strcmp(cjson_get_str(grandc), "online")) {
            config->ispFeConfig[idx].workMode = ISP_WORKMODE_ONLINE;
        } else if (!strcmp(cjson_get_str(grandc), "rawdump")) {
            config->ispFeConfig[idx].workMode = ISP_WORKMODE_RAWDUMP;
        } else if (!strcmp(cjson_get_str(grandc), "offline_capture")) {
            config->ispFeConfig[idx].workMode = ISP_WORKMODE_OFFLINE_CAPTURE;
        } else if (!strcmp(cjson_get_str(grandc), "offline_preview")) {
            config->ispFeConfig[idx].workMode = ISP_WORKMODE_OFFLINE_PREVIEW;
        } else {
            CLOG_ERROR("invalid isp work mode, valid modes: online, rawdump, offline_preview, offline_capture");
            ret -1;
            goto out;
        }

        if (config->ispFeConfig[idx].workMode == ISP_WORKMODE_ONLINE ||
            config->ispFeConfig[idx].workMode == ISP_WORKMODE_RAWDUMP) {
            // need sensor info
            grandc = cjson_get_object(child, "sensor_name");
            if (!grandc) {
                CLOG_WARNING("get isp%d sensor_name failed", idx);
                ret = -1;
                goto out;
            }
            snprintf(config->ispFeConfig[idx].sensorName,
                    sizeof(config->ispFeConfig[idx].sensorName),
                    "%s",
                    cjson_get_str(grandc));

            grandc = cjson_get_object(child, "sensor_id");
            if (!grandc) {
                CLOG_ERROR("get %s sensor_id failed", name);
                ret = -1;
                goto out;
            }
            config->ispFeConfig[idx].sensorId = cjson_get_int(grandc);

            grandc = cjson_get_object(child, "sensor_work_mode");
            if (!grandc) {
                CLOG_ERROR("get %s sensor_work_mode failed", name);
                ret = -1;
                goto out;
            }
            config->ispFeConfig[idx].sensorWorkMode = cjson_get_int(grandc);

            grandc = cjson_get_object(child, "fps");
            if (!grandc) {
                CLOG_WARNING("get %s fps failed", name);
            }
            config->ispFeConfig[idx].fps = cjson_get_int(grandc);
        }

        if (config->ispFeConfig[idx].workMode == ISP_WORKMODE_OFFLINE_PREVIEW) {
            grandc = cjson_get_object(child, "src_file");
            if (!grandc) {
                CLOG_WARNING("get isp%d src_file failed", idx);
                ret = -1;
                goto out;
            }
            snprintf(config->ispFeConfig[idx].srcFile,
                    sizeof(config->ispFeConfig[idx].srcFile),
                    "%s",
                    cjson_get_str(grandc));

            grandc = cjson_get_object(child, "in_width");
            if (!grandc) {
                CLOG_ERROR("get isp%d in_width failed", idx);
                ret = -1;
                goto out;
            }
            config->ispFeConfig[idx].inWidth = cjson_get_int(grandc);

            grandc = cjson_get_object(child, "in_height");
            if (!grandc) {
                CLOG_ERROR("get isp%d in_height failed", idx);
                ret = -1;
                goto out;
            }
            config->ispFeConfig[idx].inHeight = cjson_get_int(grandc);

            grandc = cjson_get_object(child, "bit_depth");
            if (!grandc) {
                CLOG_ERROR("get isp%d bit_depth failed", idx);
                ret = -1;
                goto out;
            }
            config->ispFeConfig[idx].bitDepth = cjson_get_int(grandc);
        }

        grandc = cjson_get_object(child, "format");
        if (!grandc) {
            CLOG_ERROR("get isp%d format failed", idx);
            ret = -1;
            goto out;
        }
        snprintf(config->ispFeConfig[idx].format,
                sizeof(config->ispFeConfig[idx].format),
                "%s",
                cjson_get_str(grandc));

        grandc = cjson_get_object(child, "out_width");
        if (!grandc) {
            CLOG_ERROR("get isp%d out_width failed", idx);
            ret = -1;
            goto out;
        }
        config->ispFeConfig[idx].outWidth = cjson_get_int(grandc);

        grandc = cjson_get_object(child, "out_height");
        if (!grandc) {
            CLOG_ERROR("get isp%d out_height failed", idx);
            ret = -1;
            goto out;
        }
        config->ispFeConfig[idx].outHeight = cjson_get_int(grandc);
    }

out:
    cjson_delete(root);
    root = NULL;
    return ret;
}
