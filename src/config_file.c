#include "config_file.h"

static int config_load_default(config *conf) {
	/* default configuration */
	
	conf->max_workers = 1;
	conf->heartbeat_interval = 3;
	
	conf->max_pending = 128000;
	conf->max_clients = 128000;
	
	conf->child_stack_size = 128000;
	
	conf->listen_port = 80;
	conf->server_name = "shortfin/0.9.5";
	conf->doc_root = "/var/www/";
	conf->index_file = "index.html";
	conf->daemonize = 1;
	
	conf->cache_files = CACHE_YES;		/* cache responses */
	conf->cache_other = 0;
	conf->cache_max_fds = 30000;		/* 30 000 open file descriptors */
	conf->cache_memory_size = 300000000; 	/* 300 Mb */
	conf->cache_turn_off_limit = 295000000;
	conf->cache_dir = "/var/www/cache/";
	
	conf->keep_alive = 1;
	conf->keep_alive_timeout = 5;
	
	conf->read_buffer_size = 4046;
	conf->write_buffer_size = 131072;
	
	conf->tcp_nodelay = 1;

	conf->chroot = "";
	
	conf->default_server = 0;
	conf->hostname = "";
	
	conf->access_log_path = "";
	conf->error_log_path = "";
	conf->access_log = 0;
	conf->error_log = 0;
	
	return 0;
}

config *config_init() {
	/* init config */
	config *conf = malloc(sizeof(config));

	conf->proxies = malloc(100 * sizeof(proxy*)); /* maximum 100 proxies per server */
	conf->proxy_count = 0;

	/* load default */
	config_load_default (conf);

	return conf;
}

int config_copy(config *dest, config *source) {
	/* copy a config to another */
	
	memcpy (dest, source, sizeof(config));
	
	/* copy all pointer values */
	config_save_value ("server-name", source->server_name, dest, NULL);
	config_save_value ("doc-root", source->doc_root, dest, NULL);
	config_save_value ("index-file", source->index_file, dest, NULL);
	config_save_value ("chroot", source->chroot, dest, NULL);
	config_save_value ("hostname", source->hostname, dest, NULL);
	config_save_value ("access-log", source->access_log_path, dest, NULL);
	config_save_value ("error-log", source->error_log_path, dest, NULL);
	config_save_value ("cache-dir", source->cache_dir, dest, NULL);
	
	return 0;
}

int config_load(char *filename, config *conf) {
	/* load configuration from file */
	
	FILE *file = NULL;
	char line[4048];
	int len, i;
 	
	if ((file = fopen(filename, "r")) == NULL) {
		perror ("ERROR opening config file");
		return -1;
	}
     
     	int is_key, is_value, next;
     	
     	char key[512];
     	char value[4096];
     	
     	int key_len = 0;
     	int value_len = 0;
     	
     	int out_of_scope = 0;
     	
	while (fgets(line, sizeof(line), file) != NULL) {  
		if ((len = strlen(line)) > 1) {
			is_key = 1;
			is_value = 0;
			next = 0;
			
			key_len = 0;
			value_len = 0;
		
			for (i = 0; i < len; ++i) {
				switch (line[i]) {
					case '#':
						/* comment */
						next = 1;	
					break;	
					
					case '=':
						if (is_key) {
							key[key_len] = '\0';
							is_key = 0;
							is_value = 1;
						}
					break;
					
					case '\t':
					case '\r':
					case '\n':
					break;
					
					case '{':
						out_of_scope = 1;
					break;
					
					case '}':
						if (out_of_scope) {
							out_of_scope = 0;
						}
					break;
					
					default:
						if (is_key) {
							if (line[i] != ' ') {
								key[key_len] = line[i];
								++key_len;
							}
						}
						else if (is_value) {
							if (line[i] != ' ' || value_len != 0) {
								value[value_len] = line[i];
								++value_len;
							}
						}
					break;
				}
				
				if (next) {
					break;
				}
			}
			
			if (!out_of_scope) {
				value[value_len] = '\0';
			
				if (key_len > 0 && value_len > 0) {
					/* save the value */
					config_save_value (key, value, conf, NULL);
				}
			}
		}
	}

	fclose (file);

	/* validate */
	if (conf->cache_turn_off_limit == 0 || conf->cache_turn_off_limit > conf->cache_memory_size) {
		/* invalid cache turn-off limit */
		conf->cache_turn_off_limit = conf->cache_memory_size * 0.90;
	}

	return 0;
}

