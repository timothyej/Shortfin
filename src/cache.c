#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "cache.h"


/* Init cache */
int cache_init(cache* c) {
    	c->used = 0;
    	c->max = 256;
    	c->size = 256;
    	c->nodes = malloc(sizeof(cnode) * c->size);

	int i;
	for (i = 0; i < c->size; ++i) {
		c->nodes[i] = NULL;
	}

    	return 0;
}


/* Init cache node */
static int cache_init_node(cnode *n, int size) {
   	n->used = 0;
   	n->value = NULL;
    	n->nodes = malloc(sizeof(cnode) * size);
    
	int i;
	for (i = 0; i < size; ++i) {
		n->nodes[i] = NULL;
	}

    	return 0;
}

/* create new node */
cnode *cache_create_node() {
   	cnode *newnode = calloc(1, sizeof(*newnode));
     	cache_init_node(newnode, 256);
     
	return newnode;
}


/* Add a new entry to the cache */
cnode *cache_add(cache *c, char* id, void* value) {
     	int len = strlen(id);
	unsigned int asc = (unsigned int) id[len-1] < 256 ? id[len-1] : 255;
	cnode *node; /* Current node */

	/* Create a new node */
	if (c->nodes[asc] == NULL) {
		c->nodes[asc] = cache_create_node();
	}

	/* Assign the start node */
	node = c->nodes[asc];
	node->used = 0;

	int i = len-1;
	while ((--i) > -1) {
		asc = (unsigned int) id[i] < 256 ? id[i] : 255;
	
		/* Create a new node */
		if (node->nodes[asc] == NULL) {
	    		node->nodes[asc] = cache_create_node();
	    		node->used = 0;
		}

		node = node->nodes[asc];
	}

	/* Save the value */
	node->value = value;

	return node;
}


/* Get cache entry */
void* cache_get(cache *c, char* id, int len) {
    	cnode *node = c->nodes[ (unsigned int)id[len-1]<256?id[len-1]:255 ];

    	int i = len-1;
	while ((--i) > -1) {
        	node = node->nodes[ (unsigned int)id[i]<256?id[i]:255 ];
    	}

	return node->value;
}

void* cache_get_exists(cache *c, char* id, int len) {
    	cnode *node = c->nodes[ (unsigned int)id[len-1]<256?id[len-1]:255 ];

	if (node == NULL)
       		return NULL;

    	int i = len-1;
	while ((--i) > -1) {
        	node = node->nodes[ (unsigned int)id[i]<256?id[i]:255 ];
        	
        	if (node == NULL)
        	     return NULL;
    	}

	return node->value;
}

cnode* cache_get_node(cache *c, char* id, int len) {
    	cnode *node = c->nodes[ (unsigned int)id[len-1]<256?id[len-1]:255 ];

    	int i = len-1;
	while ((--i) > -1) {
        	node = node->nodes[ (unsigned int)id[i]<256?id[i]:255 ];
    	}

	return node;
}

/* Check if cache entry exists */
int cache_exists(cache *c, char* id, int len) {
    	cnode *node = c->nodes[ (unsigned int)id[len-1]<256?id[len-1]:255 ];

     	if (node == NULL)
        	return 1;

    	int i = len-1;
	while ((--i) > -1) {
        	node = node->nodes[ (unsigned int)id[i]<256?id[i]:255 ];
        	
        	if (node == NULL)
        	     return 1;
    	}

	return 0;
}

/* Free cache */
int cache_free(cache *c) {
	int i;
	for (i = 0; i < 256; ++i) {
		if (c->nodes[i] != NULL) {
			cache_free_nodes (c->nodes[i]);	
		}
	}

	free (c->nodes);
	free (c);
	return 0;
}

/* Free all nodes */
int cache_free_nodes(cnode *n) {
	int i;
	for (i = 0; i < 256; ++i) {
		if (n->nodes[i] != NULL) {
			cache_free_nodes (n->nodes[i]);	
		}
	}

	if (n->value != NULL) {
		free (n->value);
	}

	free (n->nodes);
	free (n);
	return 0;
}
