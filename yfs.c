#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#define MESSAGESIZE 32
#define INODEPERBLOCK BLOCKSIZE/INODESIZE
#define MAX_FILE_SIZE NUM_DIRECT * BLOCKSIZE + (BLOCKSIZE / sizeof(int)) * BLOCKSIZE
#define CEILING(x, y) (x / y + (x % y > 0)) 

typedef struct linked_list {
	int val;
	struct linked_list *next;
} LinkedList;

void *msg_buf;
void *sector_buf;
int total_block;
int total_inode;
LinkedList *free_inodes;
LinkedList *free_blocks;

void init(void);
void add_free_inode(int free_inode_num);
void add_free_block(int free_block_num);

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
				if (current.size > MAX_FILE_SIZE) {
					fprintf(stderr, "File size exceeds maximum limit\n");
					continue;
				}
				int used_block = CEILING(current.size, BLOCKSIZE);
				int i;
				for (i = 0; i < NUM_DIRECT; i++) {
					occupied_blocks[current.direct[i]] = 1;
					used_block--;
					if (used_block == 0) break;
				}
				if (used_block > 0) {
					void *indir = Malloc(SECTORSIZE);
					ReadSector_w(current.indirect, indir);
					i = 0;
					while (used_block > 0) {
						occupied_blocks[((int *)indir)[i++]] = 1;
						used_block--;
					}
					free(indir);
				}
			}
		}
	}
	for (which_block = 1; which_block <= total_block; which_block++) {
		if (!occupied_blocks[which_block]) add_free_block(which_block);
	}
	free(occupied_blocks);
	msg_buf = Malloc(MESSAGESIZE);
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
}

int main()
{
	int pid = Fork();
	if (pid == 0) {
		//Exec
		while(1){}
	}
	init();
	int received_pid;
	while (1) {
		received_pid = Receive_w(msg_buf);
	}
}
