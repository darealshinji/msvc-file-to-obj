#pragma once

#ifdef _WIN32
# include <windows.h>
#endif
#include <stdint.h>


/* https://learn.microsoft.com/en-us/windows/win32/sysinfo/image-file-machine-constants */
#if !defined(_WIN32)
# define IMAGE_FILE_MACHINE_AMD64    0x8664
# define IMAGE_FILE_MACHINE_I386     0x014C
# define IMAGE_FILE_MACHINE_ARM64    0xAA64
# define IMAGE_FILE_MACHINE_ARMNT    0x01C4
# define IMAGE_FILE_MACHINE_UNKNOWN  0
#endif

/* set default machine type */
#if defined(_M_X64) || defined(__x86_64__)
# define IMAGE_FILE_MACHINE_DEFAULT  IMAGE_FILE_MACHINE_AMD64
#elif defined(_M_IX86) || defined(__i386__)
# define IMAGE_FILE_MACHINE_DEFAULT  IMAGE_FILE_MACHINE_I386
#elif defined(_M_ARM64) || defined(__aarch64__)
# define IMAGE_FILE_MACHINE_DEFAULT  IMAGE_FILE_MACHINE_ARM64
#elif defined(_M_ARMT) || defined(__thumb__)
# define IMAGE_FILE_MACHINE_DEFAULT  IMAGE_FILE_MACHINE_ARMNT
#else
# define IMAGE_FILE_MACHINE_DEFAULT  IMAGE_FILE_MACHINE_UNKNOWN
#endif


/* https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-image_file_header */
#if !defined(_WIN32)
# define IMAGE_FILE_RELOCS_STRIPPED  0x0001
# define IMAGE_FILE_DEBUG_STRIPPED   0x0200

typedef struct _IMAGE_FILE_HEADER {
  uint16_t Machine;
  uint16_t NumberOfSections;
  uint32_t TimeDateStamp;
  uint32_t PointerToSymbolTable;
  uint32_t NumberOfSymbols;
  uint16_t SizeOfOptionalHeader;
  uint16_t Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;
#endif //!_WIN32


/* https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-image_section_header */
#if !defined(_WIN32)
# define IMAGE_SIZEOF_SHORT_NAME         8
# define IMAGE_SCN_MEM_READ              0x40000000
# define IMAGE_SCN_CNT_INITIALIZED_DATA  0x00000040

typedef struct _IMAGE_SECTION_HEADER {
  uint8_t  Name[IMAGE_SIZEOF_SHORT_NAME];
  union {
    uint32_t PhysicalAddress;
    uint32_t VirtualSize;
  } Misc;
  uint32_t VirtualAddress;
  uint32_t SizeOfRawData;
  uint32_t PointerToRawData;
  uint32_t PointerToRelocations;
  uint32_t PointerToLinenumbers;
  uint16_t NumberOfRelocations;
  uint16_t NumberOfLinenumbers;
  uint32_t Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;
#endif //!_WIN32


typedef struct _HEADER_DATA {
  IMAGE_FILE_HEADER    FileHeader;
  IMAGE_SECTION_HEADER Sections[3];
} HEADER_DATA;


/* https://delorie.com/djgpp/doc/coff/symtab.html
 * https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#coff-symbol-table
 * https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#symbol-name-representation */
#ifndef IMAGE_SYM_CLASS_EXTERNAL
#define IMAGE_SYM_CLASS_EXTERNAL  2
#endif
#define SYMBOL_SIZEOF_SHORT_NAME  8

typedef struct _SYMBOL_TABLE_ENTRY {
  union {
    uint8_t  Name[SYMBOL_SIZEOF_SHORT_NAME];
    struct {
      uint32_t Zeroes;
      uint32_t Offset;
    } Offset;
  } u;
  uint32_t Value;
  uint16_t SectionNumber;
  uint16_t Type;
  uint8_t  StorageClass;
  uint8_t  NumberOfAuxSymbols;
  uint8_t  Unused[6];  /* unaligned struct size is 18 bytes */
} SYMBOL_TABLE_ENTRY;

