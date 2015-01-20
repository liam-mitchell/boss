#include "ring-buffer.h"

#include "kheap.h"
#include "memory.h"
#include "printf.h"

int ring_buffer_read(struct ring_buffer *rbuf, void *buf, uint32_t n)
{
    if (n == 0) {
        return 0;
    }

    if (n > rbuf->size) {
        n = rbuf->size;
    }

    uint32_t ret = n;

    while (n) {
        memcpy(buf, rbuf->first, 1);

        --n;
        --rbuf->size;
        ++rbuf->first;
        ++buf;

        if (rbuf->first == rbuf->base + rbuf->len) {
            rbuf->first = rbuf->base;
        }
    }

    return ret;
    /* uint32_t ret = n; */
    /* rbuf->size -= n; */

    /* void *end = rbuf->base + rbuf->len; */

    /* if (rbuf->first + n > end) { */
    /*     uint32_t len = end - rbuf->first; */
    /*     memcpy(buf, rbuf->first, len); */
    /*     n -= len; */
    /*     rbuf->first = rbuf->base; */
    /*     rbuf->size -= len; */
    /* } */

    /* memcpy(buf, rbuf->first, n); */
    /* rbuf->first += n; */
    /* rbuf->size -= n; */
}

int ring_buffer_write(struct ring_buffer *rbuf, void *buf, uint32_t n)
{
    if (n == 0) {
        return 0;
    }

    if (n > rbuf->len - rbuf->size) {
        n = rbuf->len - rbuf->size;
    }

    uint32_t ret = n;

    void *end = rbuf->base + rbuf->len;
    while (n) {
        memcpy(rbuf->last, buf, 1);
        ++rbuf->last;
        ++rbuf->size;
        ++buf;
        --n;

        if (rbuf->last >= end) {
            rbuf->last = rbuf->base;
        }
    }

    return ret;
}

struct ring_buffer *ring_buffer_create(uint32_t len)
{
    struct ring_buffer *ret = kmalloc(MEM_GEN, sizeof(*ret));
    ret->base = kmalloc(MEM_GEN, len);
    ret->first = ret->base;
    ret->last = ret->base;
    ret->len = len;
    ret->size = 0;

    return ret;
}

void ring_buffer_delete(struct ring_buffer *rbuf)
{
    kfree(rbuf->base);
    kfree(rbuf);
}
