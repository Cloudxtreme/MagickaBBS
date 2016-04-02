#ifndef lint
static const char rcsid[] = "$Id: zmodemr.c,v 1.1.1.1 2001/03/08 00:01:48 efalk Exp $" ;
#endif

/*
 * Copyright (c) 1995 by Edward A. Falk
 */


/**********
 *
 *
 *	@@@@@  @   @   @@@   @@@@   @@@@@  @   @  @@@@   
 *	   @   @@ @@  @   @  @   @  @      @@ @@  @   @  
 *	  @    @ @ @  @   @  @   @  @@@    @ @ @  @@@@   
 *	 @     @ @ @  @   @  @   @  @      @ @ @  @  @   
 *	@@@@@  @ @ @   @@@   @@@@   @@@@@  @ @ @  @   @  
 *
 *	ZMODEMR - receive side of zmodem protocol
 *
 * receive side of zmodem protocol
 *
 * This code is designed to be called from inside a larger
 * program, so it is implemented as a state machine where
 * practical.
 *
 * functions:
 *
 *	ZmodemRInit(ZModem *info)
 *		Initiate a connection
 *
 *	ZmodemRAbort(ZModem *info)
 *		abort transfer
 *
 * all functions return 0 on success, 1 on failure
 *
 *
 *	Edward A. Falk
 *
 *	January, 1995
 *
 *
 *
 **********/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "zmodem.h"
#include "crctab.h"

extern	int	errno ;

extern	int	ZWriteFile(u_char *buffer, int len, FILE *, ZModem *);
extern	int	ZCloseFile(ZModem *info) ;
extern	void	ZFlowControl(int onoff, ZModem *info) ;

static	u_char	zeros[4] = {0,0,0,0} ;





int
ZmodemRInit(ZModem *info)
{
	info->packetCount = 0 ;
	info->offset = 0 ;
	info->errCount = 0 ;
	info->escCtrl = info->escHibit = info->atSign = info->escape = 0 ;
	info->InputState = Idle ;
	info->canCount = info->chrCount = 0 ;
	info->filename = NULL ;
	info->interrupt = 0 ;
	info->waitflag = 0 ;
	info->attn = NULL ;
	info->file = NULL ;

	info->buffer = (u_char *)malloc(8192) ;

	info->state = RStart ;
	info->timeoutCount = 0 ;

	ZIFlush(info) ;

	/* Don't send ZRINIT right away, there might be a ZRQINIT in
	 * the input buffer.  Instead, set timeout to zero and return.
	 * This will allow ZmodemRcv() to check the input stream first.
	 * If nothing found, a ZRINIT will be sent immediately.
	 */
	info->timeout = 0 ;

	zmodemlog("ZmodemRInit[%s]: flush input, new state = RStart\n",
	    sname(info)) ;

	return 0 ;
}

int
YmodemRInit(ZModem *info)
{
	info->errCount = 0 ;
	info->InputState = Yrcv ;
	info->canCount = info->chrCount = 0 ;
	info->noiseCount = 0 ;
	info->filename = NULL ;
	info->file = NULL ;

	if( info->buffer == NULL )
	  info->buffer = (u_char *)malloc(1024) ;

	info->state = YRStart ;
	info->packetCount = -1 ;
	info->timeoutCount = 0 ;
	info->timeout = 10 ;
	info->offset = 0 ;

	ZIFlush(info) ;

	return ZXmitStr((u_char *)"C", 1, info) ;
}





extern	int	ZPF() ;
extern	int	Ignore() ;
extern	int	GotCommand() ;
extern	int	GotStderr() ;

	int	SendRinit() ;
