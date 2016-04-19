#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "message.h"

typedef struct opened_file {
	short type;
	short inode;
	int pos;
	int reuse;
} OpenedFile;

void update_fd();
int check_pathname(char *pathname);

static OpenedFile open_file_table[MAX_OPEN_FILES];
int next_fd = 0; //-1 if MAX_OPEN_FILES files are already opened

int current_dir_inode = ROOTINODE; //this should be changed back to ROOTINODE whenver directory reuse mismatch
int current_dir_reuse = 0;

extern int Open(char *pathname) {
	if (check_pathname(pathname) || next_fd == -1) return ERROR;
	MessageInode *msg = Malloc(32);
	msg->message_type = INODE;
	msg->inode = pathname[0]=='/'?ROOTINODE:current_dir_inode;
	msg->reuse = pathname[0]=='/'?0:current_dir_reuse;
	msg->name_len = strlen(pathname);
	msg->name_addr = (void *)pathname;
	msg->sym_pursue = 1;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Open file %s failed\n", pathname);
		free(msg);
		return ERROR;
	}
	open_file_table[next_fd].type = msg->file_type;
	open_file_table[next_fd].inode = msg->inode;
	open_file_table[next_fd].pos = 0;
	open_file_table[next_fd].reuse = msg->reuse;
	free(msg);
	int res = next_fd;
	update_fd();
	return res;
}

extern int Close(int fd) {
	if (open_file_table[fd].type == INODE_FREE) return ERROR;
	open_file_table[fd].type = INODE_FREE;
	if (next_fd == -1) next_fd = fd;
	return 0;
}

extern int Create(char *pathname) {
	if (check_pathname(pathname) || next_fd == -1) return ERROR;
	MessageCreate *msg = Malloc(32);
	msg->message_type = CREATE;
	msg->inode = pathname[0]=='/'?ROOTINODE:current_dir_inode;
	msg->reuse = pathname[0]=='/'?0:current_dir_reuse;
	msg->name_len = strlen(pathname);
	msg->name_addr = (void *)pathname;
	msg->type = INODE_REGULAR;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Create file %s failed\n", pathname);
		free(msg);
		return ERROR;
	}
	open_file_table[next_fd].type = INODE_REGULAR;
	open_file_table[next_fd].inode = msg->inode;
	open_file_table[next_fd].pos = 0;
	open_file_table[next_fd].reuse = msg->reuse;
	free(msg);
	int res = next_fd;
	update_fd();
	return res;
}

extern int Read(int fd, void *buf, int size) {
	if (open_file_table[fd].type == INODE_FREE) return ERROR;
	MessageIO *msg = Malloc(32);
	msg->message_type = IO;
	msg->inode = open_file_table[fd].inode;
	msg->reuse = open_file_table[fd].reuse;
	msg->len = size;
	msg->buf = buf;
	msg->read = 1;
	msg->pos = open_file_table[fd].pos;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Read file %d failed\n", fd);
		free(msg);
		return ERROR;
	}
	open_file_table[fd].pos += msg->len;
	int res = msg->len;
	free(msg);
	return res;
}

//ensure type is not dir
extern int Write(int fd, void *buf, int size) {
	if (open_file_table[fd].type == INODE_FREE || open_file_table[fd].type == INODE_DIRECTORY) return ERROR;
	MessageIO *msg = Malloc(32);
	msg->message_type = IO;
	msg->inode = open_file_table[fd].inode;
	msg->reuse = open_file_table[fd].reuse;
	msg->len = size;
	msg->buf = buf;
	msg->read = 0;
	msg->pos = open_file_table[fd].pos;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Write file %d failed\n", fd);
		free(msg);
		return ERROR;
	}
	open_file_table[fd].pos += msg->len;
	int res = msg->len;
	free(msg);
	return res;
}

extern int Seek(int fd, int offset, int whence) {
	if (open_file_table[fd].type == INODE_FREE) return ERROR;
	MessageSeek *msg = Malloc(32);
	msg->message_type = SEEK;
	msg->inode = open_file_table[fd].inode;
	msg->reuse = open_file_table[fd].reuse;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Get file size %d failed\n", fd);
		free(msg);
		return ERROR;
	}
	int size = msg->size;
	free(msg);
	switch(whence) {
		case SEEK_SET:
			if (offset < 0) {
				fprintf(stderr, "offset must be non-negative for SEEK_SET\n");
				return ERROR;
			}
			open_file_table[fd].pos = offset;
			return open_file_table[fd].pos;
		case SEEK_CUR:
			if (open_file_table[fd].pos + offset < 0) {
				fprintf(stderr, "Seek beyond the beginning of the file\n");
				return ERROR;
			}
			open_file_table[fd].pos += offset;
			return open_file_table[fd].pos;
		case SEEK_END:
			if (offset > 0) {
				fprintf(stderr, "offset must be non-positive for SEEK_END\n");
				return ERROR;
			}
			if (size + offset < 0) {
				fprintf(stderr, "Seek beyond the beginning of the file\n");
				return ERROR;
			}
			open_file_table[fd].pos = size + offset;
			return open_file_table[fd].pos;
	}
	fprintf(stderr, "Invalid whence value: %d\n", whence);
	return ERROR;
}

