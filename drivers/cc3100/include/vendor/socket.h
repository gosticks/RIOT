/*
 * socket.h - CC31xx/CC32xx Host Driver Implementation
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/
 
#ifndef __SL_SOCKET_H__
#define __SL_SOCKET_H__

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/


#ifdef    __cplusplus
extern "C" {
#endif

/*!

    \addtogroup socket
    @{

*/

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

/* Avoid redifined warning */
#undef FD_SETSIZE
#undef FD_SET
#undef FD_CLR
#undef FD_ISSET
#undef FD_ZERO
#undef fd_set

#define SL_FD_SETSIZE                         SL_MAX_SOCKETS         /* Number of sockets to select on - same is max sockets!               */
#define BSD_SOCKET_ID_MASK                     (0x0F)                 /* Index using the LBS 4 bits for socket id 0-7 */
/* Define some BSD protocol constants.  */
#define SL_SOCK_STREAM                         (1)                       /* TCP Socket                                                          */
#define SL_SOCK_DGRAM                          (2)                       /* UDP Socket                                                          */
#define SL_SOCK_RAW                            (3)                       /* Raw socket                                                          */
#define SL_IPPROTO_TCP                         (6)                       /* TCP Raw Socket                                                      */
#define SL_IPPROTO_UDP                         (17)                      /* UDP Raw Socket                                                      */
#define SL_IPPROTO_RAW                         (255)                     /* Raw Socket                                                          */
#define SL_SEC_SOCKET                          (100)                     /* Secured Socket Layer (SSL,TLS)                                      */

/* Address families.  */
#define     SL_AF_INET                         (2)                       /* IPv4 socket (UDP, TCP, etc)                                          */
#define     SL_AF_INET6                        (3)                       /* IPv6 socket (UDP, TCP, etc)                                          */
#define     SL_AF_INET6_EUI_48                 (9)
#define     SL_AF_RF                           (6)                       /* data include RF parameter, All layer by user (Wifi could be disconnected) */ 
#define     SL_AF_PACKET                       (17)
/* Protocol families, same as address families.  */
#define     SL_PF_INET                         AF_INET
#define     SL_PF_INET6                        AF_INET6
#define     SL_INADDR_ANY                      (0)                       /*  bind any address  */

/* error codes */
#define SL_SOC_ERROR                          (-1)  /* Failure.                                                             */
#define SL_SOC_OK                             ( 0)  /* Success.                                                             */
#define SL_INEXE                              (-8)   /* socket command in execution  */
#define SL_EBADF                              (-9)   /* Bad file number */
#define SL_ENSOCK                             (-10)  /* The system limit on the total number of open socket, has been reached */
#define SL_EAGAIN                             (-11)  /* Try again */
#define SL_EWOULDBLOCK                        SL_EAGAIN
#define SL_ENOMEM                             (-12)  /* Out of memory */
#define SL_EACCES                             (-13)  /* Permission denied */
#define SL_EFAULT                             (-14)  /* Bad address */
#define SL_ECLOSE                             (-15)  /* close socket operation failed to transmit all queued packets */
#define SL_EALREADY_ENABLED                   (-21)  /* Transceiver - Transceiver already ON. there could be only one */
#define SL_EINVAL                             (-22)  /* Invalid argument */
#define SL_EAUTO_CONNECT_OR_CONNECTING        (-69)  /* Transceiver - During connection, connected or auto mode started */
#define SL_CONNECTION_PENDING                  (-72)  /* Transceiver - Device is connected, disconnect first to open transceiver */
#define SL_EUNSUPPORTED_ROLE                  (-86)  /* Transceiver - Trying to start when WLAN role is AP or P2P GO */
#define SL_EDESTADDRREQ                       (-89)  /* Destination address required */
#define SL_EPROTOTYPE                         (-91)  /* Protocol wrong type for socket */
#define SL_ENOPROTOOPT                        (-92)  /* Protocol not available */
#define SL_EPROTONOSUPPORT                    (-93)  /* Protocol not supported */
#define SL_ESOCKTNOSUPPORT                    (-94)  /* Socket type not supported */
#define SL_EOPNOTSUPP                         (-95)  /* Operation not supported on transport endpoint */
#define SL_EAFNOSUPPORT                       (-97)  /* Address family not supported by protocol */
#define SL_EADDRINUSE                         (-98)  /* Address already in use */
#define SL_EADDRNOTAVAIL                      (-99)  /* Cannot assign requested address */
#define SL_ENETUNREACH                        (-101) /* Network is unreachable */
#define SL_ENOBUFS                            (-105) /* No buffer space available */
#define SL_EOBUFF                             SL_ENOBUFS 
#define SL_EISCONN                            (-106) /* Transport endpoint is already connected */
#define SL_ENOTCONN                           (-107) /* Transport endpoint is not connected */
#define SL_ETIMEDOUT                          (-110) /* Connection timed out */
#define SL_ECONNREFUSED                       (-111) /* Connection refused */
#define SL_EALREADY                           (-114) /* Non blocking connect in progress, try again */ 

