/*
 * Audio programming exercises
 *
 * Exercise 8: Resampling.
 * 	Step 1: Convert from short to float and back.
 * 	Step 2: Set up for and invoke src_process() from libsamplerate.
 *
 * compile with
 * "gcc -o playaiff8 playaiff8.c -lao -ldl -lm -lsndfile -lsamplerate"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ao/ao.h>
#include <sndfile.h>
#include <samplerate.h>

#define DEFAULT_CONVERTER SRC_SINC_MEDIUM_QUALITY
#define NEW_RATE 11025

#define BUFFSIZE 4096
#define MAX(x,y) ((x)>(y)) ? (x) : (y)
#define MIN(x,y) ((x)<(y)) ? (x) : (y)

int playfile(FILE *, int);
void floattopcm16(short *, float *, int);
void pcm16tofloat(float *, short *, int);

int main(int argc, char *argv[])
{
    FILE *fp;
    int newrate;

    if (argc < 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
	printf("Cannot open %s.\n", argv[1]);
	exit(1);
    }

    if (argv[2])
	newrate = atoi(argv[2]);
    else
	newrate = NEW_RATE;

    playfile(fp, newrate);
    fclose(fp);

    return 0;
}

int playfile(FILE *fp, int newrate)
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
    format.rate = newrate;

    printf("Start sample rate:  %d\n", sf_info.samplerate);
    printf("Ending sample rate: %d\n", newrate);

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        return 1;
    }

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
    src_data.src_ratio = (1.0 * newrate) / sf_info.samplerate;
    src_data.data_out = floatbuffer2;
    src_data.output_frames = BUFFSIZE / sf_info.channels;
    src_data.output_frames_gen = 0;

    while (1) {
	/* if floatbuffer is empty, refill it */
	if (src_data.input_frames == 0) {
	    src_data.input_frames = sf_read_float(sndfile, floatbuffer, BUFFSIZE / sf_info.channels);
	    src_data.data_in = floatbuffer;

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

	/* write output */
	output_count += src_data.output_frames_gen;
	src_data.data_in += src_data.input_frames_used * sf_info.channels;
	src_data.input_frames -= src_data.input_frames_used;

	floattopcm16(shortbuffer, floatbuffer2, src_data.output_frames_gen);
        ao_play(device, (char *)shortbuffer, src_data.output_frames_gen * sizeof(short));

    }

    src_state = src_delete(src_state);

    free(shortbuffer);
    free(floatbuffer);
    free(floatbuffer2);
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
/*
void pcm16tofloat(float *outbuf, short *inbuf, int length)
{
    int   count;
    const float div = (1.0f/32768.0f);

    for (count = 0; count <= length; count++) {
	outbuf[count] = div * (float) inbuf[count];
    }
}
*/
