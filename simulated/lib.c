#include "lib.h"

#include <stdarg.h>
#include <sys/stat.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>

static void print_string(const char *s)
{
    while (*s)
        putchar(*s++);
}

static void print_hex_digits(unsigned int val, int nbdigits)
{
    for (int i = (4 * nbdigits) - 4; i >= 0; i -= 4)
        putchar("0123456789ABCDEF"[(val >> i) % 16]);
}

static void print_hex(unsigned int val)
{
    print_hex_digits(val, 8);
}

static void print_dec(int val)
{
    if (val < 0)
    {
        putchar('-');
        val = -val;
    }
    if (val == 0)
    {
        putchar('0');
        return;
    }
    char buffer[11]; // Enough for 32-bit int
    int i = 0;
    while (val > 0)
    {
        buffer[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0)
        putchar(buffer[--i]);
}

int putchar(int c)
{
    IO_OUT(IO_UART0, c);
    return c;
}

int puts(const char *s)
{
    print_string(s);
    putchar('\n');
    return 0;
}

int printf(const char *fmt, ...)
{
    va_list ap;
    for (va_start(ap, fmt); *fmt; fmt++)
    {
        if (*fmt == '%')
        {
            fmt++;
            if (*fmt == 's')
                print_string(va_arg(ap, char *));
            else if (*fmt == 'x')
                print_hex(va_arg(ap, int));
            else if (*fmt == 'd')
                print_dec(va_arg(ap, int));
#ifdef ENABLE_PRINTF_FLOAT
            else if (*fmt == 'f')
                print_double(va_arg(ap, double));
#endif
            else if (*fmt == 'c')
                putchar(va_arg(ap, int));
            else
                putchar(*fmt);
        }
        else
            putchar(*fmt);
    }
    va_end(ap);
    return 0;
}

uint64_t getcycles(void)
{
    uint32_t lo, hi;
    asm volatile("rdcycle %0" : "=r"(lo));
    asm volatile("rdcycleh %0" : "=r"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/*
The following is adapted from Clifford Wolf's syscalls.c in the picorv32
repository. The original license information is provided below.

ISC License

Copyright (C) 2015 - 2021  Claire Xenia Wolf <claire@yosyshq.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stddef.h>
#include <errno.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UNIMPL_FUNC(_f) ".globl " #_f "\n.type " #_f ", @function\n" #_f ":\n"

// clang-format off
asm (
	".text\n"
	".align 2\n"
	UNIMPL_FUNC(_open)
	UNIMPL_FUNC(_openat)
	UNIMPL_FUNC(_lseek)
	UNIMPL_FUNC(_stat)
	UNIMPL_FUNC(_lstat)
	UNIMPL_FUNC(_fstatat)
	UNIMPL_FUNC(_isatty)
	UNIMPL_FUNC(_access)
	UNIMPL_FUNC(_faccessat)
	UNIMPL_FUNC(_link)
	UNIMPL_FUNC(_unlink)
	UNIMPL_FUNC(_execve)
	UNIMPL_FUNC(_getpid)
	UNIMPL_FUNC(_fork)
	UNIMPL_FUNC(_kill)
	UNIMPL_FUNC(_wait)
	UNIMPL_FUNC(_times)
	UNIMPL_FUNC(_gettimeofday)
	UNIMPL_FUNC(_ftime)
	UNIMPL_FUNC(_utime)
	UNIMPL_FUNC(_chown)
	UNIMPL_FUNC(_chmod)
	UNIMPL_FUNC(_chdir)
	UNIMPL_FUNC(_getcwd)
	UNIMPL_FUNC(_sysconf)
	"j unimplemented_syscall\n"
);
// clang-format on

void unimplemented_syscall()
{
    IO_OUT(IO_EXIT, 255);
    while (1) ;
    // asm volatile("ebreak");
    __builtin_unreachable();
}

ssize_t _read(int file, void *ptr, size_t len)
{
    // always EOF
    return 0;
}

ssize_t _write(int file, const void *ptr, size_t len)
{
    if (file == 1)
        for (size_t i = 0; i < len; ++i)
            putchar(((char *)ptr)[i]);

    return 0;
}

int _close(int file)
{
    // close is called before _exit()
    return 0;
}

int _fstat(int file, struct stat *st)
{
    // fstat is called during libc startup
    errno = ENOENT;
    return -1;
}

void *_sbrk(ptrdiff_t incr)
{
    extern unsigned char _end[]; // Defined by linker
    static unsigned long heap_end = 0;

    if (heap_end == 0)
        heap_end = (long)_end;

    heap_end += incr;
    return (void *)(heap_end - incr);
}

void _exit(int exit_status)
{
    IO_OUT(IO_EXIT, exit_status);
    while (1) ;
    __builtin_unreachable();
}

#ifdef __cplusplus
}
#endif
