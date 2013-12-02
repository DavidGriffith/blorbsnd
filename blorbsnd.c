/*
 * Quick test of loading sounds from a Blorb file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ao/ao.h>
#include <math.h>
#include "blorb.h"
#include "blorblow.h"

void usage(void);
void audioinit(void);
void playaiff(bb_result_t);

ao_device *device;
ao_sample_format format;
FILE *blorbFile;

int main(int argc, char *argv[])
{
    int number;
    bb_map_t *blorbMap;
    bb_result_t resource;
    bb_aux_sound_t *info;

    if (argc != 3)
	usage();
    if (sscanf(argv[2],"%d",&number) != 1)
	usage();

    blorbFile = fopen(argv[1],"rb");
    if (blorbFile == NULL)
	exit(1);
    if (bb_create_map(blorbFile,&blorbMap) != bb_err_None)
	exit(1);

    if (bb_load_resource_snd(blorbMap,bb_method_FilePos,&resource,number,&info) == bb_err_None) {
	printf("Sound resource %d found.\n",number);
	if (blorbMap->chunks[resource.chunknum].type == bb_make_id('F','O','R','M')) {
	    printf("Sound is an AIFF sample.\n");
	    printf("Size: %zu bytes.\n", resource.length);
	    playaiff(resource);
	} else if (blorbMap->chunks[resource.chunknum].type == bb_make_id('M','O','D',' ')) {
	    printf("Sound is a MOD song.\n");
	} else if (blorbMap->chunks[resource.chunknum].type == bb_make_id('O','G','G',' ')) {
	    printf("Sound is an OGG compressed sample.\n");
	}
    } else
	printf("Sound resource %d not found.\n",number);


    return 0;
}

void playaiff(bb_result_t result)
{
    char *buffer;
    uint32 count;

    printf("Attempting to play %zu bytes.\n", result.length);

    count = 0;
    buffer = malloc(sizeof(char) * result.length);
    while (count < result.length) {
	count++;
	fread(buffer, sizeof(char),
		result.length, result.data.startpos);
    }

    ao_play(device, buffer, result.length);

}



void usage(void)
{
    printf("blorbsnd: <blorbfile> <resource_number>\n");
    exit(1);
}

void audioinit(void)
{
    int default_driver;

    ao_initialize();
    default_driver = ao_default_driver_id();
    memset(&format, 0, sizeof(format));

    format.bits = 16;
    format.channels = 2;
    format.rate = 44100;
    format.byte_format = AO_FMT_LITTLE;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
	fprintf(stderr, "Error opening device.\n");
	exit(1);
    }
    return;
}

