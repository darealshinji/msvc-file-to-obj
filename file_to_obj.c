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

#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
# pragma comment(lib, "ws2_32") /* htonl() */
#endif

#include "incbin_msvc.h"  /* common macros */

/* hton*() */
#ifdef _WIN32
# include <winsock2.h>
#else
# include <arpa/inet.h>
#endif

/* htole*() */
#ifdef _WIN32
# ifdef INCBIN_LITTLE_ENDIAN
#  define htole16(x)  (x)
#  define htole32(x)  (x)
# elif defined(_MSC_VER)
#  define htole16(x)  _byteswap_ushort(htons(x))
#  define htole32(x)  _byteswap_ulong(htonl(x))
# else
#  define htole16(x)  __builtin_bswap16(htons(x))
#  define htole32(x)  __builtin_bswap32(htonl(x))
# endif
#else
# include <endian.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "simple_basename.h"

#define XSTRINGIFY(x)  #x
#define STRINGIFY(x)   XSTRINGIFY(x)

#define SUFFIX_BE      STRINGIFY(INCBIN_SUFFIX_BIG)
#define SUFFIX_LE      STRINGIFY(INCBIN_SUFFIX_LITTLE)
#define SUFFIX_BE_LEN  (sizeof(SUFFIX_BE) - 1)
#define SUFFIX_LE_LEN  (sizeof(SUFFIX_LE) - 1)


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
    const char *pname;
    char *pout, *out;

    pname = simple_basename(name);
    out = malloc(strlen(pname) + 1);

    for (pout = out; *pname != 0; pname++, pout++) {
        *pout = isalnum(*pname) ? *pname : '_';
    }

    *pout = 0;

    printf("symbol: %s\n", out);

    return out;
}

static void write_headers(FILE *fpOut, uint16_t machine, long raw_data_size)
{
    HEADER_DATA hdr;

    memset(&hdr, 0, sizeof(hdr));

    /* section headers */

    /* symbol #1 (data) */
    memcpy(hdr.Sections[0].Name, ".rdata", 6);
    hdr.Sections[0].SizeOfRawData    = htole32(raw_data_size + sizeof(uint32_t)); /* + NUL bytes */
    hdr.Sections[0].PointerToRawData = htole32(sizeof(HEADER_DATA));
    hdr.Sections[0].Characteristics  = htole32(IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);

    /* symbol #2 (BE size) */
    memcpy(hdr.Sections[1].Name, ".rdata", 6);
    hdr.Sections[1].SizeOfRawData    = htole32(sizeof(uint32_t));
    hdr.Sections[1].PointerToRawData = htole32(hdr.Sections[0].PointerToRawData + hdr.Sections[0].SizeOfRawData);
    hdr.Sections[1].Characteristics  = htole32(IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);

    /* symbol #3 (LE size) */
    memcpy(hdr.Sections[2].Name, ".rdata", 6);
    hdr.Sections[2].SizeOfRawData    = htole32(sizeof(uint32_t));
    hdr.Sections[2].PointerToRawData = htole32(hdr.Sections[1].PointerToRawData + hdr.Sections[1].SizeOfRawData);
    hdr.Sections[2].Characteristics  = htole32(IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);

    /* file header */
    hdr.FileHeader.Machine              = htole16(machine);
    hdr.FileHeader.NumberOfSections     = htole32(3);
    hdr.FileHeader.PointerToSymbolTable = htole32(hdr.Sections[2].PointerToRawData + hdr.Sections[2].SizeOfRawData);
    hdr.FileHeader.NumberOfSymbols      = htole32(3);
    hdr.FileHeader.Characteristics      = htole16(IMAGE_FILE_RELOCS_STRIPPED | IMAGE_FILE_DEBUG_STRIPPED);

    write_data(&hdr, sizeof(hdr), fpOut);
}

