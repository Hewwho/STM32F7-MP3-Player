#include "App/Inc/player.h"

#include "Middlewares/Third_Party/FatFs/src/ff.h"
#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_audio.h"
#include "MP3/helix/pub/mp3dec.h"

#define MAX_DECODED_FRAME_SIZE MAX_NGRAN * MAX_NCHAN * MAX_NSAMP
#define OUT_BUFFER_SIZE 2 * MAX_DECODED_FRAME_SIZE
//Buffer from which the audio is directly played, holds two decoded frames at a time
static char outBuffer[OUT_BUFFER_SIZE];

#define READ_BUFFER_SIZE 2 * MAINBUF_SIZE
//Buffer for reading from the given file
static unsigned char readBuffer[READ_BUFFER_SIZE];
//Pointer inside the readBuffer
static uint8_t *readPtr;
//Bytes left (to be decoded) in the readBuffer
static int bytesLeft;

typedef enum {
  BUFFER_OFFSET_NONE = 0,  
  BUFFER_OFFSET_HALF,  
  BUFFER_OFFSET_FULL,     
} BufferOffset;

static uint8_t buf_offs;

static PlayerState playerState = STOPPED;

static FIL file;
static HMP3Decoder decoder;

//Reached the end of outBuffer, time to fill the second half of outBuffer
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
    buf_offs = BUFFER_OFFSET_FULL;
	if(ProcessFrame() != 0)
	{
		TrackFinished();
	}
}

//Reached the first half of outBuffer, time to fill the first half of outBuffer
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{ 
    buf_offs = BUFFER_OFFSET_HALF;
	if(ProcessFrame() != 0)
	{
		TrackFinished();
	}
}

void InitAudio(void) {
	xprintf("INITIALIZING AUDIO CODEC...\n");
	if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE1, 60, AUDIO_FREQUENCY_44K) == 0) {
		xprintf("AUDIO INIT OK\n");
	}
	else {
		xprintf("AUDIO INIT ERROR\n");
		exit(1);
	}
	BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
}

PlayerState GetPlayerState(void) {
	return playerState;
}

void Play(const char *name) {
	xprintf("PLAYING: %s\n", name);
	playerState = PLAYING;
	
	if(f_open(&file, name, FA_READ) != FR_OK) {
		xprintf("COULDN'T OPEN THE FILE\n");
		exit(1);
	}
	
	decoder = MP3InitDecoder();
	readPtr = readBuffer;
	
	//Filling readBuffer for the first time
	if(f_read(&file, readPtr, READ_BUFFER_SIZE, &bytesLeft)) {
        xprintf("ERROR WHILE READING FROM THE FILE\n");
        exit(1);
    }
	
	//Filling both halves of outBuffer at the start
	buf_offs = BUFFER_OFFSET_HALF;
	if(ProcessFrame()) {
		TrackFinished();
	}
	buf_offs = BUFFER_OFFSET_FULL;
	if(ProcessFrame()) {
		TrackFinished();
	}
	buf_offs = BUFFER_OFFSET_NONE;
	
	MP3FrameInfo mp3FrameInfo;
	MP3GetLastFrameInfo(decoder, &mp3FrameInfo);
	//Set appropriate frequency
	if(mp3FrameInfo.samprate/1000 == 8) BSP_AUDIO_OUT_SetFrequency(AUDIO_FREQUENCY_8K);
	else if(mp3FrameInfo.samprate/1000 == 11) BSP_AUDIO_OUT_SetFrequency(AUDIO_FREQUENCY_11K);
	else if(mp3FrameInfo.samprate/1000 == 16) BSP_AUDIO_OUT_SetFrequency(AUDIO_FREQUENCY_16K);
	else if(mp3FrameInfo.samprate/1000 == 22) BSP_AUDIO_OUT_SetFrequency(AUDIO_FREQUENCY_22K);
	else if(mp3FrameInfo.samprate/1000 == 44) BSP_AUDIO_OUT_SetFrequency(AUDIO_FREQUENCY_44K);
	else if(mp3FrameInfo.samprate/1000 == 48) BSP_AUDIO_OUT_SetFrequency(AUDIO_FREQUENCY_48K);
	
	BSP_AUDIO_OUT_Play((uint16_t *) outBuffer, OUT_BUFFER_SIZE);
	BSP_AUDIO_OUT_Resume();
}

void Pause(void) {
	playerState = PAUSED;
	BSP_AUDIO_OUT_Pause();
	xprintf("PAUSED\n");
}

void Resume(void) {
	playerState = PLAYING;
	BSP_AUDIO_OUT_Resume();
	xprintf("RESUMED\n");
}

void Stop(void) {
	playerState = STOPPED;
	Reset();
	xprintf("STOPPED\n");
}

void Reset(void) {
	BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
	MP3FreeDecoder(decoder);
	memset(outBuffer, 0, OUT_BUFFER_SIZE);
	memset(readBuffer, 0, READ_BUFFER_SIZE);
	f_close(&file);
}

int ProcessFrame() {
	//Decoding one frame from readBuffer
	short frame[MAX_DECODED_FRAME_SIZE];
	if (MP3Decode(decoder, (unsigned char **) &readPtr, (int *) &bytesLeft, frame, 0)) {
		xprintf("ERROR WHILE DECODING A FRAME\n");
		return -1;
	}
	
	//Moving what's left (bytesLeft) of readBuffer to its beginning
	memcpy(readBuffer, readPtr, bytesLeft);
	
	//Moving readPtr to the empty part of readBuffer
	readPtr = readBuffer + bytesLeft;
	
	//Filling the empty part of readBuffer (READ_BUFFER_SIZE - bytesLeft) back again
	int bytesRead;
	if (f_read(&file, readPtr, (READ_BUFFER_SIZE - bytesLeft), &bytesRead)) {
        xprintf("ERROR WHILE READING FROM THE FILE\n");
        return -1;
    }

	//If the first half of outBuffer was already played then fill it back again 
	if(buf_offs == BUFFER_OFFSET_HALF) {
		memcpy(outBuffer + 0, frame, MAX_DECODED_FRAME_SIZE);
	}
	//If the second half of outBuffer was already played then fill it back again 
	else if(buf_offs == BUFFER_OFFSET_FULL) {
		memcpy(outBuffer + MAX_DECODED_FRAME_SIZE, frame, MAX_DECODED_FRAME_SIZE);
	}
	
	//If bytesRead isn't equal (is less than) to (READ_BUFFER_SIZE - bytesLeft)
	//then we reached the end of the file and couldn't read more
	if((READ_BUFFER_SIZE - bytesLeft) != bytesRead) {
		return EOF;
	}
	
	//Add the total of bytes read to what was left before
	bytesLeft += bytesRead;
	//Move readPtr back to the beginning of readBuffer
	readPtr = readBuffer;

    return 0;
}