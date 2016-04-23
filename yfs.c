#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "message.h"
#define MESSAGESIZE 32
#define INODEPERBLOCK BLOCKSIZE/INODESIZE
#define MAX_FILE_SIZE NUM_DIRECT * BLOCKSIZE + (BLOCKSIZE / sizeof(int)) * BLOCKSIZE
#define INODE2BLOCK(x) (x / INODEPERBLOCK + 1)
#define CEILING(x, y) (x / y + (x % y > 0))
#define INODECACHE 0
#define BLOCKCACHE 1 
#define MAXDIRPERBLOCK BLOCKSIZE/sizeof(struct dir_entry)
#ifndef MAX
#define MAX(x,y) (x>y?x:y)
#endif
#define BLOCKSPAN(x, y) (MAX(CEILING(y,BLOCKSIZE)-x/BLOCKSIZE,0))


void *msg_buf;
void *sector_buf;
int total_block;
int total_inode;
LinkedList *free_inodes;
LinkedList *free_blocks;

LRUNode *inode_cache_head, *inode_cache_tail, *block_cache_head, *block_cache_tail;
LRUNode *inode_cache[INODE_CACHESIZE];
LRUNode *block_cache[BLOCK_CACHESIZE];
int inode_cache_size = 0;
int block_cache_size = 0;

void init(void);
void add_free_inode(int free_inode_num);
void add_free_block(int free_block_num);
int get_free_inode();
int get_free_block();

int check_reuse(int inode, int reuse);

int create_file_by_path(int directory_inode, char *filename, short type);
int free_file_blocks(int file_inode);
void free_file_inode(int inode_num);
int has_subdir(int parent_inode, char *path, int len);
void add_dir_entry(int parent_inode, int inum, char *entry_name);
int remove_dir_entry(int parent_inode, char *entry_name);
int traverse_wrapper(char *path, int start_inode);
int dir_traverse(char *path, int current_inode, int *symlink_traversed);

int read_from_file(int inode_num, int pos, void *buf, size_t size);
int write_to_file(int inode_num, int pos, void *buf, size_t size);

void add_cache(int num, void *content, int which_cache);
void write_dirty_back();
void *get_block(int num);
struct inode *get_inode(int num);
void mark_block_dirty(int num, int dirty);
void mark_inode_dirty(int num, int dirty);
LRUNode *query_hash(int num, int which_cache);

void init(){
	Register_w(FILE_SERVER);
	sector_buf = Malloc(SECTORSIZE);
	ReadSector_w(1, sector_buf);
	total_block = ((struct fs_header *)sector_buf)->num_blocks;
	total_inode = ((struct fs_header *)sector_buf)->num_inodes;
	printf("Total block number: %d\nTotal inode number: %d\n", total_block, total_inode);

	free_inodes = Malloc(sizeof(LinkedList));
	free_inodes->val = -1;
	free_blocks = Malloc(sizeof(LinkedList));
	free_blocks->val = -1;
	char *occupied_blocks = Calloc(total_block, sizeof(char));
	int which_inode, which_block;
	for (which_block = 1; which_block <= CEILING(total_inode + 1, INODEPERBLOCK); which_block++) {
		if (which_block != 1) ReadSector_w(which_block, sector_buf);
		occupied_blocks[which_block] = 1;
		for (which_inode = 0; which_inode < INODEPERBLOCK; which_inode++) {
			if (which_block == 1 && which_inode == 0) continue;
			if (which_inode + INODEPERBLOCK * (which_block - 1) > total_inode) break;
			struct inode current = ((struct inode *)sector_buf)[which_inode];
			if (current.type == INODE_FREE) add_free_inode(which_inode + INODEPERBLOCK * (which_block - 1));
			else {
				printf("occupied inode: %d\n", which_inode);
				int used_block = CEILING(current.size, BLOCKSIZE);
				void *indir = Malloc(SECTORSIZE);
				if (used_block > NUM_DIRECT) ReadSector_w(current.indirect, indir);
				int i;
				for (i = 0; i < used_block; i++) {
					occupied_blocks[i < NUM_DIRECT?current.direct[i]:((int *)indir)[i-NUM_DIRECT]] = 1;
				}
				free(indir);
			}
		}
	}
	for (which_block = 1; which_block < total_block; which_block++) {
		if (!occupied_blocks[which_block]) add_free_block(which_block);
	}
	struct inode *root = get_inode(ROOTINODE);
	root->type = INODE_DIRECTORY;
	root->nlink = 0;
	root->reuse = 0;
	root->size = 0;
	mark_inode_dirty(ROOTINODE, 1);
	add_dir_entry(ROOTINODE, ROOTINODE, ".");
	add_dir_entry(ROOTINODE, ROOTINODE, "..");
	free(occupied_blocks);
	msg_buf = Malloc(MESSAGESIZE);
	printf("Successfully Booted\n");
}

