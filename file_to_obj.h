#pragma once

#include <stdint.h>

#ifdef _MSC_VER
# pragma comment(lib, "ws2_32")
# include <winsock2.h>   /* htonl */
# define strcasecmp  _stricmp
# define strncasecmp _strnicmp
#else
# include <arpa/inet.h>  /* htonl */
# include <endian.h>     /* htole32 */
# include <strings.h>
#endif

#if defined(_MSC_VER) && !defined(__clang__)
# define BSWAP16(x)  _byteswap_ushort(x)
# define BSWAP32(x)  _byteswap_ulong(x)
#else
# define BSWAP16(x)  __builtin_bswap16(x)
# define BSWAP32(x)  __builtin_bswap32(x)
#endif

#ifdef _MSC_VER
# if defined(_M_X64) || defined(_M_AMD64) || defined(_M_IX86)
#  define htole16(x)  x  /* x86 is always LE */
#  define htole32(x)  x  /* x86 is always LE */
# else
#  define htole16(x)  BSWAP16(htons(x))
#  define htole32(x)  BSWAP32(htonl(x))
# endif
#endif

const char *simple_basename(const char *str);
void save_to_coff(const char *infile, const char *outfile, const uint8_t *machine, const char *symbol);

