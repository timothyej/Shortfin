#include "response.h"

response *response_init(server *srv) {
	/* init a new response struct */
	response *resp = malloc(sizeof(response));
	
	resp->headers = NULL;
	resp->data = NULL;
	resp->http_packet = NULL;
	resp->content_type = NULL;
	
	resp->status = 0;
	resp->header_len = 0;
	resp->data_len = 0;
	resp->http_packet_len = 0;
	resp->content_type_len = 0;
	
	resp->cached = 0;
	
	return resp;
}

int response_build(server *srv, connection *conn) {
	/* build the response */
	conn->response = response_init(srv);
	response *resp = conn->response;
	file_item *f = NULL;
	
	/* check method */
	if (conn->request->method_type == METHOD_GET || conn->request->method_type == METHOD_HEAD) {
		/* method is GET or HEAD */
		char *filename = conn->request->uri+1;
		int filename_len = conn->request->uri_len-1;
		
		if ((f = cache_get_exists(srv->file_cache, filename, filename_len)) != NULL) {
			/* found in cache */
			resp->file = f;
			printf (" * %s %s/%s (cached)\n", conn->request->method, ((server*)conn->server)->config->hostname, filename);
			
			if (srv->config->cache_files == CACHE_YES && f->fd != -1) {
				/* using a cached file */
				resp->status = f->http_status;
				resp->cached = 1;
				
				if (conn->request->method_type == METHOD_GET) {
					/* GET */
					resp->http_packet_len = f->size;
				} else {
					/* HEAD */
					resp->http_packet_len = f->header_len;
				}
				
				return 0;
			} else {
				/* the file needs to be read again */
				response_read_file (srv, conn, f);
			}
		} else {
			/* not found in cache */
			f = malloc(sizeof(file_item));
			resp->file = f;
			printf (" * %s %s/%s\n", conn->request->method, ((server*)conn->server)->config->hostname, filename);
			
			/* filename */
			f->filename_len = filename_len;
			f->filename = malloc(filename_len+1);
			memcpy (f->filename, filename, filename_len);
			memcpy (f->filename+filename_len, "\0", 1);
	
			/* absolute path */
			int doc_root_len = strlen(srv->config->doc_root);
			f->abs_path_len = doc_root_len+filename_len;
			f->abs_path = malloc(f->abs_path_len+1);
			memcpy (f->abs_path, srv->config->doc_root, doc_root_len);
			memcpy (f->abs_path+doc_root_len, filename, filename_len);
			memcpy (f->abs_path+doc_root_len+filename_len, "\0", 1);
	
			/* fd and size */
			f->fd = -1;
			f->size = 0;
			f->header_len = 0;
	
			/* get the requested file */
			response_read_file (srv, conn, f);
				
			/* add to cache */
			cache_add (srv->file_cache, filename, f);
		}		
		
		/* file is loaded */
		if (resp->data_len < 1) {
			/* 404 Not Found */
			resp->status = 404;
		} else {
			/* 200 OK */
			resp->status = 200;
		}
	} else {
		/* the method is not GET or HEAD */
		resp->status = 501;
	}

	/* build response */
	switch (resp->status) {
		case 200:
			resp->content_type = mime_types_get_by_filename(f->filename, f->filename_len, srv->mime_types);
		break;
		case 400:
			resp->data = srv->status_codes->HTTP_400_CONTENT;
			resp->data_len = srv->status_codes->HTTP_400_CONTENT_LEN;
			resp->content_type = mime_types_get(".html", 5, srv->mime_types);
		break;
		case 404:
			resp->data = srv->status_codes->HTTP_404_CONTENT;
			resp->data_len = srv->status_codes->HTTP_404_CONTENT_LEN;
			resp->content_type = mime_types_get(".html", 5, srv->mime_types);
		break;
		case 501:
			resp->data = srv->status_codes->HTTP_501_CONTENT;
			resp->data_len = srv->status_codes->HTTP_501_CONTENT_LEN;
			resp->content_type = mime_types_get(".html", 5, srv->mime_types);
		break;
		case 500:
		default:
			resp->data = srv->status_codes->HTTP_500_CONTENT;
			resp->data_len = srv->status_codes->HTTP_500_CONTENT_LEN;
			resp->content_type = mime_types_get(".html", 5, srv->mime_types);
		break;
	}
	
	/* build HTTP packet */
	response_build_http_packet (srv, resp);
	
	/* cache the response */
	if (srv->config->cache_files == CACHE_YES) {
		if ((resp->status == 200 || srv->config->cache_other) && resp->status != 501) {
			/* first, check if there is space in the cache memory */
			if (srv->cache_used_mem + resp->http_packet_len <= srv->config->cache_memory_size) {
				/* if the file not already exist, save it */
				char *tmp_name = response_char2hex(f->abs_path);
				char *tmp_filename = malloc(strlen(srv->config->cache_dir)+strlen(tmp_name)+1);
				sprintf (tmp_filename, "%s%s", srv->config->cache_dir, tmp_name); 
				
				/* check if it exists */
				if (response_file_exist(tmp_filename) == 0) {
					/* file not found, create one */
					FILE *ff = fopen(tmp_filename, "w+"); /* open with write rights */
					fwrite (resp->http_packet, 1, resp->http_packet_len, ff);
					fclose (ff);
					
					/* increase the mem used counter */
					srv->cache_used_mem += resp->http_packet_len;
				}
				
				/* open the file again */
				if ((f->fd = open(tmp_filename, 0)) == -1) {
					perror ("ERROR could not cache the file");
				}
				
				f->size = resp->http_packet_len;
				f->header_len = resp->header_len;
				resp->cached = 1;
				
				free (tmp_name);
				free (tmp_filename);
			} else {
				/* this file does not fit in the cache mem */
				safe_warn (srv, "this file does not fit in the cache: needs %lu bytes, only %lu bytes is available.",
						 resp->http_packet_len, srv->config->cache_memory_size - srv->cache_used_mem);
			}
		}
	}

	/* HEAD? */
	if (conn->request->method_type == METHOD_HEAD) {
		resp->http_packet_len = resp->header_len;
	}

	if (f != NULL) {
		f->http_status = resp->status;
	}

	return 0;
}

