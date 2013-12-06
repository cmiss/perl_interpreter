
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "base64.h"

int main(int argc, char **argv)
{
	size_t i = 0, byte_count = 0, result = 0;
	char data[3];
	FILE *fidbin, *fid64;
	char *buf = 0;
	char *inbuffer = 0;
	size_t binsize = 0;

	if (argc != 3)
	{
		printf("usage: bin2base64str <infile> <outfile>\n");
		return 1;
	}

	fidbin = fopen(argv[1], "rb");
	fid64 = fopen(argv[2], "w");
	fseek(fidbin , 0 , SEEK_END);
	binsize = ftell(fidbin);
	rewind(fidbin);
#if defined CMISS_DEBUG
	printf("File size = %ld bytes\n", binsize);
#endif /* CMISS_DEBUG */
	inbuffer = (char*) malloc(sizeof(char)*binsize);
	result = fread(inbuffer, 1, binsize, fidbin);
	if (result != binsize)
	{
		printf("file read error\n");
		fclose(fidbin);
		return -1;
	}

	fprintf(fid64, "\"");
	data[0] = '\0';data[1] = '\0';data[2] = '\0';
	for (i = 0; i < binsize; i++)
	{
		if ((i + 1) % 4000 == 0)
		{
			fprintf(fid64, "\"\n\"");
		}
		data[byte_count] = inbuffer[i];
		byte_count++;
		if (byte_count == 3)
		{
			buf = bin2base64(data, byte_count);
			fprintf(fid64, "%c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
#if defined CMISS_DEBUG
			printf("%c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
#endif /* CMISS_DEBUG */
			byte_count = 0;
			data[0] = '\0';data[1] = '\0';data[2] = '\0';
		}
		
	}
	if (byte_count != 0)
	{
		buf = bin2base64(data, byte_count);
		fprintf(fid64, "%c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
#if defined CMISS_DEBUG
		printf("%c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
#endif /* CMISS_DEBUG */
	}
	fprintf(fid64, "\"");
	fclose(fidbin);
	fclose(fid64);
	free(inbuffer);

	return 0;
}

