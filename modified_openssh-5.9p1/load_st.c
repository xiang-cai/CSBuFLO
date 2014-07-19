#include "load_st.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ST** load_st(const char* st, int ccnt){
	FILE *fst;
	fst = NULL;
	ST** straces = NULL;

	int i, j, stsize, clusterid;
	int cur_val;
	unsigned int prob;
	char tmp;

	if((fst = fopen(st, "r")) == NULL){
		fprintf(stderr, "%s open error.\n", st);
		goto exit;
	}

	// straces[0] is not used
	straces = (ST**)malloc(sizeof(ST*) * (ccnt+1));
	
	//read traces
	for(i = 1; i <= ccnt; i++){
		// read st
		if(fscanf(fst, "%d,%d,\n", &clusterid, &stsize) != 2 || clusterid <= 0 || stsize < 0){
			fprintf(stderr, "incorrect st file format or invalid value!\n");
			goto exit;
		}
		
		straces[clusterid] = (ST*)malloc(sizeof(ST));
		straces[clusterid]->clusterid = clusterid;
		straces[clusterid]->tracelen = stsize;
		straces[clusterid]->pairs = (STPAIR*)malloc(sizeof(STPAIR) * stsize);

		// followed by packet sizes
		for(j = 0; j < stsize; j++){
			if(fscanf(fst, "%d,", &cur_val) != 1){
				fprintf(stderr, "incorrect st file format!\n");
				goto exit;
			}
			(straces[clusterid]->pairs)[j].size = cur_val;
		}
		fscanf(fst, "%c", &tmp);	// read \n at the end
		
		// followed by packet times
		for(j = 0; j < stsize; j++){
			if(fscanf(fst, "%d,", &cur_val) != 1){
				fprintf(stderr, "incorrect st file format!\n");
				goto exit;
			}
			(straces[clusterid]->pairs)[j].time = cur_val;
			(straces[clusterid]->pairs)[j].expected = 0;
		}
		fscanf(fst, "%c", &tmp);	// read \n at the end
		
		// followed by packet stop probabilities
		for(j = 0; j < stsize; j++){
			if(fscanf(fst, "%d,", &prob) != 1){
				fprintf(stderr, "incorrect st file format!\n");
				goto exit;
			}
			(straces[clusterid]->pairs)[j].sprob = prob;
		}
		fscanf(fst, "%c", &tmp);	// read \n at the end
	}

exit:
	if(fst)
		fclose(fst);
	return straces;
}

void print_single_st(ST** straces, int i){
	int j;
	if(!straces || i <= 0)
		return;
	FILE* fp = fopen("/tmp/st_merged.txt", "w");

	for(j = 0; j < straces[i]->tracelen; j++){
		if((straces[i]->pairs)[j].size == 0)
			continue;
		fprintf(fp, "%10d, %10d\n", (straces[i]->pairs)[j].size,(straces[i]->pairs)[j].expected);
	}

	fclose(fp);
}

void print_st(ST** straces, int total_clusters){
	int i,j;
	if(!straces)
		return;
	for(i = 1; i <= total_clusters; i++){
		printf("cluster %d\n", straces[i]->clusterid);
		printf("packet sizes:\n");
		for(j = 0; j < straces[i]->tracelen; j++){
			printf("%d,", (straces[i]->pairs)[j].size);
		}
		printf("\n");
		
		printf("packet times:\n");
		for(j = 0; j < straces[i]->tracelen; j++){
			printf("%d,", (straces[i]->pairs)[j].time);
		}
		printf("\n");
	
		printf("packet stop prob:\n");
		for(j = 0; j < straces[i]->tracelen; j++){
			printf("%d,", (straces[i]->pairs)[j].sprob);
		}
		printf("\n");
	
		for(j = 0; j < straces[i]->tracelen; j++){
			printf("%lld,", (straces[i]->pairs)[j].expected);
		}
		printf("\n");
	}
}