#define SL_ESEC_RSA_WRONG_TYPE_E              (-130)  /* RSA wrong block type for RSA function */
#define SL_ESEC_RSA_BUFFER_E                  (-131)  /* RSA buffer error, output too small or */
#define SL_ESEC_BUFFER_E                      (-132)  /* output buffer too small or input too large */
#define SL_ESEC_ALGO_ID_E                     (-133)  /* setting algo id error */
#define SL_ESEC_PUBLIC_KEY_E                  (-134)  /* setting public key error */
#define SL_ESEC_DATE_E                        (-135)  /* setting date validity error */
#define SL_ESEC_SUBJECT_E                     (-136)  /* setting subject name error */
#define SL_ESEC_ISSUER_E                      (-137)  /* setting issuer  name error */
#define SL_ESEC_CA_TRUE_E                     (-138)  /* setting CA basic constraint true error */
#define SL_ESEC_EXTENSIONS_E                  (-139)  /* setting extensions error */
#define SL_ESEC_ASN_PARSE_E                   (-140)  /* ASN parsing error, invalid input */
#define SL_ESEC_ASN_VERSION_E                 (-141)  /* ASN version error, invalid number */
#define SL_ESEC_ASN_GETINT_E                  (-142)  /* ASN get big _i16 error, invalid data */
#define SL_ESEC_ASN_RSA_KEY_E                 (-143)  /* ASN key init error, invalid input */
#define SL_ESEC_ASN_OBJECT_ID_E               (-144)  /* ASN object id error, invalid id */
#define SL_ESEC_ASN_TAG_NULL_E                (-145)  /* ASN tag error, not null */
#define SL_ESEC_ASN_EXPECT_0_E                (-146)  /* ASN expect error, not zero */
#define SL_ESEC_ASN_BITSTR_E                  (-147)  /* ASN bit string error, wrong id */
#define SL_ESEC_ASN_UNKNOWN_OID_E             (-148)  /* ASN oid error, unknown sum id */
#define SL_ESEC_ASN_DATE_SZ_E                 (-149)  /* ASN date error, bad size */
#define SL_ESEC_ASN_BEFORE_DATE_E             (-150)  /* ASN date error, current date before */
#define SL_ESEC_ASN_AFTER_DATE_E              (-151)  /* ASN date error, current date after */
#define SL_ESEC_ASN_SIG_OID_E                 (-152)  /* ASN signature error, mismatched oid */
#define SL_ESEC_ASN_TIME_E                    (-153)  /* ASN time error, unknown time type */
#define SL_ESEC_ASN_INPUT_E                   (-154)  /* ASN input error, not enough data */
#define SL_ESEC_ASN_SIG_CONFIRM_E             (-155)  /* ASN sig error, confirm failure */
#define SL_ESEC_ASN_SIG_HASH_E                (-156)  /* ASN sig error, unsupported hash type */
#define SL_ESEC_ASN_SIG_KEY_E                 (-157)  /* ASN sig error, unsupported key type */
#define SL_ESEC_ASN_DH_KEY_E                  (-158)  /* ASN key init error, invalid input */
#define SL_ESEC_ASN_NTRU_KEY_E                (-159)  /* ASN ntru key decode error, invalid input */
#define SL_ESEC_ECC_BAD_ARG_E                 (-170)  /* ECC input argument of wrong type */
#define SL_ESEC_ASN_ECC_KEY_E                 (-171)  /* ASN ECC bad input */
#define SL_ESEC_ECC_CURVE_OID_E               (-172)  /* Unsupported ECC OID curve type */
#define SL_ESEC_BAD_FUNC_ARG                  (-173)  /* Bad function argument provided */
#define SL_ESEC_NOT_COMPILED_IN               (-174)  /* Feature not compiled in */
#define SL_ESEC_UNICODE_SIZE_E                (-175)  /* Unicode password too big */
#define SL_ESEC_NO_PASSWORD                   (-176)  /* no password provided by user */
#define SL_ESEC_ALT_NAME_E                    (-177)  /* alt name size problem, too big */
#define SL_ESEC_AES_GCM_AUTH_E                (-180)  /* AES-GCM Authentication check failure */
#define SL_ESEC_AES_CCM_AUTH_E                (-181)  /* AES-CCM Authentication check failure */
/* ssl tls security start with -300 offset */
#define SL_ESEC_CLOSE_NOTIFY                  (-300) /* ssl/tls alerts */   
#define SL_ESEC_UNEXPECTED_MESSAGE            (-310) /* ssl/tls alerts */   
#define SL_ESEC_BAD_RECORD_MAC                (-320) /* ssl/tls alerts */                 
#define SL_ESEC_DECRYPTION_FAILED             (-321) /* ssl/tls alerts */   
#define SL_ESEC_RECORD_OVERFLOW               (-322) /* ssl/tls alerts */    
#define SL_ESEC_DECOMPRESSION_FAILURE         (-330) /* ssl/tls alerts */                 
#define SL_ESEC_HANDSHAKE_FAILURE             (-340) /* ssl/tls alerts */    
#define SL_ESEC_NO_CERTIFICATE                (-341) /* ssl/tls alerts */    
#define SL_ESEC_BAD_CERTIFICATE               (-342) /* ssl/tls alerts */          
#define SL_ESEC_UNSUPPORTED_CERTIFICATE       (-343) /* ssl/tls alerts */     
#define SL_ESEC_CERTIFICATE_REVOKED           (-344) /* ssl/tls alerts */                 
#define SL_ESEC_CERTIFICATE_EXPIRED           (-345) /* ssl/tls alerts */                 
#define SL_ESEC_CERTIFICATE_UNKNOWN           (-346) /* ssl/tls alerts */                 
#define SL_ESEC_ILLEGAL_PARAMETER             (-347) /* ssl/tls alerts */                 
#define SL_ESEC_UNKNOWN_CA                    (-348) /* ssl/tls alerts */                
#define SL_ESEC_ACCESS_DENIED                 (-349) /* ssl/tls alerts */                
#define SL_ESEC_DECODE_ERROR                  (-350) /* ssl/tls alerts */   
#define SL_ESEC_DECRYPT_ERROR                 (-351) /* ssl/tls alerts */   
#define SL_ESEC_EXPORT_RESTRICTION            (-360) /* ssl/tls alerts */    
#define SL_ESEC_PROTOCOL_VERSION              (-370) /* ssl/tls alerts */    
#define SL_ESEC_INSUFFICIENT_SECURITY         (-371) /* ssl/tls alerts */   
#define SL_ESEC_INTERNAL_ERROR                (-380) /* ssl/tls alerts */   
#define SL_ESEC_USER_CANCELLED                (-390) /* ssl/tls alerts */   
#define SL_ESEC_NO_RENEGOTIATION              (-400) /* ssl/tls alerts */   
#define SL_ESEC_UNSUPPORTED_EXTENSION         (-410) /* ssl/tls alerts */   
#define SL_ESEC_CERTIFICATE_UNOBTAINABLE      (-411) /* ssl/tls alerts */         
#define SL_ESEC_UNRECOGNIZED_NAME             (-412) /* ssl/tls alerts */   
#define SL_ESEC_BAD_CERTIFICATE_STATUS_RESPONSE  (-413) /* ssl/tls alerts */   
#define SL_ESEC_BAD_CERTIFICATE_HASH_VALUE    (-414) /* ssl/tls alerts */   
/* propierty secure */
#define SL_ESECGENERAL                        (-450)  /* error secure level general error */
#define SL_ESECDECRYPT                        (-451)  /* error secure level, decrypt recv packet fail */
#define SL_ESECCLOSED                         (-452)  /* secure layrer is closed by other size , tcp is still connected  */
#define SL_ESECSNOVERIFY                      (-453)  /* Connected without server verification */
#define SL_ESECNOCAFILE                       (-454)  /* error secure level CA file not found*/
#define SL_ESECMEMORY                         (-455)  /* error secure level No memory  space available */
#define SL_ESECBADCAFILE                      (-456)  /* error secure level bad CA file */
#define SL_ESECBADCERTFILE                    (-457)  /* error secure level bad Certificate file */
#define SL_ESECBADPRIVATEFILE                 (-458)  /* error secure level bad private file */
#define SL_ESECBADDHFILE                      (-459)  /* error secure level bad DH file */
#define SL_ESECT00MANYSSLOPENED               (-460)  /* MAX SSL Sockets are opened */
#define SL_ESECDATEERROR                      (-461)  /* connected with certificate date verification error */
#define SL_ESECHANDSHAKETIMEDOUT              (-462)  /* connection timed out due to handshake time */

