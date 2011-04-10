#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "cache.h"


/* Init cache */
int cache_init(cache* c) {
    	c->used = 0;
    	c->max = 255;
    	c->size = 255;
    	c->nodes = malloc(sizeof(cnode) * c->size);

    	return 0;
}


/* Init cache node */
int cache_init_node(cnode *n, int size) {
   	n->used = 0;
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
     	cache_init_node(newnode, 255);
     
	return newnode;
}


/* Add a new entry to the cache */
cnode *cache_add(cache *c, char* id, void* value) {
     	int len = strlen(id);
	int asc = id[len-1];
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
		asc = id[i];
	
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
    	cnode *node = c->nodes[ id[len-1] ];

    	int i = len-1;
	while ((--i) > -1) {
        	node = node->nodes[id[i]];
    	}

	return node->value;
}

void* cache_get_exists(cache *c, char* id, int len) {
    	cnode *node = c->nodes[ id[len-1] ];

	if (node == NULL)
       		return NULL;

    	int i = len-1;
	while ((--i) > -1) {
        	node = node->nodes[id[i]];
        	
        	if (node == NULL)
        	     return NULL;
    	}

	return node->value;
}

cnode* cache_get_node(cache *c, char* id, int len) {
    	cnode *node = c->nodes[ id[len-1] ];

    	int i = len-1;
	while ((--i) > -1) {
        	node = node->nodes[id[i]];
    	}

	return node;
}

/* Check if cache entry exists */
int cache_exists(cache *c, char* id, int len) {
    	cnode *node = c->nodes[ id[len-1] ];

     	if (node == NULL)
        	return 1;

    	int i = len-1;
	while ((--i) > -1) {
        	node = node->nodes[id[i]];
        	
        	if (node == NULL)
        	     return 1;
    	}

	return 0;
}

/* Free cache */
int cache_free(cache *c) {
	free (c);
	return 0;
}
