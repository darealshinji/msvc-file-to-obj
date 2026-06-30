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

#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "file_to_obj.h"
#include "incbin_msvc.h"
#include "file.h"

#define XSTRINGIFY(x)  #x
#define STRINGIFY(x)   XSTRINGIFY(x)

#define SUFFIX_BE      STRINGIFY(INCBIN_SUFFIX_BIG)
#define SUFFIX_LE      STRINGIFY(INCBIN_SUFFIX_LITTLE)
#define SUFFIX_BE_LEN  (sizeof(SUFFIX_BE) - 1)
#define SUFFIX_LE_LEN  (sizeof(SUFFIX_LE) - 1)

#define NUM_NULLBYTES  4


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

static void write_headers(FILE *fpOut, uint16_t machine, long raw_data_size)
{
    HEADER_DATA hdr;

    memset(&hdr, 0, sizeof(hdr));

    /* section headers */

    /* symbol #1 (data) */
    strncpy((char *)hdr.Sections[0].Name, ".rdata", IMAGE_SIZEOF_SHORT_NAME);
    hdr.Sections[0].SizeOfRawData    = htole32(raw_data_size + NUM_NULLBYTES);
    hdr.Sections[0].PointerToRawData = htole32(sizeof(HEADER_DATA));
    hdr.Sections[0].Characteristics  = htole32(IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);

    /* symbol #2 (BE size) */
    strncpy((char *)hdr.Sections[1].Name, ".rdata", IMAGE_SIZEOF_SHORT_NAME);
    hdr.Sections[1].SizeOfRawData    = htole32(sizeof(uint32_t));
    hdr.Sections[1].PointerToRawData = htole32(hdr.Sections[0].PointerToRawData + hdr.Sections[0].SizeOfRawData);
    hdr.Sections[1].Characteristics  = htole32(IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);

    /* symbol #3 (LE size) */
    strncpy((char *)hdr.Sections[2].Name, ".rdata", IMAGE_SIZEOF_SHORT_NAME);
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
    for (int i = 0; i < NUM_NULLBYTES; i++) {
        putc(0, fpOut);
    }

    /* data size (Big Endian) */
    l = htonl(raw_data_size);
    write_data(&l, sizeof(l), fpOut);

    /* data size (Little Endian) */
    l = htole32(raw_data_size);
    write_data(&l, sizeof(l), fpOut);
}

static void save_symbols_to_coff(FILE *fpOut, const char *symbol, uint16_t machine)
{
    SYMBOL_TABLE_ENTRY sym[3];
    const uint32_t symlen = (uint32_t)strlen(symbol);
    uint32_t strtab_size;

    /* on x86 symbols are prefixed with underscores */
    const uint8_t mangle = (machine == IMAGE_FILE_MACHINE_I386) ? 1 : 0;
    const char *pfx = mangle ? "_" : "";

    memset(&sym, 0, sizeof(sym));

    /* data */
    sym[0].StorageClass    = IMAGE_SYM_CLASS_EXTERNAL;
    sym[0].u.Offset.Offset = htole32(sizeof(strtab_size));
    sym[0].SectionNumber   = htole16(1);

    /* size BE */
    sym[1].StorageClass    = IMAGE_SYM_CLASS_EXTERNAL;
    sym[1].u.Offset.Offset = htole32(sym[0].u.Offset.Offset + mangle + symlen + 1);
    sym[1].SectionNumber   = htole16(2);

    /* size LE */
    sym[2].StorageClass    = IMAGE_SYM_CLASS_EXTERNAL;
    sym[2].u.Offset.Offset = htole32(sym[1].u.Offset.Offset + mangle + symlen + SUFFIX_BE_LEN + 1);
    sym[2].SectionNumber   = htole16(3);

    for (int i=0; i < 3; i++) {
        write_data(&sym[i], SYMBOL_TABLE_ENTRY_SIZE_UNALIGNED, fpOut);
    }

    /* string table size entry */
    strtab_size = htole32(sym[2].u.Offset.Offset + mangle + symlen + SUFFIX_LE_LEN + 1);
    write_data(&strtab_size, sizeof(strtab_size), fpOut);

    /* write NUL termintated symbol list */
    fprintf(fpOut, "%s%s", pfx, symbol);
    putc(0, fpOut);
    fprintf(fpOut, "%s%s" SUFFIX_BE, pfx, symbol);
    putc(0, fpOut);
    fprintf(fpOut, "%s%s" SUFFIX_LE, pfx, symbol);
    putc(0, fpOut);
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
void save_to_coff(const char *infile, const char *outfile, uint16_t machine, const char *symbol)
{
    FILE *fpIn, *fpOut;
    char *outTmp = NULL;
    char *symTmp = NULL;
    long raw_data_size;

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

    write_headers(fpOut, machine, raw_data_size);
    save_data_to_coff(fpIn, fpOut, raw_data_size);

    if (symbol) {
        save_symbols_to_coff(fpOut, symbol, machine);
    } else {
        symTmp = symbolic_name(infile);
        save_symbols_to_coff(fpOut, symTmp, machine);
        free(symTmp);
    }

    fclose(fpOut);
    fclose(fpIn);
}

