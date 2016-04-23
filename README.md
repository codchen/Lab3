COMP421
Lab3 - The Yalnix File System
Xiaoyu Chen (xc12) and Jiafang Jiang (jj26)

Important Code Included:
	iolib.c, iolib.h --- YFS file system library 
	yfs.c 			 --- YFS file server process
	utils.h, utils.c --- utility data structures and methods for error checking, path parsing etc.

Data Structure
-----------------------------------------------------------------------------

Struct opened_file {
	short type;
	short inode;
	int pos;
	int reuse;
} OpenedFile;

MAX_OPEN_FILES of opened_file structs function as an open file table in the 
library in user process, keeping track of status and fd position of opened files.

-----------------------

Struct lru_node {
	int dirty;
	int num;
	struct lru_node *prev;
	struct lru_node *next;
	struct lru_node *hash_prev;
	struct lru_node *hash_next;
	void *content;
} LRUNode;

Data structure for the LRU Cache. "dirty" indicates whether the content of an 
inode or a data block has been modified in cache. "num" refers to the inode
number or the block number of this node. An LRUNode contains pointers indicating 
its position in the cache and its position in the linked list of its hashmap entry.

-----------------------

Struct linked_list {
	int val;
	struct linked_list *next;
} LinkedList;

General linked list data structure linking free inodes and free data blocks.

-----------------------------------------------------------------------------

Testing
-----------------------------------------------------------------------------

random_test.c, super_test.c and intense.c are testing programs we used for testing 
the robustness of the file system.
-----------------------------------------------------------------------------
