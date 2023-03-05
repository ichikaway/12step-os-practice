#include "defines.h"
#include "kozos.h"
#include "lib.h"

int test11_2_main(int argc, char *argv[])
{
    char *p;
    int size;

    puts("test11_2 started.\n");

    puts("test11_2 send1 in.\n");
    kz_send(MSGBOX_ID_MSGBOX1, 30, "test11_2 send1 static memory\n");
    puts("test11_2 send1 out.\n");

    p = kz_kmalloc(33);
    strcpy(p, "test11_2 send2 allocated memory\n");
    puts("test11_2 send2 in.\n");
    kz_send(MSGBOX_ID_MSGBOX1, 33, p);
    puts("test11_2 send2 out.\n");


    puts("test11_2 recv1 in.\n");
    kz_recv(MSGBOX_ID_MSGBOX2, &size, &p);
    puts("test11_2 recv1 out.\n");
    puts(p);

    puts("test11_2 recv2 in.\n");
    kz_recv(MSGBOX_ID_MSGBOX2, &size, &p);
    puts("test11_2 recv2 out.\n");
    puts(p);
    kz_kmfree(p);


    puts("test11_2 exit.\n");
    return 0;
}