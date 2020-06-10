## Overview

for array allocations we do some pow(x,y) calculations when
forming the tree. can these be optimized? lets check pow()
against an int based exponentiation by squaring technique.

## Code

```
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

int ipow(int base, int exp)
{
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

#define N 10000000
int alottapow()
{
    int x = 0;
    for(int i = 0 ; i < N ; i++)
        x += pow(38, i);
    return(x);
}

int alottaipow()
{
    int x = 0;
    for(int i = 0 ; i < N ; i++)
        x += ipow(38, i);
    return(x);
}

int main()
{
    struct timespec tstart={0,0}, tend={0,0};
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    alottapow();
    clock_gettime(CLOCK_MONOTONIC, &tend);
    printf("pow took about %.5f seconds\n",
           ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
           ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));


    clock_gettime(CLOCK_MONOTONIC, &tstart);
    alottaipow();
    clock_gettime(CLOCK_MONOTONIC, &tend);
    printf("ipow took about %.5f seconds\n",
           ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
           ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));

    return (0);
}
```

## Results

On 64 bit (OSX and Linux) libc's pow is faster. On 32 bit linux, ipow was faster.

 OSX pow took about 3.01379 seconds.  ipow took about 0.84663 seconds

 Linux64 pow took about 0.42582 seconds. ipow took about 1.06445 seconds

 Linux32 pow took about 3.01379 seconds.  ipow took about 0.84663 seconds



