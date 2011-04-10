#ifndef _SAFE_H_
#define _SAFE_H_

#include <stdarg.h>

#include "base.h"

/* thread safe system calls using POSIX semaphores */

int safe_warn(server *srv, const char *format, ...);

#endif
