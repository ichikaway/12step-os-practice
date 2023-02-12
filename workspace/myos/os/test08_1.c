#include "defines.h"
#include "kozos.h"
#include "lib.h"

int test08_1_main(int argc, char *argv[])
{
    static char buf[32];

    puts("test01_1_main started.\n");

    while (1) {
        puts("> ");
        gets(buf);

        if (strncmp(buf, "echo", 4) == 0) {
            puts(buf + 4);
            puts("\n");
        } else if (strcmp(buf, "exit") == 0) {
            break;
        } else {
            puts("unknown.\n");
        }
    }

    puts("test08_1 exit. \n");
    return 0;
}