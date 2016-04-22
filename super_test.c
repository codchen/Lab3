#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>
void custom_test(char *str);
void generate_content(char *buf, int size);

int
main()
{
	char *buf = (char *) malloc(1000);
	size_t s = 1000;

    while (1) {
    	memset(buf, 0, sizeof(buf));
    	getline(&buf, &s, stdin);
    	printf("Echo %s", buf);
		buf[strcspn(buf, "\n")] = 0;
		custom_test(buf);
    	printf("\n");
    	fflush(stdout);
    }
    free(buf);

    exit(0);
}

void
custom_test(char *str) {
	char *p  = strtok(str, " ");
	char *buf;
	int code;
	int fd;
	int size;
	
	if (p == NULL) {
		printf("Please input some kernel call\n");
		return;
	} else if (strcmp(p, "create") == 0) {
		p = strtok(NULL, " ");
		code = Create(p);
		printf("[CREATE] file %s, return fd %d\n", p, code);
	} else if (strcmp(p, "open") == 0){ 
		p = strtok(NULL, " ");
		code = Open(p);
		printf("[OPEN] file %s, return fd %d\n", p, code);
	} else if (strcmp(p, "close") == 0) {
		fd = atoi(strtok(NULL, " "));
		code = Close(fd);
		printf("[CLOSE] fd %d, status: %d\n", fd, code);
	} else if (strcmp(p, "read") == 0) {
		fd = atoi(strtok(NULL, " "));
		size = atoi(strtok(NULL, " "));
		buf = (char *) malloc(size);
		code = Read(fd, buf, size);
		printf("[READ] fd: %d, read content: %s, read size: %d\n", fd, buf, code);
	} else if (strcmp(p, "write") == 0) {
		fd = atoi(strtok(NULL, " "));
		size = atoi(strtok(NULL, " "));
		buf = (char *) malloc(size);
		generate_content(buf, size);
		code = Write(fd, buf, size);
		printf("[WRITE] fd: %d, content: %s, wrote size: %d\n", fd, buf, code);
	} else if (strcmp(p, "mkdir") == 0) {
		p = strtok(NULL, " ");
		code = MkDir(p);
		printf("[MKDIR] dir: %s, return fd: %d\n", p, code);
	} else if (strcmp(p, "chdir") == 0) {
		p = strtok(NULL, " ");
		code = ChDir(p);
		printf("[CHDIR] dir: %s, return fd: %d\n", p, code);
	} else if (strcmp(p, "rmdir") == 0) {
		p = strtok(NULL, " ");
		code = RmDir(p);
		printf("[RMDIR] dir: %s, return fd: %d\n", p, code);
	} else if (strcmp(p, "stat") == 0) {
		p = strtok(NULL, " ");
		struct Stat *st = (struct Stat *) malloc(sizeof(struct Stat));
		code = Stat(p, st);
		printf("[STAT] %s, inode: %d, filetype: %d, size: %d, num_link: %d, return code: %d\n", p, st->inum, st->type, st->size, st->nlink, code);
	} else if (strcmp(p, "link") == 0) { 
		char *oldname = strtok(NULL, " ");
		char *newname = strtok(NULL, " ");
		code = Link(oldname, newname);
		printf("[LINK] old:%s, new:%s, code: %d\n", oldname, newname, code);
	} else if (strcmp(p, "symlink") == 0) { 
		char *oldname = strtok(NULL, " ");
		char *newname = strtok(NULL, " ");
		code = SymLink(oldname, newname);
		printf("[SYMLINK] old:%s, new:%s, code: %d\n", oldname, newname, code);

	} else if (strcmp(p, "exit") == 0) {
		printf("[EXIT]\n");
		exit(0);
	} else {
		printf("Unable to recognize input kernel call. Please try again\n");
	}

}

void
generate_content(char *buf, int size) {
	int itr;
	for (itr = 0; itr < size; itr++) {
		buf[itr] = 'b';
	}
}

/** test cases
 filename: 1. null [PASS]2. longer than dirnamelen 3. starts with /, end with \0

 */ 
