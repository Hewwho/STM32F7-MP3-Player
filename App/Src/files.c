#include "App/Inc/files.h"

#include "Middlewares/Third_Party/FatFs/src/ff.h"

//Used for cheking if a file is .mp3
static const char * GetExtension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

void GetFiles(Files *files) {
	files->total = 0;
	DIR dp;
	
	if(f_opendir(&dp, "1:") != FR_OK) {
		xprintf("ERROR WHILE OPENING THE DIRECTORY\n");
		exit(1);
	}
	
	FILINFO fno;
	while(files->total < MAX_FILES) {
		if(f_readdir(&dp, &fno) != FR_OK) {
			xprintf("ERROR WHILE READING THE DIRECTORY\n");
			exit(1);
		}
		
		xprintf("READ: %s\n", fno.fname);
		
		//Reject some weird files with no names
		if(fno.fname[0] == 0) {
			xprintf("BREAKING\n");
			break;
		}
		
		//If the name isn't too long and it's mp3, add it to the list
		if(strlen(fno.fname) < MAX_NAME && strcmp(GetExtension(fno.fname), "mp3") == 0) {
			strcpy(files->names[files->total],  fno.fname);
			files->total++;
		}
	}
	
	f_closedir(&dp);
}	
	