#include "lib.h"

#include <cstring>

#define EIGEN_NO_IO 1
#include <Eigen/Dense>

using namespace std;

#define ACC_CHUNK_ROWS 8
#define ACC_CHUNK_COLS 4
#define ACC_INPUT_TYPE uint8_t
#define ACC_OUTPUT_TYPE uint8_t

volatile void *const acc_in = (volatile void *)IO_ACC_WRITE;
volatile const void *const acc_out = (const volatile void *)IO_ACC_READ;

#define row_of_a ((volatile ACC_INPUT_TYPE *const)acc_in)
#define matrix_b ((volatile ACC_INPUT_TYPE *const)(acc_in + ACC_CHUNK_ROWS * sizeof(ACC_INPUT_TYPE)))
#define result_row ((volatile const ACC_OUTPUT_TYPE *const)acc_out)

typedef Eigen::Matrix<ACC_INPUT_TYPE, Eigen::Dynamic, Eigen::Dynamic> Matrix;

static void print_matrix(const Matrix &mat)
{
    for (int r = 0; r < mat.rows(); r++)
    {
        for (int c = 0; c < mat.rows(); c++)
        {
            printf("%d\t", mat(r, c));
        }
        printf("\n");
    }
}

Matrix test_custom(const Matrix &a, const Matrix &b)
{
    int n = a.rows(), m = b.cols(), p = a.cols();
    // Matrix result(n, m);
    uint8_t result[m][n];
    memset(result, 0, m * n);
    for (int r = 0; r < n; r++)
    {
        for (int c = 0; c < m; c++)
        {
            for (int k = 0; k < p; k++)
            {
                result[c][r] += a(r, k) * b(k, c);
            }
        }
    }
    return Eigen::Map<Matrix>((uint8_t *)result, n, m);
}

Matrix test_eigen(const Matrix &a, const Matrix &b)
{
    Matrix c = a * b;
    return c;
}

Matrix test_accelerator(const Matrix &a, const Matrix &b)
{
    int n = a.rows(), m = b.cols(), p = a.cols();
    uint8_t result[m][n];
    memset(result, 0, m * n);
    int chunks_c = (m + ACC_CHUNK_COLS - 1) / ACC_CHUNK_COLS;
    int chunks_r = (n + ACC_CHUNK_ROWS - 1) / ACC_CHUNK_ROWS;
    for (int chunk_c = 0; chunk_c < chunks_c; chunk_c++)
    {
        int min_c = chunk_c * ACC_CHUNK_COLS, max_c = min((chunk_c + 1) * ACC_CHUNK_COLS, m);
        for (int chunk_r = 0; chunk_r < chunks_r; chunk_r++)
        {
            int min_r = chunk_r * ACC_CHUNK_ROWS, max_r = min((chunk_r + 1) * ACC_CHUNK_ROWS, p);
            // write chunk of B first
            for (int c = min_c; c < max_c; c++)
            {
                for (int r = min_r; r < max_r; r++)
                {
                    matrix_b[(c - min_c) * ACC_CHUNK_ROWS + r - min_r] = b(r, c);
                }
            }
            // then for each row of A
            for (int r = 0; r < n; r++)
            {
                // write the row...
                for (int c = min_r; c < max_r; c++)
                {
                    row_of_a[c - min_r] = a(r, c);
                }
                // ... then read the results and accumulate.
                for (int c = min_c; c < max_c; c++)
                {
                    result[c][r] += result_row[c - min_c];
                }
            }
        }
    }
    return Eigen::Map<Matrix>((uint8_t *)result, n, m);
    // assert(a.rows() % ACC_CHUNK_SIZE == 0 && a.cols() % ACC_CHUNK_SIZE == 0 && b.rows() % ACC_CHUNK_SIZE == 0 && b.cols() % ACC_CHUNK_SIZE == 0);
}

#define test(func)                                      \
    do                                                  \
    {                                                   \
        int cycles0 = getcycles();                      \
        Matrix c = func(a, b);                          \
        int cycles1 = getcycles();                      \
        printf("Cycles used: %d\n", cycles1 - cycles0); \
        if (c != answer)                                \
        {                                               \
            printf("Incorrect! Result:\n");             \
            print_matrix(c);                            \
        }                                               \
        else                                            \
        {                                               \
            printf("Answer is correct!\n");             \
        }                                               \
    } while (0)

// for (int r = 0; r < c.rows(); r++)                                      \
//     for (int x = 0; x < c.cols(); x++)                                  \
//         if (c(r, x) != answer(r, x))                                    \
//             printf("(%d, %d): %d / %d\n", r, x, c(r, x), answer(r, x)); \

int main()
{
    Matrix a(16, 16), b(16, 16);
    // a.setRandom();
    // b.setRandom();
    for (int r = 0; r < 16; r++)
        for (int c = 0; c < 16; c++)
            a(r, c) = b(r, c) = rand() % 2 + 1;
    printf("Matrix A:\n");
    print_matrix(a);
    printf("Matrix B:\n");
    print_matrix(b);

    Matrix answer = a * b;
    printf("Expected result:\n");
    print_matrix(answer);

    printf("\n\n------ Testing Custom Method ------\n\n");
    test(test_custom);

    printf("\n\n------ Testing Eigen Method ------\n\n");
    test(test_eigen);

    printf("\n\n------ Testing with Accelerator ------\n\n");
    test(test_accelerator);

    return 0;
}
