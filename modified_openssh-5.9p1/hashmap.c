#include "hashmap.h"
#include "includes.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint32_t default_hash(char* key){
	/*
	 * Simple Bob Jenkins's hash algorithm taken from the
	 * wikipedia description.
	 */

	uint32_t hash = 0;
	char* p;
	// note, we assume the end of an url is a
	// space char ' ' (exclusive)
	if(!key)
		return 0;
	for(p = key; *p != ' ' && *p != '\0'; p++){
		hash += *p;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

int hm_lookup(hashmap hm, char* key){
	// return value for a given key
	// return -1 if key does not exist
	int bucket, i;
	kvpair* p = NULL;
	u_char tmpkey[255+2] = {0};
	DEB_PRINT("DEB: inside hm_lookup\n");
	
	if(!hm || !key)
		return -1;
	for(i = 0; key[i] != ' ' && key[i] != '\0'; i++){
		tmpkey[i] = key[i];
	}
	tmpkey[i] = '\0';

	DEB_PRINT("DEB: hash key = %s\n", tmpkey);
	bucket = default_hash(tmpkey) % HMSIZE;
	DEB_PRINT("DEB: bucket = %d\n", bucket);
	for(p = hm[bucket]; p != NULL; p = p->next){
		if(strcmp(tmpkey, p->key) == 0)
			return p->value;
	}
	return -1;
}

int hm_insert(hashmap hm, char* key, int val){
	// compute value based on key, and 
	// insert to hashmap hm
	// return 0 on success
		
	int bucket;
	kvpair* tmp;
	tmp = NULL;

	DEB_PRINT("DEB: inside hm_insert\n");
	if(!hm)
		return -1;
	if(-1 != hm_lookup(hm, key))
		return 0;	// <k,v> already exists

	//create the kvpair
	tmp = (kvpair*)malloc(sizeof(kvpair));
	strncpy(tmp->key, key, MAXSTRLEN);
	tmp->value = val;
	tmp->next = NULL;

	bucket = default_hash(key) % HMSIZE;
	if(!hm[bucket])
		hm[bucket] = tmp;
	else{
		tmp->next = hm[bucket];
		hm[bucket] = tmp;
	}
	return 0;
}

hashmap hm_build(char* fname, int* ccnt){
	FILE* fp = NULL;
	int i, webcnt, clusterid;
	char url[MAXSTRLEN];

	DEB_PRINT("DEB: inside hm_build\n");
	
	hashmap ret = (hashmap)malloc(sizeof(kvpair*) * HMSIZE);
	for(i = 0; i < HMSIZE; i++)
		ret[i] = NULL;
	
	if((fp = fopen(fname, "r")) == NULL){
		fprintf(stderr, "cannot open %s\n", fname);
		goto exit;
	}

	if(fscanf(fp, "%d,%d,\n", &webcnt, ccnt) != 2 || webcnt < 0 || *ccnt < 0){
		fprintf(stderr, "incorrect cluster file format or invalid value! 1\n");
		goto exit;
	}

	for(i = 1; i <= webcnt; i++){
		if(fscanf(fp, "%s %d,\n", url, &clusterid) != 2 || clusterid <= 0){
			fprintf(stderr, "incorrect cluster file format or invalid value! round %d\n", i);
			fprintf(stderr, "url: %s , clusterid: %d\n", url, clusterid);
			goto exit;
		}
		// insert <url, clusterid> to hashmap
		if(hm_insert(ret, url, clusterid) < 0){
			fprintf(stderr, "error inserting to hashmap!\n");
			goto exit;
		}
	}
	
exit:
	if(fp)
		fclose(fp);
	return ret;
}


void hm_free(hashmap* phm){
	if(!phm)
		return;
	hashmap hm = *phm; 
	kvpair *p, *next;
	int i;
	for(i = 0; i < HMSIZE; i++){
		if(hm[i]){
			for(p = hm[i]; p != NULL; p = next){
				next = p->next;
				free(p);
			}
		}
	}
	free(hm);
	*phm = NULL;
}



void hm_print(hashmap hm){
	kvpair* p;
	int i;
	if(!hm)
		return;
	for(i = 0; i < HMSIZE; i++){
		if(hm[i]){
			for(p = hm[i]; p != NULL; p = p->next){
				printf("%s --> cluster %d\n", p->key, p->value);
			}
		}
	}
}

/*
int main(){
	hashmap hm = hm_build("/tmp/test.txt");
	
	printf("www.google.com --> cluster %d\n", hm_lookup(hm, "www.google.com"));
	printf("www.facebook.com --> cluster %d\n", hm_lookup(hm, "www.facebook.com"));
	printf("www.weather.gov --> cluster %d\n", hm_lookup(hm, "www.weather.gov"));
	printf("pastbin.com --> cluster %d\n", hm_lookup(hm, "pastebin.com"));
	printf("astin.com --> cluster %d\n", hm_lookup(hm, "astin.com"));
	hm_free(&hm);
	printf("pt1\n");
	hm_print(hm);
	return 0;
}
*/
