void *__dso_handle = 0;

#include "lib.h"

#include <cstring>

#define EIGEN_NO_IO 1
#include <Eigen/Dense>

using namespace std;

#ifndef TEST_OTHER_METHODS
#define TEST_OTHER_METHODS 1
#endif

#ifndef TEST_ACCELERATOR
#define TEST_ACCELERATOR 1
#endif

#ifndef DEBUG
#define DEBUG 1
#endif

#if DEBUG
#define dprintf printf
#else
#define dprintf(...) /**/
#endif

#define MAX_TESTS 50

#define ACC_CHUNK_ROWS_RAW (*(volatile const uint32_t *)(IO_ACC_DREAD + 0))
#define ACC_CHUNK_COLS_RAW (*(volatile const uint32_t *)(IO_ACC_DREAD + 4))

int ACC_CHUNK_ROWS, ACC_CHUNK_COLS;

#ifndef ACC_INPUT_WIDTH
#define ACC_INPUT_WIDTH 8
#endif
#ifndef ACC_OUTPUT_WIDTH
#define ACC_OUTPUT_WIDTH 16
#endif

#define MAKEINT2(x) uint##x##_t
#define MAKEINT(x) MAKEINT2(x)
#define ACC_INPUT_TYPE MAKEINT(ACC_INPUT_WIDTH)
#define ACC_OUTPUT_TYPE MAKEINT(ACC_OUTPUT_WIDTH)

volatile void *const acc_in = (volatile void *)IO_ACC_WRITE;
volatile const void *const acc_out = (const volatile void *)IO_ACC_READ;

#define row_of_a ((volatile ACC_INPUT_TYPE *const)acc_in)
#define matrix_b ((volatile ACC_INPUT_TYPE *const)(acc_in + ACC_CHUNK_ROWS * sizeof(ACC_INPUT_TYPE)))
#define result_row ((volatile const ACC_OUTPUT_TYPE *const)acc_out)

typedef Eigen::Matrix<ACC_INPUT_TYPE, Eigen::Dynamic, Eigen::Dynamic> Matrix;
typedef Eigen::Matrix<ACC_OUTPUT_TYPE, Eigen::Dynamic, Eigen::Dynamic> OutMatrix;

template <typename M>
static void print_matrix(const Eigen::Matrix<M, Eigen::Dynamic, Eigen::Dynamic> &mat)
{
    for (int r = 0; r < min(mat.rows(), 16); r++)
    {
        for (int c = 0; c < min(mat.cols(), 16); c++)
        {
            dprintf("%d\t", mat(r, c));
        }
        dprintf("\n");
    }
}

OutMatrix test_custom(const Matrix &a, const Matrix &b)
{
    int n = a.rows(), m = b.cols(), p = a.cols();
    // Matrix result(n, m);
    ACC_OUTPUT_TYPE result[m][n];
    memset(result, 0, m * n * sizeof(ACC_OUTPUT_TYPE));
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
    return Eigen::Map<OutMatrix>((ACC_OUTPUT_TYPE *)result, n, m);
}

OutMatrix test_eigen(const Matrix &a, const Matrix &b)
{
    OutMatrix c = a.cast<ACC_OUTPUT_TYPE>() * b.cast<ACC_OUTPUT_TYPE>();
    return c;
}

OutMatrix test_accelerator(const Matrix &a, const Matrix &b)
{
    int n = a.rows(), m = b.cols(), p = a.cols();
    ACC_OUTPUT_TYPE result[m][n];
    memset(result, 0, m * n * sizeof(ACC_OUTPUT_TYPE));
    int chunks_c = (m + ACC_CHUNK_COLS - 1) / ACC_CHUNK_COLS;
    int chunks_r = (p + ACC_CHUNK_ROWS - 1) / ACC_CHUNK_ROWS;
    for (int chunk_c = 0; chunk_c < chunks_c; chunk_c++)
    {
        int min_c = chunk_c * ACC_CHUNK_COLS, max_c = min((chunk_c + 1) * ACC_CHUNK_COLS, m);
        for (int chunk_r = 0; chunk_r < chunks_r; chunk_r++)
        {
            int min_r = chunk_r * ACC_CHUNK_ROWS, max_r = min((chunk_r + 1) * ACC_CHUNK_ROWS, p);
            // write chunk of B first
            for (int c = min_c; c < min_c + ACC_CHUNK_COLS; c++)
            {
                for (int r = min_r; r < min_r + ACC_CHUNK_ROWS; r++)
                {
                    matrix_b[(c - min_c) * ACC_CHUNK_ROWS + r - min_r] = c >= max_c || r >= max_r ? 0 : b(r, c);
                }
            }
            // then for each row of A
            for (int r = 0; r < n; r++)
            {
                // write the row...
                for (int c = min_r; c < min_r + ACC_CHUNK_ROWS; c++)
                {
                    row_of_a[c - min_r] = c >= max_r ? 0 : a(r, c);
                }
                // ... then read the results and accumulate.
                for (int c = min_c; c < max_c; c++)
                {
                    result[c][r] += result_row[c - min_c];
                }
            }
        }
    }
    return Eigen::Map<OutMatrix>((ACC_OUTPUT_TYPE *)result, n, m);
}

