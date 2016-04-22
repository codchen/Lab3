#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
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

extern void WriteSector_w(int sectornum, void *buf) {
	if (WriteSector(sectornum, buf)) {
		fprintf(stderr, "Cannot write sector %d\n", sectornum);
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

extern int streq(char *path, char *name, int len) {
	int i;
	for (i = 0; i < len; i++) {
		if (path[i] != name[i]) return 0;
	}
	for (i = len; i < DIRNAMELEN; i++) {
		if (name[i] != '\0') return 0;
	}
	return 1;
}

extern void paste_name(char *dest, char *sr) {
	int len = strlen(sr);
	int i;
	for (i = 0; i < DIRNAMELEN; i++) {
		if (i < len) dest[i] = sr[i];
		else dest[i] = '\0';
	}
}

extern char *split_path(char *path) {
	int len = strlen(path);
	int i;
	for (i = len - 1; i >= 0; i--) {
		if (path[i] == '/') break;
	}
	char *name = Malloc(len - i);
	memcpy((void *)name, (void *)((long)path + i + 1), len - i - 1);
	path[(i == -1?0:i)] = '\0';
	name[len - i - 1] = '\0';
	if (strlen(name) == 0) {
		free(name);
		name = Malloc(2);
		name[0] = '.';
		name[1] = '\0';
	}
	return name;
}