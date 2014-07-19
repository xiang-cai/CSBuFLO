/* $OpenBSD: misc.c,v 1.85 2011/03/29 18:54:17 stevesk Exp $ */
/*
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
 * Copyright (c) 2005,2006 Damien Miller.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/param.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#ifdef HAVE_PATHS_H
# include <paths.h>
#include <pwd.h>
#endif
#ifdef SSH_TUN_OPENBSD
#include <net/if.h>
#endif

#include "xmalloc.h"
#include "misc.h"
#include "log.h"
#include "ssh.h"
#include "hashmap.h"

#include <math.h>

/* Modified: */

/* Modified: global var */
static int stmode = -1;	// -1: ssh 0: csbuflo; 1: supertrace;
static int cluster_id = -1;

static int padded_junk = 0;
static unsigned long long ts_end = 0;
static unsigned long long ts_start = 0; // gap interval
static int tstart_set = 0;
static int tend_set = 0;
static long tau_arr[ARRSIZE];
static int tau_index = 0;
static int tau_size = 0;

static long w2w = 0;

static unsigned long long time_arr[ARRSIZE];
static int time_index = 0;
static int time_size = 0;

static int onload_flag = 0;
static int unload_flag = 0;
static int paddingdone_flag = 0;
//static int trans_early_stop = 0;

static u_char url[255+2];
static int urllen = 0;

static ST** straces = NULL;
static hashmap clusters = NULL;
static int total_clusters = 0;

//static int sync_received = 0;
//static int sync_num = 0;

static int transcript_end = 0;
static int sent_tend = 0;

static int first_client = 1;
static int second_client = 0;
static int first_sent = 0;
static int st_index = -1;
static STPAIR* cur_item = NULL;

static int trans_padded = 0;
static int urlack = 0;

// first direction change in server's transcript
// expected received data
static unsigned long long first_expected = 0;

//static int green_light = 0;

static unsigned long long received_sofar = 0;

static unsigned long long st_idle_start = 0;

static FILE* fportlog = NULL;

static int server_entering_normal = 0;

static char lockname[30] = {'\0'};

static int trans_padded_remaining = 0;


void set_lockname(int port){
	sprintf(lockname, "/tmp/st%d.lock", port);
}

char* get_lockname(){
	return lockname;
}

void set_server_entering_normal(){
	server_entering_normal = 1;
}

void unset_server_entering_normal(){
	server_entering_normal = 0;
}

int get_server_entering_normal(){
	return server_entering_normal;
}

FILE* get_fportlog(){
	return fportlog;
}

void create_log_file(int port){
	if(fportlog)
		return;
	char fname[100];
	sprintf(fname, "/var/tmp/sshlogs/%d.log", port);
	fportlog = fopen(fname, "a");
}

void close_log_file(){
	if(fportlog)
		fclose(fportlog);
}

/*
void set_trans_early_stop(){
	trans_early_stop = 1;
}

void unset_trans_early_stop(){
	trans_early_stop = 0;
}

int get_trans_early_stop(){
	return trans_early_stop;
}
*/

void set_trans_padded_remaining(int val){
	trans_padded_remaining = val;
}

int get_trans_padded_remaining(){
	return trans_padded_remaining;
}

void decrease_trans_padded_remaining(int val){
	if(trans_padded_remaining >= val)
		trans_padded_remaining -= val;
	else
		trans_padded_remaining = 0;
}

void set_trans_padded(){
	trans_padded = 1;
}

void unset_trans_padded(){
	trans_padded = 0;
}

int get_trans_padded(){
	return trans_padded;
}

void set_urlack(){
	urlack = 1;
}

void unset_urlack(){
	urlack = 0;
}

int get_urlack(){
	return urlack;
}

void set_second_client(){
	second_client = 1;
}

void unset_second_client(){
	second_client = 0;
}

int get_second_client(){
	return second_client;
}

void set_first_client(){
	first_client = 1;
}

void unset_first_client(){
	first_client = 0;
}

int get_first_client(){
	return first_client;
}

unsigned long long get_st_idle_start(){
	return st_idle_start;
}

void set_st_idle_start(unsigned long long val){
	st_idle_start = val;
}

unsigned long long get_first_expected(){
	return first_expected;
}

int get_sent_tend(){
	return sent_tend;
}

void set_sent_tend(){
	sent_tend = 1;
}

void unset_sent_tend(){
	sent_tend = 0;
}

int get_transcript_end(){
	return transcript_end;
}

void set_transcript_end(){
	transcript_end = 1;
}

void unset_transcript_end(){
	transcript_end = 0;
}

int get_first_sent(){
	return first_sent;
}

void set_first_sent(){
	// for server side transcript use only
	// find the first outgoing packet sent by server
	// the previous packet in the transcript should be a request packet
	if(cluster_id == -1){
		return;
	}
	int local_st_index = st_index;

	int tracelen = straces[cluster_id]->tracelen;
	if(local_st_index < 0 || local_st_index >= tracelen){
		return;
	}
	STPAIR* cur = straces[cluster_id]->pairs;

	// move to first negetive item
	while(local_st_index < tracelen && cur[local_st_index].size > 0)
		local_st_index++;

	// move to next positive item
	while(local_st_index < tracelen && cur[local_st_index].size < 0)
		local_st_index++;

	if(local_st_index < tracelen){
		first_expected = cur[local_st_index].expected;
		local_st_index--;
		if(local_st_index >= 0){
			first_sent = 1;
		}
	}
}

void unset_first_sent(){
	first_sent = 0;
}
/*
void increase_sync_num(){
	sync_num++;
}

void decrease_sync_num(){
	sync_num--;
}

int get_sync_num(){
	return sync_num;
}

void increase_sync_received(){
	sync_received++;
}

void decrease_sync_received(){
	if(sync_received > 0)
		sync_received--;
}

int get_sync_received(){
	return sync_received;
}
*/
int get_padded_junk(){
	return padded_junk;
}