static	int	GotSinit() ;
static	int	GotFile() ;
static	int	GotFin() ;
static	int	GotData() ;
static	int	GotEof() ;
static	int	GotFreecnt() ;
static	int	GotFileCrc() ;
	int	ResendCrcReq() ;
	int	ResendRpos() ;

		/* sent ZRINIT, waiting for ZSINIT or ZFILE */
	StateTable	RStartOps[] = {
	  {ZSINIT,GotSinit,0,1,RSinitWait},	/* SINIT, wait for attn str */
	  {ZFILE,GotFile,0,0,RFileName},	/* FILE, wait for filename */
	  {ZRQINIT,SendRinit,0,1,RStart},	/* sender confused, resend */
	  {ZFIN,GotFin,1,0,RFinish},		/* sender shutting down */
	  {ZNAK,SendRinit,1,0,RStart},		/* RINIT was bad, resend */
#ifdef	TODO
	  {ZCOMPL,f,1,1,s},
#endif	/* TODO */
	  {ZFREECNT,GotFreecnt,0,0,RStart},	/* sender wants free space */
	  {ZCOMMAND,GotCommand,0,0,CommandData}, /* sender wants command */
	  {ZSTDERR,GotStderr,0,0,StderrData},	/* sender wants to send msg */
	  {99,ZPF,0,0,RStart},			/* anything else is an error */
	} ;

	StateTable	RSinitWaitOps[] = {	/* waiting for data */
	  {99,ZPF,0,0,RSinitWait},
	} ;

	StateTable	RFileNameOps[] = {	/* waiting for file name */
	  {99,ZPF,0,0,RFileName},
	} ;

	StateTable	RCrcOps[] = {		/* waiting for CRC */
	  {ZCRC,GotFileCrc,0,0,RFile},		  /* sender sent it */
	  {ZNAK,ResendCrcReq,0,0,RCrc},		  /* ZCRC was bad, resend */
	  {ZRQINIT,SendRinit,1,1,RStart},	  /* sender confused, restart */
	  {ZFIN,GotFin,1,1,RFinish},		  /* sender signing off */
	  {99,ZPF,0,0,RCrc},
	} ;

	StateTable	RFileOps[] = {		/* waiting for ZDATA */
	  {ZDATA,GotData,0,0,RData},		  /* got it */
	  {ZNAK,ResendRpos,0,0,RFile},		  /* ZRPOS was bad, resend */
	  {ZEOF,GotEof,0,0,RStart},		  /* end of file */
	  {ZRQINIT,SendRinit,1,1,RStart},	  /* sender confused, restart */
	  {ZFILE,ResendRpos,0,0,RFile},		  /* ZRPOS was bad, resend */
	  {ZFIN,GotFin,1,1,RFinish},		  /* sender signing off */
	  {99,ZPF,0,0,RFile},
	} ;

	/* waiting for data, but a packet could possibly arrive due
	 * to error recovery or something
	 */
	StateTable	RDataOps[] = {
	  {ZRQINIT,SendRinit,1,1,RStart},	/* sender confused, restart */
	  {ZFILE,GotFile,0,1,RFileName},	/* start a new file (??) */
	  {ZNAK,ResendRpos,1,1,RFile},		/* ZRPOS was bad, resend */
	  {ZFIN,GotFin,1,1,RFinish},		/* sender signing off */
	  {ZDATA,GotData,0,1,RData},		/* file data follows */
	  {ZEOF,GotEof,1,1,RStart},		/* end of file */
	  {99,ZPF,0,0,RData},
	} ;

	/* here if we've sent ZFERR or ZABORT.  Waiting for ZFIN */

	StateTable	RFinishOps[] = {
	  {ZRQINIT,SendRinit,1,1,RStart},	/* sender confused, restart */
	  {ZFILE,GotFile,1,1,RFileName},	/* start a new file */
	  {ZNAK,GotFin,1,1,RFinish},		/* resend ZFIN */
	  {ZFIN,GotFin,1,1,RFinish},		/* sender signing off */
	  {99,ZPF,0,0,RFinish},
	} ;




extern	char	*hdrnames[] ;

extern	int	dataSetup() ;


	/* RECEIVE-RELATED STUFF BELOW HERE */


	/* resend ZRINIT header in response to ZRQINIT or ZNAK header */