void refresh_trace(STPAIR* trace, int len){
	int s, e;
	s = e = 0;
	unsigned long long stime;
	int ssize;
	while(e < len){
		s = e;
		stime = trace[s].time;
		ssize = trace[s].size;
		e++;
		while(e < len){
			if((trace[e].size < 0 && ssize > 0) || (trace[e].size > 0 && ssize < 0))
				break;
			if(trace[e].time - stime >= 200000)
				break;
			trace[s].size += trace[e].size;
			trace[s].time = trace[e].time;
			trace[s].sprob = trace[e].sprob;
			trace[e].size = trace[e].time = trace[e].sprob = 0;
			e++;
		}
	}
}


void change_to_relative_time(ST** straces, int total_clusters){
	// the following timestamps are relative intevals to time [0]
	int i,j;
	for(i = 1; i <= total_clusters; i++){
		for(j = straces[i]->tracelen-1; j >= 0; j--){
			(straces[i]->pairs)[j].time -= (straces[i]->pairs)[0].time;
		}
		refresh_trace(straces[i]->pairs, straces[i]->tracelen);
	}
}

void split(ST** straces, int total_clusters, int send_pos){
	int i,j,k,pre_valid_index;
	int pos_pre_index, neg_pre_index, curtime, cursize;
	unsigned long long sum = 0;
	unsigned long long last_valid_time = 0;

	pos_pre_index = neg_pre_index = pre_valid_index = -1;

	for(i = 1; i <= total_clusters; i++){
		pos_pre_index = neg_pre_index = pre_valid_index = -1;
		sum = 0;

		for(j = 0; j < straces[i]->tracelen; j++){
			curtime = (straces[i]->pairs)[j].time;
			cursize = (straces[i]->pairs)[j].size;

			if(cursize > 0){
				// postive packets
				if(pos_pre_index >= 0){
					(straces[i]->pairs)[pos_pre_index].time = curtime - (straces[i]->pairs)[pos_pre_index].time;
				}
				pos_pre_index = j;

//				if(send_pos == -1)
//					continue;
				if(pre_valid_index >= 0 && (straces[i]->pairs)[pre_valid_index].size < 0){
					// direction change
					(straces[i]->pairs)[j].expected = sum;
					sum = cursize;
				}
				else{
					sum += cursize;	
				}
				pre_valid_index = j;
			}
			else if(cursize < 0){
				// negative packets
				if(neg_pre_index >= 0){
					(straces[i]->pairs)[neg_pre_index].time = curtime - (straces[i]->pairs)[neg_pre_index].time;
				}
				neg_pre_index = j;

//				if(send_pos == 1)
//					continue;
				if(pre_valid_index >= 0 && (straces[i]->pairs)[pre_valid_index].size > 0){
					// direction change
					(straces[i]->pairs)[j].expected = sum;
					sum = 0 - cursize;
				}
				else{
					sum -= cursize;
				}
				pre_valid_index = j;
			}
			else
				;
		}

	//	(straces[i]->pairs)[pos_pre_index].time = (straces[i]->pairs)[neg_pre_index].time = 0;
		for(k = straces[i]->tracelen-1; k >= 0; k--){
			if((straces[i]->pairs)[k].time > 0){
				last_valid_time = (straces[i]->pairs)[k].time;
				break;
			}
		}
		(straces[i]->pairs)[pos_pre_index].time = last_valid_time - (straces[i]->pairs)[pos_pre_index].time;
		(straces[i]->pairs)[neg_pre_index].time = last_valid_time - (straces[i]->pairs)[neg_pre_index].time;
	}	
}

void free_st(ST*** pstraces, int total_clusters){
	int i;
	if(!pstraces)
		return;
	ST** straces = *pstraces;
	// free memory
	for(i = 1; i <= total_clusters; i++){
		free(straces[i]->pairs);
	}
	free(straces);
	*pstraces = NULL;
}

/*
int main(){
	int i;
	int total_clusters = 2;
	ST** straces = load_st("./test_st.txt", total_clusters);

	print_st(straces, total_clusters);

	change_to_relative_time(straces, total_clusters);
	print_st(straces, total_clusters);

	split(straces, total_clusters, -1);
	print_st(straces, total_clusters);
	
	free_st(&straces, total_clusters);
	print_st(straces, total_clusters);
	return 0;
}
*/
