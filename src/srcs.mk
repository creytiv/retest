#
# srcs.mk
#
# Copyright (C) 2010 Creytiv.com
#

SRCS	+= aes.c
SRCS	+= aubuf.c
SRCS	+= auresamp.c
SRCS	+= base64.c
SRCS	+= bfcp.c
SRCS	+= conf.c
SRCS	+= crc32.c
SRCS	+= dns.c
SRCS	+= dsp.c
SRCS	+= dtmf.c
SRCS	+= fir.c
SRCS	+= fmt.c
SRCS	+= g711.c
SRCS	+= hash.c
SRCS	+= hmac.c
SRCS	+= http.c
SRCS	+= httpauth.c
SRCS	+= ice.c
SRCS	+= jbuf.c
SRCS	+= json.c
SRCS	+= list.c
SRCS	+= main.c
SRCS	+= mbuf.c
SRCS	+= md5.c
SRCS	+= mem.c
SRCS	+= mqueue.c
SRCS	+= natbd.c
SRCS	+= odict.c
SRCS	+= remain.c
SRCS	+= rtp.c
SRCS	+= rtcp.c
SRCS	+= sa.c
SRCS	+= sdp.c
SRCS	+= sha.c
SRCS	+= sip.c
SRCS	+= sipevent.c
SRCS	+= sipreg.c
SRCS	+= sipsess.c
SRCS	+= srtp.c
SRCS	+= stun.c
SRCS	+= sys.c
SRCS	+= tcp.c
SRCS	+= telev.c
SRCS	+= test.c
SRCS	+= tmr.c
SRCS	+= turn.c
SRCS	+= udp.c
SRCS	+= uri.c
SRCS	+= vid.c
SRCS	+= vidconv.c
SRCS	+= websock.c

ifneq ($(USE_TLS),)
SRCS	+= tls.c
SRCS	+= dtls.c

SRCS	+= combo/dtls_turn.c
SRCS	+= cert.c
endif

SRCS	+= util.c

# Mock servers
SRCS	+= mock/pf.c
SRCS	+= mock/sipsrv.c
SRCS	+= mock/stunsrv.c
SRCS	+= mock/turnsrv.c
SRCS	+= mock/nat.c
SRCS	+= mock/tcpsrv.c
