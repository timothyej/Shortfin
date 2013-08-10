#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

#include "base.h"

int config_copy(config *dest, config *source);
config *config_init();
int config_load(char *filename, config *conf);
int config_load_servers(char *filename, master_server *master_srv);
int config_save_value(char *key, char *value, config *conf, char *scope);

#endif
