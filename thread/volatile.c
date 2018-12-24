#include <stdio.h>

int main(int argc, char *argv[]) 
{
    volatile int vol = 0;
    while (1) {
        vol = vol + 1;
    }

    return 0;
}
