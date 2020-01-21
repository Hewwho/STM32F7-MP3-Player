#include "App/Inc/controller.h"

#include<string.h>
#include<stdlib.h>

#include "usb_host.h"

#include "App/Inc/screen.h"
#include "App/Inc/files.h"
#include "App/Inc/player.h"

extern ApplicationTypeDef Appli_state;

static int fileIndex = 0;
static Files files;

//Cut the filename if it's too long (only for display) and add some dots at the end
static char * cutName(char *name) {
	if(strlen(name) > 15) {
		char *tmpName = malloc(sizeof(char) * 19);
		memcpy(tmpName, name, 15);
		memcpy(tmpName + 15, "...\0", 4);
		return tmpName;
	}
	else return name;
}

//Get the full filepath from a filename
static char * getFilePath(void) {
	char *filePath = malloc(strlen("0:/") + strlen(files.names[fileIndex]) + 1);
	strcpy(filePath, "0:/");
	strcat(filePath, files.names[fileIndex]);
	return filePath;
}

//Used for player to signalize when the current track has finished playing
void TrackFinished(void) {
	xprintf("FINISHED\n");
	if(fileIndex == files.total - 1) {
		fileIndex = 0;
		Stop();
	}
	else if(files.total > 0) {
		fileIndex++;
		Reset();
		Play(getFilePath());
	}
}

void ControllerTask(void) {
	do {
		vTaskDelay(250);
	}while(Appli_state != APPLICATION_READY);
	
	xprintf("USB READY\n");

	GetFiles(&files);
	xprintf("MP3 COUNT: %d\n", files.total);
	
	InitAudio();

	LCDStart();
	LCDStart();
	
	for(;;) {
		DrawPlayer(fileIndex, files.total, cutName(files.names[fileIndex]), GetPlayerState());
		
		switch(TouchInput()) {
			case PLAYPAUSE:
				ActivePlayPause();
				
				//If it was stopped (default state), play from the beginning
				if(GetPlayerState() == STOPPED) {
					if(files.total > 0) Play(getFilePath());
				} else if(GetPlayerState() == PAUSED) {
					Resume();
				} else {
					Pause();
				}
				
				vTaskDelay(150);
				break;
			case STOP:
				ActiveStop();
				Stop();
				vTaskDelay(150);
				break;
			case PREVIOUS:
				ActivePrevious();
				
				//Decrease file index, loop around if necessary
				if(fileIndex == 0) fileIndex = files.total - 1;
				else if(files.total > 0) fileIndex--;
				
				if(GetPlayerState() == PAUSED || GetPlayerState() == STOPPED) {
					Stop();
				}
				//If it was already playing, start playing from the beginning
				else {
					Stop();
					if(files.total > 0) Play(getFilePath());
				}
				
				vTaskDelay(150);
				break;
			case NEXT:
				ActiveNext();
				
				//Increase file index, loop around if necessary
				if(fileIndex == files.total - 1) fileIndex = 0;
				else if(files.total > 0) fileIndex++;
				
				if(GetPlayerState() == PAUSED || GetPlayerState() == STOPPED) {
					Stop();
				}
				//If it was already playing, start playing from the beginning
				else {
					Stop();
					if(files.total > 0) Play(getFilePath());
				}
				vTaskDelay(150);
				break;
			default:
				break;
		}	
	}
	
}