void set_padded_junk(int val){
	padded_junk = val;
}

/*
void set_green_light(){
	green_light = 1;
}

void unset_green_light(){
	green_light = 0;
}

int get_green_light(){
	return green_light;
}
*/
long get_w2w(){
	return w2w;
}

void set_w2w(long val){
	if(val < 0)
		w2w = 0;
	else
		w2w = val;
}

unsigned long long get_received_sofar(){
	return received_sofar;
}

void set_received_sofar(unsigned long long val){
	received_sofar = val;
}

int should_jump_trans_end(unsigned int rand){
	if(cluster_id < 0 || !cur_item){
		return 1;
	}
	if(get_transcript_end() == 1)
			return 1;

	unsigned int p = cur_item->sprob;
	if((rand %100) < p){
		cur_item = NULL;
		st_index = -1;
		return 1;
	}
	return 0;
}

STPAIR* get_current_st_item(){
	return cur_item;
}

void move_to_next_st_item(int is_pos){
	DEB_PRINT("DEB: inside move_to_next_st_item\n");
	if(cluster_id < 0){
		DEB_PRINT("DEB: cluster_id < 0\n");
		return;
	}
	int tracelen = straces[cluster_id]->tracelen;
	DEB_PRINT("DEB: tracelen is %d\n", tracelen);
	if(st_index < 0 || st_index >= tracelen){
		DEB_PRINT("DEB: bef move, st_index = %d\n", st_index);
		cur_item = NULL;
		st_index = -1;
		return;
	}
	STPAIR* cur = straces[cluster_id]->pairs;

	if(is_pos > 0){
		while(st_index < tracelen && cur[st_index].size <= 0)
			st_index++;
	}
	else{
		while(st_index < tracelen && cur[st_index].size >= 0)
			st_index++;
	}

	if(st_index < tracelen){
		cur_item = &(cur[st_index]);
		DEB_PRINT("DEB: aft move, st_index = %d\n", st_index);
		st_index++;
	}
	else{
		cur_item = NULL;
		st_index = -1;
		DEB_PRINT("DEB: aft move, st_index = %d\n", st_index);
	}
}

int get_cluster_id(){
	return cluster_id;
}

void set_cluster_id(char* url, int send_pos){
	
	/* load cluster database and supertrace database */
	if(!clusters){
		DEB_PRINT("DEB: in set_cluster_id, call hm_build\n");
		clusters = hm_build("/var/tmp/clusters.txt", &total_clusters);
	}
	if(!straces){
		straces = load_st("/var/tmp/st.txt", total_clusters);
		change_to_relative_time(straces, total_clusters);
		split(straces, total_clusters, send_pos);
	}

	DEB_PRINT("DEB: in set_cluster_id, call hm_lookup\n");
	cluster_id = hm_lookup(clusters, url);
	if(cluster_id != -1){
		st_index = 0;
	}

	// for tao's trace, always play the only supertrace
//	cluster_id = 1;
//	st_index = 0;

	DEB_PRINT("DEB: in set_cluster_id, after set, id is %d\n", cluster_id);
//	print_single_st(straces, cluster_id);
}

void set_stmode(int mode){
	stmode = mode;
}

int get_stmode(){
	return stmode;
}

int get_urllen(){
	return urllen;
}

u_char* get_url(){
	return url;
}

void set_url(u_char* p, int offset){
	int i;
	
//	DEB_PRINT("DEB: ");
	for(urllen = 0, i = offset; p[i] != ' ' && urllen < 255; i++, urllen++){
		url[urllen] = p[i];
//		DEB_PRINT("%c", p[i]);
	}
//	DEB_PRINT("\n");
	
	url[urllen++] = ' ';
//	DEB_PRINT("%c", p[urllen-1]);
	url[urllen] = '\0';

	if(p[i] != ' '){
		strcpy(url, "www.google.com ");
		url[15] = '\0';
		urllen = 15;
	}
	DEB_PRINT("DEB: urllen is %d, url is: %s\n", urllen, url);
}

unsigned long long get_ts_start(void){
	return ts_start;
}

unsigned long long get_ts_end(void){
	return ts_end;
}

int get_tstart_set(void){
	return tstart_set;
}

int get_tend_set(void){
	return tend_set;
}

void set_ts_start(unsigned long long val){
	ts_start = val;
}

void set_ts_end(unsigned long long val){
	ts_end = val;
}

void set_tstart_set(int val){
	tstart_set = val;
}

void set_tend_set(int val){
	tend_set = val;
}

int get_onload_flag(){
	return onload_flag;
}

int get_unload_flag(){
	return unload_flag;
}

int get_paddingdone_flag(){
	return paddingdone_flag;
}

void set_onload_flag(){
	onload_flag = 1;
}

void unset_onload_flag(){
	onload_flag = 0;
}

void set_unload_flag(){
	unload_flag = 1;
}

void unset_unload_flag(){
	unload_flag = 0;
}

void set_paddingdone_flag(){
	paddingdone_flag = 1;
}

void unset_paddingdone_flag(){
	paddingdone_flag = 0;
}

void insert_time(unsigned long long time){
/*
	if(time_size > ARRSIZE)
		return;

	time_arr[time_index++] = time;
	time_size++;
*/
	time_arr[time_index] = time;
	time_index = (time_index+1)% ARRSIZE;
	time_size++;
}

unsigned long long get_current_time_usecs(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned long long) tv.tv_sec*1000000 + (unsigned long long) tv.tv_usec;
}


int comp_long(const void* p1, const void* p2){
	return *((long*) p1) - *((long*) p2);
}