void add_free_inode(int free_inode_num) {
	LinkedList *new = Malloc(sizeof(LinkedList));
	new->val = free_inode_num;
	new->next = free_inodes;
	free_inodes = new;
}

void add_free_block(int free_block_num) {
	LinkedList *new = Malloc(sizeof(LinkedList));
	new->val = free_block_num;
	new->next = free_blocks;
	free_blocks = new;
	mark_block_dirty(free_block_num, 0);
}

int get_free_inode() {
	if (free_inodes->val == -1) return 0;
	LinkedList *tmp = free_inodes;
	int res = free_inodes->val;
	free_inodes = free_inodes->next;
	free(tmp);
	return res;
}

int get_free_block() {
	if (free_blocks->val == -1) return 0;
	LinkedList *tmp = free_blocks;
	int res = free_blocks->val;
	free_blocks = free_blocks->next;
	free(tmp);
	return res;
}

//input file_dir, filename must be valid and null-terminated
//symlink should be NULL for non-symlink type file creation
int create_file_by_path(int directory_inode, char *filename, short type) {
	struct inode *parent = get_inode(directory_inode);
	if (parent->type != INODE_DIRECTORY) return TARGET_DIR_NOT_EXIST;
	//check if already exists
	int existing = has_subdir(directory_inode, filename, strlen(filename));
	if (existing > 0) {
		if (type == INODE_DIRECTORY || type == INODE_SYMLINK) return ENTRY_EXIST;
		if (get_inode(existing)->type == INODE_DIRECTORY) return DIR_EXIST;
		return free_file_blocks(existing);
	}
	int new_inode = get_free_inode();
	if (new_inode == 0) return NO_INODE;
	printf("inode number of file %s: %d\n", filename, new_inode);
	struct inode *file = get_inode(new_inode);
	file->type = type;
	file->nlink = 0;
	file->reuse++;
	file->size = 0;
	mark_inode_dirty(new_inode, 1);
	add_dir_entry(directory_inode, new_inode, filename);
	if (type == INODE_DIRECTORY) {
		add_dir_entry(new_inode, new_inode, ".");
		add_dir_entry(new_inode, directory_inode, "..");
	}
	printf("file %s of type %d created at inode %d\n", filename, type, directory_inode);
	return new_inode;
}

int free_file_blocks(int file_inode) {
	struct inode *file = get_inode(file_inode);
	int block_num = CEILING(file->size, BLOCKSIZE);
	int *indirect;
	if (block_num > NUM_DIRECT) indirect = (int *)get_block(file->indirect);
	int i;
	for (i = 0; i < block_num; i++) {
		if (i < NUM_DIRECT) add_free_block(file->direct[i]);
		else add_free_block(indirect[i - NUM_DIRECT]);
	}
	if (block_num > NUM_DIRECT) add_free_block(file->indirect);
	file->size = 0;
	mark_inode_dirty(file_inode, 1);
	return file_inode;
}

void free_file_inode(int inode_num) {
	free_file_blocks(inode_num);
	get_inode(inode_num)->type = INODE_FREE;
	mark_inode_dirty(inode_num, 1);
	add_free_inode(inode_num);
}

