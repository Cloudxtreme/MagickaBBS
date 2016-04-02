/*	$Id: zmodem.h,v 1.2 2001/10/25 23:56:29 efalk Exp $	*/

#ifndef	ZMODEM_H
#define	ZMODEM_H


/* Master header file for Zmodem protocol driver routines */

#if	0

   These routines are intended to be incorportated into other programs,
   and do not constitute a zmodem program by themselves, although a
   demo zmodem program that uses these routines has been included.

   All information pertaining to a transfer session is kept in a data
   structure defined by the caller.  This makes it possible for the
   caller to drive multiple simultaneous sessions.

   Caller provides I/O, timing and query routines.  Caller remains
   in control at all times.  Typical use is to call these routines
   from a select(2) loop.

   Overview:

    Transmit files:

      1 Init ZModem structure and open communications channel.
      2 Call ZmodemTInit() to begin protocol.
      3 Read characters from remote end and pass them to
	ZmodemRcv() until ZmodemRcv() returns ZmDone or an error.
      4 Call ZmodemTFile() to begin transfer of a file.
      5 Read characters from remote end and pass them to
	ZmodemRcv() until ZmodemRcv() returns ZmDone or an error.
      6 repeat steps 4&5 for all files.
      7 Call ZmodemTFinish() to indicate that all files have been
	transfered.
      8 Read characters from remote end and pass them to
	ZmodemRcv() until ZmodemRcv() returns ZmDone or an error.

    Receive files:

      1 Init ZModem structure and open communications channel.
      2 Call ZmodemRInit() to begin protocol.
      3 Read characters from remote end and pass them to
	ZmodemRcv() until ZmodemRcv() returns ZmDone or an error.



   In detail:

   1) Create a ZModem structure as defined below and fill it in.

      'ifd', 'ofd' are the file descriptors used for input and output.
	The interpretation of ifd and ofd is entirely up to the calling
	routines, since the caller will be providing I/O functions.

      'zrinitflags' is composed of the flags described below.
	They describe the receive channel capabilities
	and affect how the protocol will be carried out.
	Define these to receive files.  When sending files, these
	flags will be defined from the remote end.

      'zsinitflags' is composed of the flags described below.
	They describe the transmit channel capabilities
	and affect how the protocol will be carried out.
	Define these to send files.  When receiving files, these
	flags will be defined from the remote end.

      'attn': For transmit, this is the optional nul-terminated
	attention string to be sent by the receiver to interrupt the
	transmission.  For example, it might simply contain ^O to flush
	the buffers, or it might interrupt the sending program.  Caller
	needs to handle this interrupt properly and call ZmodemAttention().

	For receive, this is the optional nul-terminated attention string
	defined at the remote end.  Caller needs to provide a function that
	can execute this sequence.

      'timeout' is read-only, set by the zmodem package.  It is the
	timeout value in seconds.  If this much time passes without
	anything being received, caller should call the ZmodemTimeout()
	function.  Note that timeout may be zero, in which case the
	ZTimeout function should be called immediately if no characters
	are ready to be received.

      'packetsize' is set to the preferred data packet size.  Define
	this when sending files.  Recommended length values are 256
	bytes below 2400 bps, 512 at 2400 bps, and 1024 above 4800 bps
	or when the data link is known to be relatively error free.
	Ignored during receive.

      'bufsize' is set to the size of the receive buffer size.  Define
	this when receiving files.  When sending files, this will be
	defined at the other end.

      'windowsize' is used to prevent network connections from
	buffering too many characters during transmit.  Setting
	'windowsize' to nonzero causes zmodem to request status reports
	from the receiver during transmit, and to pause if more than
	this many bytes have been sent but not yet acknowledged.  Set
	to zero to turn off windowing.  Ignored during receive.

      Other fields are used to track the internal state of the zmodem
      driver functions.

     Since this is a source package, you are, of course, free to extend
     the ZModem structure as you see fit.


   2) Define the following functions:

	int
	ZXmitStr(u_char *str, int len, ZModem *info)
		Transmit a buffer.  Return 0 on success, ZmErrSys on error.

	void
	ZIFlush(ZModem *info)
		Flush all unread input on receive channel.  Do nothing if
		this is not possible.

	void
	ZOFlush(ZModem *info)
		Flush all buffered but not-yet-transmitted output
		on transmit channel.  Do nothing if this is not possible.

	int
	ZAttn(ZModem *info)
		Send attention signal defined in ZModem->attn.  Do
		nothing if this field is NULL.  Otherwise, this field
		is a nul-terminated character string to be
		transmitted.  There are two special characters defined
		below: ATTNBRK (0335) indicates that a BREAK signal
		should be sent, and ATTNPSE (0336) represents a
		1-second pause.

	void
	ZFlowControl(int onoff, ZModem *info)
		Turn flow control on or off depending on the onoff flag.

	void
	ZStatus(int type, int value, char *status)
		Called to provide status information.  Ignore or display
		at your option.  Status string is not static, so copy
		it if you need it beyond this call.  Type is defined
		below under "ZStatus() types".

	FILE *
	ZOpenFile(char *name, u_long crc, ZModem *info)

		Called when receiving files, this function decides
		whether or not to accept the specified file, and
		if so, opens it for writing and returns the stdio
		file handle.  If this function decides not to accept
		the file, or cannot open the file, it returns NULL
		and the remote sender is told to skip this file.

		info->f0-f3 are the transfer flags, described below under
		"ZFILE transfer flags".  These describe the type of
		transfer desired (binary/ascii), and conditions for the
		transfer, such as 'transfer if source newer or longer'.

		info->len is the length of the file in bytes.  info->date is
		the last modification date of the file, in seconds since
		1-jan-1970.  info->mode is the unix file mode + 01000000, or
		zero if not known.  info->filesRem and info->bytesRem are the
		number of files and bytes remaining to be transferred
		if known, zero otherwise.  'crc' is the file crc-32 value.
		This is only provided if F1 contains ZMCRC.

	int
	ZWriteFile(u_char *buffer, int len, FILE *file, ZModem *info)
		Write a buffer of data to the file.  Normally, you would
		simply call fwrite(buffer, 1, len, file), but you may
		want to translate line endings, etc.  File transfer
		flags are available as info->f0,f1,f2,f3.

		Return 0 on success, ZmErrSys on failure, with errno
		describing the failure.


	int
	ZCloseFile(ZModem *info)
		Close file after successful completion.  File modification
		date and modes should be set at this time.	

	void
	ZIdleStr(u_char *buffer, int len, ZModem *info)
		Called to pass text that is received out-of-protocol.
		This function may ignore or display this text at your
		option.


  3) Open the communications channel.

	If possible, this should be a full-duplex channel with full
	8-bit transmission.  Hardware flow control and/or XON/XOFF
	flow control should be enabled.


  4) Call these routines:

	All functions return 0 on success, nonzero on failure.  See
	"error code definitions", below.

    Send:

	int
	ZmodemTInit(ZModem *info)
		Begin a Zmodem transmit session.

	int
	ZmodemRcv(u_char *buffer, int len, ZModem *info)
		Call whenever characters are received.  If this function
		returns ZmDone, previous function has completed successfully,
		either call ZmodemTFile() to start next file, or call
		ZmodemTFinish() to terminate the session.

	int
	ZmodemTimeout(ZModem *info)
		Call whenever the timeout period expires and no
		characters have been received.

	int
	ZmodemAttention(ZModem *info)
		Call whenever the attention sequence has been received
		from the remote end.  It is safe to call this function
		from an interrupt handler.

	int
	ZmodemTFile(char *filename, char *rfilename, 
	  u_char f0,f1,f2,f3, int filesRem, int bytesRem, ZModem *info)
		Begin transmitting a file.  If filename is not NULL,
		then this function will open it.  Otherwise, info->file
		must point to a stdio stream that is open for input.
		It is preferable to provide the filename, so that
		Zmodem can transmit file size and other information.
		'rfilename' is the filename given to the remote end.
		This may be the same as filename, the file part of
		filename, or something else alltogether.  'rfilename'
		must not be longer than the smallest data packet the
		remote end might be willing to receive (about 200
		characters).  f0-f3 are transfer flags, see "ZCBIN"
		below.  'filesRem' and 'bytesRem' are the number of files
		and bytes remaining to be transmitted, if known; zero if
		not.

		If 'filename' cannot be accessed, ZmodemTFile() returns
		ZmErrCantOpen.  The link is still established, so you
		need to either proceed with the next file or call
		ZmodemTFinish().

	int
	ZmodemTFinish(ZModem *info)
		Call after final file transfer has completed successfully.

	int
	ZmodemAbort(ZModem *info)
		Call to abort transfer.  Physical connection remains
		open until you close it.


    Receive:

	int
	ZmodemRInit(ZModem *info)
		Call to get ready to receive first file.  This function
		will inform the sender that we are ready to receive.

	int
	ZmodemRcv(u_char *buffer, int len, ZModem *info)
		Call whenever characters are received.  If this
		function returns ZmDone, all file transfers have
		completed successfully.

	int
	ZmodemTimeout(ZModem *info)
		Call whenever the timeout period expires and no
		characters have been received.

	int
	ZmodemAbort(ZModem *info)
		Call to abort transfer.

    Ymodem and Xmodem:

	int YmodemTInit(ZModem *info)
	int XmodemTInit(ZModem *info)
	int YmodemRInit(ZModem *info)
	int XmodemRInit(ZModem *info)
		Same semantics as ZmodemTInit and ZmodemRInit.  It is
		not normally necessary to call the Ymodem*Init() functions
		as the Zmodem protocol will automatically switch to Ymodem
		when needed.


    Utility:

	u_long
	FileCrc(char *name)
		Return CRC-32 of file.

  5) Return Values:

	ZmDone		Done.  Proceed with next file or ZmodemTFinish
			(transmit) or exit (receive).

	ZmErrInt	Internal error.  Link has been closed.
	ZmErrSys	System error, see errno.  Link is closed.
	ZmErrNotOpen	not used.
	ZmFileTooLong	not used.
	ZmFileCantWrite	not used.
	ZmErrCantOpen	Can not open file, see errno.  Link is still open.
	ZmErrInitTo	Transmitter failed to respond to init req.  Link closed.
	ZmErrSequence	Packet received out of sequence.  Link is closed.
	ZmErrCancel	Cancelled by remote end.  Link is closed.
	ZmErrRcvTo	Remote end timed out during transfer.  Link is closed.
	ZmErrSndTo	Remote end timed out during transfer.  Link is closed.
	ZmErrCmdTo	Remote end timed out during transfer.  Link is closed.

	Note that "link is closed" means that the remote end is (presumably)
	no longer operating.  The actual communications channel is not
	closed unless you close it.  "Link is still open" means that the
	remote end is still ready to receive the next file.

