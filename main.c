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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "simple_basename.h"

/* case-insensitive string checks */
#ifdef _MSC_VER
# define STREQ(a,b)       (_stricmp(a,b) == 0)
# define STRBEG(STR,PFX)  (_strnicmp(STR, PFX, sizeof(PFX)-1) == 0)
#else
# include <strings.h>
# define STREQ(a,b)       (strcasecmp(a,b) == 0)
# define STRBEG(STR,PFX)  (strncasecmp(STR, PFX, sizeof(PFX)-1) == 0)
#endif


/* file_to_obj.c */
extern void save_to_coff(const char *infile, uint16_t machine);


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
#ifdef _WIN32
    exe = simple_basename(exe);
#endif

    printf("usage: %s [--machine=TARGET] FILE [FILE2 [..]]\n"
           "       %s --help\n"
           "\n"
           "TARGET values: ARM ARM64 X64 X86 NONE\n"
           "or 2 byte Image File Machine Constant, i.e. 0x8664\n"
           "\n", exe, exe);
}

static void try_help(const char *msg, const char *exe)
{
#ifdef _WIN32
    exe = simple_basename(exe);
#endif

    if (msg) {
        fprintf(stderr, "%s\n", msg);
    }
    fprintf(stderr, "Try `%s --help' for more information.\n", exe);
}


int main(int argc, char **argv)
{
    char *p;
    uint16_t machine = IMAGE_FILE_MACHINE_DEFAULT;
    int argind = 0;

    if (argc < 2) {
        try_help("No arguments.", argv[0]);
        return 1;
    }

    /* parse arguments */

    for (int i = 1; i < argc; i++) {
        if (STREQ(argv[i], "--help")) {
            print_help(argv[0]);
            return 0;
        }
    }

    for (int i = 1; i < argc; i++) {
        if (!STRBEG(argv[i], "--")) {
            argind = i;
            break;
        }

        p = argv[i] + 2;

        if (STREQ(p, "machine") || STREQ(p, "machine=")) {
            fprintf(stderr, "missing argument: %s\n", argv[i]);
            try_help(NULL, argv[0]);
            return 1;
        }
        else if (STRBEG(p, "machine=")) {
            p += sizeof("machine=") - 1;

            if (STRBEG(p, "0x")) {
                machine = read_hex(p);
            } else if (STREQ(p, "X64")) {
                machine = IMAGE_FILE_MACHINE_AMD64;
            } else if (STREQ(p, "X86")) {
                machine = IMAGE_FILE_MACHINE_I386;
            } else if (STREQ(p, "ARM")) {
                machine = IMAGE_FILE_MACHINE_ARMNT;
            } else if (STREQ(p, "ARM64")) {
                machine = IMAGE_FILE_MACHINE_ARM64;
            } else if (STREQ(p, "NONE")) {
                machine = IMAGE_FILE_MACHINE_UNKNOWN;
            } else {
                fprintf(stderr, "unknown argument for --machine: %s\n", p);
                try_help(NULL, argv[0]);
                return 1;
            }
            continue;
        }

        fprintf(stderr, "unknown option: %s\n", argv[i]);
        try_help(NULL, argv[0]);
        return 1;
    }

    if (argind == 0) {
        try_help("missing input file(s)", argv[0]);
        return 1;
    }

    while (argind < argc) {
        save_to_coff(argv[argind++], machine);
    }

    return 0;
}

