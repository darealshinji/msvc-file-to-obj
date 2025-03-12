/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Carsten Janssen
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
#include "file_to_obj.h"


/* case-insensitive string checks */
#define STREQ(a,b)       (strcasecmp(a,b) == 0)
#define STRBEG(STR,PFX)  (strncasecmp(STR, PFX, sizeof(PFX)-1) == 0)


/* set default machine type */
#if defined(_M_X64) || defined(_M_AMD64)
# define MDEF mx64
#elif defined(_M_IX86)
# define MDEF mx86
#elif defined(_M_ARM64)
# define MDEF marm64
#elif defined(_M_ARM) && defined(_M_THUMB)
# define MDEF marmnt
#else
# define MDEF munknown
#endif

/* IMAGE_FILE_MACHINE_AMD64 = 0x8664 */
static const uint8_t     mx64[2] = { 0x64, 0x86 };

/* IMAGE_FILE_MACHINE_I386 = 0x014C */
static const uint8_t     mx86[2] = { 0x4C, 0x01 };

/* IMAGE_FILE_MACHINE_ARM64 = 0xAA64 */
static const uint8_t   marm64[2] = { 0x64, 0xAA };

/* IMAGE_FILE_MACHINE_ARMNT = 0x01C4 */
static const uint8_t   marmnt[2] = { 0xC4, 0x01 };

/* IMAGE_FILE_MACHINE_UNKNOWN */
static const uint8_t munknown[2] = { 0x00, 0x00 };



static int read_hex(uint8_t *buf, const char *text)
{
    unsigned long val;
    uint16_t res;

    if (strlen(text) > 6) {
        fprintf(stderr, "value longer than 2 bytes: %s\n", text);
        return -1;
    }

    errno = 0;
    val = strtoul(text, NULL, 16);

    if (errno != 0) {
        perror("strtoul");
        return -1;
    }

    res = htole16((uint16_t)val);
    memcpy(buf, &res, sizeof(uint16_t));

    return 0;
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
    const uint8_t *machine = MDEF;
    uint8_t mbuf[2] = { 0, 0 };
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
                if (read_hex(mbuf, p) != 0) {
                    try_help(NULL, argv[0]);
                    return 1;
                }
                machine = mbuf;
            } else if (STREQ(p, "X86")) {
                machine = mx86;
            } else if (STREQ(p, "X64")) {
                machine = mx64;
            } else if (STREQ(p, "ARM")) {
                machine = marmnt;
            } else if (STREQ(p, "ARM64")) {
                machine = marm64;
            } else if (STREQ(p, "NONE")) {
                machine = munknown;
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
        save_to_coff(argv[argind++], NULL, machine, NULL);
    }

    return 0;
}

