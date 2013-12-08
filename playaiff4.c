/*
 * Audio programming exercises
 *
 * Exercise 4: Repeatedly filling a smaller buffer instead of sucking in
 * the whole file.  Filepointer is reset to where it was before the call.
 *
 * compile with "gcc -o playaiff4 playaiff4.c -lao -ldl -lm -lsndfile"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ao/ao.h>
#include <sndfile.h>
#include <math.h>


#define BUFFSIZE 4096

int playfile(FILE *);

int main(int argc, char *argv[])
{
    FILE *fp;

    if (argc < 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
	printf("Cannot open %s.\n", argv[1]);
	exit(2);
    }

    playfile(fp);

    return 0;
}

int playfile(FILE *fp)
{
    int default_driver;
    int frames_read;
    int count;
    int toread;
    int readnow;
    int *buffer;
    long filestart;

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
    format.bits = 32;
    format.channels = sf_info.channels;
    format.rate = sf_info.samplerate;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        return 1;
    }

    buffer = malloc(BUFFSIZE * sf_info.channels * sizeof(int));
    frames_read = 0;
    toread = sf_info.frames * sf_info.channels;

    printf("Total frames:     %d\n\n", toread);

    while (toread > 0) {
	if (toread < BUFFSIZE * sf_info.channels)
	    count = toread;
	else
	    count = BUFFSIZE * sf_info.channels;

	printf("Frames attempted: %d\n", count);

        frames_read = sf_read_int(sndfile, buffer, count);
        ao_play(device, (char *)buffer, frames_read * sizeof(int));
	toread = toread - frames_read;

	printf("frames read:      %d\n", frames_read);
	printf("frames remaining: %d\n\n", toread);
    }

    fseek(fp, filestart, SEEK_SET);
    ao_close(device);
    sf_close(sndfile);
    ao_shutdown();
    printf("Finished\n");

    return 0;
}
