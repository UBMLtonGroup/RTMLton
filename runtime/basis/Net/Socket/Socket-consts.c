#include "platform.h"

#if defined(__rtems__)
#undef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#include <sys/socket.h>
//#define MSG_DONTWAIT 0
#define IPPROTO_TCP 0
#define TCP_NODELAY 0
#define IPPROTO_TCP 0
#endif

const C_Size_t Socket_sockAddrStorageLen = sizeof (struct sockaddr_storage);
const C_Int_t Socket_AF_INET = AF_INET;
const C_Int_t Socket_AF_INET6 = AF_INET6;
const C_Int_t Socket_AF_UNIX = AF_UNIX;
const C_Int_t Socket_AF_UNSPEC = AF_UNSPEC;
const C_Int_t Socket_Ctl_SOL_SOCKET = SOL_SOCKET;
const C_Int_t Socket_Ctl_SO_ACCEPTCONN = SO_ACCEPTCONN;
const C_Int_t Socket_Ctl_SO_BROADCAST = SO_BROADCAST;
const C_Int_t Socket_Ctl_SO_DEBUG = SO_DEBUG;
const C_Int_t Socket_Ctl_SO_DONTROUTE = SO_DONTROUTE;
const C_Int_t Socket_Ctl_SO_ERROR = SO_ERROR;
const C_Int_t Socket_Ctl_SO_KEEPALIVE = SO_KEEPALIVE;
const C_Int_t Socket_Ctl_SO_LINGER = SO_LINGER;
const C_Int_t Socket_Ctl_SO_OOBINLINE = SO_OOBINLINE;
const C_Int_t Socket_Ctl_SO_RCVBUF = SO_RCVBUF;
const C_Int_t Socket_Ctl_SO_RCVLOWAT = SO_RCVLOWAT;
const C_Int_t Socket_Ctl_SO_RCVTIMEO = SO_RCVTIMEO;
const C_Int_t Socket_Ctl_SO_REUSEADDR = SO_REUSEADDR;
const C_Int_t Socket_Ctl_SO_SNDBUF = SO_SNDBUF;
const C_Int_t Socket_Ctl_SO_SNDLOWAT = SO_SNDLOWAT;
const C_Int_t Socket_Ctl_SO_SNDTIMEO = SO_SNDTIMEO;
const C_Int_t Socket_Ctl_SO_TYPE = SO_TYPE;
const C_Int_t Socket_MSG_CTRUNC = MSG_CTRUNC;
const C_Int_t Socket_MSG_DONTROUTE = MSG_DONTROUTE;
const C_Int_t Socket_MSG_DONTWAIT = MSG_DONTWAIT;
const C_Int_t Socket_MSG_EOR = MSG_EOR;
const C_Int_t Socket_MSG_OOB = MSG_OOB;
const C_Int_t Socket_MSG_PEEK = MSG_PEEK;
const C_Int_t Socket_MSG_TRUNC = MSG_TRUNC;
const C_Int_t Socket_MSG_WAITALL = MSG_WAITALL;
const C_Int_t Socket_SHUT_RD = SHUT_RD;
const C_Int_t Socket_SHUT_RDWR = SHUT_RDWR;
const C_Int_t Socket_SHUT_WR = SHUT_WR;
const C_Int_t Socket_SOCK_DGRAM = SOCK_DGRAM;
const C_Int_t Socket_SOCK_RAW = SOCK_RAW;
const C_Int_t Socket_SOCK_SEQPACKET = SOCK_SEQPACKET;
const C_Int_t Socket_SOCK_STREAM = SOCK_STREAM;
