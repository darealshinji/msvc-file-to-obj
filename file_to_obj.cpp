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
#include <fstream>
#include <iterator>  /* std::data */
#include <string>
#include <vector>
#include "file.hpp"
#include "incbin_msvc.h"
#include "utils.hpp"

#define XSTRINGIFY(x)  #x
#define STRINGIFY(x)   XSTRINGIFY(x)



template<typename T>
void write_data(const T *ptr, size_t size, std::ofstream &ofs)
{
    ofs.write(reinterpret_cast<const char *>(ptr), size);

    if (ofs.rdstate() == std::ofstream::badbit ||
        ofs.rdstate() == std::ofstream::failbit)
    {
        throw ofs.rdstate();
    }
}

template<typename T>
void write_data(const T &val, std::ofstream &ofs)
{
    write_data(reinterpret_cast<const char *>(&val), sizeof(T), ofs);
}


static bool read_data(char &ch, std::ifstream &ifs, const char *file)
{
    if (ifs.get(ch)) {
        return true;
    }

    if (ifs.rdstate() == std::ifstream::badbit ||
        ifs.rdstate() == std::ifstream::failbit)
    {
        throw std::string("failed to read data from file: ") + file;
    }

    /* EOF */
    return false;
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
    uint32_t rawdata_pointer = static_cast<uint32_t>(sizeof(IMAGE_FILE_HEADER) +
            (sizeof(IMAGE_SECTION_HEADER) * files.size() * 3));

    for (auto &file : files) {
        auto fsize = static_cast<uint32_t>(fs::file_size(file));

        for (auto i = 0; i < 3; i++) {
            /* data + 4 NUL bytes, BE size and LE size (uint32_t) */
            sec.SizeOfRawData     = htole(fsize + 4);
            sec.PointerToRawData  = htole(rawdata_pointer);
            rawdata_pointer      += fsize + 4;
            sections.push_back(sec);
            fsize = 0; /* filesize only in first entry */
        }
    }

    return rawdata_pointer;
}


static void save_file_data(std::vector<const char *> &files, std::ofstream &ofs)
{
    for (auto &file : files) {
        char ch;
        uint32_t raw_data_size = 0;

        std::ifstream ifs(file, std::ifstream::binary | std::ifstream::in);

        if (!ifs.is_open()) {
            throw std::string("failed to open file for reading: ") + file;
        }

        /* copy file data */
        while (read_data(ch, ifs, file)) {
            write_data(ch, ofs);
            raw_data_size++;
        }

        /* terminating NUL bytes */
        uint32_t val = 0;
        write_data(val, ofs);

        /* data size (Big Endian) */
        val = htobe(raw_data_size);
        write_data(val, ofs);

        /* data size (Little Endian) */
        val = htole(raw_data_size);
        write_data(val, ofs);
    }
}


static std::string symbol_table(std::vector<SYMBOL_TABLE_ENTRY> &symtab,
                                std::vector<const char *> &files,
                                uint16_t machine)
{
    SYMBOL_TABLE_ENTRY sym;
    uint16_t SectionNumber = 1;
    uint32_t size;
    std::string strtab;

    memset(&sym, 0, sizeof(sym));
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;

    strtab.insert(0, sizeof(size), 0); /* placeholder */

    for (auto &file : files) {
        std::string name = symbolic_name(file);

        /* https://learn.microsoft.com/en-us/cpp/build/reference/decorated-names?view=msvc-170#FormatC
        * the decoration is done on any 32 bit target but practically this is only relevant for i386 */
        if (machine == IMAGE_FILE_MACHINE_I386 || isdigit(name.front())) {
            name.insert(0, 1, '_');
        }

        const char *suffix[3] = {
            "",
            STRINGIFY(INCBIN_SUFFIX_BIG),
            STRINGIFY(INCBIN_SUFFIX_LITTLE)
        };

        /* data + size BE + size LE */
        for (int i = 0; i < 3; i++, SectionNumber++) {
            sym.u.Offset.Offset = htole(static_cast<uint32_t>(strtab.size()));
            sym.SectionNumber = htole(SectionNumber);
            strtab += name + suffix[i];
            strtab += '\0';
            symtab.push_back(sym);
        }
    }

    size = htole(static_cast<uint32_t>(strtab.size()));
    memcpy(std::data(strtab), &size, sizeof(size));

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
void save_to_coff(std::vector<const char *> &files, fs::path &output, uint16_t machine)
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
    std::ofstream ofs(output, std::ofstream::binary | std::ofstream::out);

    if (!ofs.is_open()) {
        throw "failed to open file for writing: " + output.string();
    }

    write_data(fhdr, ofs);

    /* write section headers */
    for (auto &e : sec_hdrs) {
        write_data(e, ofs);
    }

    /* save file data and file sizes */
    save_file_data(files, ofs);

    /* write symbol table entries */
    for (auto &e : symtab) {
        write_data(e, ofs);
    }

    /* save string table */
    write_data(std::data(strtab), strtab.size(), ofs);
}