OutMatrix (*methods[])(const Matrix &, const Matrix &) = {
#if TEST_OTHER_METHODS
    test_custom, test_eigen,
#endif
#if TEST_ACCELERATOR
    test_accelerator
#endif
};
const char *method_names[] = {
#if TEST_OTHER_METHODS
    "Custom Method", "Eigen Method",
#endif
#if TEST_ACCELERATOR
    "Using Accelerator"
#endif
};
constexpr int n_methods = sizeof(methods) / sizeof(methods[0]);

struct method_result
{
    const char *name;
    int cycles;
    bool correct;
};

struct test_result
{
    const char *test_name;
    method_result methods[n_methods];
};

test_result test_results[MAX_TESTS];
int n_test_results = 0;

void do_testcase(const char *name, int n, int m, int p)
{
    test_result res;
    res.test_name = name;

    Matrix a(n, m), b(m, p);
    a.setRandom();
    b.setRandom();

    dprintf("Matrix A:\n");
    print_matrix(a);
    dprintf("Matrix B:\n");
    print_matrix(b);

    OutMatrix answer = a.cast<ACC_OUTPUT_TYPE>() * b.cast<ACC_OUTPUT_TYPE>();
    dprintf("Expected result:\n");
    print_matrix(answer);

    for (int i = 0; i < sizeof(methods) / sizeof(methods[0]); i++)
    {
        method_result mres;
        auto &func = methods[i];
        auto &name = method_names[i];
        mres.name = name;
        dprintf("\n\n------ Testing %s ------\n\n", name);
        int cycles0 = getcycles();
        OutMatrix result = func(a, b);
        int cycles1 = getcycles();
        mres.cycles = cycles1 - cycles0;
        mres.correct = true;
        dprintf("Cycles used: %d\n", cycles1 - cycles0);
        if (result != answer)
        {
            mres.correct = false;
            dprintf("Incorrect! Result:\n");
            print_matrix(result);
        }
        else
        {
            dprintf("Answer is correct!\n");
        }
        res.methods[i] = move(mres);
    }

    test_results[n_test_results++] = move(res);
}

void print_summary(void)
{
    dprintf("\n\n\n------ T E S T   S U M M A R Y ------\n");
    for (int i = 0; i < n_test_results; i++)
    {
        auto &result = test_results[i];
        dprintf("\nTest %s:\n", result.test_name);
        for (int j = 0; j < n_methods; j++)
        {
            auto &method = result.methods[j];
            dprintf("- Test %s: %d cycles, %s\n", method.name, method.cycles, method.correct ? "correct" : "**INCORRECT!**");
        }
    }
}

void print_json(void)
{
    printf("{\"tests\":[");
    for (int i = 0; i < n_test_results; i++)
    {
        auto &result = test_results[i];
        printf("{\"name\":\"%s\",\"methods\":[", result.test_name);
        for (int j = 0; j < n_methods; j++)
        {
            auto &method = result.methods[j];
            printf("{\"name\":\"%s\",\"cycles\":%d,\"correct\":%s}", method.name, method.cycles, method.correct ? "true" : "false");
            if (j != n_methods - 1)
            {
                printf(",");
            }
        }
        printf("]}");
        if (i != n_test_results - 1)
        {
            printf(",");
        }
    }
    printf("],\"info\":{\"acc_rows\":%d,\"acc_cols\":%d,\"acc_inwidth\":%d,\"acc_outwidth\":%d}}\n", ACC_CHUNK_ROWS, ACC_CHUNK_COLS, sizeof(ACC_INPUT_TYPE), sizeof(ACC_OUTPUT_TYPE));
}

int main()
{
    ACC_CHUNK_ROWS = ACC_CHUNK_ROWS_RAW;
    ACC_CHUNK_COLS = ACC_CHUNK_COLS_RAW;

    dprintf("Accelerator size: %dx%d", ACC_CHUNK_ROWS, ACC_CHUNK_COLS);

    do_testcase("1x2 x 2x3", 1, 2, 3);
    do_testcase("16x16 x 16x16", 16, 16, 16);
    do_testcase("32x32 x 32x32", 32, 32, 32);
    do_testcase("16x32 x 32x32", 16, 32, 32);
    do_testcase("16x32 x 32x64", 16, 32, 64);
    do_testcase("64x32 x 32x16", 64, 32, 16);

    print_summary();
    print_json();

    return 0;
}
