#include "App/Inc/screen.h"

#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_lcd.h"
#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_ts.h"
#include "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.h"
#include "Utilities/Fonts/fonts.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "App/Inc/player.h"

#define LCD_X_SIZE      RK043FN48H_WIDTH
#define LCD_Y_SIZE      RK043FN48H_HEIGHT
static uint32_t lcd_image_fg[LCD_Y_SIZE][LCD_X_SIZE] __attribute__((section(".sdram"))) __attribute__((unused));
static uint32_t lcd_image_bg[LCD_Y_SIZE][LCD_X_SIZE] __attribute__((section(".sdram"))) __attribute__((unused));

static PlayerState lastPlayerState = STOPPED;

static char *lastName;

//Initializing touchscreen
static int InitTS(void) {
	uint8_t  status = 0;
	status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	if(status != TS_OK) return -1;
	return 0;
}

void ActivePlayPause(void) {
	//If player was stopped/paused draw white play icon
	if(lastPlayerState == PAUSED || lastPlayerState == STOPPED) {
		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		
		Point *points = malloc(3 * sizeof(Point));

		points[0] = (Point) {
		.X = 0.5*LCD_X_SIZE - (34*sqrt(3)/2)/2,
		.Y = 0.25*LCD_Y_SIZE - 15
		};
		points[1] = (Point) {
		.X = 0.5*LCD_X_SIZE - (34*sqrt(3)/2)/2,
		.Y = 0.25*LCD_Y_SIZE + 15
		};
		points[2] = (Point) {
		.X = 0.5*LCD_X_SIZE + (30*sqrt(3)/2)/2,
		.Y = 0.25*LCD_Y_SIZE	
		};
		BSP_LCD_FillPolygon(points, 3);
		
		free(points);
	}
	//Else draw white pause icon
	else {
		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		
		BSP_LCD_FillRect(0.5*LCD_X_SIZE - 15 + 2, 0.25*LCD_Y_SIZE - 20 + 2, 6, 36);
		BSP_LCD_FillRect(0.5*LCD_X_SIZE + 5 + 2, 0.25*LCD_Y_SIZE - 20 + 2, 6, 36);
	}
}

void ActiveStop(void) {
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	
	BSP_LCD_FillRect(0.5*LCD_X_SIZE - 17, 0.75*LCD_Y_SIZE - 17, 34, 34);
}

void ActivePrevious(void) {
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	Point *points = malloc(3 * sizeof(Point));
	
	BSP_LCD_FillRect(0.125*LCD_X_SIZE - (40*sqrt(3)/4 + 8)/2 + 2, 0.5*LCD_Y_SIZE - 20 + 2, 4, 36);
	points[0] = (Point) {
	.X = 0.125*LCD_X_SIZE + (32*sqrt(3)/4 + 8)/2,
	.Y = 0.5*LCD_Y_SIZE + 15
	};
	points[1] = (Point) {
	.X = 0.125*LCD_X_SIZE + (32*sqrt(3)/4 + 8)/2,
	.Y = 0.5*LCD_Y_SIZE - 15
	};
	points[2] = (Point) {
	.X = 0.125*LCD_X_SIZE - (26*sqrt(3)/4 - 8)/2,
	.Y = 0.5*LCD_Y_SIZE	
	};
	BSP_LCD_FillPolygon(points, 3);
	
	free(points);
}

void ActiveNext(void) {
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	Point *points = malloc(3 * sizeof(Point));
	
	BSP_LCD_FillRect(0.875*LCD_X_SIZE + (40*sqrt(3)/4 + 8)/2 - 8 + 1 + 2, 0.5*LCD_Y_SIZE - 20 + 2, 4, 36);
	points[0] = (Point) {
	.X = 0.875*LCD_X_SIZE - (32*sqrt(3)/4 + 8)/2,
	.Y = 0.5*LCD_Y_SIZE + 15
	};
	points[1] = (Point) {
	.X = 0.875*LCD_X_SIZE - (32*sqrt(3)/4 + 8)/2,
	.Y = 0.5*LCD_Y_SIZE - 15
	};
	points[2] = (Point) {
	.X = 0.875*LCD_X_SIZE + (26*sqrt(3)/4 - 8)/2,
	.Y = 0.5*LCD_Y_SIZE	
	};
	BSP_LCD_FillPolygon(points, 3);
	
	free(points);
}