/* end error codes */

/* Max payload size by protocol */
#define SL_SOCKET_PAYLOAD_TYPE_MASK            (0xF0)  /*4 bits type, 4 bits sockets id */
#define SL_SOCKET_PAYLOAD_TYPE_UDP_IPV4        (0x00)  /* 1472 bytes */
#define SL_SOCKET_PAYLOAD_TYPE_TCP_IPV4        (0x10)  /* 1460 bytes */
#define SL_SOCKET_PAYLOAD_TYPE_UDP_IPV6        (0x20)  /* 1452 bytes */
#define SL_SOCKET_PAYLOAD_TYPE_TCP_IPV6        (0x30)  /* 1440 bytes */
#define SL_SOCKET_PAYLOAD_TYPE_UDP_IPV4_SECURE (0x40)  /*            */
#define SL_SOCKET_PAYLOAD_TYPE_TCP_IPV4_SECURE (0x50)  /*            */
#define SL_SOCKET_PAYLOAD_TYPE_UDP_IPV6_SECURE (0x60)  /*            */
#define SL_SOCKET_PAYLOAD_TYPE_TCP_IPV6_SECURE (0x70)  /*            */
#define SL_SOCKET_PAYLOAD_TYPE_RAW_TRANCEIVER  (0x80)  /* 1536 bytes */
#define SL_SOCKET_PAYLOAD_TYPE_RAW_PACKET      (0x90)  /* 1536 bytes */
#define SL_SOCKET_PAYLOAD_TYPE_RAW_IP4         (0xa0)  
#define SL_SOCKET_PAYLOAD_TYPE_RAW_IP6         (SL_SOCKET_PAYLOAD_TYPE_RAW_IP4 )  

  

