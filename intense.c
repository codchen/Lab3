#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    int fd = Create("abc");
    // int r = Fork();
    // if (r == 0) {
    // 	Delay(15);
    // 	printf("%d\n",Write(fd, "hahaha", 6));
    // 	Shutdown();
    // }
    // else {
    	int i;
    	for (i = 0; i < 50; i++) {
    		// if (i == 25) {
    		// 	printf("unlink abc: %d\n", Unlink("///abc"));
    		// }
    		char name[3];
    		sprintf(name, "%d",i);
    		int tmp = Create(name);
    		Close(tmp);
    		//printf("fd: %d\n", tmp);
    		//printf("close current: %d\n",Close(tmp));
    		if (i > 30) {
    			Unlink(name);
    			//printf("unlink %s: %d\n", name, Unlink(name));
    		}
    	}
    // }
    Exit(0);
}