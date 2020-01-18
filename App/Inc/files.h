#ifndef files_h
#define files_h

#define MAX_FILES 100
#define MAX_NAME 100

typedef struct {
	int total;
	char names[MAX_FILES][MAX_NAME];
} Files;

void GetFiles(Files *files);

#endif