#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "incbin_msvc.h"

INCBIN(Lorem_txt);


int main()
{
    const char *txt = (const char *)Lorem_txt;

    printf("%s\n", txt);
    printf("GET_SIZE: %u\n", INCBIN_SIZE(Lorem_txt));
    printf("strlen: %zd\n", strlen(txt));

    return 0;
}