#endif

	/* PARAMETERS
	 *
	 * The following #defines control the behavior of the Zmodem
	 * package.  Note that these may be replaced with variables
	 * if you like.  For example, "#define DoInitRZ" may be replaced
	 * with "extern int DoInitRz" to use a global variable, or with
	 * "#define DoInitRZ (info->doInitRz)" to use a variable you
	 * add to the ZModem structure.
	 *
	 * It is assumed that the compiler is good enough to optimize
	 * "if( 0 )" and "if( 1 )" cases.  If not, you may wish to modify
	 * the source code to use #ifdef instead.
	 */

#define	DoInitRZ	1	/* send initial "rz\r" when transmitting */
#define	AllowCommand	0	/* allow remote end to execute commands */
#define	SendSample	1	/* sender can sample reverse channel */
#define	SendAttn	1	/* sender can be interrupted with Attn signal */
#define	ResponseTime	10	/* reasonable response time for sender to
				 * respond to requests from receiver */
#define	SerialNo	1	/* receiver serial # */
#define	MaxNoise	64	/* max "noise" characters before transmission
				 * pauses */
#define	MaxErrs		20	/* Max receive errors before cancel */
#define	AlwaysSinit	1	/* always send ZSINIT header, even if not
				 * needed, this makes protocol more robust */

