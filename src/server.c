#include "server.h"

int server_init(server *srv) {
	int i, e;
	
	/* build server header */
	char *tmp = "\r\nServer: ";
	srv->server_header_len = strlen(tmp)+strlen(srv->config->server_name);
	srv->server_header = malloc(srv->server_header_len+1);

	memcpy (srv->server_header, tmp, strlen(tmp));
	memcpy (srv->server_header+strlen(tmp), srv->config->server_name, strlen(srv->config->server_name));
	memcpy (srv->server_header+strlen(tmp)+strlen(srv->config->server_name), "\0", 1);

	/* init server socket */
	srv->server_socket = socket_init();

	/* init file cache */
	srv->file_cache = calloc(1, sizeof(*srv->file_cache));
	cache_init (srv->file_cache);
	srv->cache_open_fds = 0;
	srv->cache_used_mem = 0;
	
	/* load http status codes */
	srv->status_codes = status_codes_init();

	/* load mime types */
	srv->mime_types = mime_types_init();

	return 0;
}

int server_free(server *srv) {
	/* free everything allocated in server_init */
	int i;
	
	/* free server socket */
	socket_free (srv->server_socket);
	
	/* free server header */
	free (srv->server_header);
	
	/* free status codes, caches etc */
	status_codes_free (srv->status_codes);
	mime_types_free (srv->mime_types);
	cache_free (srv->file_cache);
		
	/* free server */
	free (srv);
	
	return 0;
}