void DrawPlayer(int flacIndex, int total, char *name, PlayerState playerState) {
	
	/* INFO */
	BSP_LCD_SetFont(&Font16);
	
	//If the file(name) has changed hide the previous one
	if(strcmp(name, lastName)) {
		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		BSP_LCD_FillRect(0.27*LCD_X_SIZE, 0.03*LCD_Y_SIZE, 0.47*LCD_X_SIZE, 0.05*LCD_Y_SIZE);
		lastName = name;
	}
	
	//FileName
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_DisplayStringAt(0.27*LCD_X_SIZE, 0.03*LCD_Y_SIZE, (uint8_t *) name, LEFT_MODE);
	
	//Index / Total files
	char *index = malloc(10*sizeof(char));
	sprintf(index, "%d/%d", flacIndex + 1, total);
	BSP_LCD_DisplayStringAt(0.27*LCD_X_SIZE, 0.09*LCD_Y_SIZE, (uint8_t *) index, LEFT_MODE);
	free(index);

	/* SEPARATORS */

	BSP_LCD_FillRect(0.25*LCD_X_SIZE - 2, 0, 4, LCD_Y_SIZE);
	BSP_LCD_FillRect(0.75*LCD_X_SIZE - 2, 0, 4, LCD_Y_SIZE);
	BSP_LCD_FillRect(0.25*LCD_X_SIZE, 0.5*LCD_Y_SIZE - 2, 0.5*LCD_X_SIZE, 4);
	
	/* ICONS */

	Point *points = malloc(3 * sizeof(Point));

	//If the playerState has changed (from paused/stopped to playing or the other way around)
	//then hide the previous icon
	if(lastPlayerState != playerState) {
		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		BSP_LCD_FillRect(0.5*LCD_X_SIZE - 40, 0.25*LCD_Y_SIZE - 40, 80, 80);
		BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
		lastPlayerState = playerState;
	}

	//Play/Pause icon
	if(playerState == PAUSED || playerState == STOPPED) {
		points[0] = (Point) {
		.X = 0.5*LCD_X_SIZE - (40*sqrt(3)/2)/2,
		.Y = 0.25*LCD_Y_SIZE - 20
		};
		points[1] = (Point) {
		.X = 0.5*LCD_X_SIZE - (40*sqrt(3)/2)/2,
		.Y = 0.25*LCD_Y_SIZE + 20
		};
		points[2] = (Point) {
		.X = 0.5*LCD_X_SIZE + (40*sqrt(3)/2)/2,
		.Y = 0.25*LCD_Y_SIZE	
		};
		BSP_LCD_FillPolygon(points, 3);
	} else {
		BSP_LCD_FillRect(0.5*LCD_X_SIZE - 15, 0.25*LCD_Y_SIZE - 20, 10, 40);
		BSP_LCD_FillRect(0.5*LCD_X_SIZE + 5, 0.25*LCD_Y_SIZE - 20, 10, 40);
	}
	
	//Stop icon
	BSP_LCD_FillRect(0.5*LCD_X_SIZE - 20, 0.75*LCD_Y_SIZE - 20, 40, 40);

	//Previous icon
	BSP_LCD_FillRect(0.125*LCD_X_SIZE - (40*sqrt(3)/4 + 8)/2, 0.5*LCD_Y_SIZE - 20, 8, 40);
	points[0] = (Point) {
	.X = 0.125*LCD_X_SIZE + (40*sqrt(3)/4 + 8)/2,
	.Y = 0.5*LCD_Y_SIZE + 20
	};
	points[1] = (Point) {
	.X = 0.125*LCD_X_SIZE + (40*sqrt(3)/4 + 8)/2,
	.Y = 0.5*LCD_Y_SIZE - 20
	};
	points[2] = (Point) {
	.X = 0.125*LCD_X_SIZE - (40*sqrt(3)/4 - 8)/2,
	.Y = 0.5*LCD_Y_SIZE	
	};
	BSP_LCD_FillPolygon(points, 3);

	//Next icon
	BSP_LCD_FillRect(0.875*LCD_X_SIZE + (40*sqrt(3)/4 + 8)/2 - 8 + 1, 0.5*LCD_Y_SIZE - 20, 8, 40);
	points[0] = (Point) {
	.X = 0.875*LCD_X_SIZE - (40*sqrt(3)/4 + 8)/2,
	.Y = 0.5*LCD_Y_SIZE + 20
	};
	points[1] = (Point) {
	.X = 0.875*LCD_X_SIZE - (40*sqrt(3)/4 + 8)/2,
	.Y = 0.5*LCD_Y_SIZE - 20
	};
	points[2] = (Point) {
	.X = 0.875*LCD_X_SIZE + (40*sqrt(3)/4 - 8)/2,
	.Y = 0.5*LCD_Y_SIZE	
	};
	BSP_LCD_FillPolygon(points, 3);

	free(points);
}

