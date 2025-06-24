#include "lib.h"

#include <string.h>

#define ACC_CHUNK_SIZE 4

volatile uint32_t *const acc_in = (volatile uint32_t *)IO_ACC_WRITE;
volatile const uint32_t *const acc_out = (const volatile uint32_t *)IO_ACC_READ;

#define row_of_a ((volatile uint32_t *const)acc_in)
#define matrix_b ((volatile uint32_t *const)(acc_in + ACC_CHUNK_SIZE))
#define result_row ((volatile const uint64_t *const)acc_out)

int main()
{
    for (int i = 0; i < ACC_CHUNK_SIZE; i++) {
        row_of_a[i] = 1;
    }
    for (int i = 0; i < ACC_CHUNK_SIZE; i++) {
        printf("row_of_a[%d]:%d\n", i, row_of_a[i]);
    }
    for (int c = 0; c < ACC_CHUNK_SIZE; c++) {
        for (int r = 0; r < ACC_CHUNK_SIZE; r++) {
            matrix_b[c*ACC_CHUNK_SIZE+r] = c+1;
        }
    }
    for (int c = 0; c < ACC_CHUNK_SIZE; c++) {
        for (int r = 0; r < ACC_CHUNK_SIZE; r++) {
            printf("%d ", matrix_b[c*ACC_CHUNK_SIZE+r]);
        }
        printf("\n");
    }
    for (int i = 0; i < ACC_CHUNK_SIZE; i++) {
        printf("%d:%d\n", i, result_row[i]);
    }

    return 0;
}
