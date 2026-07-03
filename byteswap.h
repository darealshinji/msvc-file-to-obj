/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Carsten Janssen
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
#include <stdlib.h>


inline uint16_t byteswap_16(uint16_t val);
inline uint32_t byteswap_32(uint32_t val);
inline uint64_t byteswap_64(uint64_t val);

inline uint16_t HtoBE16(uint16_t host_16bits);
inline uint16_t HtoLE16(uint16_t host_16bits);
inline uint16_t BEtoH16(uint16_t big_endian_16bits);
inline uint16_t LEtoH16(uint16_t little_endian_16bits);

inline uint32_t HtoBE32(uint32_t host_32bits);
inline uint32_t HtoLE32(uint32_t host_32bits);
inline uint32_t BEtoH32(uint32_t big_endian_32bits);
inline uint32_t LEtoH32(uint32_t little_endian_32bits);

inline uint64_t HtoBE64(uint64_t host_64bits);
inline uint64_t HtoLE64(uint64_t host_64bits);
inline uint64_t BEtoH64(uint64_t big_endian_64bits);
inline uint64_t LEtoH64(uint64_t little_endian_64bits);


inline uint16_t byteswap_16(uint16_t x)
{
#if defined(__GNUC__) || defined(__clang__) || defined(HAVE_BUILTIN_BSWAP16)
    return __builtin_bswap16(x);
#elif defined(_MSC_VER) || defined(HAVE_BYTESWAP_USHORT)
    return _byteswap_ushort(x);
#else
    uint16_t r = 0;
    uint8_t *px = (uint8_t *)&x;
    uint8_t *pr = (uint8_t *)&r;
    pr[0] = px[1];
    pr[1] = px[0];
    return r;
#endif
}


inline uint32_t byteswap_32(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__) || defined(HAVE_BUILTIN_BSWAP32)
    return __builtin_bswap32(x);
#elif defined(_MSC_VER) || defined(HAVE_BYTESWAP_ULONG)
    return _byteswap_ulong(x);
#else
    uint32_t r = 0;
    uint8_t *px = (uint8_t *)&x;
    uint8_t *pr = (uint8_t *)&r;
    pr[0] = px[3];
    pr[1] = px[2];
    pr[2] = px[1];
    pr[3] = px[0];
    return r;
#endif
}


inline uint64_t byteswap_64(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__) || defined(HAVE_BUILTIN_BSWAP64)
    return __builtin_bswap64(x);
#elif defined(_MSC_VER) || defined(HAVE_BYTESWAP_UINT64)
    return _byteswap_uint64(x);
#else
    uint64_t r = 0;
    uint8_t *px = (uint8_t *)&x;
    uint8_t *pr = (uint8_t *)&r;
    pr[0] = px[7];
    pr[1] = px[6];
    pr[2] = px[5];
    pr[3] = px[4];
    pr[4] = px[3];
    pr[5] = px[2];
    pr[6] = px[1];
    pr[7] = px[0];
    return r;
#endif
}


/**
 * get the Endianness at runtime; compiler optimizations will
 * effectively turn the result into a compile-time value
 */
inline uint8_t host_byteorder(void)
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


#define RETURN(ORDER, BITS) \
    return (host_byteorder() == ORDER) ? x : byteswap_##BITS(x)

#define IMPLEMENT(TYPE, BITS) \
    inline TYPE HtoBE##BITS(TYPE x) { RETURN(0xbe, BITS); } \
    inline TYPE HtoLE##BITS(TYPE x) { RETURN(0x1e, BITS); } \
    inline TYPE BEtoH##BITS(TYPE x) { RETURN(0xbe, BITS); } \
    inline TYPE LEtoH##BITS(TYPE x) { RETURN(0x1e, BITS); }

IMPLEMENT(uint16_t, 16)
IMPLEMENT(uint32_t, 32)
IMPLEMENT(uint64_t, 64)

#undef IMPLEMENT
#undef RETURN

