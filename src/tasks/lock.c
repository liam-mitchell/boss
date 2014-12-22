#include "lock.h"

#include "macros.h"

void sem_init(semaphore_t *sem)
{
    if (sem != NULL) {
        sem->lock = 0;
    }
}

void sem_spin_lock(semaphore_t *sem)
{
    while (sem->lock);

    sem->lock = 1;
}

void sem_spin_unlock(semaphore_t *sem)
{
    if (!sem->lock) {
        PANIC("Attempted to unlock an unlocked semaphore");
    }

    sem->lock = 0;
}
