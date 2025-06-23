#include "lib.h"

volatile uint32_t *const acc_in = (volatile uint32_t *)((void *)IO_ACC_WRITE);
volatile const uint32_t *const acc_out = (const volatile uint32_t *)IO_ACC_READ;

int main()
{
    printf("Writing to %x (%x)...\n", (int)acc_in, (int)IO_ACC_WRITE);
    acc_in[0] = 5;
    acc_in[1] = 5;
    acc_in[2] = 5;
    printf("Write complete!\n", acc_in);
    printf("Hello, World: %d!\n", *acc_out);
    return 0;
}
