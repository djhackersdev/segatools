#pragma once

#include <stddef.h>

#include "hook/iobuf.h"

#ifndef NDEBUG
void dump(const void *ptr, size_t nbytes);
void dump_iobuf(const struct iobuf *iobuf);
void dump_const_iobuf(const struct const_iobuf *iobuf);
#else
#define dump(ptr, nbytes)
#define dump_iobuf(iobuf)
#define dump_const_iobuf(iobuf)
#endif
