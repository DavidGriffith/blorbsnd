/*
 * Quick test of loading sounds from a Blorb file
 *
 * Exercise 4: Adding MOD support.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <ao/ao.h>
#include <sndfile.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <libmodplug/modplug.h>
#include "blorb.h"
#include "blorblow.h"

#define BUFFSIZE 4096

void usage(void);
int playaiff(FILE *, bb_result_t, int);
int playmod(FILE *, bb_result_t, int);
int playogg(FILE *, bb_result_t, int);

int mypower(int, int);
char *getfiledata(FILE *, long *);

ao_device *device;
ao_sample_format format;

int main(int argc, char *argv[])
{
    FILE *blorbFile;
    int number;
    int volume = 8;
    bb_map_t *blorbMap;
    bb_result_t resource;

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
	    playaiff(blorbFile, resource, volume);
	} else if (blorbMap->chunks[resource.chunknum].type == bb_make_id('M','O','D',' ')) {
	    printf("a MOD file.\n");
	    printf("Size: %zu bytes.\n", resource.length);
	    playmod(blorbFile, resource, volume);
	} else if (blorbMap->chunks[resource.chunknum].type == bb_make_id('O','G','G','V')) {
	    printf("an OGG compressed sample.\n");
	    printf("Size: %zu bytes.\n", resource.length);
	    playogg(blorbFile, resource, volume);
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


/*
 * This function should be able to play OGG chunks, but for some strange
 * reason, libsndfile refuses to load them.  Libsndfile is capable of
 * loading and playing OGGs when they're naked file.  I don't see what
 * the big problem is.
 *
 */
int playaiff(FILE *fp, bb_result_t result, int vol)
{
    int default_driver;
    int frames_read;
    int count;
    int toread;
    int *buffer;
    long filestart;

    int volcount;
    int volfactor;

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
    volfactor = mypower(2, -vol + 8);

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
	    buffer[volcount] /= volfactor;
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


int playmod(FILE *fp, bb_result_t result, int vol)
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

    long original_offset;

    original_offset = ftell(fp);
    fseek(fp, result.data.startpos, SEEK_SET);

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

    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;

    ModPlug_SetMasterVolume(mod, mypower(2, vol));

    buffer = malloc(BUFFSIZE * sizeof(char));
    modlen = 1;
    while (modlen != 0) {
	if (modlen == 0) break;
	modlen = ModPlug_Read(mod, buffer, BUFFSIZE * sizeof(char));
	if (modlen > 0 && ao_play(device, (char *) buffer, modlen * sizeof(char)) == 0) {
	    perror("audio write");
	    exit(1);
	}
    }
    free(buffer);
    ao_close(device);
    ao_shutdown();

    fseek(fp, original_offset, SEEK_SET);

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
    long offset;

    offset = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    (*size) = ftell(fp);
    fseek(fp, offset, SEEK_SET);
    data = (char*)malloc(*size);
    fread(data, *size, sizeof(char), fp);
    fseek(fp, offset, SEEK_SET);

    return(data);
}



int playogg(FILE *fp, bb_result_t result, int vol)
{
    ogg_int64_t toread;
    ogg_int64_t frames_read;
    ogg_int64_t count;

    vorbis_info *info;

    OggVorbis_File vf;
    int current_section;
    void *buffer;

    int default_driver;
    ao_device *device;
    ao_sample_format format;

    int volcount;
    int volfactor;

    ao_initialize();
    default_driver = ao_default_driver_id();

    fseek(fp, result.data.startpos, SEEK_SET);

    memset(&format, 0, sizeof(ao_sample_format));

    if (ov_open_callbacks(fp, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0) {
	printf("Oops.\n");
	exit(1);
    }

    info = ov_info(&vf, -1);

    format.byte_format = AO_FMT_LITTLE;
    format.bits = 16;
    format.channels = info->channels;
    format.rate = info->rate;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
	ov_clear(&vf);
        return 1;
    }

    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;
    volfactor = mypower(2, -vol + 8);

    buffer = malloc(BUFFSIZE * format.channels * sizeof(int16_t));

    frames_read = 0;
    toread = ov_pcm_total(&vf, -1) * 2 * format.channels;
    count = 0;

    while (count < toread) {
	frames_read = ov_read(&vf, (char *)buffer, BUFFSIZE, 0,2,1,&current_section);
	for (volcount = 0; volcount <= frames_read / 2; volcount++)
	    ((int16_t *) buffer)[volcount] /= volfactor;
	ao_play(device, (char *)buffer, frames_read * sizeof(char));
	count += frames_read;
    }

    ao_close(device);
    ao_shutdown();
    ov_clear(&vf);

    free(buffer);
    printf("Finished\n");

    return 0;
}
