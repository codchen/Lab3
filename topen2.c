#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

/* Try tcreate2 before this, or try this just by itself */

int
main()
{
	printf("\n%d\n\n", Open("/foo"));
	printf("\n%d\n\n", Open("/bar"));
	printf("\n%d\n\n", Open("/foo"));
	printf("\n%d\n\n", Open("/foo/zzz"));

	Shutdown();
}
