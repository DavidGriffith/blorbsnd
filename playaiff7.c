/* Converting a 1-channel stream to 2-channels
 * compile with "gcc -o playaiff7 playaiff7.c -lao -ldl -lm -lsndfile"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ao/ao.h>
#include <sndfile.h>

#define BUFFSIZE 512

void stereoize(short *, short *, size_t);
int playfile(FILE *);

int main(int argc, char *argv[])
{
    FILE *fp;

    if (argc < 1) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
	printf("Cannot open %s.\n", argv[1]);
	exit(2);
    }

    playfile(fp);
    fclose(fp);

    return 0;
}

int playfile(FILE *fp)
{
    int default_driver;
    int frames_read;
    int count;
    int toread;
    int readnow;
    short *buffer;
    short *groovy;
    long filestart;

    ao_device *device;
    ao_sample_format format;

    SNDFILE     *sndfile;
    SF_INFO     sf_info;


    ao_initialize();
    default_driver = ao_default_driver_id();

    sf_info.format = 0;

    filestart = ftell(fp);

    sndfile = sf_open_fd(fileno(fp), SFM_READ, &sf_info, 0);

    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 16;
    format.rate = sf_info.samplerate;
//    format.channels = sf_info.channels;
    format.channels = 2;

    printf("Channels: %d\n", sf_info.channels);
    printf("Samplerate: %d\n", sf_info.samplerate);

    device = ao_open_live(default_driver, &format, NULL);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        return 1;
    }

    buffer = malloc(BUFFSIZE * sf_info.channels * sizeof(short));
    groovy = malloc(BUFFSIZE * 2 * sizeof(short));

    frames_read = 0;
    toread = sf_info.frames * sf_info.channels;

    while (toread > 0) {
	if (toread < BUFFSIZE * sf_info.channels)
	    count = toread;
	else
	    count = BUFFSIZE * sf_info.channels;

        frames_read = sf_read_short(sndfile, buffer, count);

	if (sf_info.channels == 1)
	    stereoize(groovy, buffer, count * sizeof(short));
	else
	    memcpy(groovy, buffer, count * sizeof(short));

        ao_play(device, (char *)groovy, frames_read * sizeof(short));
	toread = toread - frames_read;
    }

    free(buffer);
    free(groovy);
    fseek(fp, filestart, SEEK_SET);
    ao_close(device);
    sf_close(sndfile);
    ao_shutdown();
    printf("Finished\n");

    return 0;
}

void stereoize(short *outbuf, short *inbuf, size_t length)
{
    int count;
    int outcount;

    outcount = 0;
    for (count = 0; count <= length; count++) {
	outbuf[outcount] = outbuf[outcount+1] = inbuf[count];
	outcount += 2;
    }
}
