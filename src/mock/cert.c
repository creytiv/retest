/**
 * @file cert.c  TLS Certificate
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re.h>
#include "test.h"


/**
 * Dummy certificate for testing.
 *
 *
 * It was generated like this:
 *
 *   $ openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem \
 *             -days 36500 -nodes
 *
 *
 * Dumping information:
 *
 *   $ openssl x509 -subject -dates -fingerprint -in cert.pem
 *   subject= /C=NO/ST=Retest/O=Retest AS/CN=Mr Retest/emailAddress=re@test.org
 *   notBefore=Nov 23 18:40:38 2014 GMT
 *   notAfter=Oct 30 18:40:38 2114 GMT
 *   Fingerprint=49:A4:E9:F4:80:3A:D4:38:84:F1:64:C3:B9:4B:F9:BB:80:F7:07:76
 */

const char test_certificate_rsa[] =


#if 1
/* 1024-bit */
"-----BEGIN CERTIFICATE-----\r\n"
"MIIClDCCAf2gAwIBAgIJAJ+A7xlYtlNJMA0GCSqGSIb3DQEBCwUAMGIxCzAJBgNV\r\n"
"BAYTAk5PMQ8wDQYDVQQIDAZSZXRlc3QxEjAQBgNVBAoMCVJldGVzdCBBUzESMBAG\r\n"
"A1UEAwwJTXIgUmV0ZXN0MRowGAYJKoZIhvcNAQkBFgtyZUB0ZXN0Lm9yZzAgFw0x\r\n"
"NjA4MTAwODE0MTBaGA8yMTE2MDcxNzA4MTQxMFowYjELMAkGA1UEBhMCTk8xDzAN\r\n"
"BgNVBAgMBlJldGVzdDESMBAGA1UECgwJUmV0ZXN0IEFTMRIwEAYDVQQDDAlNciBS\r\n"
"ZXRlc3QxGjAYBgkqhkiG9w0BCQEWC3JlQHRlc3Qub3JnMIGfMA0GCSqGSIb3DQEB\r\n"
"AQUAA4GNADCBiQKBgQDJKHCmm+VFXsA2cXLw5rD1e20ge/WLSMEScrtpoUN6sppi\r\n"
"s0UEhzkdSvFeOFGs2LdlNZFr5LwhPpjG7Ct/zl/R1XDo2hRxA89HDFp8GXHgLVbv\r\n"
"3zLPoA/jMCgx+qyJuTeLuqtFwveItCwaeqLKZp9IlWwN0a2QfpolDc/JGvhqPQID\r\n"
"AQABo1AwTjAdBgNVHQ4EFgQUwSovp1CZalCougoqTywvwYRJWoIwHwYDVR0jBBgw\r\n"
"FoAUwSovp1CZalCougoqTywvwYRJWoIwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0B\r\n"
"AQsFAAOBgQA8CnwJUr0WfaJLeFNMmGDmG2H4wwAUBDB2582wCa0ZpewWnzEfxeql\r\n"
"4zkOzGN82XWyXvpS/g80V138Y2e5vYaotlMCArPdrmQmvIAv59T41rXojiOAZlEP\r\n"
"C5VOmCRfQ3Epiiyn1rqdeoiwyKBxDNYjfCd0ITVGKzFe+ZGJCN7gLQ==\r\n"
"-----END CERTIFICATE-----\r\n"
"-----BEGIN PRIVATE KEY-----\r\n"
"MIICeQIBADANBgkqhkiG9w0BAQEFAASCAmMwggJfAgEAAoGBAMkocKab5UVewDZx\r\n"
"cvDmsPV7bSB79YtIwRJyu2mhQ3qymmKzRQSHOR1K8V44UazYt2U1kWvkvCE+mMbs\r\n"
"K3/OX9HVcOjaFHEDz0cMWnwZceAtVu/fMs+gD+MwKDH6rIm5N4u6q0XC94i0LBp6\r\n"
"ospmn0iVbA3RrZB+miUNz8ka+Go9AgMBAAECgYEAkuHSj/WZjfPHynlAgqMQZ89+\r\n"
"OZAKFXBEeqIIMvMsALXVgWiBZvMOQ1Nrt27MRLfiAicBTpUwwd2hVO8yPBqHX7Zv\r\n"
"y/fVLgDqQ81TwXwJ4T1DXmmJ7zno4NhXp8TiD67zWAb/6XofFHvBBFOIKu5A/RpD\r\n"
"HNSUsQKuY86pbgg+m4ECQQD0t2N4F5i0FmBa4hOLKd7VDRvxmrUsKtWypY6r5bYE\r\n"
"4RfBT2VpyBHmnPqo8+b/YW+IpUZKHr+hGlY1sfdyq92xAkEA0m7kUZ9YbxdGaRpu\r\n"
"052U1ovobEVcvDMOfU3pX7xe7Ye6dWfmVD1IwoJshM6DP0qKYfOlY9X1BW6RplkL\r\n"
"zHV8TQJBALGdtY2Bmu0C3La6JjM4laqPFDwc7Oh8wpQig9YSKTWLZqGBDftkIlH+\r\n"
"mBQuyveK8df9FXJQtQeqRO4+GHrbcVECQQCLGla/RfA1b1NSAvEj3dUXheCcQbWt\r\n"
"iqqXVjtg16qApeox1f7BzMkeGw0VNVY5LlMPsPmxZyvS7WZGajKMc1U5AkEAsFyv\r\n"
"+043R8eFPzukH9UfhmSxcGZigZI4RuPphT7av4Y3O2cn6v7BKedyu8T5laPHE9//\r\n"
"vhzwKUzRGopHSHA5xA==\r\n"
"-----END PRIVATE KEY-----\r\n"
#endif

