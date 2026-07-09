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

#ifdef _WIN32
# include <windows.h>
#else
# include <strings.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "file.hpp"
#include "utils.hpp"


static std::string shortname(const char *file)
{
    auto path = make_fs_path(file);

#ifdef _WIN32
    return path.stem().string();
#else
    return path.filename().string();
#endif
}


static bool read_hex(const char *str, uint16_t &val)
{
    char *endptr = NULL;
    unsigned long l = strtoul(str, &endptr, 16);

    if (*endptr != '\0' || l > 0xffff) {
        return false;
    }

    val = static_cast<uint16_t>(l);

    return true;
}


static void print_help(const char *file)
{
    auto stem = shortname(file);

    std::cout <<
           "usage: " << stem << " [-mTARGET] OUTPUT FILE1 [FILE2 [..]]\n"
           "       " << stem << " --help\n"
           "\n"
           "TARGET values: X64 X86 ARM64 NONE\n"
           "or 2 byte Image File Machine Constant, i.e. 0x8664"
        << std::endl;
}


static void try_help(const char *msg1, const char *msg2, const char *file)
{
    if (msg1 && *msg1) {
        if (!msg2) {
            msg2 = "";
        }

        std::cerr << "Error: " << msg1 << msg2 << std::endl;
    }

    std::cerr << "Try `" << shortname(file) << " --help' for more information." << std::endl;
}


static void try_help(const char *msg, const char *file)
{
    try_help(msg, "", file);
}


int main(int argc, char **argv)
{
#if defined(_WIN32) && defined(UTF8_EVERYWHERE)
    /* Set the process code page to UTF-8 with the activeCodePage property in
     * the appxmanifest during linking and set the console output codepage to
     * UTF-8 here. SetConsoleCP() is not required and it would keep the new
     * codepage after the program closes. */
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::vector<const char *> files;
    const char *output = NULL;
    uint16_t machine = IMAGE_FILE_MACHINE_DEFAULT;

    if (argc < 2) {
        try_help("no arguments", argv[0]);
        return 1;
    }

    /* parse arguments */

    for (int i = 1; i < argc; i++) {
        if (strcasecmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
    }

    for (int i = 1; i < argc; i++) {
        if (strcasebeg(argv[i], "-m")) {
            char *p = argv[i] + 2;

            if (!*p) {
                fprintf(stderr, "missing argument for `-m': %s\n", argv[i]);
                try_help(NULL, argv[0]);
                return 1;
            }

            if (strcasebeg(p, "0x")) {
                if (!read_hex(p, machine)) {
                    try_help("invalid argument for `-m': ", p, argv[0]);
                    return 1;
                }
            } else if (strcasecmp(p, "X64") == 0) {
                machine = IMAGE_FILE_MACHINE_AMD64;
            } else if (strcasecmp(p, "X86") == 0) {
                machine = IMAGE_FILE_MACHINE_I386;
            } else if (strcasecmp(p, "ARM64") == 0) {
                machine = IMAGE_FILE_MACHINE_ARM64;
            } else if (strcasecmp(p, "NONE") == 0) {
                machine = IMAGE_FILE_MACHINE_UNKNOWN;
            } else {
                try_help("unknown argument for `-m': ", p, argv[0]);
                return 1;
            }
        } else if (!output) {
            output = argv[i];
        } else {
            files.push_back(argv[i]);
        }
    }

    /* check input/output */
    if (!output) {
        try_help("missing output file", argv[0]);
        return 1;
    } else if (files.empty()) {
        try_help("missing input file(s)", argv[0]);
        return 1;
    }

    /* set output file extension */
    auto opath = make_fs_path(output);
    opath.replace_extension(".obj");

    /* try/catch block */
    try {
        save_to_coff(files, opath, machine);
    }
    catch (const std::string &msg) {
        std::cerr << "error: " << msg << std::endl;
        return 1;
    }
    catch (const fs::filesystem_error &ex) {
        std::cerr << "error: " << ex.what() << std::endl;
        return 1;
    }
    catch (const std::ofstream::iostate &) {
        std::cerr << "error: failed to write data to output file " << opath << std::endl;
        return 1;
    }

    std::cout << "saved data to output file " << opath << std::endl;

    return 0;
}