int config_load_servers(char *filename, master_server *master_srv) {
	/* load servers from config file */
	config *conf = NULL;
	
	FILE *file = NULL;
	char line[4048];
	int len, i;
 	
	if ((file = fopen(filename, "r")) == NULL) {
		perror ("ERROR opening config file");
		return -1;
	}
     
     	int is_key, is_value, next;
     	
     	char key[128];
     	char value[2048];
     	
     	int key_len = 0;
     	int value_len = 0;
     	
     	int out_of_scope = 1;
 	int count = 0;
     	
     	char *scope = "none";
     	
	while (fgets(line, sizeof(line), file) != NULL) {  
		if ((len = strlen(line)) > 1) {
			is_key = 1;
			is_value = 0;
			next = 0;
			
			key_len = 0;
			value_len = 0;
			
			for (i = 0; i < len; ++i) {
				switch (line[i]) {
					case '#':
						/* comment */
						next = 1;	
					break;	
					
					case '=':
						if (is_key) {
							key[key_len] = '\0';
							is_key = 0;
							is_value = 1;
						}
					break;
					
					case '\t':
					case '\r':
					case '\n':
					break;
					
					case '{':
						if (key_len == 6) {
							if (strcmp(key, "server") == 0) {
								out_of_scope = 0;
								next = 1;
								
								/* create a new server */
								master_srv->servers[count] = malloc(sizeof(server));
								server *srv = master_srv->servers[count];
		
								/* copy global config */
								srv->config = config_init();
								config_copy (srv->config, master_srv->config);
								
								conf = srv->config;
								++count;
								
								scope = "server";
							}
						}
						else if (key_len == 5) {
							if (strcmp(key, "proxy") == 0) {
								out_of_scope = 0;
								next = 1;
								
								/* create a new proxy */
								conf->proxies[conf->proxy_count] = proxy_init();
								conf->proxy_count++;
								
								scope = "proxy";
							}
						}
					break;
					
					case '}':
						if (!out_of_scope) {
							out_of_scope = 1;
							scope = "none";
						}
					break;
					
					default:
						if (is_key) {
							if (line[i] != ' ') {
								key[key_len] = line[i];
								++key_len;
							}
						}
						else if (is_value) {
							if (line[i] != ' ' || value_len != 0) {
								value[value_len] = line[i];
								++value_len;
							}
						}
					break;
				}
				
				if (next) {
					break;
				}
			}
			
			if (!out_of_scope) {
				value[value_len] = '\0';
			
				if (key_len > 0 && value_len > 0) {
					if (conf != NULL) {
						/* save the value */
						config_save_value (key, value, conf, scope);
				
						if (strcmp(key, "default") == 0) {
							/* found default server */
							if (atoi(value) == 1) {
								master_srv->server_default = count-1;
							}
						}
					}
				}
			}
		}
	}

	fclose (file);

	/* validate */
	if (conf->cache_turn_off_limit == 0 || conf->cache_turn_off_limit > conf->cache_memory_size) {
		/* invalid cache turn-off limit */
		conf->cache_turn_off_limit = conf->cache_memory_size * 0.90;
	}

	master_srv->server_count = count;

	return 0;
}

