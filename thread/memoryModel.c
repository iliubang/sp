#include <stdio.h>

struct alignas(64) DS {
    int alignas(int) a, b, c, d;
    atomic_int spinlock;
};

int main(int argc, char **argv) 
{
    volatile int a;
    int b;

    b = 0xdead;
    a = 0xc0fe;

    printf("a = %x, b = %x\n", a, b);
    return 0;
}