void update_tau_interval(unsigned long long ts_start, unsigned long long ts_end, long cur_total_realsent_bytes, int is_client){
	long tau;
//	unsigned long long min = LONG_MAX;
	long interval_arr[ARRSIZE];
	int i, size, oldest, pre;

	if(ts_start >= ts_end) // || time_size == 0)
		return;
/*
	DEB_PRINT("DEB: cur_totalreal_sent = %ld\n", cur_total_realsent_bytes);
	if(cur_total_realsent_bytes == 0)
		return;

	tau = floor((ts_end - ts_start) * TARGET_QUEUE_LEN / 1.0 / cur_total_realsent_bytes);
	DEB_PRINT("DEB: total_realsent = %ld, interval_tau = %ld\n", cur_total_realsent_bytes, tau);
*/
	for(i = 0; i < ARRSIZE; i++)
		interval_arr[i] = 0;

	if(cur_total_realsent_bytes > 0)
		insert_time(ts_end);

	if(time_size <= ARRSIZE){
		for(i = 0, size = 0, time_index = time_size-1; time_index > 0; time_index--, i++, size++){
			interval_arr[i] = (long)(time_arr[time_index] - time_arr[time_index-1]);
	//		min = time_arr[time_index] < min ? time_arr[time_index] : min;
		}
	}
	else{
		size = ARRSIZE -1;
		if(time_index == 0)
			pre = ARRSIZE-1;
		else
			pre = time_index-1;

		for(i = 0, oldest = time_index, time_index = pre; time_index != oldest; i++){
			if(time_index == 0)
				pre = ARRSIZE-1;
			else
				pre = time_index-1;
			interval_arr[i] = (long)(time_arr[time_index] - time_arr[pre]);
			time_index = pre;
		}
	}

	//reset time_arr
	time_index = time_size = 0;

//	tau = (long) min;
//	tau = tau == LONG_MAX ? (long)(ts_end-ts_start) : tau;
//	qsort(interval_arr, size, sizeof(long), comp_long);
	
	// pick median
//	tau = interval_arr[size/2];

	//pick max
//	tau = size > 0 ? interval_arr[size-1] : 0;

	//pick min
//	tau = interval_arr[0];

/*
	if(tau == 0){
		if(is_client){
			//client side
			tau = ((long)(ts_end-ts_start) + UPPER_TAU)/2;
		}
		else{
			//server side
			tau = (long)(ts_end-ts_start);
		}
	}
*/
//	tau = tau == 0 ? ((long)(ts_end-ts_start) < LOWER_TAU ? LOWER_TAU : (long)(ts_end-ts_start)) : tau;
//	tau = tau == 0 ? ((long)(ts_end-ts_start) + UPPER_TAU)/2 : tau;
//	tau = tau == 0 ? (long)(ts_end-ts_start) : tau;

/*	
	if(tau < LOWER_TAU)
		tau = LOWER_TAU;
	else if(tau > UPPER_TAU)
		tau = UPPER_TAU;
	else
		;
	
	DEB_PRINT("DEB: ticks = %d, total_realsent = %ld, interval_size = %d, interval_tau = %ld\n", size, cur_total_realsent_bytes, size, tau);
*/

	if(size == 0){
		if(is_client){
			//client side
			tau = ((long)(ts_end-ts_start) + UPPER_TAU)/2;
		}
		else{
			//server side
			tau = (long)(ts_end-ts_start);
		}

		if(tau < LOWER_TAU)
			tau = LOWER_TAU;
		else if(tau > UPPER_TAU)
			tau = UPPER_TAU;
		else
			;

		tau_arr[tau_index] = tau;
		if(tau_size < ARRSIZE)
			tau_size++;
		tau_index = (tau_index + 1) % ARRSIZE;
	}

	for(i = 0; i < size; i++){
		tau = interval_arr[i];
		if(tau == 0){
			if(is_client){
				//client side
				tau = ((long)(ts_end-ts_start) + UPPER_TAU)/2;
			}
			else{
				//server side
				tau = (long)(ts_end-ts_start);
			}
		}

		if(tau < LOWER_TAU)
			tau = LOWER_TAU;
		else if(tau > UPPER_TAU)
			tau = UPPER_TAU;
		else
			;

		tau_arr[tau_index] = tau;
		if(tau_size < ARRSIZE)
			tau_size++;
		tau_index = (tau_index + 1) % ARRSIZE;
	}
	
	DEB_PRINT("DEB: interval_size = %d\n", size);
}



long update_tau_median(long tau, long* boundary, long total_sent_bytes){
	long new_tau;

	//test only, fixed tau value
//	return tau;

	if(total_sent_bytes < *boundary)
		return tau;

	qsort(tau_arr, tau_size, sizeof(long), comp_long);

	// pick median
	new_tau = tau_arr[tau_size/2];

	// pick min
//	new_tau = tau_arr_tmp[0];

	//pick max
//	new_tau = tau_size > 0 ? tau_arr_tmp[tau_size-1] : UPPER_TAU;

//	new_tau =  (new_tau > UPPER_TAU || tau_size == 0) ? UPPER_TAU : new_tau;
//	new_tau =  (new_tau < LOWER_TAU) ? LOWER_TAU : new_tau;

	if(tau_size == 0){
		new_tau = UPPER_TAU;
	}
	else{
		new_tau = (long)pow(2, floor(log2(new_tau/1.0)));
	}

	DEB_PRINT("DEB: boundary = %ld, tau_size = %d, new tau = %ld\n", *boundary, tau_size, new_tau);
	
	tau_size = tau_index = 0;
	*boundary = (*boundary * 2);
	return new_tau;
}

