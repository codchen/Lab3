#include <stdio.h>
#include <stdlib.h>

#define ERROR_NO_DIR -1
#define ERROR_FREE_SUBDIR -2
#define ERROR_NOT_DIR -3
#define ERROR_MAX_SYM -4
#define ERROR_UNKNOWN_INODE -5
#define ERROR_DIR_EXIST -6
#define ERROR_NO_FREE_INODE -7
#define ERROR_WRITE_SYMLINK -8
#define ERROR_FILE_NOT_EXIST -9
#define ERROR_LINK_TO_DIR -10
#define ERROR_CANNOT_CREATE -11
#define ERROR_NOT_EMPTY -12
#define ERROR_ENTRY_EXIST -13
#define ERROR_NAME_TOO_LONG -14

#define MSG_ERROR_PATH -21
#define MSG_ERROR_NOEXIST -22
#define MSG_ERROR_BUF -23

typedef struct linked_list {
	int val;
	struct linked_list *next;
} LinkedList;

typedef struct lru_node {
	int dirty;
	int num;
	struct lru_node *prev;
	struct lru_node *next;
	struct lru_node *hash_prev;
	struct lru_node *hash_next;
	void *content;
} LRUNode;

extern void *Malloc(size_t size);
extern void *Calloc(size_t nitems, size_t size);
extern void Register_w(unsigned int service_id);
extern void ReadSector_w(int sectornum, void *buf);
extern void WriteSector_w(int sectornum, void *buf);
extern int Receive_w(void *msg);
extern int streq(char *path, char *name, int len);
extern void paste_name(char *dest, char *sr);
extern char *split_path(char *path);
extern int dot_entry(char *name);