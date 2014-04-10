#
# srcs.mk
#
# Copyright (C) 2010 Creytiv.com
#

SRCS	+= aes.c
SRCS	+= base64.c
SRCS	+= bfcp.c
SRCS	+= conf.c
SRCS	+= crc32.c
SRCS	+= dns.c
SRCS	+= dsp.c
SRCS	+= fir.c
SRCS	+= fmt.c
SRCS	+= g711.c
SRCS	+= hash.c
SRCS	+= hmac.c
SRCS	+= http.c
SRCS	+= httpauth.c
SRCS	+= ice.c
SRCS	+= jbuf.c
SRCS	+= list.c
SRCS	+= main.c
SRCS	+= mbuf.c
SRCS	+= md5.c
SRCS	+= mem.c
SRCS	+= rtp.c
SRCS	+= sa.c
SRCS	+= sdp.c
SRCS	+= sha.c
SRCS	+= sip.c
SRCS	+= sipsess.c
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
endif

# Mock servers
SRCS	+= mock/pf.c
SRCS	+= mock/stunsrv.c
