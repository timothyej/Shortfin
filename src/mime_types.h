#ifndef _MIME_TYPES_H_
#define _MIME_TYPES_H_

#include "base.h"

cache *mime_types_init();
int mime_typed_add(char *ext, char *mime, cache *c);
char *mime_types_get(char *ext, int len, cache *c);
char *mime_types_get_by_filename(char *filename, int len, cache *c);
int mime_types_free(cache *c);

#endif
