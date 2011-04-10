#include "proxy.h"

proxy *proxy_init() {
	/* init a new proxy config */
	proxy *p = malloc(sizeof(proxy));
	
	p->rules = malloc(500 * sizeof(proxy_rule*)); /* max 500 rules per proxy config */
	p->rule_count = 0;
	p->host = NULL;
	p->port = 0;
	p->cache = 0;
	
	return p;
}

int proxy_match_request(server *srv, request *req, connection *conn) {
	/* check if a proxy matches the request */
	
	
	
	return 0;
}

int proxy_add_rule(proxy *p, char *value) {
	/* adding a new rule to a proxy config */
	int i;
	int offset = 0;
	int arg_count = 0;
	
	/* create a new rule */
	p->rules[p->rule_count] = malloc(sizeof(proxy_rule));
	p->rules[p->rule_count]->type = PROXY_RULE_UNKNOWN;
	p->rules[p->rule_count]->value = NULL;
	
	/* find all arguments in the 'value' */
	for (i = 0; i < strlen(value); ++i) {
		if ((value[i] == ' ' && value[i+1] != ' ') || i == strlen(value)-1) {
			if (i == strlen(value)-1) {
				++i;
			}
		
			/* get the argument */
			char *arg = malloc(i-offset+1);
			memcpy (arg, value+offset, i-offset);
			arg[i-offset] = '\0';
			
			if (!(strlen(arg) == 1 && arg[0] == ' ')) {
				if (arg_count == 0) {
					/* type of rule */
					if (strcmp(arg, "ext") == 0) {
						p->rules[p->rule_count]->type = PROXY_RULE_EXT;
					}
					else if (strcmp(arg, "dir") == 0) {
						p->rules[p->rule_count]->type = PROXY_RULE_DIR;
					}
					else if (strcmp(arg, "file") == 0) {
						p->rules[p->rule_count]->type = PROXY_RULE_FILE;
					}
					else if (strcmp(arg, "ip") == 0) {
						p->rules[p->rule_count]->type = PROXY_RULE_IP;
					}
				} else {
					/* value */
					p->rules[p->rule_count]->value = malloc(strlen(arg)+1);
					memcpy (p->rules[p->rule_count]->value, arg, strlen(arg)+1);
					break;
				}
				
				arg_count++;
			}
			
			if (value[i] == ' ') {
				++i;
				offset = i;
			}
			
			free (arg);
		}
	}
	
	/* the rule has been added */
	++p->rule_count;
	
	return 0;
}

int proxy_free(proxy *p) {
	/* should free the proxy and all its rules */
	
	return 0;
}
