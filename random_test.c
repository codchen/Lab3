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
    // /a/b/c, symlink(c, /d/e), mkdir(d), chdir(d), mkdir(e), chdir(e), create(f), open(/a/b/c/f)
    printf("testing symlink\n");
    printf("[Mkdir] /a, code: %d\n", MkDir("a"));
    printf("[Mkdir] /a/b, code: %d\n", MkDir("/a/b"));
    printf("[Chdir] /a/b, code: %d\n", ChDir("/a/b"));
    printf("[SymLink] c, /d/e, code: %d\n", SymLink("c", "/d/e"));
    printf("[Chdir] /, code: %d\n", ChDir("/"));
    printf("[Mkdir] /d, code: %d\n", MkDir("d"));
    printf("[Mkdir] /d/e, code: %d\n", MkDir("/d/e"));
    printf("[Create] /d/e/f, fd: %d\n", Create("/d/e/f"));
    printf("[Open] /a/b/c/f, fd: %d\n", Open("/a/b/c/f"));

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