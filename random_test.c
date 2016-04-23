#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

void generate_content(char *buf, int size);
void open_files(char *path, int num);
void close_files(int num);


int
main()
{
    // test //////dirname//subdirname///file
    // /a/b/c(symlink, /d/e, c) (symlink, /a/b/c, /d/e))
    printf("testing seek\n");
    printf("[Mkdir] /a, fd: %d\n", MkDir("a"));
    printf("[Create] c, fd: %d\n", Create("c"));
    printf("[Mkdir] ////a//b, code: %d\n", MkDir("/a/b"));
    printf("[Chdir] /a/b, code: %d\n", ChDir("/a/b"));
    printf("[Open] c, fd: %d\n", Open("c"));
    printf("[Chdir] .., code: %d\n", ChDir(".."));
    printf("[Open] c, fd: %d\n", Open("c"));
    printf("[Chdir] .., code: %d\n", ChDir(".."));
    printf("[Open] c, fd: %d\n", Open("c"));

    Shutdown();
    exit(0);
}

void
generate_content(char *buf, int size) {
    int itr;
    for (itr = 0; itr < size; itr++) {
        buf[itr] = 'b';
    }
}

void
open_files(char *path, int num) {
    int itr;
    for (itr = 0; itr < num; itr++) {
        printf("\tfd: %d\n", Open(path));
    }

}

void
close_files(int num) {
    int itr;
    for (itr = 0; itr < num; itr++) {
        printf("\tclose fd:%d, status %d\n", itr, Close(itr));
    }
}