int
SendRinit( register ZModem *info )
{
	u_char	dbuf[4] ;

#ifdef	COMMENT
	if( info->timeoutCount >= 5 )
	  /* TODO: switch to Ymodem */
#endif	/* COMMENT */

	zmodemlog("SendRinit[%s]: send ZRINIT\n", sname(info)) ;

	info->timeout = ResponseTime ;
	dbuf[0] = info->bufsize&0xff ;
	dbuf[1] = (info->bufsize>>8)&0xff ;
	dbuf[2] = 0 ;
	dbuf[3] = info->zrinitflags ;
	return ZXmitHdrHex(ZRINIT, dbuf, info) ;
}



	/* received a ZSINIT header in response to ZRINIT */

static	int
GotSinit( register ZModem *info )
{
	zmodemlog("GotSinit[%s]: call dataSetup\n", sname(info)) ;

	info->zsinitflags = info->hdrData[4] ;
	info->escCtrl = info->zsinitflags & TESCCTL ;
	info->escHibit = info->zsinitflags & TESC8 ;
	ZFlowControl(1, info) ;
	return dataSetup(info) ;
}

	/* received rest of ZSINIT packet */

int
GotSinitData( register ZModem *info, int crcGood )
{
	info->InputState = Idle ;
	info->chrCount=0 ;
	info->state = RStart ;

	zmodemlog("GotSinitData[%s]: crcGood=%d\n", sname(info), crcGood) ;

	if( !crcGood )
	  return ZXmitHdrHex(ZNAK, zeros, info) ;

	if( info->attn != NULL )
	  free(info->attn) ;
	info->attn = NULL ;
	if( info->buffer[0] != '\0' )
	  info->attn = strdup((char *)info->buffer) ;
	return ZXmitHdrHex(ZACK, ZEnc4(SerialNo), info) ;
}


	/* got ZFILE.  Cache flags and set up to receive filename */

static	int
GotFile( register ZModem *info )
{
	zmodemlog("GotFile[%s]: call dataSetup\n", sname(info)) ;

	info->errCount = 0 ;
	info->f0 = info->hdrData[4] ;
	info->f1 = info->hdrData[3] ;
	info->f2 = info->hdrData[2] ;
	info->f3 = info->hdrData[1] ;
	return dataSetup(info) ;
}


	/* utility: see if ZOpenFile wants this file, and if
	 * so, request it from sender.
	 */

static	int
requestFile( register ZModem *info, u_long crc )
{
	info->file = ZOpenFile((char *)info->buffer, crc, info) ;

	if( info->file == NULL ) {
	  zmodemlog("requestFile[%s]: send ZSKIP\n", sname(info)) ;

	  info->state = RStart ;
	  ZStatus(FileSkip, 0, info->filename) ;
	  return ZXmitHdrHex(ZSKIP, zeros, info) ;
	}
	else {
	  zmodemlog("requestFile[%s]: send ZRPOS(%ld)\n",
	    sname(info), info->offset) ;
	  info->offset = info->f0 == ZCRESUM ? ftell(info->file) : 0 ;
	  info->state = RFile ;
	  ZStatus(FileBegin, 0, info->filename) ;
	  return ZXmitHdrHex(ZRPOS, ZEnc4(info->offset), info) ;
	}
}


	/* parse filename info. */

static	void
parseFileName( register ZModem *info, char *fileinfo )
{
	char	*ptr ;
	int	serial=0 ;

	info->len = info->mode = info->filesRem =
		info->bytesRem = info->fileType = 0 ;
	ptr = fileinfo + strlen(fileinfo) + 1 ;
	if( info->filename != NULL )
	  free(info->filename) ;
	info->filename = strdup(fileinfo) ;
	sscanf(ptr, "%d %lo %o %o %d %d %d", &info->len, &info->date,
	    &info->mode, &serial, &info->filesRem, &info->bytesRem,
	    &info->fileType) ;
}


	/* got filename.  Parse arguments from it and execute
	 * policy function ZOpenFile(), provided by caller
	 */

