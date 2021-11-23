#include "platform.h"
#if defined(__rtems__)
#undef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#undef __POSIX_VISIBLE
#define __POSIX_VISIBLE  200112
#include <netdb.h>
#include <netinet/in.h>
#endif

Word32_t Net_htonl (Word32_t w) {
  Word32_t r = htonl (w);
  return r;
}

Word32_t Net_ntohl (Word32_t w) {
  Word32_t r = ntohl (w);
  return r;
}

Word16_t Net_htons (Word16_t w) {
  Word16_t r = htons (w);
  return r;
}

Word16_t Net_ntohs (Word16_t w) {
  Word16_t r = ntohs (w);
  return r;
}
