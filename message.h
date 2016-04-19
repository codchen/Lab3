#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>

#define SYNC 1
#define SHUT 2
#define IO 3
#define INODE 4
#define UNLINK 5
#define RMDIR 6
#define LINK 7
#define CREATE 8
#define SEEK 9

typedef struct message_template {
	short message_type;
	short inode;
	int reuse;
	int len;
	void *addr;
} MessageTemplate;

typedef struct message_get_inode {
	short message_type;
	short inode; //for dir in input; for file in output
	int reuse; //for dir in input; for file in output
	int name_len;
	void *name_addr;
	short file_type;
	short sym_pursue;
	int nlink;
} MessageInode;

typedef struct message_unlink {
	short message_type;
	short inode; //for dir in input; output: SUCCESS\DELETED\NO_PATH\REUSE_MISMATCH
	int reuse; //for dir in input; for file in output
	int name_len;
	void *name_addr;
} MessageUnlink;

typedef struct message_rmdir {
	short message_type;
	short inode; //for dir in input; output: SUCCESS\DELETED\NO_PATH\NOT_EMPTY\REUSE_MISMATCH
	int reuse; //for dir in input; for file in output
	int name_len;
	void *name_addr;
} MessageRmDir;

typedef struct message_sync {
	short message_type;
} MessageSync;

typedef struct message_shut {
	short message_type;
} MessageShut;

typedef struct message_link {
	short message_type;
	short inode;
	int reuse;
	int name_len;
	void *name_addr;
	short target_inode;
	int target_reuse;
} MessageLink;

typedef struct message_create {
	short message_type;
	short inode;
	int reuse;
	int name_len;
	void *name_addr;
	short type;
} MessageCreate;

typedef struct message_io {
	short message_type;
	short inode;
	int reuse;
	int len;
	void *buf;
	short read;
	int pos;
} MessageIO;

typedef struct message_seek {
	short message_type;
	short inode;
	int reuse;
	int size;
} MessageSeek;