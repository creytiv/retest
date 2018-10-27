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

/* 2048-bit */
const char test_certificate_rsa[] =

"-----BEGIN CERTIFICATE-----\r\n"
"MIIDgTCCAmmgAwIBAgIJAIcN6jBFyKjhMA0GCSqGSIb3DQEBCwUAMFYxCzAJBgNV\r\n"
"BAYTAk5PMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBX\r\n"
"aWRnaXRzIFB0eSBMdGQxDzANBgNVBAMMBnJldGVzdDAgFw0xODEwMjcxODA1MzRa\r\n"
"GA8yMTE4MTAwMzE4MDUzNFowVjELMAkGA1UEBhMCTk8xEzARBgNVBAgMClNvbWUt\r\n"
"U3RhdGUxITAfBgNVBAoMGEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDEPMA0GA1UE\r\n"
"AwwGcmV0ZXN0MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArc68J+if\r\n"
"L8chKZmA3CwSgYJJE3Oy5bBQZP5X2U+YHbQ8en74TeZW7cA5ZFQFQZptZXBwhFIY\r\n"
"61fiHLtB8R9v7hG/SDYbb1gNpXiluDUnpzK89YmEBSEOPxmZvoeMabNNBNOizSFh\r\n"
"4B8mlmLdlE+vUuJYugbZfumwjWRXQrEmGc0idg/tQmu8MCIKGWzyrRJ9ZYwUjFtb\r\n"
"9FHYHjVd5BhWHVuLYYx8lLt96s5GCv1FBpnPZZ3khN2vXGDque2YWVLJ++i2A+HU\r\n"
"UBQErQx+Id2t0F8hvlOoiwdfnftao9rrugn1xJL0zEqNoZbW87+xV644v6o5q21R\r\n"
"fO/SPQdvY0RweQIDAQABo1AwTjAdBgNVHQ4EFgQUCB22b0XV8mVOgFnUKhbXCWV1\r\n"
"showHwYDVR0jBBgwFoAUCB22b0XV8mVOgFnUKhbXCWV1showDAYDVR0TBAUwAwEB\r\n"
"/zANBgkqhkiG9w0BAQsFAAOCAQEAX7ISa9OBdEjUGy81IU5G5LJKBuoFQNnTyOnJ\r\n"
"ZQlEOCKJIvWbb0WSA0L03VqPT1PpgMd8rtjo5M9hBkldoTxLa9Am1jN5IYlCBXlr\r\n"
"F6w0teySuWCdHbGOp4YYqga0yn/8pfZVZlWWp4Q0q+FSew4p9BFpSp5y5yLeKUZI\r\n"
"R/SkSNWCEsIZM/+iGOeTs97FghJsZKUFiSDO0pKuJHGxcDSwOQpP//FuGh+u5Lve\r\n"
"ZYxcrBTDjqq4p1MP8ypVgaRraXVeiHFrX5yVL/Nsd1xmOKLBS1Dm2GlAPFgTGOy6\r\n"
"5rE/+p+RoTySim+1ZN9ojcKryPKhyf75KtS02sQUTef1T2XO1A==\r\n"
"-----END CERTIFICATE-----\r\n"
"-----BEGIN PRIVATE KEY-----\r\n"
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCtzrwn6J8vxyEp\r\n"
"mYDcLBKBgkkTc7LlsFBk/lfZT5gdtDx6fvhN5lbtwDlkVAVBmm1lcHCEUhjrV+Ic\r\n"
"u0HxH2/uEb9INhtvWA2leKW4NSenMrz1iYQFIQ4/GZm+h4xps00E06LNIWHgHyaW\r\n"
"Yt2UT69S4li6Btl+6bCNZFdCsSYZzSJ2D+1Ca7wwIgoZbPKtEn1ljBSMW1v0Udge\r\n"
"NV3kGFYdW4thjHyUu33qzkYK/UUGmc9lneSE3a9cYOq57ZhZUsn76LYD4dRQFASt\r\n"
"DH4h3a3QXyG+U6iLB1+d+1qj2uu6CfXEkvTMSo2hltbzv7FXrji/qjmrbVF879I9\r\n"
"B29jRHB5AgMBAAECggEBAIvl/p8k55TedHv2ebk+pDqoMse8df/ZViykaPOa1Hb8\r\n"
"Tz3OG3EgeVHvSoLN+lkewvVGdtqa9kHgQDkeJOq/gimfEVc/bf/GYV2SadmGt38m\r\n"
"IOCGKsSyIbR6l7y7gDLIRrMe4ki4mP58NGQR+gZZyWYumHpL7x7vXNPCM1aUHnXe\r\n"
"yy3lIHz4FZCRXxbALbFlmGHf6jwByWnKuRh/b8PSnnwlaTCaXuXYMWMelDF9J7qY\r\n"
"YqGBSowCQhVOLJj3DdRSS9iCBSNl5vhW88FYQgEgOiLoZ0gnmM1w+3YON2xDzEkD\r\n"
"gI1BynnC1nucfxrXA0vRhraP2Z3EE5vDOzB43QTsKHECgYEA2MNmTkVnRky9UhWd\r\n"
"K7HtJsnMwib7ynph9jjFk2YT88uK5ZYjJbDDmb34xRNYB8BKh8VKgTgzkn8qp6rk\r\n"
"YiRoH4E/QiBqYtcpsDO47qhBF7ePvJPo11wvovSqCILzZUA1Trc0AYjsRvUN6Sh2\r\n"
"OGYnxz9A5hIiy42N8CdCfNvgavUCgYEAzUTPEJnoK3qkWRb/wbF+2RBr5MMzbaTW\r\n"
"OGjN88Z+oR0RgtQnYobkdXUhDvXrxEuzZ9JP2E70wFMYO2yfaak/PjW9jPY/WIRf\r\n"
"pogbycgmvC17MyNsuD5H97ecfIbZnSybKUsFh8bH4/ZFcw901BBhnwRxp0agdUVM\r\n"
"6eKeWqGhRPUCgYBqpnFOr30prJY1rea/2fI59G4nVLDsJZzPXY1wgXftqsbzQRSX\r\n"
"9cm3ei3NIUBdx/GjraGDxJgzSxg8mKt30jvczGXIblSJvx2G0Vv7KJOmTK2O9iNI\r\n"
"2tWhUsnaGDwTJC1WRnNzEeBW5Tlr73mDNFf8A5Y13NR73HDqqRZggnp/hQKBgQCN\r\n"
"FSIMkvvUBnM3GGuowUoh/vtpPBD45zalhsMnLeKS8du7Q/3d5kDXyi1yjuwA+tbQ\r\n"
"IOjoDzyBg5tAHKRkhwMEywMBA67+M91aJGqVAZA9/jSTLWHoMEZeqEBSBo1DTglH\r\n"
"FF00uRdiQz3wm0r9BlVSakeDZTOb5om6pxuXx0eEkQKBgH1GDEMzNR0pQXAAdqBh\r\n"
"hw4IgJR4OH/oy1RIsb0Cb5XwqV+3ELwidkwA72vSe79+w9/k+vApFBSF6hNrt3Vd\r\n"
"jsyQBooGg094utKUC/HCJHZVXpoDhdOq+P7wvcD49lkRwElK1TYUMsYuq2RX/h9U\r\n"
"hJKcMvPh9yea4qLzdmMGjiMl\r\n"
"-----END PRIVATE KEY-----\r\n"
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
