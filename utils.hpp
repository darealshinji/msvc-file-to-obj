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

#if !defined(_MSC_VER)
# include <strings.h>
#endif
#include <string.h>
#include <filesystem>
#include <locale>


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