#define SendOnly	0	/* compiles smaller version for send only */
#define RcvOnly		0	/* compiles smaller version for receive only */

	/* constants */


#include <stdio.h>
#include <sys/types.h>




	/* Internal State */

typedef enum zmstate {
	  /* receive */
	  RStart,		/* sent RINIT, waiting for ZFILE or SINIT */
	  RSinitWait,		/* got SINIT, waiting for data */
	  RFileName,		/* got ZFILE, waiting for filename & info */
	  RCrc,			/* got filename, want crc too */
	  RFile,		/* got filename, ready to read */
	  RData,		/* reading data */
	  RDataErr,		/* encountered error, ignoring input */
	  RFinish,		/* sent ZFIN, waiting for 'OO' */

	  /* transmit */
	  TStart,		/* waiting for INIT frame from other end */
	  TInit,		/* received INIT, sent INIT, waiting for ZACK */
	  FileWait,		/* sent file header, waiting for ZRPOS */
	  CrcWait,		/* sent file crc, waiting for ZRPOS */
	  Sending,		/* sending data subpackets, ready for int */
	  SendWait,		/* waiting for ZACK */
	  SendDone,		/* file finished, need to send EOF */
	  SendEof,		/* sent EOF, waiting for ZACK */
	  TFinish,		/* sent ZFIN, waiting for ZFIN */

	  /* general */
	  CommandData,		/* waiting for command data */
	  CommandWait,		/* waiting for command to execute */
	  StderrData,		/* waiting for stderr data */
	  Done,

	  /* x/ymodem transmit */
	  YTStart,		/* waiting for 'G', 'C' or NAK */
	  YTFile,		/* sent filename, waiting for ACK */
	  YTDataWait,		/* ready to send data, waiting for 'C' */
	  YTData,		/* sent data, waiting for ACK */
	  YTEOF,		/* sent eof, waiting for ACK */
	  YTFin,		/* sent null filename, waiting for ACK */

	  /* x/ymodem receive */
	  YRStart,		/* sent 'C', waiting for filename */
	  YRDataWait,		/* received filename, waiting for data */
	  YRData,		/* receiving filename or data */
	  YREOF			/* received first EOT, waiting for 2nd */

	} ZMState ;