//Checks for touch input
Touched TouchInput(void) {
    TS_StateTypeDef TS_State;
    BSP_TS_GetState(&TS_State);
	
	if(TS_State.touchDetected > 0) {
		uint16_t XPos = TS_State.touchX[0];
		uint16_t YPos = TS_State.touchY[0];
		/* PlayPause */
		if(XPos >= 0.25*LCD_X_SIZE && 0.75*LCD_X_SIZE >= XPos && 0.5*LCD_Y_SIZE >= YPos) {
			return PLAYPAUSE;
		}
		/* Stop */
		else if(XPos >= 0.25*LCD_X_SIZE && 0.75*LCD_X_SIZE >= XPos && YPos > 0.5*LCD_Y_SIZE ) {
			return STOP;
		}
		/* Previous */
		else if(0.25*LCD_X_SIZE > XPos) {
			return PREVIOUS;
		}
		/* Next */
		else {
			return NEXT;
		}
	}
	
	return NO;
}


//Stolen from the initial project
void LCDStart(void) {
	BSP_LCD_Init();

	BSP_LCD_LayerDefaultInit(0, (unsigned int)lcd_image_bg);
	BSP_LCD_LayerDefaultInit(1, (unsigned int)lcd_image_fg);

	/* Enable the LCD */ 
	BSP_LCD_DisplayOn(); 

	/* Select the LCD Background Layer  */
	BSP_LCD_SelectLayer(0);
	BSP_LCD_Clear(LCD_COLOR_WHITE);

	BSP_LCD_SetColorKeying(1,LCD_COLOR_WHITE);

	/* Select the LCD Foreground Layer  */
	BSP_LCD_SelectLayer(1);

	/* Clear the Foreground Layer */ 
	BSP_LCD_Clear(LCD_COLOR_WHITE);

	/* Configure the transparency for foreground and background :
	Increase the transparency */
	BSP_LCD_SetTransparency(0, 255);
	BSP_LCD_SetTransparency(1, 255);

	/* Init touchscreen */
	if(InitTS()) {
		exit(1);
	}
	
	BSP_LCD_SelectLayer(0);
	/*BSP_LCD_Clear(LCD_COLOR_BLACK);
	vTaskDelay(100);
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	vTaskDelay(1000);
	BSP_LCD_Clear(LCD_COLOR_BLACK);
	vTaskDelay(1000);
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	*/
}
