#ifndef __RING_BUFFER_H_
#define __RING_BUFFER_H_

#include <stdint.h>

struct ring_buffer {
    void *base;
    void *first;
    void *last;
    uint32_t len;
    uint32_t size;
};

int ring_buffer_read(struct ring_buffer *rbuf, void *buf, uint32_t n);
int ring_buffer_write(struct ring_buffer *rbuf, void *buf, uint32_t n);
struct ring_buffer *ring_buffer_create(uint32_t len);
void ring_buffer_delete(struct ring_buffer *rbuf);

#endif // __RING_BUFFER_H_
