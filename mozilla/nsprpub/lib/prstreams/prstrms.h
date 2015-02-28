/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/*
 * Robin J. Maxwell 11-22-96
 */

#ifndef _PRSTRMS_H
#define _PRSTRMS_H

#include "prtypes.h"
#include "prio.h"

#ifdef _MSC_VER
#pragma warning( disable : 4275)
#endif
#include <iostream.h>

#if defined(AIX) && defined(__64BIT__)
typedef long PRstreambuflen;
#else
typedef int PRstreambuflen;
#endif

#if defined (PRFSTREAMS_BROKEN)

// fix it sometime

#define	 PRfilebuf	streambuf
#define  PRifstream	ifstream
#define	 PRofstream	ofstream
#define	 PRfstream	fstream

#else

class PR_IMPLEMENT(PRfilebuf): public streambuf
{
public:
    PRfilebuf();
    PRfilebuf(PRFileDesc *fd);
    PRfilebuf(PRFileDesc *fd, char * buffptr, int bufflen);
    ~PRfilebuf();
    virtual	int	overflow(int=EOF);
    virtual	int	underflow();
    virtual	streambuf *setbuf(char *buff, PRstreambuflen bufflen);
    virtual	streampos seekoff(streamoff, ios::seek_dir, int);
    virtual int sync();
    PRfilebuf *open(const char *name, int mode, int flags);
   	PRfilebuf *attach(PRFileDesc *fd);
    PRfilebuf *close();
   	int	is_open() const {return (_fd != 0);}
    PRFileDesc *fd(){return _fd;}

private:
    PRFileDesc * _fd;
    PRBool _opened;
	PRBool _allocated;
};

class PR_IMPLEMENT(PRifstream): public istream {
public:
	PRifstream();
	PRifstream(const char *, int mode=ios::in, int flags = 0);
	PRifstream(PRFileDesc *);
	PRifstream(PRFileDesc *, char *, int);
	~PRifstream();

	streambuf * setbuf(char *, int);
	PRfilebuf* rdbuf(){return (PRfilebuf*) ios::rdbuf(); }

	void attach(PRFileDesc *fd);
	PRFileDesc *fd() {return rdbuf()->fd();}

	int is_open(){return rdbuf()->is_open();}
	void open(const char *, int mode=ios::in, int flags= 0);
	void close();
};

class PR_IMPLEMENT(PRofstream) : public ostream {
public:
	PRofstream();
	PRofstream(const char *, int mode=ios::out, int flags = 0);
	PRofstream(PRFileDesc *);
	PRofstream(PRFileDesc *, char *, int);
	~PRofstream();

	streambuf * setbuf(char *, int);
	PRfilebuf* rdbuf() { return (PRfilebuf*) ios::rdbuf(); }

	void attach(PRFileDesc *);
	PRFileDesc *fd() {return rdbuf()->fd();}

	int is_open(){return rdbuf()->is_open();}
	void open(const char *, int =ios::out, int = 0);
	void close();
};
	
class PR_IMPLEMENT(PRfstream) : public iostream {
public:
	PRfstream();
	PRfstream(const char *name, int mode, int flags= 0);
	PRfstream(PRFileDesc *fd);
	PRfstream(PRFileDesc *fd, char *buff, int bufflen);
	~PRfstream();

	streambuf * setbuf(char *, int);
	PRfilebuf* rdbuf(){ return (PRfilebuf*) ostream::rdbuf(); }

	void attach(PRFileDesc *);
	PRFileDesc *fd() { return rdbuf()->fd(); }

	int is_open() { return rdbuf()->is_open(); }
	void open(const char *, int, int = 0);
	void close();
};

#endif

#endif /* _PRSTRMS_H */
