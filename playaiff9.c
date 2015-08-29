/*
 * Audio programming exercises
 *
 * Exercise 9: Mixing
 *
 * compile with
 * "gcc -o playaiff9 playaiff9.c -lao -ldl -lm -lsndfile -lsamplerate"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ao/ao.h>
#include <sndfile.h>
#include <samplerate.h>
#include <math.h>

#define DEFAULT_CONVERTER SRC_SINC_MEDIUM_QUALITY
#define NEW_RATE 44100

#define BUFFSIZE 4096
#define MAX(x,y) ((x)>(y)) ? (x) : (y)
#define MIN(x,y) ((x)<(y)) ? (x) : (y)

int playfile(FILE *, int);
int mypower(int, int);
void floattopcm16(short *, float *, int);
void pcm16tofloat(float *, short *, int);

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

    playfile(fp, volume);

    return 0;
}

int playfile(FILE *fp, int vol)
{
    int default_driver;
    int frames_read;
    int count;
    int toread;
    int readnow;
    float *floatbuffer;
    float *floatbuffer2;
    short *shortbuffer;
    long filestart;

    int volcount;

    ao_device *device;
    ao_sample_format format;
    SNDFILE     *sndfile;
    SF_INFO	sf_info;

    SRC_STATE	*src_state;
    SRC_DATA	src_data;
    int		error;
    double	max = 0.0;
    sf_count_t	output_count = 0;

    ao_initialize();
    default_driver = ao_default_driver_id();

    sf_info.format = 0;

    filestart = ftell(fp);

    sndfile = sf_open_fd(fileno(fp), SFM_READ, &sf_info, 0);

    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 16;
    format.channels = sf_info.channels;
    format.rate = NEW_RATE;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        return 1;
    }

    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;

    floatbuffer = malloc(BUFFSIZE * sf_info.channels * sizeof(float));
    floatbuffer2 = malloc(BUFFSIZE * sf_info.channels * sizeof(float));
    shortbuffer = malloc(BUFFSIZE * sf_info.channels * sizeof(short));
    frames_read = 0;
    toread = sf_info.frames * sf_info.channels;

    /* Set up for conversion */
    if ((src_state = src_new(DEFAULT_CONVERTER, sf_info.channels, &error)) == NULL) {
	printf("Error: src_new() failed: %s.\n", src_strerror(error));
	exit(1);
    }
    src_data.end_of_input = 0;
    src_data.input_frames = 0;
    src_data.data_in = floatbuffer;
    src_data.src_ratio = (1.0 * NEW_RATE) / sf_info.samplerate;
    src_data.data_out = floatbuffer2;
    src_data.output_frames = BUFFSIZE / sf_info.channels;

    while (1) {
	/* if floatbuffer is empty, refill it */
	if (src_data.input_frames == 0) {
	    src_data.input_frames = sf_read_float(sndfile, floatbuffer, BUFFSIZE / sf_info.channels);
	    src_data.data_in = floatbuffer;
	    printf("Frames read: %d\n", src_data.input_frames);

	    /* mark end of input */
	    if (src_data.input_frames < BUFFSIZE / sf_info.channels)
		src_data.end_of_input = SF_TRUE;
	}

	if ((error = src_process(src_state, &src_data))) {
	    printf("Error: %s\n", src_strerror(error));
	    exit(1);
	}

	/* terminate if done */
	if (src_data.end_of_input && src_data.output_frames_gen == 0)
	    break;

	/* apply gain */

	/* write output */
	floattopcm16(shortbuffer, floatbuffer2, src_data.output_frames_gen);
	output_count += src_data.output_frames_gen;
	src_data.data_in += src_data.input_frames_used * sf_info.channels;
	src_data.input_frames -= src_data.input_frames_used;

//	for (volcount = 0; volcount <= frames_read; volcount++)
//	    shortbuffer[volcount] /= mypower(2, -vol + 8);

        ao_play(device, (char *)shortbuffer, src_data.output_frames_gen * sizeof(short));


    }

    src_state = src_delete(src_state);

    free(shortbuffer);
    free(floatbuffer);
    fseek(fp, filestart, SEEK_SET);
    ao_close(device);
    sf_close(sndfile);
    ao_shutdown();
    printf("Finished\n");

    return 0;
}


/* Convert back to shorts */
void floattopcm16(short *outbuf, float *inbuf, int length)
{
    int   count;

    const float mul = (32768.0f);
    for (count = 0; count <= length; count++) {
	int32_t tmp = (int32_t)(mul * inbuf[count]);
	tmp = MAX( tmp, -32768 ); // CLIP < 32768
	tmp = MIN( tmp, 32767 );  // CLIP > 32767
	outbuf[count] = tmp;
    }
}


/* Convert the buffer to floats. (before resampling) */
void pcm16tofloat(float *outbuf, short *inbuf, int length)
{
    int   count;

    const float div = (1.0f/32768.0f);
    for (count = 0; count <= length; count++) {
	outbuf[count] = div * (float) inbuf[count];
    }
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

