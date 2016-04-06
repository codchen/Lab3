#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

/*
 * Create empty files named "file00" through "file31" in "/".
 */
int
main()
{
	int fd;
	int i;
	char name[7];

	for (i = 0; i < 32; i++) {
		sprintf(name, "file%02d", i);
		fd = Create(name);
		Close(fd);
	}

	Shutdown();
}
