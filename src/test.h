/**
 * @file test.h  Interface to regression testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */


/*
 * Special negative error code for a skipped test
 */
#define ESKIPPED (-1000)


#define TEST_EQUALS(expected, actual)				\
	if ((expected) != (actual)) {				\
		(void)re_fprintf(stderr, "\n");			\
		DEBUG_WARNING("TEST_EQUALS: %s:%u: %s():"	\
			      " expected=%d(0x%x), actual=%d(0x%x)\n",	\
			      __FILE__, __LINE__, __func__,	\
			      (expected), (expected),		\
			      (actual), (actual));		\
		err = EINVAL;					\
		goto out;					\
	}

#define TEST_NOT_EQUALS(expected, actual)			\
	if ((expected) == (actual)) {				\
		(void)re_fprintf(stderr, "\n");			\
		DEBUG_WARNING("TEST_NOT_EQUALS: %s:%u:"		\
			      " expected=%d != actual=%d\n",	\
			      __FILE__, __LINE__,		\
			      (expected), (actual));		\
		err = EINVAL;					\
		goto out;					\
	}

#define TEST_MEMCMP(expected, expn, actual, actn)			\
	if (expn != actn ||						\
	    0 != memcmp((expected), (actual), (expn))) {		\
		(void)re_fprintf(stderr, "\n");				\
		DEBUG_WARNING("TEST_MEMCMP: %s:%u:"			\
			      " %s(): failed\n",			\
			      __FILE__, __LINE__, __func__);		\
		test_hexdump_dual(stderr,				\
				  expected, expn,			\
				  actual, actn);			\
		err = EINVAL;						\
		goto out;						\
	}

#define TEST_STRCMP(expected, expn, actual, actn)			\
	if (expn != actn ||						\
	    0 != memcmp((expected), (actual), (expn))) {		\
		(void)re_fprintf(stderr, "\n");				\
		DEBUG_WARNING("TEST_STRCMP: %s:%u:"			\
			      " failed\n",				\
			      __FILE__, __LINE__);			\
		(void)re_fprintf(stderr,				\
				 "expected string: (%zu bytes)\n"	\
				 "\"%b\"\n",				\
				 (size_t)(expn),			\
				 (expected), (size_t)(expn));		\
		(void)re_fprintf(stderr,				\
				 "actual string: (%zu bytes)\n"		\
				 "\"%b\"\n",				\
				 (size_t)(actn),			\
				 (actual), (size_t)(actn));		\
		err = EINVAL;						\
		goto out;						\
	}

#define TEST_ASSERT(actual)						\
	if (!(actual)) {						\
		(void)re_fprintf(stderr, "\n");				\
		DEBUG_WARNING("TEST_ASSERT: %s:%u:"			\
			      " actual=%d\n",				\
			      __FILE__, __LINE__,			\
			      (actual));				\
		err = EINVAL;						\
		goto out;						\
	}

#define TEST_ERR(err)							\
	if ((err)) {							\
		(void)re_fprintf(stderr, "\n");				\
		DEBUG_WARNING("TEST_ERR: %s:%u:"			\
			      " (%m)\n",				\
			      __FILE__, __LINE__,			\
			      (err));					\
		goto out;						\
	}

#define TEST_SACMP(expect, actual, flags)				\
	if (!sa_cmp((expect), (actual), (flags))) {			\
									\
		(void)re_fprintf(stderr, "\n");				\
		DEBUG_WARNING("TEST_SACMP: %s:%u:"			\
			      " %s(): failed\n",			\
			      __FILE__, __LINE__, __func__);		\
		DEBUG_WARNING("expected: %J\n", (expect));		\
		DEBUG_WARNING("actual:   %J\n", (actual));		\
		err = EADDRNOTAVAIL;					\
		goto out;						\
	}