int should_set_writefd(int truly_idle, unsigned long long time_to_write, struct timeval** tvp, struct timeval* tv){
	unsigned long long now = get_current_time_usecs();
	unsigned long long diff;

	if(get_stmode() == 0 && truly_idle){
		DEB_PRINT("DEB: truly_idle, don't set fd\n");
		tv->tv_sec = 5;
		tv->tv_usec = 0;
		*tvp = tv;
		return 0;
	}
	if(time_to_write < now)
		return 1;
	diff = time_to_write - now;
	tv->tv_sec = diff / 1000000;
	tv->tv_usec = diff % 1000000;
	*tvp = tv;
	DEB_PRINT("DEB: time_to_write >= now , don't set fd, new tv_sec %ld, new tv_usec %ld\n", tv->tv_sec, tv->tv_usec);
	return 0;
}

int channel_idle(unsigned long long* now_usecs, int buf_len, int outbuf_remain, unsigned long long* idle_start){
	*now_usecs = get_current_time_usecs();
	//	DEB_PRINT("DEB: outbuf_remain is %d\n", outbuf_remain);
//	if(buf_len > outbuf_remain){
	if(buf_len - outbuf_remain >= 32){
		// not in idle state
		DEB_PRINT("DEB: buf_len > %d, idle_start reset to now\n", outbuf_remain);
		*idle_start = *now_usecs;
	}

	//may be in idle state
	if(*idle_start + 2000000 < *now_usecs){
		DEB_PRINT("DEB: channel is idle for 2 seconds\n");
		return 1;
	}

	if(onload_flag == 1){
		DEB_PRINT("DEB: onload_flag is 1, channel idle\n");
		return 1;
	}

	//here onload_flag == 0
	return 0;
}

/*end of modification*/

/* remove newline at end of string */
char *
chop(char *s)
{
	char *t = s;
	while (*t) {
		if (*t == '\n' || *t == '\r') {
			*t = '\0';
			return s;
		}
		t++;
	}
	return s;

}

/* set/unset filedescriptor to non-blocking */
int
set_nonblock(int fd)
{
	int val;

	val = fcntl(fd, F_GETFL, 0);
	if (val < 0) {
		error("fcntl(%d, F_GETFL, 0): %s", fd, strerror(errno));
		return (-1);
	}
	if (val & O_NONBLOCK) {
		debug3("fd %d is O_NONBLOCK", fd);
		return (0);
	}
	debug2("fd %d setting O_NONBLOCK", fd);
	val |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, val) == -1) {
		debug("fcntl(%d, F_SETFL, O_NONBLOCK): %s", fd,
		    strerror(errno));
		return (-1);
	}
	return (0);
}

int
unset_nonblock(int fd)
{
	int val;

	val = fcntl(fd, F_GETFL, 0);
	if (val < 0) {
		error("fcntl(%d, F_GETFL, 0): %s", fd, strerror(errno));
		return (-1);
	}
	if (!(val & O_NONBLOCK)) {
		debug3("fd %d is not O_NONBLOCK", fd);
		return (0);
	}
	debug("fd %d clearing O_NONBLOCK", fd);
	val &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, val) == -1) {
		debug("fcntl(%d, F_SETFL, ~O_NONBLOCK): %s",
		    fd, strerror(errno));
		return (-1);
	}
	return (0);
}

const char *
ssh_gai_strerror(int gaierr)
{
	if (gaierr == EAI_SYSTEM)
		return strerror(errno);
	return gai_strerror(gaierr);
}

void
set_cork(int fd, int opt)
{

	if (setsockopt(fd, IPPROTO_TCP, TCP_CORK, &opt, sizeof opt) == -1)
		error("setsockopt TCP_CORK: %.100s", strerror(errno));
}

/* disable nagle on socket */
void
set_nodelay(int fd)
{
	int opt;
	socklen_t optlen;

	optlen = sizeof opt;
	if (getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, &optlen) == -1) {
		debug("getsockopt TCP_NODELAY: %.100s", strerror(errno));
		return;
	}
	if (opt == 1) {
		debug2("fd %d is TCP_NODELAY", fd);
		return;
	}
	opt = 1;
	debug2("fd %d setting TCP_NODELAY", fd);
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof opt) == -1)
		error("setsockopt TCP_NODELAY: %.100s", strerror(errno));
}

/* Characters considered whitespace in strsep calls. */
#define WHITESPACE " \t\r\n"
#define QUOTE	"\""

/* return next token in configuration line */
char *
strdelim(char **s)
{
	char *old;
	int wspace = 0;

	if (*s == NULL)
		return NULL;

	old = *s;

	*s = strpbrk(*s, WHITESPACE QUOTE "=");
	if (*s == NULL)
		return (old);

	if (*s[0] == '\"') {
		memmove(*s, *s + 1, strlen(*s)); /* move nul too */
		/* Find matching quote */
		if ((*s = strpbrk(*s, QUOTE)) == NULL) {
			return (NULL);		/* no matching quote */
		} else {
			*s[0] = '\0';
			*s += strspn(*s + 1, WHITESPACE) + 1;
			return (old);
		}
	}

	/* Allow only one '=' to be skipped */
	if (*s[0] == '=')
		wspace = 1;
	*s[0] = '\0';

	/* Skip any extra whitespace after first token */
	*s += strspn(*s + 1, WHITESPACE) + 1;
	if (*s[0] == '=' && !wspace)
		*s += strspn(*s + 1, WHITESPACE) + 1;

	return (old);
}