typedef	struct {
	  int	ifd ;		/* input fd, for use by caller's routines */
	  int	ofd ;		/* output fd, for use by caller's routines */
	  FILE	*file ;		/* file being transfered */
	  int	zrinitflags ;	/* receiver capabilities, see below */
	  int	zsinitflags ;	/* sender capabilities, see below */
	  char	*attn ;		/* attention string, see below */
	  int	timeout ;	/* timeout value, in seconds */
	  int	bufsize ;	/* receive buffer size, bytes */
	  int	packetsize ;	/* preferred transmit packet size */
	  int	windowsize ;	/* max window size */

	  /* file attributes: read-only */

	  int	filesRem, bytesRem ;
	  u_char f0,f1,f2,f3 ;	/* file flags */
	  int	len,mode,fileType ; /* file flags */
	  u_long date ;		/* file date */

	  /* From here down, internal to Zmodem package */

	  ZMState state ;	/* protocol internal state */
	  char	*filename ;	/* filename */
	  char	*rfilename ;	/* remote filename */
	  int	crc32 ;		/* use 32-bit crc */
	  int	pktLen ;	/* length of this packet */
	  int	DataType ;	/* input data type */
	  int	PacketType ;	/* type of this packet */
	  int	rcvlen ;
	  int	chrCount ;	/* chars received in current header/buffer */
	  int	crcCount ;	/* crc characters remaining at end of buffer */
	  int	canCount ;	/* how many CAN chars received? */
	  int	noiseCount ;	/* how many noise chars received? */
	  int	errorFlush ;	/* ignore incoming data because of error */
	  u_char *buffer ;	/* data buffer */
	  u_long offset ;	/* file offset */
	  u_long lastOffset ;	/* last acknowledged offset */
	  u_long zrposOffset ;	/* last offset specified w/zrpos */
	  int	ylen, bufp ;	/* len,location of last Ymodem packet */
	  int	fileEof ;	/* file eof reached */
	  int	packetCount ;	/* # packets received */
	  int	errCount ;	/* how many data errors? */
	  int	timeoutCount ;	/* how many times timed out? */
	  int	windowCount ;	/* how much data sent in current window */
	  int	atSign ;	/* last char was '@' */
	  int	lastCR ;	/* last char was CR */
	  int	escCtrl ;	/* other end requests ctrl chars be escaped */
	  int	escHibit ;	/* other end requests hi bit be escaped */
	  int	escape ;	/* next character is escaped */
	  int	interrupt ;	/* received attention signal */
	  int	waitflag ;	/* next send should wait */
	  			/* parser state */
	  enum {Idle, Padding, Inhdr, Indata, Finish, Ysend, Yrcv} InputState ;
	  enum {XMODEM, YMODEM, ZMODEM} Protocol ;
	  u_char hdrData[9] ;	/* header type and data */
	  u_char fileFlags[4] ;	/* file xfer flags */
	  u_long crc ;		/* crc of incoming header/data */
	  enum {Full, StrWindow, SlidingWindow, Segmented} Streaming ;
	} ZModem ;


	/* ZRINIT flags.  Describe receiver capabilities */

