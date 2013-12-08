/*
 * Quick test of loading sounds from a Blorb file
 *
 * Exercise 4: Figure out why OGG files won't play.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ao/ao.h>
#include <sndfile.h>
#include "blorb.h"
#include "blorblow.h"

#define BUFFSIZE 4096

void usage(void);
int playsample(FILE *, bb_result_t, int);
int mypower(int, int);

ao_device *device;
ao_sample_format format;

int main(int argc, char *argv[])
{
    FILE *blorbFile;
    int number;
    int volume = 8;
    bb_map_t *blorbMap;
    bb_result_t resource;
    bb_aux_sound_t *info;

    if (argv[2])
	number = atoi(argv[2]);
    else
	usage();

    if (argv[3])
	volume = atoi(argv[3]);
    else
	volume = 8;

    blorbFile = fopen(argv[1], "rb");
    if (blorbFile == NULL)
	exit(1);
    if (bb_create_map(blorbFile, &blorbMap) != bb_err_None)
	exit(1);

    if (bb_load_resource(blorbMap, bb_method_FilePos, &resource, bb_ID_Snd, number) == bb_err_None) {
	printf("Sound resource %d is ",number);
	if (blorbMap->chunks[resource.chunknum].type == bb_make_id('F','O','R','M')) {
	    printf("an AIFF sample.\n");
	    printf("Size: %zu bytes.\n", resource.length);
	    playfile(blorbFile, resource, volume);
	} else if (blorbMap->chunks[resource.chunknum].type == bb_make_id('M','O','D',' ')) {
	    printf("a MOD file.\n");
	} else if (blorbMap->chunks[resource.chunknum].type == bb_make_id('O','G','G','V')) {
	    printf("an OGG compressed sample.\n");
	    printf("Size: %zu bytes.\n", resource.length);
	    playfile(blorbFile, resource, volume);
	} else
	    printf("something else.  This should not happen.\n");
    } else
	printf("Sound resource %d not found.\n",number);

    return 0;
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

void usage(void)
{
    printf("usage: blorbsnd <blorbfile> <resource_number>\n");
    exit(1);
}



int playfile(FILE *fp, bb_result_t result, int vol)
{
    int default_driver;
    int frames_read;
    int count;
    int toread;
    int readnow;
    int *buffer;
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

    lseek(fileno(fp), result.data.startpos, SEEK_SET);
    sndfile = sf_open_fd(fileno(fp), SFM_READ, &sf_info, 0);

    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 32;
    format.channels = sf_info.channels;
    format.rate = sf_info.samplerate;

    printf("channels: %d\n", format.channels);
    printf("rate:     %d\n", format.rate);

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        return 1;
    }

    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;

    buffer = malloc(BUFFSIZE * sf_info.channels * sizeof(int));
    frames_read = 0;
    toread = sf_info.frames * sf_info.channels;

    while (toread > 0) {
	if (toread < BUFFSIZE * sf_info.channels)
	    count = toread;
	else
	    count = BUFFSIZE * sf_info.channels;

        frames_read = sf_read_int(sndfile, buffer, count);

	for (volcount = 0; volcount <= frames_read; volcount++)
	    buffer[volcount] /= mypower(2, -vol + 8);

        ao_play(device, (char *)buffer, frames_read * sizeof(int));
	toread = toread - frames_read;
    }

    free(buffer);
    fseek(fp, filestart, SEEK_SET);
    ao_close(device);
    sf_close(sndfile);
    ao_shutdown();
    printf("Finished\n");

    return 0;
}
