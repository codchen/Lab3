#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    int fd = Create('abc');
    int r = Fork();
    if (r == 0) {
    	Delay(5);
    	printf("%d\n",Write(fd, "hahaha", 6));
    }
/*	Shutdown(); */
    Shutdown();
    exit(0);
}