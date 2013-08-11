#include <fcntl.h>
#include <netinet/tcp.h>

#include "socket.h"

sock *socket_init() {
	/* init socket (for listening) */
	sock *s = malloc(sizeof(sock));

	s->listen_port = 0;
	bzero ((char*) &s->serv_addr, sizeof(s->serv_addr));
	s->clilen = sizeof(s->cli_addr);
	
    	return s;
}

static int socket_setnonblocking(int fd) {
	int flags;

	/* If they have O_NONBLOCK, use the Posix way to do it */
#if 1//defined(O_NONBLOCK)
	/* FIXME: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
	if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
	/* Otherwise, use the old way of doing it */
	flags = 1;
	return ioctl(fd, FIOBIO, &flags);
#endif
}

static int socket_listen_setup(master_server *master_srv, int fd) {
	/* disable / enable the Nagle (TCP No Delay) algorithm */
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&master_srv->config->tcp_nodelay, sizeof(master_srv->config->tcp_nodelay)) == -1) {
		perror ("ERROR setsockopt(TCP_NODELAY)");
		return -1;
	}

	/* set send buffer size */
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&master_srv->config->write_buffer_size, sizeof(master_srv->config->write_buffer_size)) == -1) {
		perror ("ERROR setsockopt(SO_SNDBUF)");
		return -1;
	}

	/* set read buffer size */
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&master_srv->config->read_buffer_size, sizeof(master_srv->config->read_buffer_size)) == -1) {
		perror ("ERROR setsockopt(SO_RCVBUF)");
		return -1;
	}

	/* set defer accept to 5 sec */
	int val = 5;
	if (setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, (char*)&val, sizeof(int)) == -1) {
		perror ("ERROR setsockopt(TCP_DEFER_ACCEPT)");
		return -1;
	}

	/* non-blocking socket */
	socket_setnonblocking (fd);

	return 0;
}

int socket_listen(server *srv) {
	sock *s = srv->server_socket;

	/* opening socket */
	if ((s->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror ("ERROR opening socket");
		return -1;
	}

	int opt = 1;

	/* set socket to allow multiple connections */
	if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == -1) {
		perror ("ERROR on setsockopt(SO_REUSEADDR)");
		return -1;
	}
	
	/* set socket options */
	if (socket_listen_setup(srv->master, s->fd) == -1) {
		return -1;
	}
	
	/* bind socket */
	s->listen_port = srv->config->listen_port;
	s->serv_addr.sin_family = AF_INET;
	s->serv_addr.sin_addr.s_addr = INADDR_ANY;
	s->serv_addr.sin_port = htons(srv->config->listen_port);
	
	if (bind(s->fd, (struct sockaddr*) &s->serv_addr, sizeof(s->serv_addr)) == -1) {
		perror ("ERROR on binding");
		return -1;
	}
	
	/* start listen */
	if (listen(s->fd, srv->config->max_pending) < 0) {
		perror ("ERROR on listen");
		return -1;
	}
	
	socket_setquickack (s->fd);
	
	return 0;
}

int socket_accept(sock *s) {
	/* accept a new connection */
	return accept(s->fd, (struct sockaddr*) &s->cli_addr, &s->clilen);
}

int socket_read(int fd, char *buffer, int len) {
	/* read data from socket */
	return read(fd, buffer, len);
}

int socket_close(sock *s) {
	/* close a socket */
	close (s->fd);
	return 0;
}

int socket_free(sock *s) {
	free (s);
	return 0;
}

int socket_setup(master_server *master_srv, int fd) {
	/* disable / enable the Nagle (TCP No Delay) algorithm */
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&master_srv->config->tcp_nodelay, sizeof(master_srv->config->tcp_nodelay)) == -1) {
		perror ("ERROR setsockopt(TCP_NODELAY)");
		return -1;
	}
	
	/* set send buffer size */
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&master_srv->config->write_buffer_size, sizeof(master_srv->config->write_buffer_size)) == -1) {
		perror ("ERROR setsockopt(SO_SNDBUF)");
		return -1;
	}
	
	/* set read buffer size */
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&master_srv->config->read_buffer_size, sizeof(master_srv->config->read_buffer_size)) == -1) {
		perror ("ERROR setsockopt(SO_RCVBUF)");
		return -1;
	}
	
	return 0;
}

int socket_setquickack(int fd) {
	int arg = 0;
	if (setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, (char*)&arg, sizeof(arg)) == -1) {
		perror ("ERROR setsockopt(TCP_QUICKACK)");
		return -1;
	}
	
	return 0;
}
