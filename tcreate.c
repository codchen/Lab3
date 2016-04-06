#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    int err;
    write(2, "A\n", 2);
    err = Create("/foo");
    fprintf(stderr, "Create returned %d\n", err);

    Sync();
    Delay(3);
    fprintf(stderr, "Done with Sync\n");

/*	Shutdown(); */

    exit(0);
}
