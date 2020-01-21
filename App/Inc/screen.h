#ifndef screen_h
#define screen_h

#include <stdbool.h>
#include "App/Inc/player.h"

typedef enum TouchState { PLAYPAUSE, STOP, PREVIOUS, NEXT, NO } TouchState;

void ActivePlayPause(void);

void ActiveStop(void);

void ActivePrevious(void);

void ActiveNext(void);

void DrawPlayer(int flacIndex, int total, char *name, PlayerState playerState);

TouchState TouchInput(void);

void LCDStart(void);

#endif