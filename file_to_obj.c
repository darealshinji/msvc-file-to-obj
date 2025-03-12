/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2024-2025 Carsten Janssen
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

#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
# pragma comment(lib, "ws2_32")
# include <winsock2.h>   /* htonl */
# define strcasecmp  _stricmp
# define strncasecmp _strnicmp
#else
# include <arpa/inet.h>  /* htonl */
# include <endian.h>     /* htole32 */
#endif

#include "file_to_obj.h"

#if defined(_MSC_VER) && !defined(__clang__)
# define BSWAP32(x)  _byteswap_ulong(x)
#else
# define BSWAP32(x)  __builtin_bswap32(x)
#endif

#ifdef _MSC_VER
# if defined(_M_X64) || defined(_M_AMD64) || defined(_M_IX86)
#  define htole32(x)  (x)  /* x86 is always LE */
# else
#  define htole32(x)  BSWAP32(htonl(x))
# endif
#endif

#define SUFFIX_BE      "__size_BE__"
#define SUFFIX_LE      "__size_LE__"
#define SUFFIX_BE_LEN  (sizeof(SUFFIX_BE) - 1)
#define SUFFIX_LE_LEN  (sizeof(SUFFIX_LE) - 1)

/* write entire buffer to stream */
#define WRITE_BUF(BUF, STREAM) \
    write_data(&BUF, sizeof(BUF), STREAM)

#define SET_LE_UINT32(BUF,OFF,VAL) \
{ \
    const uint32_t tmp = htole32(VAL); \
    uint8_t *ptr = BUF; \
    memcpy(ptr + OFF, &tmp, sizeof(uint32_t)); \
}


static const uint32_t nullbytes = 0;


static FILE *open_file(const char *name, const char *mode)
{
    FILE *fp = fopen(name, mode);

    if (!fp) {
        perror("fopen()");
        fprintf(stderr, "(%s)\n", name);
        exit(1);
    }

    return fp;
}

static void write_data(const void *ptr, const size_t size, FILE *fp)
{
    if (fwrite(ptr, 1, size, fp) != size) {
        perror("fwrite()");
        exit(1);
    }
}

static char *symbolic_name(const char *name)
{
    char *p, *out;

    name = simple_basename(name);
    out = malloc(strlen(name) + 1);

    for (p = out; *name != 0; name++, p++) {
        if (isalnum(*name)) {
            *p = (char)tolower(*name);
        } else {
            *p = '_';
        }
    }

    *p = 0;

    printf("symbol: %s\n", out);

    return out;
}

static void write_header(FILE *fpOut, const uint8_t *machine, long hSizeOfRawData)
{
    uint8_t coff_header[20] = {
        0, 0,           /* Machine */
        0x03, 0,        /* NumberOfSections */
        0, 0, 0, 0,     /* TimeDateStamp */
        0, 0, 0, 0,     /* PointerToSymbolTable */
        0x03, 0, 0, 0,  /* NumberOfSymbols */
        0, 0,           /* SizeOfOptionalHeader */
        0x01, 0x02      /* Characteristics
                          (IMAGE_FILE_RELOCS_STRIPPED |
                           IMAGE_FILE_DEBUG_STRIPPED) */
    };

    const uint8_t off_PointerToSymbolTable = 8;

    uint8_t section_table[40] = {
        '.','r','d','a','t','a', 0, 0, /* Name (.rdata) */
        0, 0, 0, 0,       /* VirtualSize */
        0, 0, 0, 0,       /* VirtualAddress */
        0, 0, 0, 0,       /* SizeOfRawData */
        0, 0, 0, 0,       /* PointerToRawData */
        0, 0, 0, 0,       /* PointerToRelocations */
        0, 0, 0, 0,       /* PointerToLinenumbers */
        0, 0,             /* NumberOfRelocations */
        0, 0,             /* NumberOfLinenumbers */
        0x40, 0, 0, 0x40  /* Characteristics
                            (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_READ) */
    };

    const uint8_t off_SizeOfRawData = 16;
    const uint8_t off_PointerToRawData = 20;
    uint32_t hPointerToSymbolTable, hHdrSize;

    /* COFF header */
    memcpy(&coff_header, machine, 2);
    hHdrSize = sizeof(coff_header) + 3*sizeof(section_table);
    hPointerToSymbolTable = hHdrSize + hSizeOfRawData + 2*sizeof(uint32_t);
    SET_LE_UINT32(coff_header, off_PointerToSymbolTable, hPointerToSymbolTable);
    WRITE_BUF(coff_header, fpOut);

    /* section table */
    SET_LE_UINT32(section_table, off_SizeOfRawData, hSizeOfRawData);
    SET_LE_UINT32(section_table, off_PointerToRawData, hHdrSize);
    WRITE_BUF(section_table, fpOut); /* symbol #1 (data) */

    SET_LE_UINT32(section_table, off_SizeOfRawData, sizeof(uint32_t));
    SET_LE_UINT32(section_table, off_PointerToRawData, hHdrSize + hSizeOfRawData);
    WRITE_BUF(section_table, fpOut); /* symbol #2 (BE size) */

    SET_LE_UINT32(section_table, off_PointerToRawData,
        hHdrSize + hSizeOfRawData + sizeof(uint32_t));
    WRITE_BUF(section_table, fpOut); /* symbol #3 (LE size) */
}

