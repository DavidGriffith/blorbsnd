/*
 * Audio programming exercises
 *
 * Exercise 3: Passing a filepointer to a function that plays the file.
 *
 * compile with "gcc -o playaiff3 playaiff3.c -lao -ldl -lm -lsndfile"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ao/ao.h>
#include <sndfile.h>
#include <math.h>

ao_device *device;
ao_sample_format format;

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

    fclose(fp);
}

int playfile(FILE *fp)
{
    int default_driver;
    int frames_read;
    int *buffer;

    SNDFILE     *sndfile;
    SF_INFO     sf_info;

    ao_initialize();
    default_driver = ao_default_driver_id();

    sf_info.format = 0;
    sndfile = sf_open_fd(fileno(fp), SFM_READ, &sf_info, 0);

    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 32;
    format.channels = sf_info.channels;
    format.rate = sf_info.samplerate;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        exit(1);
    }

    buffer = malloc(sizeof(int) * sf_info.frames * sf_info.channels);
    frames_read = sf_read_int(sndfile, buffer, sf_info.frames * sizeof(int));

    fseek(fp, 0, SEEK_SET);

    ao_play(device, (char *)buffer, sf_info.frames * sizeof(int));
    ao_close(device);
    ao_shutdown();
    free(buffer);

    sf_close(sndfile);

    return 0;
}
