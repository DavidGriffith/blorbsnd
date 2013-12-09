/*
 * OGG programming exercises
 *
 * Exercise 1: Loading OGG and playing stream without the use of libsndfile.
 *
 * compile with "gcc -o playogg playogg.c -lao -ldl -lm -lvorbis -logg"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ao/ao.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

//#define BUFFSIZE 4096
#define BUFFSIZE 8192

static int playogg(FILE *, int);
static int mypower(int, int);

int main(int argc, char *argv[])
{
    FILE *fp;
    int volume = 8;

    if (argc < 2) {
	printf("usage: %s <filename> <volume>\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
	printf("Cannot open %s.\n", argv[1]);
	exit(EXIT_FAILURE);
    }

    if (argv[2])
	volume = atoi(argv[2]);

    playogg(fp, volume);

    fclose(fp);
    return 0;
}

static int playogg(FILE *fp, int vol)
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

    ao_initialize();
    default_driver = ao_default_driver_id();

    memset(&format, 0, sizeof(ao_sample_format));

    if (ov_open_callbacks(fp, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0) {
	printf("Oops.\n");
	exit(1);
    }

    info = ov_info(&vf, -1);

    format.byte_format = AO_FMT_LITTLE;
    format.bits = 16;
    format.channels = info->channels;
    format.rate = (int)info->rate;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
	ov_clear(&vf);
        return 1;
    }

    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;

    buffer = malloc(BUFFSIZE * format.channels * sizeof(int16_t));

    toread = ov_pcm_total(&vf, -1) * 2 * format.channels;
    frames_read = 0;
    count = 0;

    while (count < toread) {
	frames_read = ov_read(&vf, (char *)buffer, BUFFSIZE, 0,2,1,&current_section);
	for (volcount = 0; volcount <= frames_read / 2; volcount++)
	    ((int16_t *) buffer)[volcount] /= mypower(2, -vol + 8);
	ao_play(device, (char *)buffer, frames_read);
	count += frames_read;
    }

    ao_close(device);
    ao_shutdown();
    ov_clear(&vf);
    free(buffer);
    printf("Finished\n");

    return 0;
}


static int mypower(int base, int exp)
{
    if (exp == 0)
	return 1;
    else if (exp % 2)
	return base * mypower(base, exp - 1);
    else {
 	int temp = mypower(base, exp / 2);
	return temp * temp;
    }
}
