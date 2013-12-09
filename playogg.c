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

    playogg(fp, volume);

    fclose(fp);
    return 0;
}

int playogg(FILE *fp, int vol)
{
    ogg_int64_t toread;
    ogg_int64_t frames_read;
    ogg_int64_t count;
    void *buffer;
    int volcount;

    vorbis_info *info;

    OggVorbis_File vf;
    int eof=0;
    int current_section;

    int default_driver;
    ao_device *device;
    ao_sample_format format;

    ao_initialize();
    default_driver = ao_default_driver_id();

    memset(&format, 0, sizeof(ao_sample_format));

    if(ov_open(fp, &vf, NULL, 0) < 0) {
	fprintf(stderr,"Input does not appear to be an Ogg bitstream.\n");
	exit(1);
    }

    info = ov_info(&vf, -1);

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 16;
    format.channels = info->channels;
    format.rate = info->rate;

    printf("Sample Rate:      %d\n", info->rate);
    printf("Sample Channels:  %d\n", info->channels);

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        return 1;
    }

    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;

    buffer = malloc(BUFFSIZE * sizeof(int16_t));

    toread = ov_pcm_total(&vf, -1);
    frames_read = 0;

    printf("Total frames:     %zu\n", toread);

    while (toread > 0) {
	if (toread < BUFFSIZE * sizeof(int16_t))
	    count = toread;
	else
	    count = BUFFSIZE;

	frames_read = ov_read(&vf, (char *)buffer, count, 0, 2, 1, 
&current_section);

	for (volcount = 0; volcount <= frames_read / 2; volcount++)
	    ((int16_t *) buffer)[volcount] /= mypower(2, -vol + 8);

	ao_play(device, (char *)buffer, frames_read);
	toread -= frames_read;
    }
    free(buffer);
    ov_clear(&vf);
    ao_close(device);
    ao_shutdown();
    printf("Finished\n");

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