static void save_data_to_coff(FILE *fpIn, FILE *fpOut, uint32_t raw_data_size)
{
    uint8_t buffer[1024];
    size_t nread;
    uint32_t uSizeOfRawData;

    memset(buffer, 0, sizeof(buffer));

    /* copy file data */
    while ((nread = fread(&buffer, 1, sizeof(buffer), fpIn)) != 0) {
        write_data(&buffer, nread, fpOut);
    }

    /* terminating NUL bytes */
    WRITE_BUF(nullbytes, fpOut);

    /* data size (Big Endian) */
    uSizeOfRawData = htonl(raw_data_size);
    WRITE_BUF(uSizeOfRawData, fpOut);

    /* data size (Little Endian) */
    uSizeOfRawData = htole32(raw_data_size);
    WRITE_BUF(uSizeOfRawData, fpOut);
}

static void save_symtab_to_coff(FILE *fpOut, const char *symbol, uint8_t mangle)
{
    uint8_t symbol_table[18] = {
        0, 0, 0, 0,     /* Name (assume it's longer than 8 bytes) */
        0x04, 0, 0, 0,  /* Offset into string table */
        0, 0, 0, 0,     /* Value */
        0x01, 0,        /* SectionNumber */
        0, 0,           /* Type */
        0x02,           /* StorageClass (external) */
        0               /* NumberOfAuxSymbols */
    };

    const uint8_t off_SectionNumber = 12;
    uint32_t symbol_len, leSizeOfStringTable, hSizeOfStringTable;

    symbol_len = (uint32_t)strlen(symbol);
    hSizeOfStringTable = sizeof(uint32_t); /* string table size */

    /* symbol table #1 (data) */
    WRITE_BUF(symbol_table, fpOut);
    hSizeOfStringTable += mangle + symbol_len + 1;

    /* symbol table #2 (size BE) */
    SET_LE_UINT32(symbol_table, 4, hSizeOfStringTable);
    symbol_table[off_SectionNumber] = 0x02;
    WRITE_BUF(symbol_table, fpOut);
    hSizeOfStringTable += mangle + symbol_len + SUFFIX_BE_LEN + 1;

    /* symbol table #3 (size LE) */
    SET_LE_UINT32(symbol_table, 4, hSizeOfStringTable);
    symbol_table[off_SectionNumber] = 0x03;
    WRITE_BUF(symbol_table, fpOut);
    hSizeOfStringTable += mangle + symbol_len + SUFFIX_LE_LEN + 1;

    /* string table size */
    leSizeOfStringTable = htole32(hSizeOfStringTable);
    WRITE_BUF(leSizeOfStringTable, fpOut);

    /* _<symbol> + NUL */
    if (mangle) { fputc('_', fpOut); }
    write_data(symbol, symbol_len + 1, fpOut);

    /* _<symbol>_size_BE + NUL */
    if (mangle) { fputc('_', fpOut); }
    write_data(symbol, symbol_len, fpOut);
    write_data(SUFFIX_BE, SUFFIX_BE_LEN + 1, fpOut);

    /* _<symbol>_size_LE + NUL */
    if (mangle) { fputc('_', fpOut); }
    write_data(symbol, symbol_len, fpOut);
    write_data(SUFFIX_LE, SUFFIX_LE_LEN + 1, fpOut);
}

/**
 * https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 *
 * Write data to COFF object file:
 *
 * COFF header (20 bytes)
 * section table (40 bytes)
 *
 * file data size (4 bytes Big Endian)
 * file data (n bytes)
 * 4 NUL bytes to terminate text data
 *
 * symbol table (18 bytes)
 *
 * string table size (4 bytes)
 * string table (list of null-terminated strings)
 */
void save_to_coff(const char *infile, const char *outfile, const uint8_t *machine, const char *symbol)
{
    FILE *fpIn, *fpOut;
    char *outTmp = NULL;
    char *symTmp = NULL;
    long raw_data_size, hSizeOfRawData;
    uint8_t mangle = 0;

    /* open files */
    fpIn = open_file(infile, "rb");

    if (outfile) {
        fpOut = open_file(outfile, "wb");
    } else {
        outTmp = malloc(strlen(infile) + 5);
        strcpy(outTmp, infile);
        strcat(outTmp, ".obj");
        fpOut = open_file(outTmp, "wb");
        free(outTmp);
    }

    /* get file size */
    fseek(fpIn, 0, SEEK_END);

    if ((raw_data_size = ftell(fpIn)) == -1) {
        perror("ftell()");
        exit(1);
    }

    rewind(fpIn);

    hSizeOfRawData = raw_data_size + sizeof(nullbytes);

    write_header(fpOut, machine, hSizeOfRawData);
    save_data_to_coff(fpIn, fpOut, raw_data_size);

    /* on x86 symbols are prefixed with underscores
     * (IMAGE_FILE_MACHINE_I386 = 0x014C) */
    if (machine[0] == 0x4C && machine[1] == 0x01) {
        mangle = 1;
    }

    if (symbol) {
        save_symtab_to_coff(fpOut, symbol, mangle);
    } else {
        symTmp = symbolic_name(infile);
        save_symtab_to_coff(fpOut, symTmp, mangle);
        free(symTmp);
    }

    fclose(fpOut);
    fclose(fpIn);
}

