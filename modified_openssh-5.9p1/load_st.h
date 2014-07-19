#ifndef LOAD_ST_H
#define LOAD_ST_H

typedef struct _stpair{
	int size;
	int time;
	// sync_msg = 0 -- no sync
	// sync_msg = 1 -- wait
	// sync_msg = 2 -- done
	// sync_msg = 3 -- wait and done
//	int sync_msg;
	unsigned int sprob;	
// expect to receive this much data for the previous burst (opposite direction)
	unsigned long long expected;	
}STPAIR;

typedef struct _st{
	int clusterid;
	int tracelen;
	STPAIR* pairs;
}ST;

ST** load_st(const char* st, int ccnt);
void print_single_st(ST** straces, int cluster_id);
void print_st(ST** straces, int total_clusters);
void change_to_relative_time(ST** straces, int total_clusters);
void split(ST** straces, int total_clusters, int send_pos);
void free_st(ST*** pstraces, int total_clusters);

#endif
