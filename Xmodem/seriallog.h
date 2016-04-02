/*	$Id: seriallog.h,v 1.2 2001/10/25 23:56:29 efalk Exp $	*/

#ifndef	SERIALLOG_H
#define	SERIALLOG_H


extern	FILE	*SerialLogFile ;
extern	void	SerialLogFlush() ;
extern	void	SerialLog(const void *data, int len, int w) ;

#endif
