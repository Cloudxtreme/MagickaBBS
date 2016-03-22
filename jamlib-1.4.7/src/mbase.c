/*
   JAMLIB - A JAM subroutine library
   Copyright (C) 1999 Bj�rn Stenberg

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   Changes made by Johan Billing 2000-04-16:

   - Added support for Win32 and Linux
   - Changed JAM_OpenMB to open files in binary mode
   - Changed source to use feof() instead of errno == EPASTEOF
   - Changed source to use structrw to read and write structures
   - Fixed broken JAM_FindUser()
   - #includes string.h and stdlib.h instead of memory.h

*/

/***********************************************************************
 **
 **  MBASE.C -- Message base handling
 **
 **  Author: Bj�rn Stenberg (bjorn.stenberg@sth.frontec.se)
 **
 ***********************************************************************/
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <jamlib/jam.h>
#include "structrw.h"

#if defined( __OS2__ )
# include <os2.h>	/* ANSI C does not include file locking :-( */
#endif

#if defined( __WIN32__ )
# include <sys/locking.h>
# include <io.h>
#endif

#if defined( __UNIX__ )
# if HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
# if HAVE_UNISTD_H
#  include <unistd.h>
# endif
# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif
# ifdef USE_LOCKF
#  include <sys/file.h>
# endif
#endif

#define OS_ERROR_OFFSET 10000

#if defined( __OS2__ )
# define JAM_Sleep( _x_ )	DosSleep( (_x_)*1000 )
#endif

#if defined( __WIN32__ )
# define JAM_Sleep(x) _sleep((x)*1000)
#endif

#if defined( __UNIX__ )
# define JAM_Sleep(x) sleep(x)
#endif

/***********************************************************************
 **
 **  File-global functions {{{
 **/
