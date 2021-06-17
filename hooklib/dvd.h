#pragma once

#include <windows.h>

#include <stdbool.h>

struct dvd_config {
    bool enable;
};

/* Init is not thread safe because API hook init is not thread safe blah
    blah blah you know the drill by now. */

void dvd_hook_init(const struct dvd_config *cfg, HINSTANCE self);
