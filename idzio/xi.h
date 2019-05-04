#pragma once

/* Can't call this xinput.h or it will conflict with <xinput.h> */

#include <windows.h>

#include "idzio/backend.h"

HRESULT idz_xi_init(const struct idz_io_backend **backend);