int response_build_http_packet(server *srv, response *resp) {
	/* build HTTP packet */
	char *status;
	int status_len = 0;
	status_codes *sc = srv->status_codes;
	
	char *hdr1 = NULL;
	char *hdr2 = NULL;
	
	int hdr1_len = 0;
	int hdr2_len = 0;
	
	/* headers */
	if (resp->content_type != NULL) {
		resp->content_type_len = strlen(resp->content_type);
		hdr1_len = 16+resp->content_type_len;
	} else {
		resp->content_type_len = 0;
	}
	
	if (hdr1_len > 0) {
		hdr1 = malloc(hdr1_len+1);
		hdr1_len = sprintf(hdr1, "\r\nContent-Type: %s", resp->content_type);
	}
	
	if (resp->data_len > 0) {
		hdr2_len = 30; /* 18 + 12 */
		hdr2 = malloc(hdr2_len+1);
		hdr2_len = sprintf(hdr2, "\r\nContent-Length: %lu", resp->data_len);		
	}

	/* calculate len */
	resp->http_packet_len = sc->HTTP_VER_LEN;
	
	switch (resp->status) {
		case 200:
			status = sc->HTTP_200;
			status_len = sc->HTTP_200_LEN;
		break;
		
		case 400:
			status = sc->HTTP_400;
			status_len = sc->HTTP_400_LEN;
		break;
		
		case 404:
			status = sc->HTTP_404;
			status_len = sc->HTTP_404_LEN;
		break;
		
		case 501:
			status = sc->HTTP_501;
			status_len = sc->HTTP_501_LEN;
		break;
		
		case 500:
		default:
			status = sc->HTTP_500;
			status_len = sc->HTTP_500_LEN;
		break;
	}
	
	resp->header_len = sc->HTTP_VER_LEN + status_len + hdr1_len + hdr2_len + srv->server_header_len+4;
	
	resp->http_packet_len = resp->header_len + resp->data_len;
	
	/* build http packet */
	resp->http_packet = malloc(resp->http_packet_len+1);
	
	memcpy (resp->http_packet, sc->HTTP_VER, sc->HTTP_VER_LEN);
	memcpy (resp->http_packet+sc->HTTP_VER_LEN, status, status_len);
	
	if (hdr1_len > 0) {
		memcpy (resp->http_packet+sc->HTTP_VER_LEN+status_len, hdr1, hdr1_len);
	}
	
	if (hdr2_len > 0) {
		memcpy (resp->http_packet+sc->HTTP_VER_LEN+status_len+hdr1_len, hdr2, hdr2_len);
	}	
	
	memcpy (resp->http_packet+sc->HTTP_VER_LEN+status_len+hdr1_len+hdr2_len, srv->server_header, srv->server_header_len);
	
	memcpy (resp->http_packet+resp->header_len-4, "\r\n\r\n", 4);
	memcpy (resp->http_packet+resp->header_len, resp->data, resp->data_len);
	memcpy (resp->http_packet+resp->header_len+resp->data_len, "\0", 1);
	
	/* free some memory */
	if (hdr1 != NULL) {
		free (hdr1);
	}
	
	if (hdr2 != NULL) {
		free (hdr2);
	}
	
	if (resp->status == 200) {
		/* only free this if it's a 200 OK */
		/* all the other response types is loaded into memory */
		free (resp->data);
		resp->data = NULL;
	}	
	
	return 0;
}

