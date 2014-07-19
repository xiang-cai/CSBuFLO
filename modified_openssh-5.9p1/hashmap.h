#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>

#define HMSIZE 5000
#define MAXSTRLEN 500

typedef struct _kvpair{
	char key[MAXSTRLEN];
	int value;
	struct _kvpair* next;
}kvpair;

typedef kvpair** hashmap;

uint32_t default_hash(char* key);
int hm_lookup(hashmap hm, char* key);
int hm_insert(hashmap hm, char* key, int val);
hashmap hm_build(char* fname, int* total_clusters);
void hm_free(hashmap* phm);
void hm_print(hashmap hm);

#endif