int config_save_value(char *key, char *value, config *conf, char *scope) {
	/* save config value */
	
	if (scope != NULL) {
		/* proxy settings */
		if (strcmp(scope, "proxy") == 0) {
			if (strcmp(key, "port") == 0) {
				conf->proxies[conf->proxy_count-1]->port = atoi(value);
			}
			else if (strcmp(key, "host") == 0) {
				conf->proxies[conf->proxy_count-1]->host = strdup(value);
			}
			else if (strcmp(key, "cache") == 0) {
				conf->proxies[conf->proxy_count-1]->cache = atoi(value);
			}
			else if (strcmp(key, "new-rule") == 0) {
				/* add a new rule */
				proxy_add_rule (conf->proxies[conf->proxy_count-1], value);
			}
		
			return 0;
		}
	}
	
	if (strcmp(key, "listen-port") == 0) {
		conf->listen_port = atoi(value);
	}
	else if (strcmp(key, "max-workers") == 0) {
		conf->max_workers = atoi(value);
	}
	else if (strcmp(key, "child-stack-size") == 0) {
		conf->child_stack_size = atoi(value);
	}
	else if (strcmp(key, "max-pending") == 0) {
		conf->max_pending = atoi(value);
	}
	else if (strcmp(key, "max-clients") == 0) {
		conf->max_clients = atoi(value);
	}
	else if (strcmp(key, "server-name") == 0) {
		conf->server_name = strdup(value);
	}
	else if (strcmp(key, "doc-root") == 0) {
		conf->doc_root = strdup(value);
	}
	else if (strcmp(key, "index-file") == 0) {
		conf->index_file = strdup(value);
	}
	else if (strcmp(key, "daemonize") == 0) {
		conf->daemonize = atoi(value);
	}
	else if (strcmp(key, "cache-files") == 0) {
		if (strcmp(value, "yes") == 0) {
			conf->cache_files = CACHE_YES;
		}
		else if (strcmp(value, "no") == 0) {
			conf->cache_files = CACHE_NONE;
		}
		else if (strcmp(value, "fd") == 0) {
			conf->cache_files = CACHE_FD;
		}
	}
	else if (strcmp(key, "cache-other") == 0) {
		conf->cache_other = atoi(value);
	}
	else if (strcmp(key, "cache-max-fds") == 0) {
		conf->cache_max_fds = atoi(value);
	}
	else if (strcmp(key, "cache-memory-size") == 0) {
		conf->cache_memory_size = atol(value);
	}
	else if (strcmp(key, "cache-turn-off-limit") == 0) {
		conf->cache_turn_off_limit = atol(value);
	}
	else if (strcmp(key, "cache-dir") == 0) {
		conf->cache_dir = strdup(value);
	}
	else if (strcmp(key, "read-buffer-size") == 0) {
		conf->read_buffer_size = atoi(value);
	}
	else if (strcmp(key, "write-buffer-size") == 0) {
		conf->write_buffer_size = atoi(value);
	}
	else if (strcmp(key, "tcp-nodelay") == 0) {
		conf->tcp_nodelay = atoi(value);
	}
	else if (strcmp(key, "chroot") == 0) {
		conf->chroot = strdup(value);
	}
	else if (strcmp(key, "hostname") == 0) {
		conf->hostname = strdup(value);
	}
	else if (strcmp(key, "access-log") == 0) {
		conf->access_log_path = strdup(value);
		conf->access_log = 1;
	}
	else if (strcmp(key, "error-log") == 0) {
		conf->error_log_path = strdup(value);
		conf->error_log = 1;
	}
	else if (strcmp(key, "keep-alive") == 0) {
		conf->keep_alive = atoi(value);
	}
	else if (strcmp(key, "keep-alive-timeout") == 0) {
		conf->keep_alive_timeout = atoi(value);
	}
	else if (strcmp(key, "heartbeat-interval") == 0) {
		conf->heartbeat_interval = atoi(value);
	}
	else if (strcmp(key, "default") == 0) {
		conf->default_server = atoi(value);
	}
	else {
		/* key not found */
		printf (" * Config key '%s' is uknown.\n", key);
	}
	
	return 0;
}