#define SL_SOL_SOCKET          (1)   /* Define the socket option category. */
#define SL_IPPROTO_IP          (2)   /* Define the IP option category.     */
#define SL_SOL_PHY_OPT         (3)   /* Define the PHY option category.    */

#define SL_SO_RCVBUF           (8)   /* Setting TCP receive buffer size */
#define SL_SO_KEEPALIVE        (9)   /* Connections are kept alive with periodic messages */
#define SL_SO_RCVTIMEO         (20)  /* Enable receive timeout */
#define SL_SO_NONBLOCKING      (24)  /* Enable . disable nonblocking mode  */
#define SL_SO_SECMETHOD        (25)  /* security metohd */
#define SL_SO_SECURE_MASK      (26)  /* security mask */
#define SL_SO_SECURE_FILES     (27)  /* security files */
#define SL_SO_CHANGE_CHANNEL   (28)  /* This option is available only when transceiver started */
#define SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME (30) /* This option used to configue secure file */
#define SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME (31) /* This option used to configue secure file */
#define SL_SO_SECURE_FILES_CA_FILE_NAME          (32) /* This option used to configue secure file */
#define SL_SO_SECURE_FILES_DH_KEY_FILE_NAME      (33) /* This option used to configue secure file */

#define SL_IP_MULTICAST_IF     (60) /* Specify outgoing multicast interface */
#define SL_IP_MULTICAST_TTL    (61) /* Specify the TTL value to use for outgoing multicast packet. */
#define SL_IP_ADD_MEMBERSHIP   (65) /* Join IPv4 multicast membership */
#define SL_IP_DROP_MEMBERSHIP  (66) /* Leave IPv4 multicast membership */
#define SL_IP_HDRINCL          (67) /* Raw socket IPv4 header included. */
#define SL_IP_RAW_RX_NO_HEADER (68) /* Proprietary socket option that does not includeIPv4/IPv6 header (and extension headers) on received raw sockets*/
#define SL_IP_RAW_IPV6_HDRINCL (69) /* Transmitted buffer over IPv6 socket contains IPv6 header. */