extern int Link(char *oldname, char *newname) {
	if (check_pathname(oldname) || check_pathname(newname)) return ERROR;
	MessageInode *msg = Malloc(32);
	msg->message_type = INODE;
	msg->inode = oldname[0]=='/'?ROOTINODE:current_dir_inode;
	msg->reuse = oldname[0]=='/'?0:current_dir_reuse;
	msg->name_len = strlen(oldname);
	msg->name_addr = (void *)oldname;
	msg->sym_pursue = 1;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Old name not exist: %s\n", oldname);
		free(msg);
		return ERROR;
	}
	if (msg->file_type == INODE_DIRECTORY) {
		fprintf(stderr, "Cannot link to a directory: %s\n", oldname);
		free(msg);
		return ERROR;
	}
	short target_inode = msg->inode;
	int target_reuse = msg->reuse;
	free(msg);
	MessageLink *msg_link = Malloc(32);
	msg_link->message_type = LINK;
	msg_link->inode = newname[0]=='/'?ROOTINODE:current_dir_inode;
	msg_link->reuse = newname[0]=='/'?0:current_dir_reuse;
	msg_link->name_len = strlen(newname);
	msg_link->name_addr = (void *)newname;
	msg_link->target_inode = target_inode;
	msg_link->target_reuse = target_reuse;
	Send((void *)msg_link, -FILE_SERVER);
	if (msg_link->inode < 0) {
		fprintf(stderr, "Add link %s failed\n", newname);
		free(msg_link);
		return ERROR;
	}
	free(msg_link);
	return 0;
}

extern int Unlink(char *pathname) {
	if (check_pathname(pathname)) return ERROR;
	MessageUnlink *msg = Malloc(32);
	msg->message_type = UNLINK;
	msg->inode = pathname[0]=='/'?ROOTINODE:current_dir_inode;
	msg->reuse = pathname[0]=='/'?0:current_dir_reuse;
	msg->name_len = strlen(pathname);
	msg->name_addr = (void *)pathname;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Pathname %s not exist\n", pathname);
		free(msg);
		return ERROR;
	}
	return 0;
}

extern int SymLink(char *oldname, char *newname) {
	if (check_pathname(oldname) || check_pathname(newname) || strlen(oldname) == 0) return ERROR;
	MessageCreate *msg = Malloc(32);
	msg->message_type = CREATE;
	msg->inode = newname[0]=='/'?ROOTINODE:current_dir_inode;
	msg->reuse = newname[0]=='/'?0:current_dir_reuse;
	msg->name_len = strlen(newname);
	msg->name_addr = (void *)newname;
	msg->type = INODE_SYMLINK;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Create symbolic link %s failed\n", newname);
		free(msg);
		return ERROR;
	}
	int inode = msg->inode;
	int reuse = msg->reuse;
	free(msg);
	MessageIO *msg_write = Malloc(32);
	msg_write->message_type = IO;
	msg_write->inode = inode;
	msg_write->reuse = reuse;
	msg_write->len = strlen(oldname);
	msg_write->buf = (void *)oldname;
	msg_write->read = 0;
	msg_write->pos = 0;
	Send((void *)msg_write, -FILE_SERVER);
	if (msg_write->inode < 0) {
		fprintf(stderr, "Symbolic file %s not exist. Please try again\n", newname);
		free(msg_write);
		return ERROR;
	}
	if (msg_write->len != strlen(oldname)) {
		//TODO: delete symbolic file
		fprintf(stderr, "Write symbolic file %s failed\n", newname);
		free(msg_write);
		return ERROR;
	}
	free(msg_write);
	return 0;
}

