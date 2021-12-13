#include "platform.h"

#if defined(__rtems__)
#include <netdb.h>
#include <netinet/in.h>
#endif

const C_Size_t NetHostDB_inAddrSize = sizeof (struct in_addr);
const C_Int_t NetHostDB_INADDR_ANY = INADDR_ANY;
