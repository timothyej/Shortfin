# Automake file for shortfin

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

bin_PROGRAMS = shortfin
shortfin_SOURCES = src/master_server.c src/server.c src/safe.c src/worker.c src/socket.c src/cache.c src/request.c src/response.c src/connection.c src/status_codes.c src/mime_types.c src/config_file.c src/proxy.c src/lock.c src/events.c
shortfin_LDFLAGS = -Wl,--no-as-needed -lpthread

dist_noinst_SCRIPTS = autogen.sh

dist_sysconf_DATA = config/shortfin.conf

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/logs/shortfin
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/www
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/www/cache
