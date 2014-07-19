/* $OpenBSD: misc.h,v 1.48 2011/03/29 18:54:17 stevesk Exp $ */

/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#ifndef _MISC_H
#define _MISC_H

/* Modified: clientloop.c, serverloop.c*/

#define ARRSIZE 1000
#define TARGET_QUEUE_LEN 548 //448 //748 //948 //1448
#define INITIAL_TAU 8192
#define LOWER_TAU 4096 //8192
#define UPPER_TAU 32768 //65536
//#define GAP 100000	// use 0.1s interval to compute data incoming rate
#define INITIAL_BOUNDARY 16384 //32768 //131072
//#define UPDATE_INTERVAL 1500000 // tau update interval 1.5s
//#define ALPHA 1	//overhead threshold

#include "load_st.h"

FILE* get_fportlog();
void create_log_file(int port);
void close_log_file();

unsigned long long get_first_expected();

unsigned long long get_ts_start(void);
unsigned long long get_ts_end(void);
int get_tstart_set(void);
int get_tend_set(void);

void set_first_client();
void unset_first_client();
int get_first_client();

void set_second_client();
void unset_second_client();
int get_second_client();

void set_trans_padded_remaining(int val);
int get_trans_padded_remaining();
void decrease_trans_padded_remaining(int val);

void set_trans_padded();
void unset_trans_padded();
int get_trans_padded();

//void set_trans_early_stop();
//void unset_trans_early_stop();
//int get_trans_early_stop();

void set_urlack();
void unset_urlack();
int get_urlack();

int should_jump_trans_end(unsigned int rand);

void set_ts_end(unsigned long long val);
void set_ts_start(unsigned long long val);
void set_tstart_set(int val);
void set_tend_set(int val);
	
int get_onload_flag();
int get_unload_flag();
int get_paddingdone_flag();

void set_stmode(int mode);
int get_stmode();

//void increase_sync_num();
//void decrease_sync_num();
//int get_sync_num();

int get_first_sent();
void unset_first_sent();
void set_first_sent();

void set_lockname(int port);
char* get_lockname();

void set_server_entering_normal();
void unset_server_entering_normal();
int get_server_entering_normal();

//void increase_sync_received();
//void decrease_sync_received();
//int get_sync_received();

unsigned long long get_current_time_usecs(void);
int should_set_writefd(int truly_idle, unsigned long long time_to_write, struct timeval** tvp, struct timeval* tv);
int channel_idle(unsigned long long* now_usecs, int buf_len, int outbuf_remain, unsigned long long* idle_start);

void set_onload_flag();
void unset_onload_flag();
void set_unload_flag();
void unset_unload_flag();
void set_paddingdone_flag();
void unset_paddingdone_flag();

int get_sent_tend();
void set_sent_tend();
void unset_sent_tend();

int get_transcript_end();
void set_transcript_end();
void unset_transcript_end();

unsigned long long get_st_idle_start();
void set_st_idle_start(unsigned long long val);

//void set_green_light();
//void unset_green_light();
//int get_green_light();

unsigned long long get_received_sofar();
void set_received_sofar(unsigned long long val);

long get_w2w();
void set_w2w(long val);
int get_padded_junk();
void set_padded_junk(int val);

int get_urllen();
u_char* get_url();
void set_url(u_char* p, int offset);

int get_cluster_id();
void set_cluster_id(char* url, int send_pos);
void move_to_next_st_item(int is_pos);
STPAIR* get_current_st_item();

long update_tau(long tau, long* boundary, unsigned long long* time_to_update, float* pre_rate, long total_sent, unsigned long long old_ts, long old_real_incoming, unsigned long long cur_ts, long cur_real_incoming, int outbuf_remain);
void update_tau_interval(unsigned long long ts_start, unsigned long long ts_end, long cur_total_realsent_bytes, int is_client);
long update_tau_median(long tau, long* boundary, long total_sent);
int comp_long(const void *p1, const void* p2);
void insert_time(unsigned long long time);

/* misc.c */

char	*chop(char *);
char	*strdelim(char **);
int	 set_nonblock(int);
int	 unset_nonblock(int);
void	 set_nodelay(int);
void 	 set_cork(int fd, int opt);
int	 a2port(const char *);
int	 a2tun(const char *, int *);
char	*put_host_port(const char *, u_short);
char	*hpdelim(char **);
char	*cleanhostname(char *);
char	*colon(char *);
long	 convtime(const char *);
char	*tilde_expand_filename(const char *, uid_t);
char	*percent_expand(const char *, ...) __attribute__((__sentinel__));
char	*tohex(const void *, size_t);
void	 sanitise_stdfd(void);
void	 ms_subtract_diff(struct timeval *, int *);
void	 ms_to_timeval(struct timeval *, int);
void	 sock_set_v6only(int);

struct passwd *pwcopy(struct passwd *);
const char *ssh_gai_strerror(int);

typedef struct arglist arglist;
struct arglist {
	char    **list;
	u_int   num;
	u_int   nalloc;
};
void	 addargs(arglist *, char *, ...)
	     __attribute__((format(printf, 2, 3)));
void	 replacearg(arglist *, u_int, char *, ...)
	     __attribute__((format(printf, 3, 4)));
void	 freeargs(arglist *);

int	 tun_open(int, int);

/* Common definitions for ssh tunnel device forwarding */
#define SSH_TUNMODE_NO		0x00
#define SSH_TUNMODE_POINTOPOINT	0x01
#define SSH_TUNMODE_ETHERNET	0x02
#define SSH_TUNMODE_DEFAULT	SSH_TUNMODE_POINTOPOINT
#define SSH_TUNMODE_YES		(SSH_TUNMODE_POINTOPOINT|SSH_TUNMODE_ETHERNET)

#define SSH_TUNID_ANY		0x7fffffff
#define SSH_TUNID_ERR		(SSH_TUNID_ANY - 1)
#define SSH_TUNID_MAX		(SSH_TUNID_ANY - 2)

/* Functions to extract or store big-endian words of various sizes */
u_int64_t	get_u64(const void *)
    __attribute__((__bounded__( __minbytes__, 1, 8)));
u_int32_t	get_u32(const void *)
    __attribute__((__bounded__( __minbytes__, 1, 4)));
u_int16_t	get_u16(const void *)
    __attribute__((__bounded__( __minbytes__, 1, 2)));
void		put_u64(void *, u_int64_t)
    __attribute__((__bounded__( __minbytes__, 1, 8)));
void		put_u32(void *, u_int32_t)
    __attribute__((__bounded__( __minbytes__, 1, 4)));
void		put_u16(void *, u_int16_t)
    __attribute__((__bounded__( __minbytes__, 1, 2)));

struct bwlimit {
	size_t buflen;
	u_int64_t rate, thresh, lamt;
	struct timeval bwstart, bwend;
};

void bandwidth_limit_init(struct bwlimit *, u_int64_t, size_t);
void bandwidth_limit(struct bwlimit *, size_t);

int parse_ipqos(const char *);
const char *iptos2str(int);
void mktemp_proto(char *, size_t);

/* readpass.c */

#define RP_ECHO			0x0001
#define RP_ALLOW_STDIN		0x0002
#define RP_ALLOW_EOF		0x0004
#define RP_USE_ASKPASS		0x0008

char	*read_passphrase(const char *, int);
int	 ask_permission(const char *, ...) __attribute__((format(printf, 1, 2)));
int	 read_keyfile_line(FILE *, const char *, char *, size_t, u_long *);

#endif /* _MISC_H */
