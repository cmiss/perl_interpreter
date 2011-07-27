

#include <stdio.h>
#include <string.h>

#include "base64.h"

static char test_str[] =
#include "rr.h"
;

int main(int argc, char **argv)
{
	int i, j, char_count = 0, byte_count;
	char data[4], *bin;
	FILE *fid;
	if (argc != 2)
	{
		printf("usage: hex2bin <outfile>\n");
		return 1;
	}
	printf("hex2bin\n");

	fid = fopen(argv[1], "w");
	for (i = 0; i < strlen(test_str); i++)
	{
		data[char_count] = test_str[i];
		char_count++;
		if (char_count == 4)
		{
			bin = base642bin(data, &byte_count);
			for (j = 0; j < byte_count; j++)
			{
				//printf("%c", bin[j]);
				fprintf(fid, "%c", bin[j]);
			}
			char_count = 0;
		}
	}
	if (char_count != 0)
		printf("errorrrrrr\n");

	//printf(" '%d'", (int)strlen(test_str));
	fclose(fid);

	return 0;
}

