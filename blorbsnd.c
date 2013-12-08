/*
 * Quick test of loading sounds from a Blorb file
 *
 * Exercise 1: Parsing AIFF headers manually.
 *
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

FILE *blorbFile;

#define MAXCHAN 8
#define gshort( b) (((int)((b)[0]) << 8) + (int)((b)[1]))
#define glong( b) (((int)((b)[0]) << 24) + ((int)((b)[1]) << 16) +\
        ((int)((b)[2]) << 8) + (int)((b)[3]))

typedef struct {
    short		channels;
    short		samplesize;
    int			samplerate;
    int			valid;
    unsigned long	samplecount;
} aiffinfo;

aiffinfo getaiffinfo(FILE *);
static int IeeeExtendedToLong(unsigned char *);

void usage(void)
{
    printf("blorbsnd: <blorbfile> <resource_number>\n");
    exit(1);
}

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
	printf("Sound resource %d is ",number);
	if (blorbMap->chunks[resource.chunknum].type == bb_make_id('F','O','R','M')) {
	    printf("an AIFF sample of %zu bytes.\n", resource.length);
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
    aiffinfo info;
    int default_driver;

    int num;

    ao_device *device;
    ao_sample_format format;

    if (fseek(blorbFile, result.data.startpos, SEEK_SET) != 0) {
        printf("Seek failure #1\n");
        exit(1);
    }

    info = getaiffinfo(blorbFile);

    if (!info.valid) {
        printf("Invalid AIFF file.\n");
        exit(1);
    }

    printf("Chunknum:    %d\n", result.chunknum);
    printf("Channels:    %d\n", info.channels);
    printf("Frames:      %d\n", info.samplecount);
    printf("Rate:        %d\n", info.samplerate);
    printf("Startpos:    %d\n", result.data.startpos);
    printf("Length:      %d\n", result.length);

    ao_initialize();
    default_driver = ao_default_driver_id();
    memset(&format, 0, sizeof(format));

    format.bits = info.samplesize;
    format.channels = info.channels;
    format.rate = info.samplerate;
    format.byte_format = AO_FMT_NATIVE;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        exit(1);
    }

    buffer = malloc(sizeof(char) * info.samplecount);
    fread(buffer, sizeof(char), info.samplecount, blorbFile);
    ao_play(device, buffer, info.samplecount * sizeof(char));

    ao_close(device);
    ao_shutdown();
}



aiffinfo getaiffinfo(FILE *fp)
{
    int size;
    int len;
    int offset;
    int blocksize;
    int found = 0;
    unsigned char chunk[18];
    unsigned char fid[4];
    aiffinfo info;

    info.samplesize = 0;
    info.valid = 0;

    if (fread(chunk, 1, 4, fp) < 4) return info;
    if (memcmp(chunk,"FORM",4)) return info;
    if (fread(chunk, 1, 4, fp) < 4)  return info;
    size = glong(chunk);
    if (size & 1) size++;
    if (size < 20) return info;
    if (fread(chunk, 1, 4, fp) < 4) return info;
    if (memcmp(chunk, "AIFF", 4)) return info;

    size -= 4;
    while (size > 8) {
        if (fread(fid, 1, 4, fp) < 4) return info;    // chunck id
        if (fread(chunk, 1, 4, fp) < 4) return info;    // and len
        size -= 8;
        len = glong(chunk);
        if (len < 0) return info;
        if (len & 1) len++;
        size -= len;
        if (size < 0) return info;
        if (memcmp(fid, "COMM", 4) == 0) {
            if (len != 18) return info;
            if (fread(chunk, 1, 18, fp) < 18) return info;
            info.channels = gshort(chunk);
            if (info.channels < 1) return info;
            if (info.channels > MAXCHAN) return info;
            info.samplecount = glong(chunk+2);
            if (info.samplecount < 1) return info;
            info.samplerate = IeeeExtendedToLong(chunk + 8);
            if (info.samplerate <= 0) return info;
            info.samplesize = gshort(chunk + 6);
            if (info.samplesize < 1 || info.samplesize > 16) return info;
        } else if (memcmp(fid,"SSND",4)==0){
            if (!info.channels) return info;
            if (fread(chunk, 1, 4, fp) < 4) return info;
            offset = glong(chunk);
            if (fread(chunk, 1, 4, fp) < 4) return info;
            blocksize = glong(chunk);
            if (blocksize) return info;
            if (offset) fseek(fp, offset, SEEK_CUR);
            found = 1;
            break;
        } else fseek (fp, len, SEEK_CUR);
    }

    if (!found) return info;
    if (!info.channels) return info;

    info.valid = 1;
    return info;
}




/****************************************************************
 * Extended precision IEEE floating-point conversion routine.
 ****************************************************************/

#ifndef Uint32
#define Uint32 unsigned int
#endif
#ifndef HUGE_INT32
#define HUGE_INT32 0x7fffffff
#endif                                          /* HUGE_VAL */

static int IeeeExtendedToLong( unsigned char *bytes)
{
    int f = 0;
    int expon;
    Uint32 hiMant;
    Uint32 loMant;

    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant = ((Uint32) (bytes[2] & 0xFF) << 24)
        | ((Uint32) (bytes[3] & 0xFF) << 16)
        | ((Uint32) (bytes[4] & 0xFF) << 8)
        | ((Uint32) (bytes[5] & 0xFF));
    loMant = ((Uint32) (bytes[6] & 0xFF) << 24)
        | ((Uint32) (bytes[7] & 0xFF) << 16)
        | ((Uint32) (bytes[8] & 0xFF) << 8)
        | ((Uint32) (bytes[9] & 0xFF));

    if (expon == 0 && hiMant == 0 && loMant == 0) f = 0;
    else if (expon == 0x7FFF) f = HUGE_INT32;
    else {
        expon -= 16382;
        expon = 32-expon;
        if (expon < 0) f = HUGE_INT32;
        else f = hiMant >> expon;
    }

    if (bytes[0] & 0x80)
        return -f;
    else
        return f;
}