struct passwd *
pwcopy(struct passwd *pw)
{
	struct passwd *copy = xcalloc(1, sizeof(*copy));

	copy->pw_name = xstrdup(pw->pw_name);
	copy->pw_passwd = xstrdup(pw->pw_passwd);
	copy->pw_gecos = xstrdup(pw->pw_gecos);
	copy->pw_uid = pw->pw_uid;
	copy->pw_gid = pw->pw_gid;
#ifdef HAVE_PW_EXPIRE_IN_PASSWD
	copy->pw_expire = pw->pw_expire;
#endif
#ifdef HAVE_PW_CHANGE_IN_PASSWD
	copy->pw_change = pw->pw_change;
#endif
#ifdef HAVE_PW_CLASS_IN_PASSWD
	copy->pw_class = xstrdup(pw->pw_class);
#endif
	copy->pw_dir = xstrdup(pw->pw_dir);
	copy->pw_shell = xstrdup(pw->pw_shell);
	return copy;
}

/*
 * Convert ASCII string to TCP/IP port number.
 * Port must be >=0 and <=65535.
 * Return -1 if invalid.
 */
int
a2port(const char *s)
{
	long long port;
	const char *errstr;

	port = strtonum(s, 0, 65535, &errstr);
	if (errstr != NULL)
		return -1;
	return (int)port;
}

int
a2tun(const char *s, int *remote)
{
	const char *errstr = NULL;
	char *sp, *ep;
	int tun;

	if (remote != NULL) {
		*remote = SSH_TUNID_ANY;
		sp = xstrdup(s);
		if ((ep = strchr(sp, ':')) == NULL) {
			xfree(sp);
			return (a2tun(s, NULL));
		}
		ep[0] = '\0'; ep++;
		*remote = a2tun(ep, NULL);
		tun = a2tun(sp, NULL);
		xfree(sp);
		return (*remote == SSH_TUNID_ERR ? *remote : tun);
	}

	if (strcasecmp(s, "any") == 0)
		return (SSH_TUNID_ANY);

	tun = strtonum(s, 0, SSH_TUNID_MAX, &errstr);
	if (errstr != NULL)
		return (SSH_TUNID_ERR);

	return (tun);
}

#define SECONDS		1
#define MINUTES		(SECONDS * 60)
#define HOURS		(MINUTES * 60)
#define DAYS		(HOURS * 24)
#define WEEKS		(DAYS * 7)

/*
 * Convert a time string into seconds; format is
 * a sequence of:
 *      time[qualifier]
 *
 * Valid time qualifiers are:
 *      <none>  seconds
 *      s|S     seconds
 *      m|M     minutes
 *      h|H     hours
 *      d|D     days
 *      w|W     weeks
 *
 * Examples:
 *      90m     90 minutes
 *      1h30m   90 minutes
 *      2d      2 days
 *      1w      1 week
 *
 * Return -1 if time string is invalid.
 */
long
convtime(const char *s)
{
	long total, secs;
	const char *p;
	char *endp;

	errno = 0;
	total = 0;
	p = s;

	if (p == NULL || *p == '\0')
		return -1;

	while (*p) {
		secs = strtol(p, &endp, 10);
		if (p == endp ||
		    (errno == ERANGE && (secs == LONG_MIN || secs == LONG_MAX)) ||
		    secs < 0)
			return -1;

		switch (*endp++) {
		case '\0':
			endp--;
			break;
		case 's':
		case 'S':
			break;
		case 'm':
		case 'M':
			secs *= MINUTES;
			break;
		case 'h':
		case 'H':
			secs *= HOURS;
			break;
		case 'd':
		case 'D':
			secs *= DAYS;
			break;
		case 'w':
		case 'W':
			secs *= WEEKS;
			break;
		default:
			return -1;
		}
		total += secs;
		if (total < 0)
			return -1;
		p = endp;
	}

	return total;
}

/*
 * Returns a standardized host+port identifier string.
 * Caller must free returned string.
 */
char *
put_host_port(const char *host, u_short port)
{
	char *hoststr;

	if (port == 0 || port == SSH_DEFAULT_PORT)
		return(xstrdup(host));
	if (asprintf(&hoststr, "[%s]:%d", host, (int)port) < 0)
		fatal("put_host_port: asprintf: %s", strerror(errno));
	debug3("put_host_port: %s", hoststr);
	return hoststr;
}

/*
 * Search for next delimiter between hostnames/addresses and ports.
 * Argument may be modified (for termination).
 * Returns *cp if parsing succeeds.
 * *cp is set to the start of the next delimiter, if one was found.
 * If this is the last field, *cp is set to NULL.
 */
char *
hpdelim(char **cp)
{
	char *s, *old;

	if (cp == NULL || *cp == NULL)
		return NULL;

	old = s = *cp;
	if (*s == '[') {
		if ((s = strchr(s, ']')) == NULL)
			return NULL;
		else
			s++;
	} else if ((s = strpbrk(s, ":/")) == NULL)
		s = *cp + strlen(*cp); /* skip to end (see first case below) */

	switch (*s) {
	case '\0':
		*cp = NULL;	/* no more fields*/
		break;

	case ':':
	case '/':
		*s = '\0';	/* terminate */
		*cp = s + 1;
		break;

	default:
		return NULL;
	}

	return old;
}

char *
cleanhostname(char *host)
{
	if (*host == '[' && host[strlen(host) - 1] == ']') {
		host[strlen(host) - 1] = '\0';
		return (host + 1);
	} else
		return host;
}

char *
colon(char *cp)
{
	int flag = 0;

	if (*cp == ':')		/* Leading colon is part of file name. */
		return NULL;
	if (*cp == '[')
		flag = 1;

	for (; *cp; ++cp) {
		if (*cp == '@' && *(cp+1) == '[')
			flag = 1;
		if (*cp == ']' && *(cp+1) == ':' && flag)
			return (cp+1);
		if (*cp == ':' && !flag)
			return (cp);
		if (*cp == '/')
			return NULL;
	}
	return NULL;
}

