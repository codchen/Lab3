#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
	int status;
	static char buffer[1024];
	struct Stat sb;
	int fd;

	status = Create("/a");
	printf("Create status %d\n", status);

	status = SymLink("/a", "/b");
	printf("SymLink status %d\n", status);

	status = ReadLink("/b", buffer, sizeof(buffer));
	printf("ReadLink status %d\n", status);
	printf("link = '%s'\n", buffer);

	status = Stat("/a", &sb);
	printf("Stat status %d\n", status);
	printf("/a: inum %d type %d size %d nlink %d\n",
	    sb.inum, sb.type, sb.size, sb.nlink);

	status = Stat("/b", &sb);
	printf("Stat status %d\n", status);
	printf("/b: inum %d type %d size %d nlink %d\n",
	    sb.inum, sb.type, sb.size, sb.nlink);

	status = SymLink("/00/11/22/33/44/55/66/77/88/99", "/xxx");
	printf("SymLink status %d\n", status);

	status = SymLink("00/11/22/33/44/55/66/77/88/99", "yyy");
	printf("SymLink status %d\n", status);

	fd = Open("/a");
	printf("Open /a status %d\n", fd);

	status = Write(fd, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 27);
	printf("Write /a status %d\n", status);

	fd = Open("b");
	printf("Open b status %d\n", fd);

	status = Read(fd, buffer, sizeof(buffer));
	printf("Read b status %d\n", status);
	printf("buffer = '%s'\n", buffer);

	Shutdown();
}
