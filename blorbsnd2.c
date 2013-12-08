/*
 * Quick test of loading sounds from a Blorb file
 *
 * Exercise 2: Parsing AIFF headers using libsndfile.
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

void usage(void);
void playaiff(FILE *, bb_result_t);

ao_device *device;
ao_sample_format format;

int main(int argc, char *argv[])
{
    FILE *blorbFile;
    int number;
    bb_map_t *blorbMap;
    bb_result_t resource;
    bb_aux_sound_t *info;

    if (argc != 3)
	usage();
    if (sscanf(argv[2],"%d",&number) != 1)
	usage();

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
	    playaiff(blorbFile, resource);
	} else if (blorbMap->chunks[resource.chunknum].type == bb_make_id('M','O','D',' ')) {
	    printf("a MOD song.\n");
	} else if (blorbMap->chunks[resource.chunknum].type == bb_make_id('O','G','G',' ')) {
	    printf("an OGG compressed sample.\n");
	}
    } else
	printf("Sound resource %d not found.\n",number);

    return 0;
}

void playaiff(FILE *fp, bb_result_t result)
{
    SNDFILE		*sndfile;
    SF_INFO		sf_info;
    int			default_driver;
    char		*buffer;
    int			blorbNo;

    printf("Offset: %zu\n", result.data.startpos);
    printf("Chunknum: %d\n\n", result.chunknum);

    if (fseek(fp, result.data.startpos, SEEK_SET) != 0) {
	printf("Seek failure #1\n");
	exit(1);
    }

    ao_initialize();
    default_driver = ao_default_driver_id();

    sf_info.format = 0;
    sndfile = sf_open_fd(fileno(fp), SFM_READ, &sf_info, 0);

    if (fseek(fp, result.data.startpos, SEEK_SET) != 0) {
	printf("Seek failure #2\n");
	exit(1);
    }

    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 32;
    format.channels = sf_info.channels;
    format.rate = sf_info.samplerate;

    printf("Channels: %d\n", sf_info.channels);
    printf("Frames:   %d\n", sf_info.frames);
    printf("Rate:     %d\n", sf_info.samplerate);

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        exit(1);
    }

    buffer = malloc(sizeof(char) * sf_info.frames * sf_info.channels);

    sf_readf_int(sndfile, (int *) buffer, sf_info.frames);



    printf("Now playing...\n");
    ao_play(device, buffer, sf_info.frames);

}



void usage(void)
{
    printf("usage: blorbsnd <blorbfile> <resource_number>\n");
    exit(1);
}

