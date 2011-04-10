#include "lock.h"

int lock_init(lock *l) {
	/* init a new semaphore */
	sem_init (l, 0, 1);
	
	return 0;
}

int lock_lock(lock *l) {
	/* lock the process */
	sem_wait (l);
	
	return 0;
}

int lock_unlock(lock *l) {
	/* unlock the process */
	sem_post (l);
	
	return 0;
}

int lock_free(lock *l) {
	/* destroy the semaphore */
	sem_destroy (l);
	
	return 0;
}
