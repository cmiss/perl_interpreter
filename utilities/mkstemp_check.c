
#include <windows.h>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>

#define PERLDLL
#include "EXTERN.h"
#include "perl.h"

int mkstemp(char *template_name)
{
	DWORD path_size;
	char path_buffer[MAX_PATH+1];
	//char tempfilename[MAX_PATH];
	UINT unique_number;

	path_size = GetTempPath( MAX_PATH, path_buffer);
	unique_number = GetTempFileName(path_buffer, "pin", 0, template_name);
	printf("template name: %s\n", template_name);
	return _open(template_name, _O_RDWR | _O_BINARY);
}

int myfunc()
{
	char tmp_name[MAX_PATH+1];
	int fd = mkstemp(tmp_name);
	perl_alloc();
	return fd;
}

int main()
{
	int fd = myfunc();
	printf("testing for mkstemp: %d\n", fd);
	close(fd);
	return 0;
}

