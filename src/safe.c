#include "safe.h"

int safe_warn(server *srv, const char *format, ...) {
	/* displays a warning */
	lock_lock (&srv->master->lock_log);
	
	va_list args;
	printf ("WARNING ");
	
	va_start (args, format);
	vprintf (format, args);
	va_end (args);
	
	printf ("\n");

	lock_unlock (&srv->master->lock_log);

	return 0;
}