#define	CANFDX	1	/* Rx is Full duplex */
#define	CANOVIO	2	/* Rx can overlap I/O */
#define	CANBRK	4	/* Rx can send a break */
#define	CANCRY	010	/* Rx can decrypt */
#define	CANLZW	020	/* Rx can uncompress */
#define	CANFC32	040	/* Rx can use 32-bit crc */
#define	ESCCTL	0100	/* Rx needs control chars escaped */
#define	ESC8	0200	/* Rx needs 8th bit escaped. */

	/* ZSINIT flags.  Describe sender capabilities */

#define	TESCCTL	0100	/* Tx needs control chars escaped */
#define	TESC8	0200	/* Tx needs 8th bit escaped. */


	/* ZFILE transfer flags */

	/* F0 */
#define	ZCBIN	1	/* binary transfer */
#define	ZCNL	2	/* convert NL to local eol convention */
#define	ZCRESUM	3	/* resume interrupted file xfer, or append to a
			   growing file. */

	/* F1 */
#define	ZMNEWL	1	/* transfer if source newer or longer */
#define	ZMCRC	2	/* transfer if different CRC or length */
#define	ZMAPND	3	/* append to existing file, if any */
#define	ZMCLOB	4	/* replace existing file */
#define	ZMNEW	5	/* transfer if source is newer */
#define	ZMDIFF	6	/* transfer if dates or lengths different */
#define	ZMPROT	7	/* protect: transfer only if dest doesn't exist */
#define	ZMCHNG	8	/* change filename if destination exists */
#define	ZMMASK	037	/* mask for above. */
#define	ZMSKNOLOC 0200	/* skip if not present at Rx end */

	/* F2 */
#define	ZTLZW	1	/* lzw compression */
#define	ZTRLE	3	/* run-length encoding */

	/* F3 */
#define	ZCANVHDR 1	/* variable headers ok */
#define	ZRWOVR	4	/* byte position for receive window override/256 */
#define	ZXSPARS	64	/* encoding for sparse file ops. */



	/* ATTN string special characters.  All other characters sent verbose */