extern int ReadLink(char *pathname, char *buf, int len) {
	if (check_pathname(pathname)) return ERROR;
	MessageInode *msg = Malloc(32);
	msg->message_type = INODE;
	msg->inode = pathname[0]=='/'?ROOTINODE:current_dir_inode;
	msg->reuse = pathname[0]=='/'?0:current_dir_reuse;
	msg->name_len = strlen(pathname);
	msg->name_addr = (void *)pathname;
	msg->sym_pursue = 0;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Read symbolic file %s failed\n", pathname);
		free(msg);
		return ERROR;
	}
	int inode = msg->inode;
	int reuse = msg->reuse;
	free(msg);
	MessageIO *msg_read = Malloc(32);
	msg_read->message_type = IO;
	msg_read->inode = inode;
	msg_read->reuse = reuse;
	msg_read->len = len;
	msg_read->buf = (void *)buf;
	msg_read->read = 1;
	msg_read->pos = 0;
	Send((void *)msg_read, -FILE_SERVER);
	if (msg_read->inode < 0) {
		fprintf(stderr, "Symbolic file %s not exist. Please try again\n", pathname);
		free(msg_read);
		return ERROR;
	}
	int res = msg_read->len;
	free(msg_read);
	return res;
}

extern int MkDir(char *pathname) {
	if (check_pathname(pathname)) return ERROR;
	MessageCreate *msg = Malloc(32);
	msg->message_type = CREATE;
	msg->inode = pathname[0]=='/'?ROOTINODE:current_dir_inode;
	msg->reuse = pathname[0]=='/'?0:current_dir_reuse;
	msg->name_len = strlen(pathname);
	msg->name_addr = (void *)pathname;
	msg->type = INODE_DIRECTORY;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Create directory %s failed\n", pathname);
		free(msg);
		return ERROR;
	}
	free(msg);
	return 0;
}

extern int RmDir(char *pathname) {
	if (check_pathname(pathname)) return ERROR;
	MessageRmDir *msg = Malloc(32);
	msg->message_type = RMDIR;
	msg->inode = pathname[0]=='/'?ROOTINODE:current_dir_inode;
	msg->reuse = pathname[0]=='/'?0:current_dir_reuse;
	msg->name_len = strlen(pathname);
	msg->name_addr = (void *)pathname;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Remove directory %s failed\n", pathname);
		free(msg);
		return ERROR;
	}
	return 0;
}

extern int ChDir(char *pathname) {
	if (check_pathname(pathname)) return ERROR;
	MessageInode *msg = Malloc(32);
	msg->message_type = INODE;
	msg->inode = pathname[0]=='/'?ROOTINODE:current_dir_inode;
	msg->reuse = pathname[0]=='/'?0:current_dir_reuse;
	msg->name_len = strlen(pathname);
	msg->name_addr = (void *)pathname;
	msg->sym_pursue = 1;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Chang to directory %s failed\n", pathname);
		free(msg);
		return ERROR;
	}
	current_dir_inode = msg->inode;
	current_dir_reuse = msg->reuse;
	free(msg);
	return 0;
}

extern int Stat(char *pathname, struct Stat *statbuf) {
	if (check_pathname(pathname)) return ERROR;
	MessageInode *msg = Malloc(32);
	msg->message_type = INODE;
	msg->inode = pathname[0]=='/'?ROOTINODE:current_dir_inode;
	msg->reuse = pathname[0]=='/'?0:current_dir_reuse;
	msg->name_len = strlen(pathname);
	msg->name_addr = (void *)pathname;
	msg->sym_pursue = 1;
	Send((void *)msg, -FILE_SERVER);
	if (msg->inode < 0) {
		fprintf(stderr, "Get stat of file %s failed\n", pathname);
		free(msg);
		return ERROR;
	}
	statbuf->inum = msg->inode;
	statbuf->type = msg->file_type;
	statbuf->size = msg->name_len;
	statbuf->nlink = msg->nlink;
	free(msg);
	return 0;
}

extern int Sync() {
	MessageSync *msg = Malloc(32);
	msg->message_type = SYNC;
	Send((void *)msg, -FILE_SERVER);
	return 0;
}

extern int Shutdown() {
	MessageShut *msg = Malloc(32);
	msg->message_type = SHUT;
	Send((void *)msg, -FILE_SERVER);
	return 0;
}

void update_fd(){
	int next = (next_fd + 1) % MAX_OPEN_FILES;
	while (open_file_table[next].type != INODE_FREE) {
		next = (next + 1) % MAX_OPEN_FILES;
		if (next == next_fd) break;
	}
	next_fd = (next == next_fd?-1:next);
}

int check_pathname(char *pathname) {
	if (strlen(pathname) > MAXPATHNAMELEN - 1) {
		fprintf(stderr, "pathname %s too long\n", pathname);
		return 1;
	}
	else return 0;
}

