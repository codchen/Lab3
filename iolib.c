#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <stdio.h>

typedef struct opened_file {
	int inode;
	int pos;
} OpenedFile;

OpenedFile open_file_table[MAX_OPEN_FILES];
int next_fd = 0; //-1 if MAX_OPEN_FILES files are already opened

extern int Open(char * pathname) {

	return 0;
}

extern int Close(int);
extern int Create(char *);
extern int Read(int, void *, int);
extern int Write(int, void *, int);
extern int Seek(int, int, int);
extern int Link(char *, char *);
extern int Unlink(char *);
extern int SymLink(char *, char *);
extern int ReadLink(char *, char *, int);
extern int MkDir(char *);
extern int RmDir(char *);
extern int ChDir(char *);
extern int Stat(char *, struct Stat *);
extern int Sync(void);
extern int Shutdown(void);
