/*	@(#)zmodemsys.c 1.1 95/06/28 Edward Falk	*/

#include <unistd.h>

	/* small utilities for porting between systems */



#ifndef	HAVE_STRDUP

char	*
strdup( char *str )
{
	char	*rval ;
	int	len = strlen(str) + 1 ;
	rval = (char *)malloc(len) ;
	strcpy(rval,str) ;
	return rval ;
}

#endif
