#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "jvs/jvs-bus.h"

void jvs_bus_transact(
        struct jvs_node *head,
        const void *bytes,
        size_t nbytes,
        struct iobuf *resp)
{
    struct jvs_node *node;

    assert(bytes != NULL);
    assert(resp != NULL);

    for (node = head ; node != NULL ; node = node->next) {
        node->transact(node, bytes, nbytes, resp);
    }
}

bool jvs_node_sense(struct jvs_node *node)
{
    if (node != NULL) {
        return node->sense(node);
    } else {
        return false;
    }
}
