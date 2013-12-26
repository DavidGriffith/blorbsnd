/*
 * Audio programming exercises
 *
 * Exercise 8: Resampling.
 * 	Step 1: Convert from short to float and back.
 * 	Step 2: Set up for and invoke src_process() from libsamplerate.
 *
 * compile with
 * "gcc -o playaiff5 playaiff5.c -lao -ldl -lm -lsndfile -lsamplerate"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ao/ao.h>
#include <sndfile.h>
#include <samplerate.h>
#include <math.h>

#define BUFFSIZE 4096
#define MAX(x,y) ((x)>(y)) ? (x) : (y)
#define MIN(x,y) ((x)<(y)) ? (x) : (y)

int playfile(FILE *, int);
int mypower(int, int);
void floattopcm16(short *, float *, int);
void pcm16tofloat(float *, short *, int);

int main(int argc, char *argv[])
{
    FILE *fp;
    int volume = 8;

    if (argc < 2) {
	printf("usage: %s <filename> <volume>\n", argv[0]);
	exit(1);
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
	printf("Cannot open %s.\n", argv[1]);
	exit(2);
    }

    if (argv[2])
	volume = atoi(argv[2]);

    playfile(fp, volume);

    return 0;
}

int playfile(FILE *fp, int vol)
{
    int default_driver;
    int frames_read;
    int count;
    int toread;
    int readnow;
    short *shortbuffer;
    float *floatbuffer;
    short *newbuffer;
    long filestart;

    int volcount;

    ao_device *device;
    ao_sample_format format;

    SNDFILE     *sndfile;
    SF_INFO     sf_info;


    ao_initialize();
    default_driver = ao_default_driver_id();

    sf_info.format = 0;

    filestart = ftell(fp);

    sndfile = sf_open_fd(fileno(fp), SFM_READ, &sf_info, 0);

    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 16;
    format.channels = sf_info.channels;
    format.rate = sf_info.samplerate;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        return 1;
    }

    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;

    shortbuffer = malloc(BUFFSIZE * sf_info.channels * sizeof(short));
    newbuffer = malloc(BUFFSIZE * sf_info.channels * sizeof(short));
    floatbuffer = malloc(BUFFSIZE * sf_info.channels * sizeof(float));
    frames_read = 0;
    toread = sf_info.frames * sf_info.channels;

    while (toread > 0) {
	if (toread < BUFFSIZE * sf_info.channels)
	    count = toread;
	else
	    count = BUFFSIZE * sf_info.channels;

        frames_read = sf_read_short(sndfile, shortbuffer, count);

printf("Frames to go:    %zu\n", toread);
printf("Frames read:     %zu\n\n", frames_read);

	pcm16tofloat(floatbuffer, shortbuffer, frames_read);
	floattopcm16(newbuffer, floatbuffer, frames_read);

	for (volcount = 0; volcount <= frames_read; volcount++)
	    shortbuffer[volcount] /= mypower(2, -vol + 8);

        ao_play(device, (char *)newbuffer, frames_read * sizeof(short));
	toread = toread - frames_read;
    }

    free(shortbuffer);
    free(floatbuffer);
    fseek(fp, filestart, SEEK_SET);
    ao_close(device);
    sf_close(sndfile);
    ao_shutdown();
    printf("Finished\n");

    return 0;
}


/* Convert back to shorts */
void floattopcm16(short *outbuf, float *inbuf, int length)
{
    int   count;

    const float mul = (32768.0f);
    for (count = 0; count <= length; count++) {
	int32_t tmp = (int32_t)(mul * inbuf[count]);
	tmp = MAX( tmp, -32768 ); // CLIP < 32768
	tmp = MIN( tmp, 32767 );  // CLIP > 32767
	outbuf[count] = tmp;
    }
}


/* Convert the buffer to floats. (before resampling) */
void pcm16tofloat(float *outbuf, short *inbuf, int length)
{
    int   count;

    const float div = (1.0f/32768.0f);
    for (count = 0; count <= length; count++) {
	outbuf[count] = div * (float) inbuf[count];
    }
}

int mypower(int base, int exp) {
    if (exp == 0)
	return 1;
    else if (exp % 2)
	return base * mypower(base, exp - 1);
    else {
 	int temp = mypower(base, exp / 2);
	return temp * temp;
    }
}

