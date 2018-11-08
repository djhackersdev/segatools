#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "hook/iobuf.h"

struct jvs_node {
    struct jvs_node *next;
    void (*transact)(
            struct jvs_node *node,
            const void *bytes,
            size_t nbytes,
            struct iobuf *resp);
    bool (*sense)(struct jvs_node *node);
};

void jvs_bus_transact(
        struct jvs_node *head,
        const void *bytes,
        size_t nbytes,
        struct iobuf *resp);

bool jvs_node_sense(struct jvs_node *node);