#define	ATTNBRK	'\335'	/* send break signal */
#define	ATTNPSE	'\336'	/* pause for one second */


	/* ZStatus() types */

#define	RcvByteCount	0	/* value is # bytes received */
#define	SndByteCount	1	/* value is # bytes sent */
#define	RcvTimeout	2	/* receiver did not respond, aborting */
#define	SndTimeout	3	/* value is # of consecutive send timeouts */
#define	RmtCancel	4	/* remote end has cancelled */
#define	ProtocolErr	5	/* protocol error has occurred, val=hdr */
#define	RemoteMessage	6	/* message from remote end */
#define	DataErr		7	/* data error, val=error count */
#define	FileErr		8	/* error writing file, val=errno */
#define	FileBegin	9	/* file transfer begins, str=name */
#define	FileEnd		10	/* file transfer ends, str=name */
#define	FileSkip	11	/* file being skipped, str=name */


	/* error code definitions [O] means link still open */

#define	ZmDone		-1	/* done */
#define	ZmErrInt	-2	/* internal error */
#define	ZmErrSys	-3	/* system error, see errno */
#define	ZmErrNotOpen	-4	/* communication channel not open */
#define	ZmErrCantOpen	-5	/* can't open file, see errno [O] */
#define	ZmFileTooLong	-6	/* remote filename too long [O] */
#define	ZmFileCantWrite	-7	/* could not write file, see errno */
#define	ZmDataErr	-8	/* too many data errors */
#define	ZmErrInitTo	-10	/* transmitter failed to respond to init req. */
#define	ZmErrSequence	-11	/* packet received out of sequence */
#define	ZmErrCancel	-12	/* cancelled by remote end */
#define	ZmErrRcvTo	-13	/* remote receiver timed out during transfer */
#define	ZmErrSndTo	-14	/* remote sender timed out during transfer */
#define	ZmErrCmdTo	-15	/* remote command timed out */


	/* zmodem-supplied functions: */


extern	int	ZmodemTInit(ZModem *info) ;
extern	int	ZmodemTFile(char *file, char *rmtname,
		  u_int f0, u_int f1, u_int f2, u_int f3,
		  int filesRem, int bytesRem, ZModem *info) ;
extern	int	ZmodemTFinish(ZModem *info) ;
extern	int	ZmodemAbort(ZModem *info) ;
extern	int	ZmodemRInit(ZModem *info) ;
extern	int	ZmodemRcv(u_char *str, int len, ZModem *info) ;
extern	int	ZmodemTimeout(ZModem *info) ;
extern	int	ZmodemAttention(ZModem *info) ;

extern	int	YmodemTInit(ZModem *info) ;
extern	int	XmodemTInit(ZModem *info) ;
extern	int	YmodemRInit(ZModem *info) ;
extern	int	XmodemRInit(ZModem *info) ;

extern	u_long	FileCrc(char *name) ;
extern	char	*sname(ZModem *) ;
extern	char	*sname2(ZMState) ;

#ifdef	DEBUG
extern	FILE	*zmodemlogfile ;
extern	void	zmodemlog(const char *, ...) ;
#else
#define	zmodemlog
#endif




	/* caller-supplied functions: */


extern	int	ZXmitChr(u_char c, ZModem *info) ;
extern	int	ZXmitStr(u_char *str, int len, ZModem *info) ;
extern	void	ZIFlush(ZModem *info) ;
extern	void	ZOFlush(ZModem *info) ;
extern	int	ZAttn(ZModem *info) ;
extern	void	ZStatus(int type, int value, char *status) ;
extern	FILE	*ZOpenFile(char *name, u_long crc, ZModem *info) ;


	/* From here on down, internal to Zmodem package */


	/* ZModem character definitions */

