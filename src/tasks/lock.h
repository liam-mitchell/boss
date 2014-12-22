#ifndef __LOCK_H_
#define __LOCK_H_

#include <stdint.h>

typedef struct semaphore {
    uint8_t lock;
} semaphore_t;

void sem_init(semaphore_t *sem);
void sem_spin_lock(semaphore_t *sem);
void sem_spin_unlock(semaphore_t *sem);

#endif