static void save_data_to_coff(FILE *fpIn, FILE *fpOut, uint32_t raw_data_size)
{
    uint8_t buffer[1024];
    size_t nread;
    uint32_t l;

    memset(buffer, 0, sizeof(buffer));

    /* copy file data */
    while ((nread = fread(&buffer, 1, sizeof(buffer), fpIn)) != 0) {
        write_data(&buffer, nread, fpOut);
    }

    /* terminating NUL bytes */
    l = 0;
    write_data(&l, sizeof(l), fpOut);

    /* data size (Big Endian) */
    l = htonl(raw_data_size);
    write_data(&l, sizeof(l), fpOut);

    /* data size (Little Endian) */
    l = htole32(raw_data_size);
    write_data(&l, sizeof(l), fpOut);
}

static void save_symbols_to_coff(FILE *fpOut, const char *filename, uint16_t machine)
{
    SYMBOL_TABLE_ENTRY sym[3];
    char *symbol;
    const char *pfx = "";
    uint32_t symlen, strtab_size;

    symbol = symbolic_name(filename);
    symlen = (uint32_t)strlen(symbol) + 1; /* + NUL byte */

    /* https://learn.microsoft.com/en-us/cpp/build/reference/decorated-names?view=msvc-170#FormatC
     * the decoration is done on any 32 bit target but practically this is only relevant for i386 */
    if (machine == IMAGE_FILE_MACHINE_I386 || isdigit(*symbol)) {
        pfx = "_";
        symlen++;
    }

    memset(&sym, 0, sizeof(sym));

    /* data */
    sym[0].StorageClass    = IMAGE_SYM_CLASS_EXTERNAL;
    sym[0].u.Offset.Offset = htole32(sizeof(strtab_size));
    sym[0].SectionNumber   = htole16(1);

    /* size BE */
    sym[1].StorageClass    = IMAGE_SYM_CLASS_EXTERNAL;
    sym[1].u.Offset.Offset = htole32(sym[0].u.Offset.Offset + symlen);
    sym[1].SectionNumber   = htole16(2);

    /* size LE */
    sym[2].StorageClass    = IMAGE_SYM_CLASS_EXTERNAL;
    sym[2].u.Offset.Offset = htole32(sym[1].u.Offset.Offset + symlen + SUFFIX_BE_LEN);
    sym[2].SectionNumber   = htole16(3);

    /* save symbol table entries */
    for (int i=0; i < 3; i++) {
        write_data(&sym[i], sizeof(sym[i]) - sizeof(sym[i].Unused), fpOut);
    }

    /* string table size entry */
    strtab_size = htole32(sym[2].u.Offset.Offset + symlen + SUFFIX_LE_LEN);
    write_data(&strtab_size, sizeof(strtab_size), fpOut);

    /* write NUL termintated symbol name list */
    fprintf(fpOut, "%s%s%c",              pfx, symbol, 0);
    fprintf(fpOut, "%s%s" SUFFIX_BE "%c", pfx, symbol, 0);
    fprintf(fpOut, "%s%s" SUFFIX_LE "%c", pfx, symbol, 0);

    free(symbol);
}

/**
 * https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 *
 * Write data to COFF object file:
 *
 * COFF header (20 bytes)
 * section table (40 bytes)
 *
 * file data (n bytes)
 * 4 NUL bytes to terminate text data
 * file data size (4 bytes Big Endian)
 * file data size (4 bytes Little Endian)
 *
 * symbol table (18 bytes)
 *
 * string table size (4 bytes)
 * string table (list of null-terminated strings)
 */
void save_to_coff(const char *infile, uint16_t machine)
{
    FILE *fpIn, *fpOut;
    char *out = NULL;
    long raw_data_size;

    /* open input file */
    fpIn = open_file(infile, "rb");

    /* open output file */
    out = malloc(strlen(infile) + 5);
    strcpy(out, infile);
    strcat(out, ".obj");
    fpOut = open_file(out, "wb");
    free(out);

    /* get file size */
    if (fseek(fpIn, 0, SEEK_END) == -1) {
        perror("fseek()");
        exit(1);
    }

    if ((raw_data_size = ftell(fpIn)) == -1) {
        perror("ftell()");
        exit(1);
    }

    rewind(fpIn);

    /* save data */
    write_headers(fpOut, machine, raw_data_size);
    save_data_to_coff(fpIn, fpOut, raw_data_size);
    save_symbols_to_coff(fpOut, infile, machine);

    fclose(fpOut);
    fclose(fpIn);
}