//won't make dirty
//return number of valid entries if len < 0
int has_subdir(int parent_inode, char *path, int len) {
	struct inode *parent = get_inode(parent_inode);
	int block_used = CEILING(parent->size, BLOCKSIZE);
	int dirs_num = parent->size / sizeof(struct dir_entry);
	int examined_dir = 0;
	int res = 0;
	int i;
	for (i = 0; i < block_used; i++) {
		struct dir_entry *dirs = (struct dir_entry *)get_block(i<NUM_DIRECT?parent->direct[i]:((int *)get_block(parent->indirect))[i-NUM_DIRECT]);
		int k = 0;
		while (examined_dir < dirs_num && k < MAXDIRPERBLOCK) {
			//printf("sub-directory of %d: %s\n", parent_inode,dirs[k].name);
			if (len < 0 && dirs[k].inum != 0) res++;
			if (len >= 0 && dirs[k].inum != 0 && streq(path, dirs[k].name, len)) return dirs[k].inum;
			examined_dir++;
			k++;
		}
	}
	return (len < 0?res:TARGET_DIR_NOT_EXIST);
}

int remove_dir_entry(int parent_inode, char *entry_name) {
	if (dot_entry(entry_name)) return UNLINK_REMOVE_DOT;
	struct inode *parent = get_inode(parent_inode);
	if (parent->type != INODE_DIRECTORY) return TARGET_DIR_NOT_EXIST;
	int block_num = CEILING(parent->size, BLOCKSIZE);
	int dirs_num = parent->size / sizeof(struct dir_entry);
	int checked_dir = 0;
	struct dir_entry *current_block;
	int current_block_num;
	int i;
	for (i = 0; i < block_num; i++) {
		if (i < NUM_DIRECT) {
			current_block_num = parent->direct[i];
		}
		else {
			current_block_num = ((int *)get_block(parent->indirect))[i - NUM_DIRECT];
		}
		current_block = (struct dir_entry *)get_block(current_block_num);
		int k;
		for (k = 0; k < MAXDIRPERBLOCK; k++) {
			if (checked_dir == dirs_num) return TARGET_DIR_NOT_EXIST;
			checked_dir++;
			if (current_block[k].inum != 0 && streq(entry_name, current_block[k].name, strlen(entry_name))) {
				int res = current_block[k].inum;
				current_block[k].inum = 0;
				mark_block_dirty(current_block_num, 1);
				return res;
			}
		}
	}
	return TARGET_DIR_NOT_EXIST;
}

//entry_name must be null-terminated
void add_dir_entry(int parent_inode, int inum, char *entry_name) {
	struct inode *entry_node = get_inode(inum);
	entry_node->nlink++;
	mark_inode_dirty(inum, 1);
	struct dir_entry *new_entry = Malloc(sizeof(struct dir_entry));
	new_entry->inum = inum;
	paste_name(new_entry->name, entry_name);
	struct inode *parent = get_inode(parent_inode);
	int dirs_num = parent->size / sizeof(struct dir_entry);
	if (dirs_num == 0) {
		write_to_file(parent_inode, 0, (void *)new_entry, sizeof(struct dir_entry));
		free(new_entry);
		return;
	}
	int dirs_checked = 0;
	struct dir_entry *current_block = (struct dir_entry *)get_block(parent->direct[0]); //first block must exist for . and ..
	while (dirs_checked < dirs_num) {
		if (dirs_checked % MAXDIRPERBLOCK == 0) {
			if (dirs_checked / MAXDIRPERBLOCK < NUM_DIRECT) 
				current_block = (struct dir_entry *)get_block(parent->direct[dirs_checked / MAXDIRPERBLOCK]);
			else
				current_block = (struct dir_entry *)get_block(((int *)get_block(parent->indirect))[dirs_checked / MAXDIRPERBLOCK - NUM_DIRECT]);
		}
		if (current_block[dirs_checked % MAXDIRPERBLOCK].inum == 0) {
			write_to_file(parent_inode, dirs_checked * sizeof(struct dir_entry), (void *)new_entry, sizeof(struct dir_entry));
			free(new_entry);
			return;
		}
		dirs_checked++;
	}
	write_to_file(parent_inode, parent->size, (void *)new_entry, sizeof(struct dir_entry));
	free(new_entry);
}

