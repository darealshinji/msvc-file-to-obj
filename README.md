Save files directly into COFF object format for use with MSVC and make them
accessible externally in other translation units.

Usage
-----

Compile tool and test:
```
cl -W3 -O2 file_to_obj.c

file_to_obj Lorem.txt
cl -W3 -O2 test.c Lorem.txt.obj
```

The tool itself can also be compiled with MinGW or on Linux.

Using the tool like `file_to_obj data.bin` will generate a file `data.bin.obj`
with the symbols `data_bin data_bin_INCBIN_SIZE_BIG data_bin_INCBIN_SIZE_LITTLE`.
The symbols will be prefixed with an underscore if the filename begins with a digit.
4 NUL bytes will be appended to the data so you can use it as NUL termintated text
in different encodings. Those NUL bytes are not counted in the data size values.

Example:
``` C
#include "incbin_msvc.h"

/**
 * reference external symbols:
 * uint8_t[] data byte array
 * uint32_t data size, Big Endian
 * uint32_t data size, Little Endian
 */
INCBIN(data_bin);

/* to use the data simply cast to any type, e.g. const char* */
const char *text = (const char *)data_bin;

/* use the INCBIN_SIZE() macro to receive the correct size
 * value for the target platfrom (excluding terminating NUL) */
printf("size: %u\n", INCBIN_SIZE(data_bin));
```