int
GotFileName( register ZModem *info, int crcGood )
{
	info->InputState = Idle ;
	info->chrCount=0 ;

	if( !crcGood ) {
	zmodemlog("GotFileName[%s]: bad crc, send ZNAK\n", sname(info)) ;
	  info->state = RStart ;
	  return ZXmitHdrHex(ZNAK, zeros, info) ;
	}

	parseFileName(info, (char *)info->buffer) ;

	if( (info->f1 & ZMMASK) == ZMCRC ) {
	  info->state = RCrc ;
	  return ZXmitHdrHex(ZCRC, zeros, info) ;
	}

	zmodemlog("GotFileName[%s]: good crc, call requestFile\n",
		sname(info)) ;
	info->state = RFile ;
	return requestFile(info,0) ;
}


int
ResendCrcReq(ZModem *info)
{
	zmodemlog("ResendCrcReq[%s]: send ZCRC\n", sname(info)) ;
	return ZXmitHdrHex(ZCRC, zeros, info) ;
}


	/* received file CRC, now we're ready to accept or reject */

static	int
GotFileCrc( register ZModem *info )
{
	zmodemlog("GotFileCrc[%s]: call requestFile\n", sname(info)) ;
	return requestFile(info, ZDec4(info->hdrData+1)) ;
}


	/* last ZRPOS was bad, resend it */

int
ResendRpos( register ZModem *info )
{
	zmodemlog("ResendRpos[%s]: send ZRPOS(%ld)\n",
	  sname(info), info->offset) ;
	return ZXmitHdrHex(ZRPOS, ZEnc4(info->offset), info) ;
}


	/* recevied ZDATA header */

static	int
GotData( register ZModem *info )
{
	int	err ;

	zmodemlog("GotData[%s]:\n", sname(info)) ;

	if( ZDec4(info->hdrData+1) != info->offset ) {
	  if( info->attn != NULL  &&  (err=ZAttn(info)) != 0 )
	    return err ;
	  zmodemlog("  bad, send ZRPOS(%ld)\n", info->offset);
	  return ZXmitHdrHex(ZRPOS, ZEnc4(info->offset), info) ;
	}

	/* Let's do it! */
	zmodemlog("  call dataSetup\n");
	return dataSetup(info) ;
}


	/* Utility: flush input, send attn, send specified header */

static	int
fileError( register ZModem *info, int type, int data )
{
	int	err ;

	info->InputState = Idle ;
	info->chrCount=0 ;

	if( info->attn != NULL  &&  (err=ZAttn(info)) != 0 )
	  return err ;
	return ZXmitHdrHex(type, ZEnc4(data), info) ;
}

	/* received file data */

int
GotFileData( register ZModem *info, int crcGood )
{
	/* OK, now what?  Fushing the buffers and executing the
	 * attn sequence has likely chopped off the input stream
	 * mid-packet.  Now we switch to idle mode and treat all
	 * incoming stuff like noise until we get a new valid
	 * packet.
	 */

	if( !crcGood ) {		/* oh bugger, an error. */
	  zmodemlog(
	    "GotFileData[%s]: bad crc, send ZRPOS(%ld), new state = RFile\n",
	    sname(info), info->offset) ;
	  ZStatus(DataErr, ++info->errCount, NULL) ;
	  if( info->errCount > MaxErrs ) {
	    ZmodemAbort(info) ;
	    return ZmDataErr ;
	  }
	  else {
	    info->state = RFile ;
	    info->InputState = Idle ;
	    info->chrCount=0 ;
	    return fileError(info, ZRPOS, info->offset) ;
	  }
	}

	if( ZWriteFile(info->buffer, info->chrCount, info->file, info) )
	{
	  /* RED ALERT!  Could not write the file. */
	  ZStatus(FileErr, errno, NULL) ;
	  info->state = RFinish ;
	  info->InputState = Idle ;
	  info->chrCount=0 ;
	  return fileError(info, ZFERR, errno) ;
	}

	zmodemlog("GotFileData[%s]: %ld.%d,",
	  sname(info), info->offset, info->chrCount) ;
	info->offset += info->chrCount ;
	ZStatus(RcvByteCount, info->offset, NULL) ;

	/* if this was the last data subpacket, leave data mode */
	if( info->PacketType == ZCRCE  ||  info->PacketType == ZCRCW ) {
	  zmodemlog("  ZCRCE|ZCRCW, new state RFile") ;
	  info->state = RFile ;
	  info->InputState = Idle ;
	  info->chrCount=0 ;
	}
	else {
	  zmodemlog("  call dataSetup") ;
	  (void) dataSetup(info) ;
	}

	if( info->PacketType == ZCRCQ || info->PacketType == ZCRCW ) {
	  zmodemlog(",  send ZACK\n") ;
	  return ZXmitHdrHex(ZACK, ZEnc4(info->offset), info) ;
	}
	else
	zmodemlog("\n") ;

	return 0 ;
}


	/* received ZEOF packet, file is now complete */