#define SL_SO_PHY_RATE              (100)   /* WLAN Transmit rate */
#define SL_SO_PHY_TX_POWER          (101)   /* TX Power level */  
#define SL_SO_PHY_NUM_FRAMES_TO_TX  (102)   /* Number of frames to transmit */
#define SL_SO_PHY_PREAMBLE          (103)   /* Preamble for transmission */

#define SL_SO_SEC_METHOD_SSLV3                             (0)  /* security metohd SSL v3*/
#define SL_SO_SEC_METHOD_TLSV1                             (1)  /* security metohd TLS v1*/
#define SL_SO_SEC_METHOD_TLSV1_1                           (2)  /* security metohd TLS v1_1*/
#define SL_SO_SEC_METHOD_TLSV1_2                           (3)  /* security metohd TLS v1_2*/
#define SL_SO_SEC_METHOD_SSLv3_TLSV1_2                     (4)  /* use highest possible version from SSLv3 - TLS 1.2*/
#define SL_SO_SEC_METHOD_DLSV1                             (5)  /* security metohd DTL v1  */

#define SL_SEC_MASK_SSL_RSA_WITH_RC4_128_SHA               (1 << 0)
#define SL_SEC_MASK_SSL_RSA_WITH_RC4_128_MD5               (1 << 1)
#define SL_SEC_MASK_TLS_RSA_WITH_AES_256_CBC_SHA           (1 << 2)
#define SL_SEC_MASK_TLS_DHE_RSA_WITH_AES_256_CBC_SHA       (1 << 3)
#define SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA     (1 << 4)
#define SL_SEC_MASK_TLS_ECDHE_RSA_WITH_RC4_128_SHA         (1 << 5)
#define SL_SEC_MASK_SECURE_DEFAULT                         ((SEC_MASK_TLS_ECDHE_RSA_WITH_RC4_128_SHA  <<  1)  -  1)

#define SL_MSG_DONTWAIT                                   (0x00000008)  /* Nonblocking IO */

/* AP DHCP Server - IP Release reason code */
#define SL_IP_LEASE_PEER_RELEASE     (0)
#define SL_IP_LEASE_PEER_DECLINE     (1)
#define SL_IP_LEASE_EXPIRED          (2)

/* possible types when receiving SL_SOCKET_ASYNC_EVENT*/
#define SSL_ACCEPT                                (1) /* accept failed due to ssl issue ( tcp pass) */
#define RX_FRAGMENTATION_TOO_BIG                  (2) /* connection less mode, rx packet fragmentation > 16K, packet is being released */
#define OTHER_SIDE_CLOSE_SSL_DATA_NOT_ENCRYPTED   (3) /* remote side down from secure to unsecure */



#ifdef SL_INC_STD_BSD_API_NAMING

#define FD_SETSIZE                          SL_FD_SETSIZE        
                                                                       
#define SOCK_STREAM                         SL_SOCK_STREAM        
#define SOCK_DGRAM                          SL_SOCK_DGRAM         
#define SOCK_RAW                            SL_SOCK_RAW           
#define IPPROTO_TCP                         SL_IPPROTO_TCP        
#define IPPROTO_UDP                         SL_IPPROTO_UDP        
#define IPPROTO_RAW                         SL_IPPROTO_RAW        
                                                                       
#define AF_INET                             SL_AF_INET            
#define AF_INET6                            SL_AF_INET6           
#define AF_INET6_EUI_48                     SL_AF_INET6_EUI_48
#define AF_RF                               SL_AF_RF              
#define AF_PACKET                           SL_AF_PACKET              
                                                                       