int read_from_file(int inode_num, int pos, void *buf, size_t size) {
	struct inode *file = get_inode(inode_num);
	if (pos >= file->size) return 0;
	int end = MIN(pos + size, file->size);
	int block_span = BLOCKSPAN(pos,end);
	void *current_block;
	int read = 0;
	int i;
	for (i = 0; i < block_span; i++) {
		int s = (i == 0)?pos%BLOCKSIZE:0;
		int e = (i == block_span - 1)?(end-1)%BLOCKSIZE+1:BLOCKSIZE;
		if (pos / BLOCKSIZE + i < NUM_DIRECT) current_block = get_block(file->direct[pos / BLOCKSIZE + i]);
		else current_block = get_block(((int *)get_block(file->indirect))[pos / BLOCKSIZE + i - NUM_DIRECT]);
		memcpy((void *)((long)buf+read), (void *)((long)current_block+s), e - s);
		read += e - s;
	}
	return read;
}

int write_to_file(int inode_num, int pos, void *buf, size_t size) {
	int written = 0;
	struct inode *file = get_inode(inode_num);
	if (pos > file->size) {
		void *tmp = Calloc(pos - file->size, sizeof(char));
		written += write_to_file(inode_num, file->size, tmp, pos - file->size);
		free(tmp);
	}
	int end = MIN(pos+size, MAX_FILE_SIZE);
	int block_span = BLOCKSPAN(pos,end);
	int existing_span = BLOCKSPAN(pos, file->size);
	int current_block_num;
	void *current_block;
	int i;
	for (i = 0; i < block_span; i++) {
		int s = (i == 0)?pos%BLOCKSIZE:0;
		int e = (i == block_span - 1)?(end-1)%BLOCKSIZE+1:BLOCKSIZE;
		if (i >= existing_span) {
			current_block_num = get_free_block();
			if (current_block_num == 0) {
				end = pos + written;
				break;
			}
			if (pos / BLOCKSIZE + i < NUM_DIRECT) file->direct[pos/BLOCKSIZE+i] = current_block_num;
			else {
				((int *)get_block(file->indirect))[pos / BLOCKSIZE + i - NUM_DIRECT] = current_block_num;
				mark_block_dirty(file->indirect, 1);
			}
		}
		else {
			if (pos / BLOCKSIZE + i < NUM_DIRECT) current_block_num = file->direct[pos / BLOCKSIZE + i];
			else current_block_num = ((int *)get_block(file->indirect))[pos / BLOCKSIZE + i - NUM_DIRECT];
		}
		current_block = get_block(current_block_num);
		memcpy((void *)((long)current_block+s), (void *)((long)buf+written), e - s);
		mark_block_dirty(current_block_num, 1);
		written += e - s;
	}
	file->size = end;
	mark_inode_dirty(inode_num,1);
	return written;
}

int traverse_wrapper(char *path, int start_inode) {
	int *symlink_traversed = Malloc(sizeof(int));
	*symlink_traversed = 0;
	int res = dir_traverse(path, start_inode, symlink_traversed);
	free(symlink_traversed);
	return res;
}

