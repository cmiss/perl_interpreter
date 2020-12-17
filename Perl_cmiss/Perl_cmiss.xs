#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "perl_cmiss.h"

static int
not_here(char *s)
{
    croak("%s not implemented on this architecture", s);
    return -1;
}

static double
constant(char *name, int arg)
{
    errno = 0;
    switch (*name) {
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static XS(XS_Perl_cmiss_constant);
static XS(XS_Perl_cmiss_cmiss);

MODULE = Perl_cmiss		PACKAGE = Perl_cmiss		

PROTOTYPES: DISABLE

double
constant(name,arg)
	char *		name
	int		arg

int
cmiss(name)
	char *		name
	CODE:
	{
		SV *sv_variable;
		IV internal_interpreter_pointer;

		sv_variable = perl_get_sv("Perl_cmiss::internal_interpreter_structure", TRUE);
		internal_interpreter_pointer = SvIV(sv_variable);
		
		RETVAL = CMISS_PERL_CALLBACK((void *)internal_interpreter_pointer, name);
	}
	OUTPUT:
	RETVAL
