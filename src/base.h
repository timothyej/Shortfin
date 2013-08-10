#ifndef _BASE_H_
#define _BASE_H_

#include <config.h>

#include <ctype.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>

#ifdef HAVE_SYS_EPOLL_H
	#include <sys/epoll.h>
#elif defined(HAVE_SYS_EVENT_H)
	#include <sys/types.h>
	#include <sys/event.h>
	#include <sys/time.h>
#endif

#include "cache.h"


/* connection event types */
#define CONN_STARTED  1
#define CONN_READING  2
#define CONN_ERROR    3
#define CONN_WRITING  4
#define CONN_CLOSED   5
#define CONN_INACTIVE 6

/* HTTP method types */
#define METHOD_UNKNOWN	0
#define METHOD_HEAD	1
#define METHOD_GET	2
#define METHOD_POST	3

/* cache types */
#define CACHE_NONE	-1
#define CACHE_YES	1
#define CACHE_FD	0

/* proxy rule types */
#define PROXY_RULE_UNKNOWN	-1
#define PROXY_RULE_EXT		0
#define PROXY_RULE_DIR		1
#define PROXY_RULE_FILE		2
#define PROXY_RULE_IP		3


typedef struct {
	sem_t mutex;
} lock;

typedef struct {
	char *HTTP_VER;
	int   HTTP_VER_LEN;

	char *HTTP_200;
	char *HTTP_302;
	char *HTTP_400;
	char *HTTP_404;
	char *HTTP_500;
	char *HTTP_501;
	
	int   HTTP_200_LEN;
	int   HTTP_302_LEN;
	int   HTTP_400_LEN;
	int   HTTP_404_LEN;
	int   HTTP_500_LEN;
	int   HTTP_501_LEN;
	
	char *HTTP_400_CONTENT;
	char *HTTP_404_CONTENT;
	char *HTTP_500_CONTENT;
	char *HTTP_501_CONTENT;
	
	int   HTTP_400_CONTENT_LEN;
	int   HTTP_404_CONTENT_LEN;
	int   HTTP_500_CONTENT_LEN;
	int   HTTP_501_CONTENT_LEN;
} status_codes;

typedef struct stringpair {
	/* used by headers */
	char key[128];
	char value[1024];
} stringpair;

typedef struct {
	int fd;
	int listen_port;
	
	socklen_t clilen;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
} sock;

typedef struct {
	FILE *fd; 		/* file descriptor, -1 = closed */

	char *filename;		/* filename */
	int filename_len;
	
	char *abs_path; 	/* absolute path */
	int abs_path_len;
	
	unsigned long size; 	/* file size */
	int header_len;		/* header length */ 
	
	int http_status;
} file_item;

typedef struct {
	char *method;  		/* GET | POST | HEAD etc. */
	int method_type;
	char *uri;
	int version;
	
	int method_len;
	int uri_len;
	
	stringpair **headers;
	int header_count;
	
	char *host;
	int host_len;
	
	int keep_alive;
} request;

typedef struct {
	int status; 		/* 200, 404 etc. */
	char *headers;
	char *data;
	char *http_packet; 	/* headers + data */
	char *content_type;
	
	unsigned long header_len;
	unsigned long data_len;
	unsigned long http_packet_len;
	int content_type_len;
	
	file_item *file;	/* file info */
	
	int cached; 		/* if 1 then; this response is cached */
} response;

typedef struct {
	int fd; 		/* socket */
	int port;		/* connected on this port */
	void *server;		/* which server this connection is assigned to */
	
	request *request;
	response *response;
	
	time_t start_ts; 	/* when the connection was initialized */
	time_t last_event;	/* when the last event took place */
	
	int status;		/* connection status */
	
	char *read_buffer;
	long buffer_len;
} connection;

typedef struct {
	int fd;
#ifdef HAVE_SYS_EPOLL_H
	struct epoll_event *events;
#elif defined(HAVE_SYS_EVENT_H)
	struct kevent changes;
	struct kevent *events;
	int nevents;
#endif
} event_handler;

typedef struct {
	int type;
	char *value;
} proxy_rule;

/* proxy struct */
typedef struct {
	proxy_rule **rules;
	int rule_count;
	
	char *host;
	int port;
	
	int cache; /* 1 or 0, cache files or not, in memory only */
} proxy;

typedef struct {
	/* config */
	int listen_port;
	
	int max_workers;
	int max_pending;
	int max_clients;
	
	int child_stack_size;
	
	int heartbeat_interval;
	
	int read_buffer_size;
	int write_buffer_size;
	
	int tcp_nodelay; 	/* Nagle (TCP No Delay) algorithm */
	
	char *server_name; 	/* server name, used in headers */
	
	char *doc_root; 	/* document root */
	char *index_file;
	int daemonize;
	
	int cache_files; 	/* 1 = cache files, 0 = cache file descriptors, -1 = cache nothing */
	int cache_other;
	int cache_max_fds;
	char *cache_dir;

	unsigned long cache_memory_size;
	unsigned long cache_turn_off_limit;

	char *chroot;
	int keep_alive;
	int keep_alive_timeout;
	
	int default_server;
	char *hostname;
	
	/* logging */
	char *access_log_path;
	char *error_log_path;
	int access_log;
	int error_log;
	
	proxy **proxies;	/* proxy configurations */
	int proxy_count;
} config;

typedef struct {
	int running;
	pid_t pid;		/* Process ID */

	char *config_file;	/* file path to the config file */
	config *config;		/* holds master server settings */

	void **servers;
	int server_count;
	int server_default;
	
	cache *servers_by_port;
	void **servers_by_fd;
	cache *servers_by_host;
	
	void **workers; 	/* keep information and pointers to all workers */
	void **worker_map;	/* map an fd to a worker, just pointers */
	lock lock_log; 		/* semaphore lock between processes when logging */
} master_server;

typedef struct {
	int running; 		/* on / off? */
	
	config *config;
	
	char *server_header;
	int server_header_len;
	
	sock *server_socket;	/* listening socket */
	
	cache *file_cache; 	/* file cache */
	int cache_open_fds;
	unsigned long cache_used_mem;
	
	status_codes *status_codes;
	cache *mime_types;

	master_server *master;	/* pointer to the master server */
} server;

typedef struct {
	int num; 			/* which process number */
	master_server *master_srv;	/* master server (configs and servers), not shared */
	pid_t pid;			/* Process ID */
	connection **conns; 		/* connections */
	time_t heartbeat;		/* update this every 5 seconds */
	
	event_handler ev_handler;	/* event handler - epoll, kqueue */
} worker;

#ifndef _LOCK_H_
#include "lock.h"
#endif

#ifndef _SERVER_H_
#include "server.h"
#endif

#ifndef _WORKER_H_
#include "worker.h"
#endif

#ifndef _SAFE_H_
#include "safe.h"
#endif

#ifndef _SOCKET_H_
#include "socket.h"
#endif

#ifndef _REQUEST_H_
#include "request.h"
#endif

#ifndef _PROXY_H_
#include "proxy.h"
#endif

#ifndef _RESPONSE_H_
#include "response.h"
#endif

#ifndef _CONNECTION_H_
#include "connection.h"
#endif

#ifndef _CONFIG_FILE_H_
#include "config_file.h"
#endif

#ifndef _EVENTS_H_
#include "events.h"
#endif

#ifndef _STATUS_CODES_H_
#include "status_codes.h"
#endif

#ifndef _MIME_TYPES_H_
#include "mime_types.h"
#endif

#endif
