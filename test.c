#include <stdio.h>
#include <string.h>
#include "incbin_msvc.h"

INCBIN(Lorem1_txt);
INCBIN(Lorem2_txt);
INCBIN(Lorem3_txt);


int main()
{
    const char *txt1 = (const char *)Lorem1_txt;
    const char *txt2 = (const char *)Lorem2_txt;
    const char *txt3 = (const char *)Lorem3_txt;

    printf("%s%s%s\n", txt1, txt2, txt3);
    printf("data size: %u\n", INCBIN_SIZE(Lorem1_txt) + INCBIN_SIZE(Lorem2_txt) + INCBIN_SIZE(Lorem3_txt));
    printf("strlen: %zd\n", strlen(txt1) + strlen(txt2) + strlen(txt3));

    return 0;
}