/* function to assist building execv() arguments */
void
addargs(arglist *args, char *fmt, ...)
{
	va_list ap;
	char *cp;
	u_int nalloc;
	int r;

	va_start(ap, fmt);
	r = vasprintf(&cp, fmt, ap);
	va_end(ap);
	if (r == -1)
		fatal("addargs: argument too long");

	nalloc = args->nalloc;
	if (args->list == NULL) {
		nalloc = 32;
		args->num = 0;
	} else if (args->num+2 >= nalloc)
		nalloc *= 2;

	args->list = xrealloc(args->list, nalloc, sizeof(char *));
	args->nalloc = nalloc;
	args->list[args->num++] = cp;
	args->list[args->num] = NULL;
}

void
replacearg(arglist *args, u_int which, char *fmt, ...)
{
	va_list ap;
	char *cp;
	int r;

	va_start(ap, fmt);
	r = vasprintf(&cp, fmt, ap);
	va_end(ap);
	if (r == -1)
		fatal("replacearg: argument too long");

	if (which >= args->num)
		fatal("replacearg: tried to replace invalid arg %d >= %d",
		    which, args->num);
	xfree(args->list[which]);
	args->list[which] = cp;
}

void
freeargs(arglist *args)
{
	u_int i;

	if (args->list != NULL) {
		for (i = 0; i < args->num; i++)
			xfree(args->list[i]);
		xfree(args->list);
		args->nalloc = args->num = 0;
		args->list = NULL;
	}
}

/*
 * Expands tildes in the file name.  Returns data allocated by xmalloc.
 * Warning: this calls getpw*.
 */
char *
tilde_expand_filename(const char *filename, uid_t uid)
{
	const char *path;
	char user[128], ret[MAXPATHLEN];
	struct passwd *pw;
	u_int len, slash;

	if (*filename != '~')
		return (xstrdup(filename));
	filename++;

	path = strchr(filename, '/');
	if (path != NULL && path > filename) {		/* ~user/path */
		slash = path - filename;
		if (slash > sizeof(user) - 1)
			fatal("tilde_expand_filename: ~username too long");
		memcpy(user, filename, slash);
		user[slash] = '\0';
		if ((pw = getpwnam(user)) == NULL)
			fatal("tilde_expand_filename: No such user %s", user);
	} else if ((pw = getpwuid(uid)) == NULL)	/* ~/path */
		fatal("tilde_expand_filename: No such uid %ld", (long)uid);

	if (strlcpy(ret, pw->pw_dir, sizeof(ret)) >= sizeof(ret))
		fatal("tilde_expand_filename: Path too long");

	/* Make sure directory has a trailing '/' */
	len = strlen(pw->pw_dir);
	if ((len == 0 || pw->pw_dir[len - 1] != '/') &&
	    strlcat(ret, "/", sizeof(ret)) >= sizeof(ret))
		fatal("tilde_expand_filename: Path too long");

	/* Skip leading '/' from specified path */
	if (path != NULL)
		filename = path + 1;
	if (strlcat(ret, filename, sizeof(ret)) >= sizeof(ret))
		fatal("tilde_expand_filename: Path too long");

	return (xstrdup(ret));
}

/*
 * Expand a string with a set of %[char] escapes. A number of escapes may be
 * specified as (char *escape_chars, char *replacement) pairs. The list must
 * be terminated by a NULL escape_char. Returns replaced string in memory
 * allocated by xmalloc.
 */
char *
percent_expand(const char *string, ...)
{
#define EXPAND_MAX_KEYS	16
	u_int num_keys, i, j;
	struct {
		const char *key;
		const char *repl;
	} keys[EXPAND_MAX_KEYS];
	char buf[4096];
	va_list ap;

	/* Gather keys */
	va_start(ap, string);
	for (num_keys = 0; num_keys < EXPAND_MAX_KEYS; num_keys++) {
		keys[num_keys].key = va_arg(ap, char *);
		if (keys[num_keys].key == NULL)
			break;
		keys[num_keys].repl = va_arg(ap, char *);
		if (keys[num_keys].repl == NULL)
			fatal("%s: NULL replacement", __func__);
	}
	if (num_keys == EXPAND_MAX_KEYS && va_arg(ap, char *) != NULL)
		fatal("%s: too many keys", __func__);
	va_end(ap);

	/* Expand string */
	*buf = '\0';
	for (i = 0; *string != '\0'; string++) {
		if (*string != '%') {
 append:
			buf[i++] = *string;
			if (i >= sizeof(buf))
				fatal("%s: string too long", __func__);
			buf[i] = '\0';
			continue;
		}
		string++;
		/* %% case */
		if (*string == '%')
			goto append;
		for (j = 0; j < num_keys; j++) {
			if (strchr(keys[j].key, *string) != NULL) {
				i = strlcat(buf, keys[j].repl, sizeof(buf));
				if (i >= sizeof(buf))
					fatal("%s: string too long", __func__);
				break;
			}
		}
		if (j >= num_keys)
			fatal("%s: unknown key %%%c", __func__, *string);
	}
	return (xstrdup(buf));
#undef EXPAND_MAX_KEYS
}

/*
 * Read an entire line from a public key file into a static buffer, discarding
 * lines that exceed the buffer size.  Returns 0 on success, -1 on failure.
 */
int
read_keyfile_line(FILE *f, const char *filename, char *buf, size_t bufsz,
   u_long *lineno)
{
	while (fgets(buf, bufsz, f) != NULL) {
		if (buf[0] == '\0')
			continue;
		(*lineno)++;
		if (buf[strlen(buf) - 1] == '\n' || feof(f)) {
			return 0;
		} else {
			debug("%s: %s line %lu exceeds size limit", __func__,
			    filename, *lineno);
			/* discard remainder of line */
			while (fgetc(f) != '\n' && !feof(f))
				;	/* nothing */
		}
	}
	return -1;
}