#define PF_INET                             SL_PF_INET            
#define PF_INET6                            SL_PF_INET6           
                                                                       
#define INADDR_ANY                          SL_INADDR_ANY                                                   
#define ERROR                               SL_SOC_ERROR                                                                                                                
#define INEXE                               SL_INEXE                 
#define EBADF                               SL_EBADF                 
#define ENSOCK                              SL_ENSOCK                
#define EAGAIN                              SL_EAGAIN                
#define EWOULDBLOCK                         SL_EWOULDBLOCK           
#define ENOMEM                              SL_ENOMEM                
#define EACCES                              SL_EACCES                
#define EFAULT                              SL_EFAULT                
#define EINVAL                              SL_EINVAL                
#define EDESTADDRREQ                        SL_EDESTADDRREQ          
#define EPROTOTYPE                          SL_EPROTOTYPE            
#define ENOPROTOOPT                         SL_ENOPROTOOPT           
#define EPROTONOSUPPORT                     SL_EPROTONOSUPPORT       
#define ESOCKTNOSUPPORT                     SL_ESOCKTNOSUPPORT       
#define EOPNOTSUPP                          SL_EOPNOTSUPP            
#define EAFNOSUPPORT                        SL_EAFNOSUPPORT          
#define EADDRINUSE                          SL_EADDRINUSE            
#define EADDRNOTAVAIL                       SL_EADDRNOTAVAIL         
#define ENETUNREACH                         SL_ENETUNREACH           
#define ENOBUFS                             SL_ENOBUFS               
#define EOBUFF                              SL_EOBUFF                
#define EISCONN                             SL_EISCONN               
#define ENOTCONN                            SL_ENOTCONN              
#define ETIMEDOUT                           SL_ETIMEDOUT             
#define ECONNREFUSED                        SL_ECONNREFUSED          

#define SOL_SOCKET                          SL_SOL_SOCKET         
#define IPPROTO_IP                          SL_IPPROTO_IP                     
#define SO_KEEPALIVE                        SL_SO_KEEPALIVE            
                                                                       
#define SO_RCVTIMEO                         SL_SO_RCVTIMEO        
#define SO_NONBLOCKING                      SL_SO_NONBLOCKING     
                                                                       
#define IP_MULTICAST_IF                     SL_IP_MULTICAST_IF    
#define IP_MULTICAST_TTL                    SL_IP_MULTICAST_TTL   
#define IP_ADD_MEMBERSHIP                   SL_IP_ADD_MEMBERSHIP  
#define IP_DROP_MEMBERSHIP                  SL_IP_DROP_MEMBERSHIP 
                                                                       
#define socklen_t                           SlSocklen_t
#define timeval                             SlTimeval_t
#define sockaddr                            SlSockAddr_t
#define in6_addr                            SlIn6Addr_t
#define sockaddr_in6                        SlSockAddrIn6_t
#define in_addr                             SlInAddr_t
#define sockaddr_in                         SlSockAddrIn_t
                                                                       
#define MSG_DONTWAIT                        SL_MSG_DONTWAIT       
                                                                       
#define FD_SET                              SL_FD_SET  
#define FD_CLR                              SL_FD_CLR  
#define FD_ISSET                            SL_FD_ISSET
#define FD_ZERO                             SL_FD_ZERO 
#define fd_set                              SlFdSet_t    

#define socket                              sl_Socket
#define close                               sl_Close
#define accept                              sl_Accept
#define bind                                sl_Bind
#define listen                              sl_Listen
#define connect                             sl_Connect
#define select                              sl_Select
#define setsockopt                          sl_SetSockOpt
#define getsockopt                          sl_GetSockOpt
#define recv                                sl_Recv
#define recvfrom                            sl_RecvFrom
#define write                               sl_Write
#define send                                sl_Send
#define sendto                              sl_SendTo
#define gethostbyname                       sl_NetAppDnsGetHostByName
#define htonl                               sl_Htonl
#define ntohl                               sl_Ntohl
#define htons                               sl_Htons
#define ntohs                               sl_Ntohs
#endif



#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __SOCKET_H__ */


