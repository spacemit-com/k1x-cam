#ifndef __TUNING_SERVE_H
#define __TUNING_SERVE_H

enum tuning_objs_index {
    TUNING_OBJS_ISP0 = 0,
    TUNING_OBJS_ISP1,
    TUNING_OBJS_CPP0,
    TUNING_OBJS_CPP1,
    TUNING_OBJS_MAX,
};

struct tuning_objs_config {
    char objs_is_enabled[TUNING_OBJS_MAX];
};

int tuning_server_init(struct tuning_objs_config cfg);
void tuning_server_deinit();
#endif
