/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2024-2026 Carsten Janssen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
# define INCBIN_EXTERN  extern "C"
#else
# define INCBIN_EXTERN  extern
#endif

/* suffix used by symbols */
#define INCBIN_SUFFIX_BIG       _INCBIN_SIZE_BIG
#define INCBIN_SUFFIX_LITTLE    _INCBIN_SIZE_LITTLE
#define INCBIN_SYMLEN_BIG(x)    x ## _INCBIN_SIZE_BIG
#define INCBIN_SYMLEN_LITTLE(x) x ## _INCBIN_SIZE_LITTLE


/**
 * get the Endianness at runtime; compiler optimizations will
 * effectively turn the result into a compile-time value
 */
inline uint8_t incbin_byteorder(void)
{
    const uint32_t num = 0xAABBCCDD;
    const uint8_t *p = (const uint8_t *)&num;

    if (p[0]==0xAA &&
        p[1]==0xBB &&
        p[2]==0xCC &&
        p[3]==0xDD)
    {
        /* Big Endian */
        return 0xbe;
    }

    if (p[0]==0xDD &&
        p[1]==0xCC &&
        p[2]==0xBB &&
        p[3]==0xAA)
    {
        /* Little Endian */
        return 0x1e;
    }

    /* Mixed Endian? */
    return 0;
}


/**
 * reference the data
 *
 * "INCBIN(data);" for example will become:
 *   extern const uint8_t  data[];
 *   extern const uint32_t data_INCBIN_SIZE_BIG;
 *   extern const uint32_t data_INCBIN_SIZE_LITTLE;
 */
#define INCBIN(SYMBOL) \
    INCBIN_EXTERN const uint8_t  SYMBOL[]; \
    INCBIN_EXTERN const uint32_t INCBIN_SYMLEN_BIG(SYMBOL); \
    INCBIN_EXTERN const uint32_t INCBIN_SYMLEN_LITTLE(SYMBOL)


/* receive data size */
#define INCBIN_SIZE(SYMBOL) \
    (incbin_byteorder() == 0xBE \
     ? INCBIN_SYMLEN_BIG(SYMBOL) \
     : INCBIN_SYMLEN_LITTLE(SYMBOL))

