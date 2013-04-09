#include "request.h"

char *strtolower(char *str) {
	if (str == NULL) return str;
	int i;
	for (i = 0; str[i]; ++i) {
		str[i] = tolower(str[i]);
	}
	return str;
}

request *request_init(server *srv) {
	/* init a new request struct */
	int i;
	
	request *req = malloc(sizeof(request));
	req->headers = malloc(sizeof(stringpair*) * 50);
	
	for (i = 0; i < 50; ++i) {
		req->headers[i] = malloc(sizeof(stringpair));
	}

	req->uri = NULL;
	req->method = NULL;
	req->version = 0;
	
	req->method_len = 0;
	req->uri_len = 0;
	req->header_count = 0;
	
	req->host = NULL;
	req->host_len = 0;
	
	req->keep_alive = 1;
	
	return req;
}

int request_parse(server *srv, connection *conn) {	
	/* parse the http request */
	conn->request = request_init(srv);
	request *req = conn->request;
	
	char *data = conn->read_buffer;
	int data_len = conn->buffer_len;

	int i;
	int header_count = 0;
	int done = 0;
	int is_headers = 0;
	int is_key = 1;
	int is_uri = 0;
	int cur_len = 0;
	int is_method = 1;
	
	stringpair **headers = req->headers;
	
	req->uri = malloc(1024);
	char *uri = req->uri;
	int uri_len = 0;
	
	req->method = malloc(9);
	char *method = req->method;
	int method_len = 0;
	
	for (i = 0; i < data_len && !done; ++i) {
		unsigned char cur = data[i];
		
		if (data_len - i > 2) {
			if (data[i] == '\r' && data[i+1] == '\n' &&
				data[i+2] == '\r' && data[i+3] == '\n') {
			    	done = 1;
			}
		}
		
		/* before headers */
		if (!is_headers) {
			switch (cur) {
				case '\r':
				/* now we enter the headers */
					is_headers = 1;
					is_key = 1;
					++i;
				break;
			
				case ' ':
				/* uri starts or ends */
					if (is_method) {
						is_uri = 1;
						is_method = 0;
					} else {
						is_uri = 0;
					}
				break;

				case '?':
				/* query string starts */
					is_uri = 0;
				break;
				
				default:
					if (is_uri && uri_len < 1023) {
						uri[uri_len] = cur;
						++uri_len;
					} else if (is_method && method_len < 8) {
						method[method_len] = cur;
						++method_len;
					}
			}
		} else {
		/* in headers */
			if (cur == ':' && is_key) {
				is_key = 0;
				cur_len = 0;
				++i;
			} else if (cur == '\r') {
				if (header_count < 50) {
					headers[header_count]->value[cur_len] = '\0';
					
					if (strncmp(headers[header_count]->key, "Host", 4) == 0) {
					/* if host */
						req->host = headers[header_count]->value;
						req->host_len = cur_len;
					} else if (strncmp(headers[header_count]->key, "Connection", 10) == 0) {
					/* keep-alive? */
						char *connection = strtolower(headers[header_count]->value);
						if (strncmp(connection, "keep-alive", 10) != 0) {
							/* this request is not keep-alive */
							req->keep_alive = 0;
						}
					}
				}
				
				cur_len = 0;
				is_key = 1;
				
				++header_count;
				++i;	
			} else if (cur) {
				if (is_key) {
					if (cur_len < 127) {
						headers[header_count]->key[cur_len] = cur; 
						++cur_len;
					}
				} else {	
					if (cur_len < 1023) {
						headers[header_count]->value[cur_len] = cur; 
						++cur_len;
					}
				}
			}
		}
	}

	method[method_len] = '\0';
	req->method_len = method_len;

	uri[uri_len] = '\0';
	req->uri_len = uri_len;

	req->header_count = header_count;

	/* determine method */
	if (method_len > 0) {
		if (strncmp(conn->request->method, "GET", 3) == 0) {
			req->method_type = METHOD_GET;
		} else if (strncmp(conn->request->method, "HEAD", 4) == 0) {
			req->method_type = METHOD_HEAD;
		} else if (strncmp(conn->request->method, "POST", 4) == 0) {
			req->method_type = METHOD_POST;
		} else {
			/* unknown method */
			req->method_type = METHOD_UNKNOWN;	
		}
	} else {
		/* unknown method */
		req->method_type = METHOD_UNKNOWN;
	}

	return 0;
}

int request_free(request *req) {
	int i;

	if (req->method != NULL) {
		free (req->method);
		req->method = NULL;
	}
	
	if (req->uri != NULL) {
		free (req->uri);
		req->uri = NULL;
	}
	
	/* free headers */
	for (i = 0; i < 50; ++i) {
		free (req->headers[i]);
		req->headers[i] = NULL;
	} 
	
	free (req->headers);
	free (req);
	
	return 0;
}
