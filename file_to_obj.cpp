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
#endif
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <filesystem>
#include <iterator>  /* std::data */
#include <string>
#include <vector>
#include "file.hpp"
#include "incbin_msvc.h"
#include "utils.hpp"


#define XSTRINGIFY(x)  #x
#define STRINGIFY(x)   XSTRINGIFY(x)

namespace fs = std::filesystem;



static FILE *open_file(const char *name, const char *mode)
{
    FILE *fp = fopen(name, mode);

    if (!fp) {
        std::string msg = "fopen(): ";
        msg += strerror(errno);
        msg += " [";
        msg += name;
        msg += ']';

        throw msg;
    }

    return fp;
}


static void write_data(const void *ptr, const size_t size, FILE *fp, const char *output)
{
    if (fwrite(ptr, 1, size, fp) != size) {
        std::string msg = "fwrite(): ";
        msg += strerror(errno);
        msg += " [";
        msg += output;
        msg += ']';

        throw msg;
    }
}


static std::string symbolic_name(const char *name)
{
    auto path = make_fs_path(name);
    std::string str = path.filename().string();

    for (size_t i=0; i < str.size(); i++) {
        if (!isalnum(str[i])) {
            str[i] = '_';
        }
    }

    return str;
}


static uint32_t section_headers(std::vector<const char *> files, std::vector<IMAGE_SECTION_HEADER> &sections)
{
    IMAGE_SECTION_HEADER sec;

    memset(&sec, 0, sizeof(sec));
    memcpy(sec.Name, ".rdata", 6);
    sec.Characteristics = htole(IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);

    /* data offset starts after section headers */
    uint32_t PointerToRawData = static_cast<uint32_t>(sizeof(IMAGE_FILE_HEADER) +
            (sizeof(IMAGE_SECTION_HEADER) * files.size() * 3));

    for (auto &file : files) {
        uint32_t sizes[3] = {
            static_cast<uint32_t>(fs::file_size(file)) + 4, /* data + NUL bytes */
            4, /* BE size */
            4  /* LE size */
        };

        for (int i = 0; i < 3; i++) {
            sec.SizeOfRawData     = htole(sizes[i]);
            sec.PointerToRawData  = htole(PointerToRawData);
            PointerToRawData     += sizes[i];
            sections.push_back(sec);
        }
    }

    return PointerToRawData;
}


static void save_file_data(std::vector<const char *> &files, FILE *fpOut, const char *output)
{
    uint8_t buffer[1024];
    size_t nread;

    memset(buffer, 0, sizeof(buffer));

    for (auto &file : files) {
        uint32_t raw_data_size = 0;
        FILE *fpIn = open_file(file, "rb");

        /* copy file data */
        while ((nread = fread(&buffer, 1, sizeof(buffer), fpIn)) != 0) {
            write_data(&buffer, nread, fpOut, output);
            raw_data_size += static_cast<uint32_t>(nread);
        }

        /* terminating NUL bytes */
        memset(buffer, 0, 4);
        write_data(&buffer, 4, fpOut, output);

        /* data size (Big Endian) */
        uint32_t val = htobe(raw_data_size);
        write_data(&val, sizeof(val), fpOut, output);

        /* data size (Little Endian) */
        val = htole(raw_data_size);
        write_data(&val, sizeof(val), fpOut, output);

        fclose(fpIn);
    }
}


static std::string symbol_table(std::vector<SYMBOL_TABLE_ENTRY> &symtab,
                                std::vector<const char *> &files,
                                uint16_t machine)
{
    SYMBOL_TABLE_ENTRY sym;
    uint16_t SectionNumber = 1;
    std::string strtab;

    const char *suffix[3] = {
        "",
        STRINGIFY(INCBIN_SUFFIX_BIG),
        STRINGIFY(INCBIN_SUFFIX_LITTLE)
    };

    memset(&sym, 0, sizeof(sym));
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;

    for (auto &file : files) {
        std::string name = symbolic_name(file);

        /* https://learn.microsoft.com/en-us/cpp/build/reference/decorated-names?view=msvc-170#FormatC
        * the decoration is done on any 32 bit target but practically this is only relevant for i386 */
        if (machine == IMAGE_FILE_MACHINE_I386 || isdigit(name.front())) {
            name.insert(0, 1, '_');
        }

        /* data + size BE + size LE */
        for (int i = 0; i < 3; i++, SectionNumber++) {
            sym.u.Offset.Offset = htole(static_cast<uint32_t>(strtab.size() + 4)); /* + 4 bytes size entry */
            sym.SectionNumber = htole(SectionNumber);
            strtab += name + suffix[i];
            strtab += '\0';
            symtab.push_back(sym);
        }
    }

    return strtab;
}


/**
 * https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 *
 * Write data to COFF object file:
 *
 * COFF file header
 * section table (section headers)
 *
 * file data
 * 4 NUL bytes to terminate text data
 * file data size (4 bytes Big Endian)
 * file data size (4 bytes Little Endian)
 * [...]
 *
 * symbol table
 *
 * string table (4 bytes strtab size + list of null-terminated strings)
 */
void save_to_coff(std::vector<const char *> &files, const char *output, uint16_t machine)
{
    IMAGE_FILE_HEADER fhdr;
    std::vector<IMAGE_SECTION_HEADER> sec_hdrs;
    std::vector<SYMBOL_TABLE_ENTRY> symtab;

    /* section headers */
    uint32_t PointerToSymbolTable = section_headers(files, sec_hdrs);

    /* file header */
    memset(&fhdr, 0, sizeof(fhdr));
    fhdr.Machine              = htole(machine);
    fhdr.NumberOfSections     = htole(static_cast<uint16_t>(sec_hdrs.size()));
    fhdr.PointerToSymbolTable = htole(PointerToSymbolTable);
    fhdr.NumberOfSymbols      = fhdr.NumberOfSections;
    fhdr.Characteristics      = htole<uint16_t>(IMAGE_FILE_RELOCS_STRIPPED | IMAGE_FILE_DEBUG_STRIPPED);

    /* symbol table + string table */
    auto strtab = symbol_table(symtab, files, machine);

    /* write file header */
    FILE *fpOut = open_file(output, "wb");
    write_data(&fhdr, sizeof(fhdr), fpOut, output);

    /* write section headers */
    for (auto &e : sec_hdrs) {
        write_data(&e, sizeof(e), fpOut, output);
    }

    /* save file data and file sizes */
    save_file_data(files, fpOut, output);

    /* write symbol table entries */
    for (auto &e : symtab) {
        write_data(&e, sizeof(e), fpOut, output);
    }

    /* save string table */
    uint32_t val = htole(static_cast<uint32_t>(strtab.size() + 4));
    write_data(&val, sizeof(val), fpOut, output);
    write_data(std::data(strtab), strtab.size(), fpOut, output);

    fclose(fpOut);
}

