#include <stdio.h>
#include "arith.h"

#ifndef Long
#define Long long
#endif

typedef unsigned Long Ulong;

#undef HAVE_IEEE
#ifdef IEEE_8087
#define _0 1
#define _1 0
#define _3 3
static int perm[4] = { 0, 1, 2, 3 };
#define HAVE_IEEE
#endif
#ifdef IEEE_MC68k
#define _0 0
#define _1 1
#define _3 0
static int perm[4] = { 3, 2, 1, 0 };
#define HAVE_IEEE
#endif

#define UL (unsigned long)

 int
mainX(void)
{
#ifdef HAVE_IEEE
        typedef union {
                float f;
                double d;
                Ulong L[4];
#ifndef NO_LONG_LONG
                unsigned short u[5];
                long double D;
#endif
                } __attribute__((__may_alias__)) U;
        U a, b, c;
        int i;

        a.L[0] = b.L[0] = 0x7f800000;
        c.f = a.f - b.f;
        printf("#define f_QNAN 0x%lx\n", UL (c.L[0] & 0x7fffffff));
        a.L[_0] = b.L[_0] = 0x7ff00000;
        a.L[_1] = b.L[_1] = 0;
        c.d = a.d - b.d;        /* quiet NaN */
        c.L[_0] &= 0x7fffffff;
        printf("#define d_QNAN0 0x%lx\n", UL c.L[_0]);
        printf("#define d_QNAN1 0x%lx\n", UL c.L[_1]);
#ifndef NO_LONG_LONG
#ifdef Gen_ld_QNAN
        if (sizeof(a.D) >= 16) {
                b.D = c.D = a.d;
                if (printf("") < 0)
                        c.D = 37;       /* never executed; just defeat optimization */
                a.L[0] = a.L[1] = a.L[2] = a.L[3] = 0;
                a.D = b.D - c.D;
                a.L[_3] &= 0x7fffffff;
                for(i = 0; i < 4; i++)
                        printf("#define ld_QNAN%d 0x%lx\n", i, UL a.L[perm[i]]);
                }
#endif
#endif
#endif /* HAVE_IEEE */
        return 0;
        }
