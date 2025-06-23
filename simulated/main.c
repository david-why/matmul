#include "lib.h"

#include <string.h>

volatile uint32_t *const acc_in = (volatile uint32_t *)((void *)IO_ACC_WRITE);
volatile const uint32_t *const acc_out = (const volatile uint32_t *)IO_ACC_READ;

uint64_t multiply_with_naive(const int *data, int n)
{
    uint64_t val = 1;
    for (int i = 0; i < n; i++)
    {
        val *= data[i];
    }
    return val;
}

uint64_t multiply_with_accelerator(const int *data, int n)
{
    if (n % 3)
    {
        return -1;
    }
    uint64_t val = 1;
    for (int i = 0; i < n / 3; i++)
    {
        *acc_in = data[i*3 + 0];
        *(acc_in+1) = data[i*3 + 1];
        *(acc_in+2) = data[i*3 + 2];
        val *= *(volatile uint64_t *)acc_out;
    }
    return val;
}

int main()
{
    // printf("Writing to %x (%x)...\n", (int)acc_in, (int)IO_ACC_WRITE);
    // acc_in[0] = 5;
    // acc_in[1] = 5;
    // acc_in[2] = 5;
    // printf("Write complete!\n", acc_in);
    // printf("Hello, World: %d!\n", *acc_out);
    const static int data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    uint64_t naive0 = getcycles();
    int naive_result = multiply_with_naive(data, 9);
    uint64_t naive1 = getcycles();
 
    uint64_t acc0 = getcycles();
    int acc_result = multiply_with_accelerator(data, 9);
    uint64_t acc1 = getcycles();

    printf("naive: %d, %d\n", naive_result, naive1 - naive0);
    printf("acc  : %d, %d\n", acc_result, acc1 - acc0);

    return 0;
}