//symlin_traversed needs to be malloced and freed by the caller
int dir_traverse(char *path, int current_inode, int *symlink_traversed) {
	int e = 0;
	while (path[e] != '/' && path[e] != '\0') e++;
	int inode;
	if (e == 0) inode = current_inode;
	else inode = has_subdir(current_inode, path, e);
	if (inode < 0) return TARGET_DIR_NOT_EXIST;
	struct inode *next = get_inode(inode);
	if (path[e] == '\0') {
		if (next->type == INODE_SYMLINK) {
			if (*symlink_traversed == MAXSYMLINKS) return MAX_SYM;
			*symlink_traversed = *symlink_traversed + 1;
			char *buf = (char *)Malloc(next->size+1);
			read_from_file(inode,0,(void *)buf,next->size);
			buf[next->size] = '\0';
			inode = dir_traverse(buf, (buf[0] == '/'?ROOTINODE:current_inode), symlink_traversed);
			free(buf);
			return inode;
		}
		return inode;
	}
	switch(next->type) {
		case INODE_FREE:
			return TARGET_DIR_NOT_EXIST;
		case INODE_REGULAR:
			return TARGET_DIR_NOT_EXIST;
		case INODE_DIRECTORY:
			return dir_traverse((char *)((long)path+e+1),inode,symlink_traversed);
		case INODE_SYMLINK:
			if (*symlink_traversed == MAXSYMLINKS) return MAX_SYM;
			*symlink_traversed = *symlink_traversed + 1;
			char *buf = (char *)Malloc(next->size+1);
			read_from_file(inode,0,(void *)buf,next->size);
			buf[next->size] = '\0';
			if (buf[0] == '/') inode = dir_traverse((char *)((long)buf+1),ROOTINODE, symlink_traversed);
			else inode = dir_traverse(buf, current_inode, symlink_traversed);
			free(buf);
			if (inode <= 0) return inode;
			return dir_traverse((char *)((long)path+e+1),inode,symlink_traversed);
		default:
			return TARGET_DIR_NOT_EXIST;
	}
}

int main(int argc, char **argv)
{
	init();
	int pid = Fork();
	if (pid == 0) {
		if (argc < 2) {
			//no test process
			while(1){};
		}
		else {
			Exec(argv[1], argv + 1);
		}
	}
	int received_pid;
	while (1) {
		received_pid = Receive_w(msg_buf);
		MessageTemplate *tmp = (MessageTemplate *)msg_buf;
		switch(tmp->message_type) {
			case SYNC: {
				write_dirty_back();
				Reply(msg_buf, received_pid);
				continue;
			}
			case SHUT: {
				write_dirty_back();
				Reply(msg_buf, received_pid);
				free(msg_buf);
				free(sector_buf);
				exit(1);
			}
		}
		short inode = tmp->inode;
		int reuse = tmp->reuse;
		if (get_inode(inode)->type == INODE_FREE) {
			tmp->inode = CURRENT_DIR_NOT_EXIST;
			Reply(msg_buf, received_pid);
			continue;
		}
		if (get_inode(inode)->reuse != reuse) {
			tmp->inode = CURRENT_DIR_REUSED;
			Reply(msg_buf, received_pid);
			continue;
		}
		if (tmp->message_type == SEEK) {
			MessageSeek *msg_seek = (MessageSeek *)msg_buf;
			msg_seek->size = get_inode(inode)->size;
			Reply(msg_buf, received_pid);
			continue;
		}
		int len = tmp->len;
		char *buf = Malloc(len + 1);
		if (tmp->message_type == IO) {
			MessageIO *msg_io = (MessageIO *)msg_buf;
			if (msg_io->read) {
				msg_io->len = read_from_file(inode, msg_io->pos, (void *)buf, len); //NEED ERROR CHECK
				CopyTo(received_pid, msg_io->buf, (void *)buf, len);
			}
			else {
				CopyFrom(received_pid, (void *)buf, msg_io->buf, len); //NEED ERROR CHECK
				msg_io->len = write_to_file(inode, msg_io->pos, (void *)buf, len);
			}
			Reply(msg_buf, received_pid);
			free(buf);
			continue;
		}
		CopyFrom(received_pid, buf, tmp->addr, len); //NEED ERROR CHECK
		buf[len] = '\0';
		char *name = split_path(buf);
		if (strlen(name) > DIRNAMELEN) {
			tmp->inode = NAME_TOO_LONG;
			Reply(msg_buf, received_pid);
			free(buf);
			free(name);
			continue;
		}
		int parent_inode = traverse_wrapper(buf, inode);
		if (parent_inode <= 0) {
			tmp->inode = TARGET_DIR_NOT_EXIST;
			Reply(msg_buf, received_pid);
			free(buf);
			free(name);
			continue;
		}
		switch(tmp->message_type) {
			case INODE: {		
				MessageInode *msg_inode = (MessageInode *)msg_buf;
				msg_inode->inode = (msg_inode->sym_pursue?traverse_wrapper(name, parent_inode):has_subdir(parent_inode,name,strlen(name)));
				if (msg_inode->inode > 0) {
					msg_inode->reuse = get_inode(msg_inode->inode)->reuse;
					msg_inode->file_type = get_inode(msg_inode->inode)->type;
					msg_inode->name_len = get_inode(msg_inode->inode)->size;
					msg_inode->nlink = get_inode(msg_inode->inode)->nlink;
				}
				break;
			}
			case UNLINK: {
				MessageUnlink *msg_unlink = (MessageUnlink *)msg_buf;
				int target_inode = has_subdir(parent_inode, name, strlen(name));
				if (target_inode < 0 || get_inode(target_inode)->type == INODE_FREE) {
					msg_unlink->inode = TARGET_DIR_NOT_EXIST;
				}
				else if (get_inode(target_inode)->type == INODE_DIRECTORY) {
					msg_unlink->inode = LINK_DIRECTORY;
				}
				else {
					msg_unlink->inode = remove_dir_entry(parent_inode, name);
					if (msg_unlink->inode > 0) {
						get_inode(msg_unlink->inode)->nlink--;
						if (get_inode(msg_unlink->inode)->nlink == 0) free_file_inode(msg_unlink->inode);
					}
				}
				break;
			}
			case RMDIR: {
				MessageRmDir *msg_rm = (MessageRmDir *)msg_buf;
				int this_inode = has_subdir(parent_inode, name, strlen(name));
				if (this_inode < 0 || get_inode(this_inode)->type != INODE_DIRECTORY) {
					msg_rm->inode = TARGET_DIR_NOT_EXIST;
				}
				else if (has_subdir(this_inode, NULL, -1) > 2) {
					msg_rm->inode = DIR_NOT_EMPTY;
				}
				else {
					msg_rm->inode = remove_dir_entry(parent_inode, name);
					if (msg_rm->inode > 0) 
						free_file_inode(this_inode);
				}
				break;
			}
			case LINK: {
				MessageLink *msg_link = (MessageLink *)msg_buf;
				struct inode *target = get_inode(msg_link->target_inode);
				if (target->type == INODE_FREE || target->reuse != msg_link->target_reuse) msg_link->inode = TARGET_DIR_NOT_EXIST;
				else if (has_subdir(parent_inode, name, strlen(name)) > 0) msg_link->inode = ENTRY_EXIST;
				else add_dir_entry(parent_inode, msg_link->target_inode, name);
				break;
			}
			case CREATE: {
				MessageCreate *msg_create = (MessageCreate *)msg_buf;
				msg_create->inode = create_file_by_path(parent_inode, name, msg_create->type);
				if (msg_create->inode > 0) msg_create->reuse = get_inode(msg_create->inode)->reuse;
				break;
			}
		}
		free(buf);
		free(name);
		Reply(msg_buf, received_pid);
	}
}

