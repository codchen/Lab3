#include <stdio.h>
#include <stdlib.h>

extern void *Malloc(size_t size);
extern void *Calloc(size_t nitems, size_t size);
extern void Register_w(unsigned int service_id);
extern void ReadSector_w(int sectornum, void *buf);
extern int Receive_w(void *msg);