#pragma once

#include <stdint.h>
#include "defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int printf(const char *fmt, ...);
    int puts(const char *s);
    int putchar(int c);

    uint64_t getcycles(void);

#define RAM_START 0x00000000
#define ROM_START 0x00800000

#define IO_BASE 0x01000000
#define IO_UART0 0x01001000
#define IO_EXIT 0x01002000
#define IO_MEMREADS 0x01003000

#define IO_OUT(port, value) (*(volatile uint32_t *)(port)) = (value)
#define IO_IN(port) (*(volatile uint32_t *)(port))

#ifdef __cplusplus
}
#endif
