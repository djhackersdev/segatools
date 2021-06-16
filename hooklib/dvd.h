#pragma once

#include <windows.h>

#include <stdbool.h>

struct dvd_config {
    bool enable;
};

void dvd_hook_init(const struct dvd_config *cfg, HINSTANCE self);