int response_read_file(server *srv, connection *conn, file_item *f) {
	response *resp = conn->response;
	
	if (f->fd == -1) {
		/* not a cached file descriptor, open it again */
		if (f->filename_len == 0) {
			response_get_index_file (srv, f);
		} else if (response_file_exist(f->abs_path) == 0) {
			response_get_index_file (srv, f);
		}
		
		if (!(f->fd = fopen(f->abs_path, "rb"))) {
			f->fd = -1;
			return NULL;
		}

		/* get file size */
		fseek (f->fd, 0, SEEK_END);
		f->size = ftell(f->fd);
		fseek (f->fd, 0, SEEK_SET);
		
		if (srv->config->cache_files == CACHE_FD) {
			++srv->cache_open_fds;
		}
	}
	
	if (f->size < 1) {
		fclose (f->fd);
		f->fd = -1;
		return NULL;
	}
	
	/* allocate memory */
	if (!(resp->data = malloc(f->size+1))) {
		fclose (f->fd);
		f->fd = -1;
		safe_warn (srv, "could not allocate memory for file.");
		return NULL;
	}

	/* read file contents into buffer */
	fread (resp->data, f->size, 1, f->fd);
	fseek (f->fd, 0, SEEK_SET);
	
	resp->data_len = f->size;
	
	/* dont't cache file descriptors, close it */
	if (srv->config->cache_files != CACHE_FD) {
		fclose (f->fd);
		f->fd = -1;
	}

	return 0;
}

int response_file_exist(char *filename) {
	/* check if a file exists on disc */
	struct stat sts;
	if (stat(filename, &sts) == -1 && errno == ENOENT) {
    		return 0;
	}
	
	if (S_ISDIR(sts.st_mode)) {
		return 0;
	}
	
	return 1;
}

int response_get_index_file(server *srv, file_item *f) {
	/* get index file */
	int len = 0;
	char *abs_path = malloc(f->abs_path_len+strlen(srv->config->index_file)+2);
	
	memcpy (abs_path, f->abs_path, f->abs_path_len);
	
	if (f->abs_path[f->abs_path_len-1] == '/') {
		memcpy (abs_path+f->abs_path_len, srv->config->index_file, strlen(srv->config->index_file)+1);
		len = f->abs_path_len + strlen(srv->config->index_file);
	} else {
		memcpy (abs_path+f->abs_path_len, "/", 1);
		memcpy (abs_path+f->abs_path_len+1, srv->config->index_file, strlen(srv->config->index_file)+1);
		len = f->abs_path_len + strlen(srv->config->index_file) + 1;
	}
	
	free (f->abs_path);
	
	f->abs_path = abs_path;
	f->abs_path_len = len;
	
	f->filename = srv->config->index_file;
	f->filename_len = strlen(f->filename);
	
	return 0;
}

int response_free(response *resp) {
	if (resp->headers != NULL) {
		free (resp->headers);
		resp->headers = NULL;
	}

	if (resp->http_packet != NULL) {
		free (resp->http_packet);
		resp->http_packet = NULL;
	}
	
	free (resp);
	
	return 0;
}

char *response_char2hex(char *str) {
	int str_len = strlen(str);
	char *new = malloc(str_len*3+1);
	int i;
	
	memcpy (new, "\0", 1);
	
	for (i = 0; i < str_len; ++i) {
		sprintf (new, "%s%02X", new, (unsigned char*) str[i]);
	}
	
	return new;
}
