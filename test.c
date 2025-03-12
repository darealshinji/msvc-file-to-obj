#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "incbin_msvc.h"

INCBIN(lorem_txt);


int main()
{
    const char *txt = (const char *)lorem_txt;

    printf("%s\n", txt);
    printf("GET_SIZE: %u\n", GETINC_SIZE(lorem_txt));
    printf("strlen: %zd\n", strlen(txt));

    return 0;
}