static	int
GotEof( register ZModem *info )
{
	zmodemlog("GotEof[%s]: offset=%ld\n", sname(info), info->offset) ;
	if( ZDec4(info->hdrData+1) != info->offset ) {
	  zmodemlog("  bad length, state = RFile\n") ;
	  info->state = RFile ;
	  return 0 ;		/* it was probably spurious */
	}

	/* TODO: if we can't close the file, send a ZFERR */

	ZCloseFile(info) ; info->file = NULL ;
	ZStatus(FileEnd, 0, info->filename) ;
	if( info->filename != NULL ) {
	  free(info->filename) ;
	  info->filename = NULL ;
	}
	return SendRinit(info) ;
}



	/* got ZFIN, respond in kind */

static	int
GotFin( register ZModem *info )
{
	zmodemlog("GotFin[%s]: send ZFIN\n", sname(info)) ;
	info->InputState = Finish ;
	info->chrCount = 0 ;
	if( info->filename != NULL )
	  free(info->filename) ;
	return ZXmitHdrHex(ZFIN, zeros, info) ;
}



static	int
GotFreecnt( register ZModem *info )
{
	/* TODO: how do we find free space on system? */
	return ZXmitHdrHex(ZACK, ZEnc4(0xffffffff), info) ;
}



	/* YMODEM */

static	u_char	AckStr[1] = {ACK} ;
static	u_char	NakStr[1] = {NAK} ;
static	u_char	CanStr[2] = {CAN,CAN} ;

static	int	ProcessPacket() ;
static	int	acceptPacket() ;
static	int	rejectPacket() ;
static	int	calcCrc() ;

int
YrcvChar( char c, register ZModem *info )
{
	int	err ;

	if( info->canCount >= 2 ) {
	  ZStatus(RmtCancel, 0, NULL) ;
	  return ZmErrCancel ;
	}

	switch( info->state ) {
	  case YREOF:
	    if( c == EOT ) {
	      ZCloseFile(info) ; info->file = NULL ;
	      ZStatus(FileEnd, 0, info->filename) ;
	      if( info->filename != NULL )
		free(info->filename) ;
	      if( (err = acceptPacket(info)) != 0 )
		return err ;
	      info->packetCount = -1 ;
	      info->offset = 0 ;
	      info->state = YRStart ;
	      return ZXmitStr((u_char *)"C", 1, info) ;
	    }
	    /* else, drop through */

	  case YRStart:
	  case YRDataWait:
	    switch( c ) {
	      case SOH:
	      case STX:
		info->pktLen = c == SOH ? (128+4) : (1024+4) ;
		info->state = YRData ;
		info->chrCount = 0 ;
		info->timeout = 1 ;
		info->noiseCount = 0 ;
		info->crc = 0 ;
		break ;

	      case EOT:
		/* ignore first EOT to protect against false eot */
		info->state = YREOF ;
		return rejectPacket(info) ;

	      default:
		if( ++info->noiseCount > 135 )
		  return ZXmitStr(NakStr, 1, info) ;
		break ;
	    }
	    break ;

	  case YRData:
	    info->buffer[info->chrCount++] = c ;
	    if( info->chrCount >= info->pktLen )
	      return ProcessPacket(info) ;
	    break ;

	  default:
	    break ;
	}

	return 0 ;
}


