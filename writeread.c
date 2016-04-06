#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    int fd;
    int nch;
    int status;
    char ch;

    fd = Create("/xxxxxx");
    printf("Create fd %d\n", fd);

    nch = Write(fd, "abcdefghijklmnopqrstuvwxyz", 26);
    printf("Write nch %d\n", nch);

    nch = Write(fd, "0123456789", 10);
    printf("Write nch %d\n", nch);

    status = Close(fd);
    printf("Close status %d\n", status);

    Sync();

    fd = Open("/xxxxxx");
    printf("Open fd %d\n", fd);

    while (1) {
	nch = Read(fd, &ch, 1);
	printf("Read nch %d\n", nch);
	if (nch <= 0)
	    break;
	printf("ch 0x%x '%c'\n", ch, ch);
    }

    status = Close(fd);
    printf("Close status %d\n", status);

    Shutdown();
}
