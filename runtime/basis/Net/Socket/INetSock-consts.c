#include "platform.h"

#if defined(__rtems__)
#undef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#include <sys/socket.h>
#define MSG_DONTWAIT 0
#define IPPROTO_TCP 0
#define TCP_NODELAY 0
#define IPPROTO_TCP 0
#endif

const C_Int_t Socket_INetSock_Ctl_IPPROTO_TCP = IPPROTO_TCP;
const C_Int_t Socket_INetSock_Ctl_TCP_NODELAY = TCP_NODELAY;
