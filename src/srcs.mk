#
# srcs.mk
#
# Copyright (C) 2010 Creytiv.com
#

SRCS	+= base64.c
SRCS	+= bfcp.c
SRCS	+= conf.c
SRCS	+= crc32.c
SRCS	+= dns.c
SRCS	+= fmt.c
SRCS	+= g711.c
SRCS	+= hash.c
SRCS	+= hmac.c
SRCS	+= httpauth.c
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
SRCS	+= vidconv.c

ifneq ($(USE_TLS),)
SRCS	+= tls.c
endif