void add_cache(int num, void *content, int which_cache) {
	LRUNode *new_node = Malloc(sizeof(LRUNode));
	new_node->dirty = 0;
	new_node->num = num;
	new_node->prev = NULL;
	new_node->hash_prev = NULL;
	new_node->content = content;
	LRUNode *head = (which_cache == INODECACHE)?inode_cache_head:block_cache_head;
	LRUNode *tail = (which_cache == INODECACHE)?inode_cache_tail:block_cache_tail;
	LRUNode **cache = (which_cache == INODECACHE)?inode_cache:block_cache;
	if (which_cache == INODECACHE) {
		if (inode_cache_size == 0) inode_cache_tail = new_node;
		else {
			head->prev = new_node;
			new_node->next = head;
		}
		inode_cache_head = new_node;
	}
	else {
		if (block_cache_size == 0) block_cache_tail = new_node;
		else {
			head->prev = new_node;
			new_node->next = head;
		}
		block_cache_head = new_node;
	}
	int cond = which_cache == INODECACHE?inode_cache_size < INODE_CACHESIZE:block_cache_size < BLOCK_CACHESIZE;
	if (cond) {
		if (which_cache == INODECACHE) inode_cache_size++;
		else block_cache_size++;
	}
	else {
		if (tail->hash_prev == NULL) 
			cache[(tail->num)%((which_cache == INODECACHE)?INODE_CACHESIZE:BLOCK_CACHESIZE)] = tail->hash_next;
		else tail->hash_prev->hash_next = tail->hash_next;
		if (tail->hash_next != NULL) tail->hash_next = tail->hash_prev;
		if (which_cache == INODECACHE) inode_cache_tail = tail->prev;
		else block_cache_tail = tail->prev;
		tail = tail->prev;
		if (tail->next->dirty) {
			if (which_cache == INODECACHE) {
				int block_num = INODE2BLOCK(tail->next->num);
				void *block = get_block(block_num);
				memcpy(&(((struct inode *)block)[(tail->next->num)%INODEPERBLOCK]), tail->next->content, INODESIZE);
				mark_block_dirty(block_num, 1);
			}
			else WriteSector_w(tail->next->num, tail->next->content);
		}
		free(tail->next->content);
		free(tail->next);
		tail->next = NULL;
	}
	new_node->hash_next = cache[num % ((which_cache == INODECACHE)?INODE_CACHESIZE:BLOCK_CACHESIZE)];
	if (new_node->hash_next != NULL) new_node->hash_next->hash_prev = new_node;
	cache[num % ((which_cache == INODECACHE)?INODE_CACHESIZE:BLOCK_CACHESIZE)] = new_node;
}

