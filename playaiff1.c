/*
 * Audio programming exercises
 *
 * Exercise 1: AIFF player with headers parsed manually
 *
 * compile with "gcc -o playaiff playaiff.c -lao -ldl -lm"
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ao/ao.h>
#include <math.h>

#define MAXCHAN 8

ao_device *device;
ao_sample_format format;

#define gshort( b) (((int)((b)[0]) << 8) + (int)((b)[1]))
#define glong( b) (((int)((b)[0]) << 24) + ((int)((b)[1]) << 16) +\
        ((int)((b)[2]) << 8) + (int)((b)[3]))

typedef struct {
    short	  channels;
    short	  samplesize;
    double	  samplerate;
    unsigned long samplecount;
    int		  valid;
} aiffinfo;

aiffinfo getaiffinfo(FILE *);
static int IeeeExtendedToLong(unsigned char *);

int main(int argc, char *argv[])
{
    int default_driver;
    char *buffer;

    aiffinfo info;

    FILE *fp;

    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    ao_initialize();
    default_driver = ao_default_driver_id();
    memset(&format, 0, sizeof(format));

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
	printf("Cannot open %s.\n", argv[1]);
	exit(2);
    }

    info = getaiffinfo(fp);

    if (!info.valid) {
	printf("Invalid AIFF file.\n");
	exit(1);
    }

    format.bits = info.samplesize;
    format.channels = info.channels;
    format.rate = info.samplerate;
    format.byte_format = AO_FMT_LITTLE;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        exit(1);
    }

    buffer = malloc(sizeof(char) * info.samplecount);

    fread(buffer, sizeof(char), info.samplecount, fp);
    ao_play(device, buffer, info.samplecount);
    ao_close(device);
    ao_shutdown();

    return 0;
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
	    if (offset) fseek(fp, offset,SEEK_CUR);
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