int
tun_open(int tun, int mode)
{
#if defined(CUSTOM_SYS_TUN_OPEN)
	return (sys_tun_open(tun, mode));
#elif defined(SSH_TUN_OPENBSD)
	struct ifreq ifr;
	char name[100];
	int fd = -1, sock;

	/* Open the tunnel device */
	if (tun <= SSH_TUNID_MAX) {
		snprintf(name, sizeof(name), "/dev/tun%d", tun);
		fd = open(name, O_RDWR);
	} else if (tun == SSH_TUNID_ANY) {
		for (tun = 100; tun >= 0; tun--) {
			snprintf(name, sizeof(name), "/dev/tun%d", tun);
			if ((fd = open(name, O_RDWR)) >= 0)
				break;
		}
	} else {
		debug("%s: invalid tunnel %u", __func__, tun);
		return (-1);
	}

	if (fd < 0) {
		debug("%s: %s open failed: %s", __func__, name, strerror(errno));
		return (-1);
	}

	debug("%s: %s mode %d fd %d", __func__, name, mode, fd);

	/* Set the tunnel device operation mode */
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "tun%d", tun);
	if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1)
		goto failed;

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) == -1)
		goto failed;

	/* Set interface mode */
	ifr.ifr_flags &= ~IFF_UP;
	if (mode == SSH_TUNMODE_ETHERNET)
		ifr.ifr_flags |= IFF_LINK0;
	else
		ifr.ifr_flags &= ~IFF_LINK0;
	if (ioctl(sock, SIOCSIFFLAGS, &ifr) == -1)
		goto failed;

	/* Bring interface up */
	ifr.ifr_flags |= IFF_UP;
	if (ioctl(sock, SIOCSIFFLAGS, &ifr) == -1)
		goto failed;

	close(sock);
	return (fd);

 failed:
	if (fd >= 0)
		close(fd);
	if (sock >= 0)
		close(sock);
	debug("%s: failed to set %s mode %d: %s", __func__, name,
	    mode, strerror(errno));
	return (-1);
#else
	error("Tunnel interfaces are not supported on this platform");
	return (-1);
#endif
}

void
sanitise_stdfd(void)
{
	int nullfd, dupfd;

	if ((nullfd = dupfd = open(_PATH_DEVNULL, O_RDWR)) == -1) {
		fprintf(stderr, "Couldn't open /dev/null: %s\n",
		    strerror(errno));
		exit(1);
	}
	while (++dupfd <= 2) {
		/* Only clobber closed fds */
		if (fcntl(dupfd, F_GETFL, 0) >= 0)
			continue;
		if (dup2(nullfd, dupfd) == -1) {
			fprintf(stderr, "dup2: %s\n", strerror(errno));
			exit(1);
		}
	}
	if (nullfd > 2)
		close(nullfd);
}

char *
tohex(const void *vp, size_t l)
{
	const u_char *p = (const u_char *)vp;
	char b[3], *r;
	size_t i, hl;

	if (l > 65536)
		return xstrdup("tohex: length > 65536");

	hl = l * 2 + 1;
	r = xcalloc(1, hl);
	for (i = 0; i < l; i++) {
		snprintf(b, sizeof(b), "%02x", p[i]);
		strlcat(r, b, hl);
	}
	return (r);
}

u_int64_t
get_u64(const void *vp)
{
	const u_char *p = (const u_char *)vp;
	u_int64_t v;

	v  = (u_int64_t)p[0] << 56;
	v |= (u_int64_t)p[1] << 48;
	v |= (u_int64_t)p[2] << 40;
	v |= (u_int64_t)p[3] << 32;
	v |= (u_int64_t)p[4] << 24;
	v |= (u_int64_t)p[5] << 16;
	v |= (u_int64_t)p[6] << 8;
	v |= (u_int64_t)p[7];

	return (v);
}

u_int32_t
get_u32(const void *vp)
{
	const u_char *p = (const u_char *)vp;
	u_int32_t v;

	v  = (u_int32_t)p[0] << 24;
	v |= (u_int32_t)p[1] << 16;
	v |= (u_int32_t)p[2] << 8;
	v |= (u_int32_t)p[3];

	return (v);
}

u_int16_t
get_u16(const void *vp)
{
	const u_char *p = (const u_char *)vp;
	u_int16_t v;

	v  = (u_int16_t)p[0] << 8;
	v |= (u_int16_t)p[1];

	return (v);
}

void
put_u64(void *vp, u_int64_t v)
{
	u_char *p = (u_char *)vp;

	p[0] = (u_char)(v >> 56) & 0xff;
	p[1] = (u_char)(v >> 48) & 0xff;
	p[2] = (u_char)(v >> 40) & 0xff;
	p[3] = (u_char)(v >> 32) & 0xff;
	p[4] = (u_char)(v >> 24) & 0xff;
	p[5] = (u_char)(v >> 16) & 0xff;
	p[6] = (u_char)(v >> 8) & 0xff;
	p[7] = (u_char)v & 0xff;
}

void
put_u32(void *vp, u_int32_t v)
{
	u_char *p = (u_char *)vp;

	p[0] = (u_char)(v >> 24) & 0xff;
	p[1] = (u_char)(v >> 16) & 0xff;
	p[2] = (u_char)(v >> 8) & 0xff;
	p[3] = (u_char)v & 0xff;
}


void
put_u16(void *vp, u_int16_t v)
{
	u_char *p = (u_char *)vp;

	p[0] = (u_char)(v >> 8) & 0xff;
	p[1] = (u_char)v & 0xff;
}

