#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include "utils.h"

extern void *Malloc(size_t size) {
	void *res = malloc(size);
	if (res == NULL) {
		fprintf(stderr, "Malloc error\n");
		exit(1);
	}
	return res;
}

extern void *Calloc(size_t nitems, size_t size) {
	void *res = calloc(nitems, size);
	if (res == NULL) {
		fprintf(stderr, "Calloc error\n");
		exit(1);
	}
	return res;
}

extern void Register_w(unsigned int service_id) {
	if (Register(service_id)) {
		fprintf(stderr, "Cannot register YFS service. \
			Maybe another process has already registered for that service\n");
		exit(1);
	}
}

extern void ReadSector_w(int sectornum, void *buf) {
	if (ReadSector(sectornum, buf)) {
		fprintf(stderr, "Cannot read sector %d\n", sectornum);
		exit(1);
	}
}

extern int Receive_w(void *msg) {
	int res = Receive(msg);
	if (res == ERROR) {
		fprintf(stderr, "Got error when receiving message\n");
		exit(1);
	}
	return res;
}