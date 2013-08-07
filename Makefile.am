# Automake file for shortfin

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

bin_PROGRAMS = shortfin
shortfin_SOURCES =		\
	src/cache.c		\
	src/config_file.c	\
	src/connection.c	\
	src/events.c		\
	src/lock.c		\
	src/master_server.c	\
	src/mime_types.c	\
	src/proxy.c		\
	src/request.c		\
	src/response.c		\
	src/safe.c		\
	src/server.c		\
	src/socket.c		\
	src/status_codes.c	\
	src/worker.c		\
	src/base.h		\
	src/cache.h		\
	src/config_file.h	\
	src/connection.h	\
	src/events.h		\
	src/lock.h		\
	src/mime_types.h	\
	src/proxy.h		\
	src/request.h		\
	src/response.h		\
	src/safe.h		\
	src/server.h		\
	src/socket.h		\
	src/status_codes.h	\
	src/worker.h
shortfin_LDFLAGS = -Wl,--no-as-needed -lpthread

dist_noinst_SCRIPTS = autogen.sh

dist_sysconf_DATA = config/shortfin.conf

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/logs/shortfin
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/www
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/www/cache