#define	ZDLE	030	/* zmodem escape is CAN */
#define	ZPAD	'*'	/* pad */
#define	ZBIN	'A'	/* introduces 16-bit crc binary header */
#define	ZHEX	'B'	/* introduces 16-bit crc hex header */
#define	ZBIN32	'C'	/* introduces 32-bit crc binary header */
#define	ZBINR32	'D'	/* introduces RLE packed binary frame w/32-bit crc */
#define	ZVBIN	'a'	/* alternate ZBIN */
#define	ZVHEX	'b'	/* alternate ZHEX */
#define	ZVBIN32	'c'	/* alternate ZBIN32 */
#define	ZVBINR32 'd'	/* alternate ZBINR32 */
#define	ZRESC	0177	/* RLE flag/escape character */



	/* ZModem header type codes */

#define	ZRQINIT	0	/* request receive init */
#define	ZRINIT	1	/* receive init */
#define	ZSINIT	2	/* send init sequence, define Attn */
#define	ZACK	3	/* ACK */
#define	ZFILE	4	/* file name, from sender */
#define	ZSKIP	5	/* skip file command, from receiver */
#define	ZNAK	6	/* last packet was garbled */
#define	ZABORT	7	/* abort */
#define	ZFIN	8	/* finish session */
#define	ZRPOS	9	/* resume file from this position, from receiver */
#define	ZDATA	10	/* data packets to follow, from sender */
#define	ZEOF	11	/* end of file, from sender */
#define	ZFERR	12	/* fatal i/o error, from receiver */
#define	ZCRC	13	/* request for file crc, from receiver */
#define	ZCHALLENGE 14	/* "send this number back to me", from receiver */
#define	ZCOMPL	15	/* request is complete */
#define	ZCAN	16	/* other end cancelled with CAN-CAN-CAN-CAN-CAN */
#define	ZFREECNT 17	/* request for free bytes on filesystem */
#define	ZCOMMAND 18	/* command, from sending program */
#define	ZSTDERR	19	/* output this message to stderr */


	/* ZDLE escape sequences */


#define	ZCRCE	'h'	/* CRC next, frame ends, header follows */
#define	ZCRCG	'i'	/* CRC next, frame continues nonstop */
#define	ZCRCQ	'j'	/* CRC next, send ZACK, frame continues nonstop */
#define	ZCRCW	'k'	/* CRC next, send ZACK, frame ends */
#define	ZRUB0	'l'	/* translate to 0177 */
#define	ZRUB1	'm'	/* translate to 0377 */


	/* ascii definitions */

#define	SOH	1	/* ^A */
#define	STX	2	/* ^B */
#define	EOT	4	/* ^D */
#define	ACK	6	/* ^F */
#define	DLE	16	/* ^P */
#define	XON	17	/* ^Q */
#define	XOFF	19	/* ^S */
#define	NAK	21	/* ^U */
#define	SYN	22	/* ^V */
#define	CAN	24	/* ^X */



extern	int	ZXmitHdr() ;
extern	int	ZXmitHdrHex() ;
extern	int	ZXmitHdrBin() ;
extern	int	ZXmitHdrBin32() ;
extern	u_char	*putZdle( u_char *ptr, u_char c, ZModem *info ) ;

extern	u_char	*ZEnc4() ;
extern	u_long	ZDec4() ;


	/* state table entry.  There is one row of the table per
	 * possible state.  Each row is a row of all reasonable
	 * inputs for this state.  The entries are sorted so that
	 * the most common inputs occur first, to reduce search time
	 * Unexpected input is reported and ignored, as it might be
	 * caused by echo or something.
	 *
	 * Extra ZRINIT headers are the receiver trying to resync.
	 */

typedef	struct {
	  int	type ;		/* frame type */
	  int	(*func)() ;	/* transition function */
	  int	IFlush ;	/* flag: flush input first */
	  int	OFlush ;	/* flag: flush output first */
	  ZMState newstate ;	/* new state.  May be overridden by func */
	} StateTable ;




#endif
