#pragma once

#include <stddef.h>
#include <stdint.h>

uint32_t crc32(const void *src, size_t nbytes, uint32_t in);