void write_dirty_back(){
	LRUNode *current = inode_cache_head;
	while (current != NULL) {
		if (current->dirty){
			int block_num = INODE2BLOCK(current->num);
			void *block = get_block(block_num);
			memcpy(&(((struct inode *)block)[(current->num)%INODEPERBLOCK]), current->content, INODESIZE);
			mark_block_dirty(block_num, 1);
			mark_inode_dirty(current->num, 0);
		}
		current = current->next;
	}
	current = block_cache_head;
	while (current != NULL) {
		if (current->dirty) {
			WriteSector_w(current->num, current->content);
			mark_block_dirty(current->num, 0);
		}
		current = current->next;
	}
}

void *get_block(int num) {
	LRUNode *block = query_hash(num, BLOCKCACHE);
	if (block != NULL) return block->content;
	void *buf = Malloc(SECTORSIZE);
	ReadSector_w(num, buf);
	add_cache(num, buf, BLOCKCACHE);
	return buf;
}

struct inode *get_inode(int num) {
	LRUNode *inode = query_hash(num, INODECACHE);
	if (inode != NULL) return (struct inode *)(inode->content);
	void *block = get_block(INODE2BLOCK(num));
	void *buf = Malloc(INODESIZE);
	memcpy(buf, &(((struct inode *)block)[num%INODEPERBLOCK]), INODESIZE);
	add_cache(num, buf, INODECACHE);
	return (struct inode *)buf;
}

void mark_block_dirty(int num, int dirty) {
	LRUNode *block = query_hash(num, BLOCKCACHE);
	if (block == NULL) {
		//fprintf(stderr, "block %d not in cache\n", num);
		return;
	}
	block->dirty = dirty;
}

void mark_inode_dirty(int num, int dirty) {
	LRUNode *inode = query_hash(num, INODECACHE);
	if (inode == NULL) {
		//fprintf(stderr, "inode %d not in cache\n", num);
		return;
	}
	inode->dirty = dirty;
}

LRUNode *query_hash(int num, int which_cache) {
	LRUNode *head = which_cache == INODECACHE?inode_cache_head:block_cache_head;
	LRUNode **cache = which_cache == INODECACHE?inode_cache:block_cache;
	LRUNode *cur = cache[num%(which_cache == INODECACHE?INODE_CACHESIZE:BLOCK_CACHESIZE)];
	while (cur != NULL) {
		if (cur->num == num) {
			if (cur->prev != NULL) {
				cur->prev->next = cur->next;
				if (cur->next != NULL) cur->next->prev = cur->prev;
				cur->next = head;
				head->prev = cur;
				cur->prev = NULL;
				if (which_cache == INODECACHE) inode_cache_head = cur;
				else block_cache_head = cur;
			}
			return cur;
		}
		cur = cur->hash_next;
	}
	return NULL;
}