int
YrcvTimeout( register ZModem *info )
{
	switch( info->state )
	{
	  case YRStart:
	    if( info->timeoutCount >= 10 ) {
	      (void) ZXmitStr(CanStr, 2, info) ;
	      return ZmErrInitTo ;
	    }
	    return ZXmitStr((u_char *)"C", 1, info) ;

	  case YRDataWait:
	  case YREOF:
	  case YRData:
	    if( info->timeoutCount >= 10 ) {
	      (void) ZXmitStr(CanStr, 2, info) ;
	      return ZmErrRcvTo ;
	    }
	    return ZXmitStr(NakStr, 1, info) ;
	  default: return 0 ;
	}
}



static	int
ProcessPacket( register ZModem *info )
{
	int	idx = (u_char) info->buffer[0] ;
	int	idxc = (u_char) info->buffer[1] ;
	int	crc0, crc1 ;
	int	err ;

	info->state = YRDataWait ;

	if( idxc != 255 - idx ) {
	  ZStatus(DataErr, ++info->errCount, NULL) ;
	  return rejectPacket(info) ;
	}

	if( idx == (info->packetCount%256) )	/* quietly ignore dup */
	  return acceptPacket(info) ;

	if( idx != (info->packetCount+1)%256 ) { /* out of sequence */
	  (void) ZXmitStr(CanStr, 2, info) ;
	  return ZmErrSequence ;
	}

	crc0 = (u_char)info->buffer[info->pktLen-2] << 8 |
	      (u_char)info->buffer[info->pktLen-1] ;
	crc1 = calcCrc(info->buffer+2, info->pktLen-4) ;
	if( crc0 != crc1 ) {
	  ZStatus(DataErr, ++info->errCount, NULL) ;
	  return rejectPacket(info) ;
	}

	++info->packetCount ;

	if( info->packetCount == 0 )		/* packet 0 is filename */
	{
	  if( info->buffer[2] == '\0' ) {	/* null filename is FIN */
	    (void) acceptPacket(info) ;
	    return ZmDone ;
	  }

	  parseFileName(info, (char *)info->buffer+2) ;
	  info->file = ZOpenFile(info->filename, 0, info) ;
	  if( info->file == NULL ) {
	    (void) ZXmitStr(CanStr, 2, info) ;
	    return ZmErrCantOpen ;
	  }
	  if( (err = acceptPacket(info)) != 0 )
	    return err ;
	  return ZXmitStr((u_char *)"C", 1, info) ;
	}


	if( ZWriteFile(info->buffer+2, info->pktLen-4, info->file, info) ) {
	  ZStatus(FileErr, errno, NULL) ;
	  (void) ZXmitStr(CanStr, 2, info) ;
	  return ZmErrSys ;
	}
	info->offset += info->pktLen-4 ;
	ZStatus(RcvByteCount, info->offset, NULL) ;

	(void) acceptPacket(info) ;
	return 0 ;
}


static	int
rejectPacket( register ZModem *info )
{
	info->timeout = 10 ;
	return ZXmitStr(NakStr, 1, info) ;
}


static	int
acceptPacket( register ZModem *info )
{
	info->state = YRDataWait ;
	info->timeout = 10 ;
	return ZXmitStr(AckStr, 1, info) ;
}


static	int
calcCrc( u_char *str, int len )
{
	int	crc = 0 ;
	while( --len >= 0 )
	  crc = updcrc(*str++, crc) ;
	crc = updcrc(0,crc) ; crc = updcrc(0,crc) ;
	return crc & 0xffff ;
}