;


/**
 * X509/PEM certificate with ECDSA keypair
 *
 *  $ openssl ecparam -out ec_key.pem -name prime256v1 -genkey
 *  $ openssl req -new -key ec_key.pem -x509 -nodes -days 3650 -out cert.pem
 */
const char test_certificate_ecdsa[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIB/zCCAaWgAwIBAgIJAOM89Ziwo6HsMAoGCCqGSM49BAMCMFwxCzAJBgNVBAYT\r\n"
"AkRFMQ8wDQYDVQQIDAZSZXRlc3QxDzANBgNVBAoMBlJldGVzdDEPMA0GA1UEAwwG\r\n"
"cmV0ZXN0MRowGAYJKoZIhvcNAQkBFgtyZUB0ZXN0LmNvbTAeFw0xNjExMTEyMTQ1\r\n"
"NDdaFw0yNjExMDkyMTQ1NDdaMFwxCzAJBgNVBAYTAkRFMQ8wDQYDVQQIDAZSZXRl\r\n"
"c3QxDzANBgNVBAoMBlJldGVzdDEPMA0GA1UEAwwGcmV0ZXN0MRowGAYJKoZIhvcN\r\n"
"AQkBFgtyZUB0ZXN0LmNvbTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABBN9PbWr\r\n"
"n1jFQszb3d7Ahhu+P07nEQsH2uWbuVI/tuAStTWv5FOGrrK1Mkc8D8vaiHJSAI+Y\r\n"
"arsGUcpXvbyf6ZajUDBOMB0GA1UdDgQWBBQH/YADwfvE31Huriy3dwrSszHHQjAf\r\n"
"BgNVHSMEGDAWgBQH/YADwfvE31Huriy3dwrSszHHQjAMBgNVHRMEBTADAQH/MAoG\r\n"
"CCqGSM49BAMCA0gAMEUCIAlRWLW8qA4hlX2ikv+odJe/z2cWeIGcHNUsAaQhCw6s\r\n"
"AiEA4wvqDaBH7urPrCKPITOdeI7eL95RR3KIGFHoP71lrjk=\r\n"
"-----END CERTIFICATE-----\r\n"
"-----BEGIN EC PARAMETERS-----\r\n"
"BggqhkjOPQMBBw==\r\n"
"-----END EC PARAMETERS-----\r\n"
"-----BEGIN EC PRIVATE KEY-----\r\n"
"MHcCAQEEIPcsRIqjUcYgeDLtL0Nm69R5pUZ9Hhb7/HZHH5vwAiCgoAoGCCqGSM49\r\n"
"AwEHoUQDQgAEE309taufWMVCzNvd3sCGG74/TucRCwfa5Zu5Uj+24BK1Na/kU4au\r\n"
"srUyRzwPy9qIclIAj5hquwZRyle9vJ/plg==\r\n"
"-----END EC PRIVATE KEY-----\r\n"
	;
