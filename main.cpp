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
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include "file.hpp"
#include "utils.hpp"



static uint16_t read_hex(const char *text)
{
    unsigned long val;

    if (strlen(text) > 6) {
        fprintf(stderr, "value longer than 2 bytes: %s\n", text);
        exit(1);
    }

    errno = 0;
    val = strtoul(text, NULL, 16);

    if (errno != 0) {
        perror("strtoul");
        exit(1);
    }

    return (uint16_t)val;
}

static void print_help(const char *exe)
{
    auto path = make_fs_path(exe);
    exe = path.stem().string().c_str();

    std::cout << "usage: " << exe << " [--machine=TARGET] OUTPUT FILE1 [FILE2 [..]]\n"
        << "       " << exe << " --help\n"
           "\n"
           "TARGET values: X64 X86 ARM64 NONE\n"
           "or 2 byte Image File Machine Constant, i.e. 0x8664"
        << std::endl;
}

static void try_help(const char *msg1, const char *msg2, const char *exe)
{
    auto path = make_fs_path(exe);
    exe = path.stem().string().c_str();

    if (msg1) {
        std::cerr << "Error: " << msg1 << msg2 << std::endl;
    }

    std::cerr << "Try `" << exe << " --help' for more information." << std::endl;
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
    char *p;
    uint16_t machine = IMAGE_FILE_MACHINE_DEFAULT;
    int argind = 0;

    if (argc < 2) {
        try_help("No arguments.", "", argv[0]);
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
        if (!strbeg(argv[i], "--")) {
            if (!output) {
                output = argv[i];
                continue;
            } else {
                argind = i;
                break;
            }
        }

        p = argv[i] + 2;

        if (strcasecmp(p, "machine") == 0 || strcasecmp(p, "machine=") == 0) {
            fprintf(stderr, "missing argument: %s\n", argv[i]);
            try_help(NULL, "", argv[0]);
            return 1;
        }
        else if (strbeg(p, "machine=")) {
            p += sizeof("machine=") - 1;

            if (strbeg(p, "0x")) {
                machine = read_hex(p);
            } else if (strcasecmp(p, "X64") == 0) {
                machine = IMAGE_FILE_MACHINE_AMD64;
            } else if (strcasecmp(p, "X86") == 0) {
                machine = IMAGE_FILE_MACHINE_I386;
            } else if (strcasecmp(p, "ARM64") == 0) {
                machine = IMAGE_FILE_MACHINE_ARM64;
            } else if (strcasecmp(p, "NONE") == 0) {
                machine = IMAGE_FILE_MACHINE_UNKNOWN;
            } else {
                try_help("unknown argument for --machine: ", p, argv[0]);
                return 1;
            }
            continue;
        }

        try_help("unknown option: ", argv[i], argv[0]);
        return 1;
    }

    if (!output) {
        try_help("missing output file", "", argv[0]);
        return 1;
    } else if (argind == 0) {
        try_help("missing input file(s)", "", argv[0]);
        return 1;
    }

    for (int i = argind; i < argc; i++) {
        files.push_back(argv[i]);
    }

    save_to_coff(files, output, machine);

    return 0;
}

