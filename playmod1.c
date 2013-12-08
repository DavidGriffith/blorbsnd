/*
 * MOD programming exercises
 *
 * Exercise 1: Loading with libmodplug and playing with libao
 *
 * compile with "gcc -o playaiff5 playaiff5.c -lao -ldl -lm -lmodplug"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libmodplug/modplug.h>
#include <ao/ao.h>
#include <math.h>

#define BUFFSIZE 4096

int playmod(FILE *, int);
int mypower(int, int);
char *getfiledata(FILE *, long *);

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

    playmod(fp, volume);

    fclose(fp);
    return 0;
}

int playmod(FILE *fp, int vol)
{
    unsigned char *buffer;
    int modlen;

    int default_driver;
    ao_device *device;
    ao_sample_format format;

    char *filedata;
    long size;
    ModPlugFile *mod;
    ModPlug_Settings settings;

    ao_initialize();
    default_driver = ao_default_driver_id();

    ModPlug_GetSettings(&settings);

    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 16;
    format.channels = 2;
    format.rate = 44100;

    /* Note: All "Basic Settings" must be set before ModPlug_Load. */
    settings.mResamplingMode = MODPLUG_RESAMPLE_FIR; /* RESAMP */
    settings.mChannels = 2;
    settings.mBits = 16;
    settings.mFrequency = 44100;
    settings.mStereoSeparation = 128;
    settings.mMaxMixChannels = 256;

    /* insert more setting changes here */
    ModPlug_SetSettings(&settings);

    /* remember to free() filedata later */
    filedata = getfiledata(fp, &size);

    mod = ModPlug_Load(filedata, size);
    if (!mod) {
	printf("Unable to load module\n");
	free(filedata);
	return 1;
    }

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        return 1;
    }

    buffer = malloc(BUFFSIZE * sizeof(char));
    modlen = 1;
    while (modlen != 0) {
	if (modlen == 0) break;
	modlen = ModPlug_Read(mod, buffer, BUFFSIZE);
	if (modlen > 0 && ao_play(device, buffer, modlen) == 0) {
	    perror("audio write");
	    exit(1);
	}
    }
    free(buffer);
    ao_close(device);
    ao_shutdown();
    printf("Finished\n");

    return 0;
}

/*
 * libmodplug requires the whole file to be pulled into memory.
 * This function does that and then closes the file.
 */
char *getfiledata(FILE *fp, long *size)
{
    char *data;

    fseek(fp, 0L, SEEK_END);
    (*size) = ftell(fp);
    rewind(fp);
    data = (char*)malloc(*size);
    fread(data, *size, sizeof(char), fp);
    rewind(fp);

    return(data);
}
