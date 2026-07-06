/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2026 Carsten Janssen

 Permission is hereby  granted, free of charge, to any  person obtaining a copy
 of this software and associated  documentation files (the "Software"), to deal
 in the Software  without restriction, including without  limitation the rights
 to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
**/

#pragma once

#ifdef _MSC_VER
# include <intrin.h>
#else
# include <strings.h>
#endif
#ifdef __MINGW32__
# include <locale>
#endif
#include <stdint.h>
#include <string.h>
#include <bit>       /* std::endian, std::byteswap */
#include <concepts>  /* std::integral */
#include <filesystem>


/* MSVC compat */
#ifdef _MSC_VER

inline int strcasecmp(const char *a, const char *b) {
    return _stricmp(a, b);
}

inline int strncasecmp(const char *a, const char *b, size_t n) {
    return _strnicmp(a, b, n);
}

#endif //_MSC_VER


/* case-insensitive comparison if string begins with prefix */
template<size_t N>
bool strbeg(const char *str, char const (&pfx)[N]) {
    return (strncasecmp(str, pfx, N-1) == 0);
}


template<typename T>
std::filesystem::path make_fs_path(T name)
{
#ifdef __MINGW32__
    return std::filesystem::path(name, std::locale());
#else
    return std::filesystem::path(name);
#endif
}


template<std::integral T>
constexpr T byteswap(T val) noexcept
{
#ifdef __cpp_lib_byteswap

    return std::byteswap(val);

#elif defined(_MSC_VER) && !defined(__clang__)

    if constexpr (sizeof(T) == 2) {
        return _byteswap_ushort(static_cast<uint16_t>(val));
    } else if constexpr (sizeof(T) == 4) {
        return _byteswap_ulong(static_cast<uint32_t>(val));
    }

#elif defined(__GNUC__) || defined(__clang__)

    if constexpr (sizeof(T) == 2) {
        return __builtin_bswap16(static_cast<uint16_t>(val));
    } else if constexpr (sizeof(T) == 4) {
        return __builtin_bswap32(static_cast<uint32_t>(val));
    }

#else

#error "byteswap not implemented"

#endif
}


template<std::integral T>
constexpr T htobe(T value) noexcept
{
    if constexpr (std::endian::native == std::endian::big) {
        return value;
    } else {
        return byteswap(value);
    }
}


template<std::integral T>
constexpr T htole(T value) noexcept
{
    if constexpr (std::endian::native == std::endian::little) {
        return value;
    } else {
        return byteswap(value);
    }
}
