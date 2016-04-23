#include <stdio.h>
#include <stdlib.h>

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