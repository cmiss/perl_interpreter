
#include <stdio.h>
#include <string.h>

static const char base64table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *bin2base64(char *data, int byte_count)
{
	static char base64[4];
	int i = 0, index;
	long u = 0;

	for (i = 0; i < 4; i++)
	{
		if (i < 3)
			u += ((data[i] & 0xff) << (2 - i) * 8);
		index = ((u >> (3 - i) * 6) & 0x3f);
		base64[i] = base64table[index];
	}
	if (byte_count == 1)
	{
		base64[2] = '=';
		base64[3] = '=';
	}
	else if (byte_count == 2)
	{
		base64[3] = '=';
	}

	return base64;
}

char *base642bin(char *data, int *byte_count)
{
	static char bin[3];
	char *ch, chstr[2];
	int i, index;
	long u = 0;

	chstr[1] = '\0';
	for (i = 0; i < 4; i++)
	{
		chstr[0] = data[i];
		ch = strstr(base64table, chstr);
		if (ch)
		{
			index = ch - base64table;
			u += (index << (3 - i) * 6);
		}
	}
	*byte_count = 3;
	if (data[3] == '=')
		*byte_count = 2;
	if (data[2] == '=')
		*byte_count = 1;
	for (i = 0; i < *byte_count; i++)
	{
		bin[i] = (char)(u >> (2 - i) * 8) & 0xff;
	}

	return bin;
}

