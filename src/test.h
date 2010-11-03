/**
 * @file test.h  Interface to regression testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */


/* Module API */
int test_base64(void);
int test_conf(void);
int test_crc32(void);
int test_fmt_pl(void);
int test_fmt_pl_u32(void);
int test_fmt_pl_u64(void);
int test_fmt_print(void);
int test_fmt_regex(void);
int test_fmt_snprintf(void);
int test_fmt_str(void);
int test_g711_alaw(void);
int test_g711_ulaw(void);
int test_hash(void);
int test_hmac_sha1(void);
int test_jbuf(void);
int test_list(void);
int test_list_ref(void);
int test_mbuf(void);
int test_mbuf_human_time(void);
int test_md5(void);
int test_mem(void);
int test_rtp(void);
int test_rtcp_encode(void);
int test_rtcp_decode(void);
int test_sa_class(void);
int test_sa_cmp(void);
int test_sa_decode(void);
int test_sa_ntop(void);
int test_sdp_all(void);
int test_sdp_bfcp(void);
int test_sdp_parse(void);
int test_sdp_oa(void);
int test_sha1(void);
int test_sip_addr(void);
int test_sip_apply(void);
int test_sip_hdr(void);
int test_sip_msg(void);
int test_sip_param(void);
int test_sip_parse(void);
int test_sip_via(void);
int test_sipsess(void);
int test_stun_req(void);
int test_stun_resp(void);
int test_stun_reqltc(void);
int test_sys_div(void);
int test_sys_endian(void);
int test_tcp(void);
int test_telev(void);
int test_tmr(void);
int test_turn(void);
int test_udp(void);
int test_uri(void);
int test_uri_cmp(void);
int test_uri_encode(void);
int test_uri_headers(void);
int test_uri_user(void);
int test_uri_params_headers(void);


struct mbuf;
int fuzzy_rtp(struct mbuf *mb);
int fuzzy_rtcp(struct mbuf *mb);
int fuzzy_sipmsg(struct mbuf *mb);
int fuzzy_stunmsg(struct mbuf *mb);
int fuzzy_sdpsess(struct mbuf *mb);


/* High-level API */
int test_reg(const char *name);
int test_oom(const char *name);
int test_perf(const char *name, uint32_t n);
int test_fuzzy(const char *name);
