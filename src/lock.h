#ifndef _LOCK_H_
#define _LOCK_H_

/* mutex lock between processes with semaphores */

#include <semaphore.h>
#include "base.h"

int lock_init(lock *l);
int lock_lock(lock *l);
int lock_unlock(lock *l);
int lock_free(lock *l);

#endif