int jam_Open( s_JamBase* Base_PS, uchar* Filename_PC, char* Mode_PC );
int jam_Lock( s_JamBase* Base_PS, int DoLock_I );
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  JAM_OpenMB - Open message base {{{
 **/
int JAM_OpenMB( uchar* Basename_PC, s_JamBase** NewArea_PPS )
{
    s_JamBase*  Base_PS;
    int		Status_I;

    if ( !NewArea_PPS )
	return JAM_BAD_PARAM;

    *NewArea_PPS = NULL;

    Base_PS = (s_JamBase*) calloc( 1, sizeof( s_JamBase ) );
    if (!Base_PS)
	return JAM_NO_MEMORY;

    *NewArea_PPS = Base_PS;

    Status_I = jam_Open( Base_PS, Basename_PC, "r+b" );
    if ( Status_I )
	return Status_I;

    return 0;
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  JAM_CreateMB - Create a new message base {{{
 **/
int JAM_CreateMB( uchar*	Basename_PC,
		  ulong		BaseMsg_I,
		  s_JamBase**	NewArea_PPS )
{
    s_JamBaseHeader	Base_S;
    int			Status_I;
    s_JamBase*		Base_PS;

    if ( !NewArea_PPS || !BaseMsg_I )
	return JAM_BAD_PARAM;

    *NewArea_PPS = NULL;

    Base_PS = (s_JamBase*) calloc( 1, sizeof( s_JamBase ) );
    if (!Base_PS)
	return JAM_NO_MEMORY;

    *NewArea_PPS = Base_PS;

    Status_I = jam_Open( Base_PS, Basename_PC, "w+b" );
    if ( Status_I )
	return Status_I;

    Base_S.DateCreated = time(NULL);
    Base_S.ModCounter  = 0;
    Base_S.ActiveMsgs  = 0;
    Base_S.PasswordCRC = 0xffffffff;
    Base_S.BaseMsgNum  = BaseMsg_I;
    memset( &Base_S.RSRVD, 0, sizeof( Base_S.RSRVD ) );

    Status_I = JAM_LockMB( Base_PS, 0 );
    if ( Status_I )
	/* FIXME: Base was created, but we can not lock it
         * (normaly this should not happen)
         * Shouldn't we remove recently created files? */
	return Status_I;

    Status_I = JAM_WriteMBHeader( Base_PS, &Base_S );
    if ( Status_I ) {
	JAM_UnlockMB( Base_PS );
	return Status_I;
    }

    JAM_UnlockMB( Base_PS );

    return 0;
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  JAM_CloseMB - Close message base {{{
 **/
int JAM_CloseMB( s_JamBase* Base_PS )
{
    if ( Base_PS->Locked_I ) {
	int Status_I = JAM_UnlockMB( Base_PS );
	if ( Status_I )
	    return Status_I;
    }

    if ( Base_PS->HdrFile_PS ) {
	fclose( Base_PS->HdrFile_PS ); Base_PS->HdrFile_PS = NULL;
    }
    if ( Base_PS->TxtFile_PS ) {
	fclose( Base_PS->TxtFile_PS ); Base_PS->TxtFile_PS = NULL;
    }
    if ( Base_PS->IdxFile_PS ) {
	fclose( Base_PS->IdxFile_PS ); Base_PS->IdxFile_PS = NULL;
    }
    if ( Base_PS->LrdFile_PS ) {
	fclose( Base_PS->LrdFile_PS ); Base_PS->LrdFile_PS = NULL;
    }

    Base_PS->Locked_I = 0;
    return 0;
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  JAM_RemoveMB - Remove a message base {{{
 **/
int JAM_RemoveMB( s_JamBase* Base_PS, uchar* Basename_PC )
{
    uchar Filename_AC[250];
    int   Status_AI[4];

    /* .JHR file */
    sprintf( Filename_AC, "%s%s", Basename_PC, EXT_HDRFILE );
    Status_AI[0] = remove( Filename_AC );
    if ( Status_AI[0] )
	Base_PS->Errno_I = errno;

    /* .JDT file */
    sprintf( Filename_AC, "%s%s", Basename_PC, EXT_TXTFILE );
    Status_AI[1] = remove( Filename_AC );
    if ( Status_AI[1] )
	Base_PS->Errno_I = errno;

    /* .JDX file */
    sprintf( Filename_AC, "%s%s", Basename_PC, EXT_IDXFILE );
    Status_AI[2] = remove( Filename_AC );
    if ( Status_AI[2] )
	Base_PS->Errno_I = errno;

    /* .JLR file */
    sprintf( Filename_AC, "%s%s", Basename_PC, EXT_LRDFILE );
    Status_AI[3] = remove( Filename_AC );
    if ( Status_AI[3] )
	Base_PS->Errno_I = errno;

    if (Status_AI[0] || Status_AI[1] || Status_AI[2] || Status_AI[3])
	return JAM_IO_ERROR;

    return 0;
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  JAM_GetMBSize - Get the number of messages in message base {{{
 **/
int JAM_GetMBSize( s_JamBase* Base_PS, ulong* Messages_PI )
{
    ulong Offset_I;

    /* go to end of index file */
    if ( fseek( Base_PS->IdxFile_PS, 0, SEEK_END ) ) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

    Offset_I = ftell( Base_PS->IdxFile_PS );
    if ( Offset_I == -1 ) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

    *Messages_PI = Offset_I / sizeof( s_JamIndex );

    return 0;
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  JAM_LockMB - Lock message base {{{
 **/
int JAM_LockMB( s_JamBase* Base_PS, int Timeout_I )
{
    if ( Base_PS->Locked_I )
	return 0;

    switch ( Timeout_I ) {
    case -1:
	/* unlimited timeout */
	while ( jam_Lock( Base_PS, 1 ) == JAM_LOCK_FAILED )
	    JAM_Sleep( 1 );
	return 0;

    case 0:
	/* no timeout */
	return jam_Lock( Base_PS, 1 );

    default: 
	/* X seconds timeout */
	{
	    time_t Time_I = time(NULL) + Timeout_I;

	    while ( time(NULL) < Time_I )
	    {
		int Result_I;

		Result_I = jam_Lock( Base_PS, 1 );

		if ( Result_I == JAM_LOCK_FAILED )
		    JAM_Sleep( 1 );
		else
		    return Result_I;
	    }
	    return JAM_LOCK_FAILED;
	}
    }
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  JAM_UnlockMB - Flush all writes and unlock message base {{{
 **/
int JAM_UnlockMB( s_JamBase* Base_PS )
{
    fflush( Base_PS->HdrFile_PS );
    fflush( Base_PS->TxtFile_PS );
    fflush( Base_PS->IdxFile_PS );
    fflush( Base_PS->LrdFile_PS );

    return jam_Lock( Base_PS, 0 );
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  JAM_ReadMBHeader - Read message base header {{{
 **/
int JAM_ReadMBHeader( s_JamBase* Base_PS, s_JamBaseHeader* Header_PS )
{
    if ( !Header_PS || !Base_PS )
	return JAM_BAD_PARAM;

    if ( fseek( Base_PS->HdrFile_PS, 0, SEEK_SET ) ) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

    if ( 1 > freadjambaseheader(Base_PS->HdrFile_PS,Header_PS) ) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

    return 0;
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  JAM_WriteMBHeader - Write message base header {{{
 **/
int JAM_WriteMBHeader( s_JamBase* Base_PS, s_JamBaseHeader* Header_PS )
{
    if ( !Header_PS || !Base_PS )
	return JAM_BAD_PARAM;

    if ( !Base_PS->Locked_I )
	return JAM_NOT_LOCKED;

    if ( fseek( Base_PS->HdrFile_PS, 0, SEEK_SET ) ) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

    /* ensure header looks right */
    memcpy( Header_PS->Signature, HEADERSIGNATURE, 4 );
    Header_PS->ModCounter++;

    if ( 1 > fwritejambaseheader(Base_PS->HdrFile_PS,Header_PS) ) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

    fflush( Base_PS->HdrFile_PS );

    return 0;
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  JAM_FindUser - Scan scan file for messages to a user {{{
 **/
int JAM_FindUser( s_JamBase*	Base_PS,
		  ulong		UserCrc_I,
		  ulong		StartMsg_I,
		  ulong*	MsgNo_PI )
{
    ulong MsgNo_I;

    /* go to start message */
    if ( fseek( Base_PS->IdxFile_PS, StartMsg_I * sizeof( s_JamIndex ), SEEK_SET ) ) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

    /* scan file */
    for ( MsgNo_I = StartMsg_I; ; MsgNo_I++ ) {

	s_JamIndex Index_S;

	if ( 1 > freadjamindex(Base_PS->IdxFile_PS,&Index_S) ) {

	    if ( feof(Base_PS->IdxFile_PS) )
		return JAM_NO_USER;

	    Base_PS->Errno_I = errno;
	    return JAM_IO_ERROR;
	}

	if ( Index_S.UserCRC == UserCrc_I )
	    break;
    }

    *MsgNo_PI = MsgNo_I;
    return 0;
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  jam_Lock - Lock/unlock a message base {{{
 **/
int jam_Lock( s_JamBase* Base_PS, int DoLock_I )
{
#if defined(__OS2__)
    FILELOCK Area_S;
    APIRET   Status_I;
    ULONG    Timeout_I = 0;
    int      Handle_I;

    Handle_I = fileno( Base_PS->HdrFile_PS );
    if ( Handle_I == -1 ) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

    Area_S.lOffset = 0;
    Area_S.lRange  = 1;

    if ( DoLock_I )
	Status_I = DosSetFileLocks( Handle_I, NULL, &Area_S, Timeout_I, 0 );
    else
	Status_I = DosSetFileLocks( Handle_I, &Area_S, NULL, Timeout_I, 0 );

    if ( Status_I ) {
	if ( 232 == Status_I )
	    return JAM_LOCK_FAILED;

	Base_PS->Errno_I = Status_I + OS_ERROR_OFFSET;
	return JAM_IO_ERROR;
    }

    if ( DoLock_I )
	Base_PS->Locked_I = 1;
    else
	Base_PS->Locked_I = 0;

    return 0;
#elif defined(__WIN32__)
    int      Handle_I,Status_I;

    fseek(Base_PS->HdrFile_PS,0,SEEK_SET); /* Lock from start of file */

    Handle_I = fileno( Base_PS->HdrFile_PS );
    if ( Handle_I == -1 ) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

    if ( DoLock_I )
	Status_I = _locking(Handle_I,_LK_NBLCK,1);
    else
	Status_I = _locking(Handle_I,_LK_UNLCK,1);

    if ( Status_I )
	return JAM_LOCK_FAILED;

    if ( DoLock_I )
	Base_PS->Locked_I = 1;
    else
	Base_PS->Locked_I = 0;

    return 0;
#elif defined(__UNIX__)
    int      Handle_I,Status_I;

    fseek(Base_PS->HdrFile_PS,0,SEEK_SET); /* Lock from start of file */

    Handle_I = fileno( Base_PS->HdrFile_PS );
    if ( Handle_I == -1 ) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

#ifdef USE_LOCKF
    if(DoLock_I) Status_I = lockf(Handle_I, F_TLOCK, 1);
    else         Status_I = lockf(Handle_I, F_ULOCK, 1);
#else
    {
	struct flock fl;

	if(DoLock_I) fl.l_type=F_WRLCK;
	else         fl.l_type=F_UNLCK;

	fl.l_whence=SEEK_SET;
	fl.l_start=0;
	fl.l_len=1;
	fl.l_pid=getpid();

	Status_I=fcntl(Handle_I,F_SETLK,&fl);
    }
#endif
    if ( Status_I ) {
	Base_PS->Errno_I = errno;
	return JAM_LOCK_FAILED;
    }

    if ( DoLock_I )
	Base_PS->Locked_I = 1;
    else
	Base_PS->Locked_I = 0;

    return 0;
#else
#error Unsupported platform
#endif
}
/** }}}
 **
 ***********************************************************************/

/***********************************************************************
 **
 **  jam_Open - Open/create message base files {{{
 **/
int jam_Open( s_JamBase* Base_PS, uchar* Basename_PC, char* Mode_PC )
{
    uchar Filename_AC[250];

    /* .JHR file */
    sprintf( Filename_AC, "%s%s", Basename_PC, EXT_HDRFILE );
    Base_PS->HdrFile_PS = fopen( Filename_AC, Mode_PC );
    if (!Base_PS->HdrFile_PS) {
	Base_PS->Errno_I = errno;
	return JAM_IO_ERROR;
    }

    /* .JDT file */
    sprintf( Filename_AC, "%s%s", Basename_PC, EXT_TXTFILE );
    Base_PS->TxtFile_PS = fopen( Filename_AC, Mode_PC );
    if (!Base_PS->TxtFile_PS) {
	Base_PS->Errno_I = errno;
	fclose( Base_PS->HdrFile_PS ); Base_PS->HdrFile_PS = NULL;
	return JAM_IO_ERROR;
    }

    /* .JDX file */
    sprintf( Filename_AC, "%s%s", Basename_PC, EXT_IDXFILE );
    Base_PS->IdxFile_PS = fopen( Filename_AC, Mode_PC );
    if (!Base_PS->IdxFile_PS) {
	Base_PS->Errno_I = errno;
	fclose( Base_PS->HdrFile_PS ); Base_PS->HdrFile_PS = NULL;
	fclose( Base_PS->TxtFile_PS ); Base_PS->TxtFile_PS = NULL;
	return JAM_IO_ERROR;
    }

    /* .JLR file */
    sprintf( Filename_AC, "%s%s", Basename_PC, EXT_LRDFILE );
    Base_PS->LrdFile_PS = fopen( Filename_AC, Mode_PC );
    if (!Base_PS->LrdFile_PS) {
	Base_PS->Errno_I = errno;
	fclose( Base_PS->HdrFile_PS ); Base_PS->HdrFile_PS = NULL;
	fclose( Base_PS->TxtFile_PS ); Base_PS->TxtFile_PS = NULL;
	fclose( Base_PS->IdxFile_PS ); Base_PS->IdxFile_PS = NULL;
	return JAM_IO_ERROR;
    }

    return 0;
}
/** }}}
 **
 ***********************************************************************/
/* Local variables:
 * vim:set ts=8 sts=4 sw=4:
 * vim600:fdm=marker:
 */