void
ms_subtract_diff(struct timeval *start, int *ms)
{
	struct timeval diff, finish;

	gettimeofday(&finish, NULL);
	timersub(&finish, start, &diff);	
	*ms -= (diff.tv_sec * 1000) + (diff.tv_usec / 1000);
}

void
ms_to_timeval(struct timeval *tv, int ms)
{
	if (ms < 0)
		ms = 0;
	tv->tv_sec = ms / 1000;
	tv->tv_usec = (ms % 1000) * 1000;
}

void
bandwidth_limit_init(struct bwlimit *bw, u_int64_t kbps, size_t buflen)
{
	bw->buflen = buflen;
	bw->rate = kbps;
	bw->thresh = bw->rate;
	bw->lamt = 0;
	timerclear(&bw->bwstart);
	timerclear(&bw->bwend);
}	

/* Callback from read/write loop to insert bandwidth-limiting delays */
void
bandwidth_limit(struct bwlimit *bw, size_t read_len)
{
	u_int64_t waitlen;
	struct timespec ts, rm;

	if (!timerisset(&bw->bwstart)) {
		gettimeofday(&bw->bwstart, NULL);
		return;
	}

	bw->lamt += read_len;
	if (bw->lamt < bw->thresh)
		return;

	gettimeofday(&bw->bwend, NULL);
	timersub(&bw->bwend, &bw->bwstart, &bw->bwend);
	if (!timerisset(&bw->bwend))
		return;

	bw->lamt *= 8;
	waitlen = (double)1000000L * bw->lamt / bw->rate;

	bw->bwstart.tv_sec = waitlen / 1000000L;
	bw->bwstart.tv_usec = waitlen % 1000000L;

	if (timercmp(&bw->bwstart, &bw->bwend, >)) {
		timersub(&bw->bwstart, &bw->bwend, &bw->bwend);

		/* Adjust the wait time */
		if (bw->bwend.tv_sec) {
			bw->thresh /= 2;
			if (bw->thresh < bw->buflen / 4)
				bw->thresh = bw->buflen / 4;
		} else if (bw->bwend.tv_usec < 10000) {
			bw->thresh *= 2;
			if (bw->thresh > bw->buflen * 8)
				bw->thresh = bw->buflen * 8;
		}

		TIMEVAL_TO_TIMESPEC(&bw->bwend, &ts);
		while (nanosleep(&ts, &rm) == -1) {
			if (errno != EINTR)
				break;
			ts = rm;
		}
	}

	bw->lamt = 0;
	gettimeofday(&bw->bwstart, NULL);
}

/* Make a template filename for mk[sd]temp() */
void
mktemp_proto(char *s, size_t len)
{
	const char *tmpdir;
	int r;

	if ((tmpdir = getenv("TMPDIR")) != NULL) {
		r = snprintf(s, len, "%s/ssh-XXXXXXXXXXXX", tmpdir);
		if (r > 0 && (size_t)r < len)
			return;
	}
	r = snprintf(s, len, "/tmp/ssh-XXXXXXXXXXXX");
	if (r < 0 || (size_t)r >= len)
		fatal("%s: template string too short", __func__);
}

static const struct {
	const char *name;
	int value;
} ipqos[] = {
	{ "af11", IPTOS_DSCP_AF11 },
	{ "af12", IPTOS_DSCP_AF12 },
	{ "af13", IPTOS_DSCP_AF13 },
	{ "af14", IPTOS_DSCP_AF21 },
	{ "af22", IPTOS_DSCP_AF22 },
	{ "af23", IPTOS_DSCP_AF23 },
	{ "af31", IPTOS_DSCP_AF31 },
	{ "af32", IPTOS_DSCP_AF32 },
	{ "af33", IPTOS_DSCP_AF33 },
	{ "af41", IPTOS_DSCP_AF41 },
	{ "af42", IPTOS_DSCP_AF42 },
	{ "af43", IPTOS_DSCP_AF43 },
	{ "cs0", IPTOS_DSCP_CS0 },
	{ "cs1", IPTOS_DSCP_CS1 },
	{ "cs2", IPTOS_DSCP_CS2 },
	{ "cs3", IPTOS_DSCP_CS3 },
	{ "cs4", IPTOS_DSCP_CS4 },
	{ "cs5", IPTOS_DSCP_CS5 },
	{ "cs6", IPTOS_DSCP_CS6 },
	{ "cs7", IPTOS_DSCP_CS7 },
	{ "ef", IPTOS_DSCP_EF },
	{ "lowdelay", IPTOS_LOWDELAY },
	{ "throughput", IPTOS_THROUGHPUT },
	{ "reliability", IPTOS_RELIABILITY },
	{ NULL, -1 }
};

int
parse_ipqos(const char *cp)
{
	u_int i;
	char *ep;
	long val;

	if (cp == NULL)
		return -1;
	for (i = 0; ipqos[i].name != NULL; i++) {
		if (strcasecmp(cp, ipqos[i].name) == 0)
			return ipqos[i].value;
	}
	/* Try parsing as an integer */
	val = strtol(cp, &ep, 0);
	if (*cp == '\0' || *ep != '\0' || val < 0 || val > 255)
		return -1;
	return val;
}

const char *
iptos2str(int iptos)
{
	int i;
	static char iptos_str[sizeof "0xff"];

	for (i = 0; ipqos[i].name != NULL; i++) {
		if (ipqos[i].value == iptos)
			return ipqos[i].name;
	}
	snprintf(iptos_str, sizeof iptos_str, "0x%02x", iptos);
	return iptos_str;
}
void
sock_set_v6only(int s)
{
#ifdef IPV6_V6ONLY
	int on = 1;

	debug3("%s: set socket %d IPV6_V6ONLY", __func__, s);
	if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) == -1)
		error("setsockopt IPV6_V6ONLY: %s", strerror(errno));
#endif
}