/* Module API */
int test_aes(void);
int test_aes_gcm(void);
int test_aubuf(void);
int test_auresamp(void);
int test_base64(void);
int test_bfcp(void);
int test_bfcp_bin(void);
int test_conf(void);
int test_crc32(void);
int test_dns_hdr(void);
int test_dns_rr(void);
int test_dns_dname(void);
int test_dsp(void);
int test_dtmf(void);
int test_fir(void);
int test_fmt_human_time(void);
int test_fmt_param(void);
int test_fmt_pl(void);
int test_fmt_pl_u32(void);
int test_fmt_pl_u64(void);
int test_fmt_pl_x3264(void);
int test_fmt_pl_float(void);
int test_fmt_print(void);
int test_fmt_regex(void);
int test_fmt_snprintf(void);
int test_fmt_str(void);
int test_fmt_str_error(void);
int test_fmt_unicode(void);
int test_fmt_unicode_decode(void);
int test_g711_alaw(void);
int test_g711_ulaw(void);
int test_hash(void);
int test_hmac_sha1(void);
int test_hmac_sha256(void);
int test_http(void);
int test_http_loop(void);
#ifdef USE_TLS
int test_https_loop(void);
#endif
int test_httpauth_chall(void);
int test_httpauth_resp(void);
int test_ice_loop(void);
int test_ice_loop_turn(void);
int test_ice_cand(void);
int test_ice_lite(void);
int test_jbuf(void);
int test_json(void);
int test_json_bad(void);
int test_json_file(void);
int test_json_unicode(void);
int test_json_array(void);
int test_list(void);
int test_list_ref(void);
int test_list_sort(void);
int test_mbuf(void);
int test_md5(void);
int test_mem(void);
int test_mem_reallocarray(void);
int test_mem_secure(void);
int test_mqueue(void);
int test_natbd(void);
int test_odict(void);
int test_odict_array(void);
int test_remain(void);
int test_rtp(void);
int test_rtcp_encode(void);
int test_rtcp_encode_afb(void);
int test_rtcp_decode(void);
int test_rtcp_packetloss(void);
int test_sa_class(void);
int test_sa_cmp(void);
int test_sa_decode(void);
int test_sa_ntop(void);
int test_sdp_all(void);
int test_sdp_bfcp(void);
int test_sdp_parse(void);
int test_sdp_oa(void);
int test_sdp_extmap(void);
int test_sha1(void);
int test_sip_addr(void);
int test_sip_apply(void);
int test_sip_hdr(void);
int test_sip_msg(void);
int test_sip_param(void);
int test_sip_parse(void);
int test_sip_via(void);
int test_sipevent(void);
int test_sipreg_udp(void);
int test_sipreg_tcp(void);
#ifdef USE_TLS
int test_sipreg_tls(void);
#endif
int test_sipsess(void);
int test_srtp(void);
int test_srtcp(void);
int test_srtp_gcm(void);
int test_srtcp_gcm(void);
int test_stun_req(void);
int test_stun_resp(void);
int test_stun_reqltc(void);
int test_stun(void);
int test_sys_div(void);
int test_sys_endian(void);
int test_sys_rand(void);
int test_tcp(void);
int test_telev(void);
int test_tmr(void);
int test_turn(void);
int test_turn_tcp(void);
int test_udp(void);
int test_uri(void);
int test_uri_cmp(void);
int test_uri_encode(void);
int test_uri_headers(void);
int test_uri_user(void);
int test_uri_params_headers(void);
int test_uri_escape(void);
int test_vid(void);
int test_vidconv(void);
int test_vidconv_scaling(void);
int test_vidconv_pixel_formats(void);
int test_websock(void);
#ifdef USE_TLS
int test_dtls(void);
int test_dtls_srtp(void);
int test_tls(void);
int test_tls_ec(void);
int test_tls_selfsigned(void);
int test_tls_certificate(void);
#endif

#ifdef USE_TLS
int test_dtls_turn(void);
#endif


#ifdef USE_TLS
extern const char test_certificate_rsa[];
extern const char test_certificate_ecdsa[];
#endif


/* High-level API */
int  test_reg(const char *name, bool verbose);
int  test_oom(const char *name, bool verbose);
int  test_perf(const char *name, bool verbose);
int  test_multithread(void);
void test_listcases(void);


void test_hexdump_dual(FILE *f,
		       const void *ep, size_t elen,
		       const void *ap, size_t alen);
int re_main_timeout(uint32_t timeout_ms);
int test_load_file(struct mbuf *mb, const char *filename);
int test_write_file(struct mbuf *mb, const char *filename);
void test_set_datapath(const char *path);
const char *test_datapath(void);


/* util */
bool odict_compare(const struct odict *dict1, const struct odict *dict2);


/*
 * Mock objects
 */


struct pf {
	struct udp_helper *uh;
	struct udp_sock *us;
	char name[16];
};

int pf_create(struct pf **pfp, struct udp_sock *us, const char *name);


struct stunserver {
	struct udp_sock *us;
	struct tcp_sock *ts;
	struct tcp_conn *tc;
	struct mbuf *mb;
	struct sa laddr;
	struct sa laddr_tcp;
	struct sa paddr;
	uint32_t nrecv;
	int err;
};

int stunserver_alloc(struct stunserver **stunp);
const struct sa *stunserver_addr(const struct stunserver *stun, int proto);


struct turnserver {
	struct udp_sock *us;
	struct sa laddr;
	struct tcp_sock *ts;
	struct sa laddr_tcp;
	struct tcp_conn *tc;
	struct sa paddr;
	struct mbuf *mb;
	struct udp_sock *us_relay;
	struct sa cli;
	struct sa relay;

	struct channel {
		uint16_t nr;
		struct sa peer;
	} chanv[4];
	size_t chanc;

	struct sa permv[4];
	size_t permc;

	size_t n_allocate;
	size_t n_createperm;
	size_t n_chanbind;
	size_t n_send;
	size_t n_raw;
	size_t n_recv;
};

int turnserver_alloc(struct turnserver **turnp);


enum natbox_type {
	NAT_INBOUND_SNAT,  /* NOTE: must be installed on receiving socket */
	NAT_FIREWALL,
};

/**
 * A simple NAT-box that can be hooked onto a UDP-socket.
 *
 * The NAT behaviour is port-preserving and will rewrite the source
 * IP-address to the public address.
 */
struct nat {
	enum natbox_type type;
	struct sa public_addr;
	struct udp_helper *uh;
	struct udp_sock *us;
	struct sa bindingv[16];
	size_t bindingc;
};

int nat_alloc(struct nat **natp, enum natbox_type type,
	      struct udp_sock *us, const struct sa *public_addr);


/*
 * TCP Server
 */

enum behavior {
	BEHAVIOR_NORMAL,
	BEHAVIOR_REJECT
};

struct tcp_server {
	struct tcp_sock *ts;
	enum behavior behavior;
	struct sa laddr;
};

int tcp_server_alloc(struct tcp_server **srvp, enum behavior behavior);


/*
 * SIP Server
 */

struct sip_server {
	struct sip *sip;
	struct sip_lsnr *lsnr;
	bool terminate;

	unsigned n_register_req;
};

int sip_server_alloc(struct sip_server **srvp);
int sip_server_uri(struct sip_server *srv, char *uri, size_t sz,
		   enum sip_transp tp);
