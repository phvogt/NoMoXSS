/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */
/*
 * tcp.h  interface to MCS's TCP routines
 */
 
#include <Dialogs.h>
#include <OpenTransport.h>
#include <OpenTptInternet.h>

//#include "MacTCPCommonTypes.h"
//#include "AddressXlation.h"
//#include "TCPPB.h"
//#include "GetMyIPAddr.h"
#include <utime.h>	/* for the UNIX-y struct timeval */

//#include <MacTCP.h>

#ifndef TCP_BUFSIZ
#define TCP_BUFSIZ	8192
#endif /* TCP_BUFSIZ */

typedef struct tcpstream {
	EndpointRef			tcps_ep;			/* OpenTransport end point */
	short				tcps_data;			/* count of packets on read queue */
	short				drefnum;			/* driver ref num, for convenience */
	Boolean				tcps_connected;		/* true if connection was made */
	Boolean				tcps_terminated;	/* true if connection no longer exists */
	InetHost			tcps_remoteaddr;	/* Address of our peer */
	InetPort			tcps_remoteport;	/* Port number of our peer */
	unsigned char		*tcps_buffer;		/* buffer given over to system to use */
} tcpstream, *tcpstreamptr;

#define TCP_IS_TERMINATED( tsp )		(tsp)->tcps_terminated

/*
 * function prototypes
 */
OSStatus	tcp_init(void);
Boolean		tcp_have_opentransport( void );
tcpstream 	*tcpopen( unsigned char * buf, long buflen );
InetPort	tcpconnect( tcpstream *s, InetHost addr, InetPort port );
short		tcpclose( tcpstream *s );
long		tcpread(  tcpstream *s, UInt8 timeout, unsigned char * rbuf,
				unsigned short rbuflen, DialogPtr dlp );
long		tcpwrite( tcpstream *s, unsigned char * wbuf, unsigned short wbuflen );
short		tcpselect( tcpstream *s, struct timeval * timeout );
short		tcpreadready( tcpstream *tsp );
short		tcpwriteready( tcpstream *tsp );
short		tcpgetpeername( tcpstream * tsp, InetHost *addrp, InetPort *portp );

short		gethostinfobyname( char *host, InetHostInfo *hip );
short		gethostinfobyaddr(InetHost addr, InetHostInfo *hip );
short		ipaddr2str( InetHost ipaddr, char *addrstr );
