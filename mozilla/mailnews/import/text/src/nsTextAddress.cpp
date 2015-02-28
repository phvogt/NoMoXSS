/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nscore.h"
#include "nsTextAddress.h"

#include "nsIServiceManager.h"
#include "nsIImportService.h"
#include "nsAddrDatabase.h"
#include "nsAbBaseCID.h"
#include "nsIAbCard.h"
#include "nsReadableUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "TextDebugLog.h"

#define kWhitespace    " \t\b\r\n"

// If we get a line longer than 32K it's just toooooo bad!
#define kTextAddressBufferSz    (64 * 1024)

// ONLY included for the ldif code that was copied over from nsAddressBook.cpp
// Maybe re-write that code and get rid of this? (isspace is what is needed)
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"


nsTextAddress::nsTextAddress()
{
    m_database = nsnull;
    m_fieldMap = nsnull;
    m_LFCount = 0;
    m_CRCount = 0;
}

nsTextAddress::~nsTextAddress()
{
    NS_IF_RELEASE( m_database);
    NS_IF_RELEASE( m_fieldMap);
}


void nsTextAddress::ConvertToUnicode( const char *pStr, nsString& str)
{
    if (!m_pService) {
        m_pService = do_GetService( NS_IMPORTSERVICE_CONTRACTID);
    }
    if (m_pService) {
        m_pService->SystemStringToUnicode( pStr, str);
    }
    else
        str.AssignWithConversion( pStr);
}

nsresult nsTextAddress::ImportLDIF( PRBool *pAbort, const PRUnichar *pName, nsIFileSpec *pSrc, nsIAddrDatabase *pDb, nsString& errors, PRUint32 *pProgress)
{
    NS_IF_RELEASE( m_database);
    NS_IF_RELEASE( m_fieldMap);
    m_database = pDb;
    m_fieldMap = nsnull;
    NS_ADDREF( m_database);
    
    nsresult rv = pSrc->OpenStreamForReading();
    if (NS_FAILED( rv)) {
        IMPORT_LOG0( "*** Error opening address file for reading\n");
        return( rv);
    }

    rv = ParseLdifFile(pSrc, pProgress);
  NS_ASSERTION(NS_SUCCEEDED(rv), "parse ldif failed");

    pSrc->CloseStream();

  rv = pDb->Commit(nsAddrDBCommitType::kLargeCommit);
    return rv;
}


nsresult nsTextAddress::ImportAddresses( PRBool *pAbort, const PRUnichar *pName, nsIFileSpec *pSrc, nsIAddrDatabase *pDb, nsIImportFieldMap *fieldMap, nsString& errors, PRUint32 *pProgress)
{
    // Open the source file for reading, read each line and process it!
    NS_IF_RELEASE( m_database);
    NS_IF_RELEASE( m_fieldMap);
    m_database = pDb;
    m_fieldMap = fieldMap;
    NS_ADDREF( m_fieldMap);
    NS_ADDREF( m_database);

    nsresult rv = pSrc->OpenStreamForReading();
    if (NS_FAILED( rv)) {
        IMPORT_LOG0( "*** Error opening address file for reading\n");
        return( rv);
    }
    
    char *pLine = new char[kTextAddressBufferSz];
    PRBool    eof = PR_FALSE;
    rv = pSrc->Eof( &eof);
    if (NS_FAILED( rv)) {
        IMPORT_LOG0( "*** Error checking address file for eof\n");
        pSrc->CloseStream();
        return( rv);
    }
    
    PRInt32    loc;
    PRInt32    lineLen = 0;
    while (!(*pAbort) && !eof && NS_SUCCEEDED( rv)) {
        rv = pSrc->Tell( &loc);
        if (NS_SUCCEEDED( rv) && pProgress)
            *pProgress = (PRUint32)loc;
        rv = ReadRecord( pSrc, pLine, kTextAddressBufferSz, m_delim, &lineLen);
        if (NS_SUCCEEDED( rv)) {
            rv = ProcessLine( pLine, strlen( pLine), errors);
            if (NS_FAILED( rv)) {
                IMPORT_LOG0( "*** Error processing text record.\n");
            }
            else
                rv = pSrc->Eof( &eof);
        }
    }
    
    rv = pSrc->CloseStream();
    
    delete [] pLine;
    
    if (!eof) {
        IMPORT_LOG0( "*** Error reading the address book, didn't reach the end\n");
        return( NS_ERROR_FAILURE);
    }
    
    rv = pDb->Commit(nsAddrDBCommitType::kLargeCommit);
    return rv;
}



nsresult nsTextAddress::ReadRecord( nsIFileSpec *pSrc, char *pLine, PRInt32 bufferSz, char delim, PRInt32 *pLineLen)
{
    PRBool        wasTruncated;
    char *        pRead;
    PRInt32        lineLen = 0;
    nsresult    rv;
    do {
        if (lineLen) {
            if ((lineLen + 2) < bufferSz) {
                memcpy( pLine + lineLen, "\x0D\x0A", 2);
                lineLen += 2;
                pLine[lineLen] = 0;
            }
        }
        wasTruncated = PR_FALSE;
        pRead = pLine;
        pRead += lineLen;
        rv = pSrc->ReadLine( &pRead, bufferSz - lineLen, &wasTruncated);
        if (wasTruncated) {
            pLine[bufferSz - 1] = 0;
            IMPORT_LOG0( "Unable to read line from file, buffer too small\n");
            rv = NS_ERROR_FAILURE;
        }
        else if (NS_SUCCEEDED( rv)) {
            lineLen = strlen( pLine);
        }
    } while (NS_SUCCEEDED( rv) && !IsLineComplete( pLine, lineLen, delim));

    *pLineLen = lineLen;
    return( rv);
}


nsresult nsTextAddress::ReadRecordNumber( nsIFileSpec *pSrc, char *pLine, PRInt32 bufferSz, char delim, PRInt32 *pLineLen, PRInt32 rNum)
{
    PRInt32    rIndex = 0;
    nsresult rv = pSrc->Seek( 0);
    if (NS_FAILED( rv))
        return( rv);
    
    PRBool    eof = PR_FALSE;

    while (!eof && (rIndex <= rNum)) {
        if (NS_FAILED( rv = ReadRecord( pSrc, pLine, bufferSz, delim, pLineLen)))
            return( rv);
        if (rIndex == rNum)
            return( NS_OK);
        rIndex++;
        rv = pSrc->Eof( &eof);
        if (NS_FAILED( rv))
            return( rv);
    }

    return( NS_ERROR_FAILURE);
}




/*
    Find out if the given line appears to be a complete record or
    if it needs more data because the line ends with a quoted value non-terminated
*/
PRBool nsTextAddress::IsLineComplete( const char *pLine, PRInt32 len, char delim)
{
    char    tab = 9;
    if (delim == tab)
        tab = 0;

    PRBool    quoted = PR_FALSE;
    PRBool    wasDelim = PR_FALSE;

    while (len) {
        // always skip white space?
        while (len && ((*pLine == ' ') || (*pLine == tab))) {
            pLine++;
            len--;
        }
        if (len && wasDelim && (*pLine == '"')) {
            quoted = PR_TRUE;
            wasDelim = PR_FALSE;
            pLine++;
            len--;
        }
        else if (len && quoted && (*pLine == '"')) {
            quoted = PR_FALSE;
            pLine++;
            len--;
        }
        else if (len) {
            if (!quoted && (*pLine == delim))
                wasDelim = PR_TRUE;
            else
                wasDelim = PR_FALSE;
            pLine++;
            len--;
        }
    }
    if (quoted)
        return( PR_FALSE);
    return( PR_TRUE);
}

PRInt32 nsTextAddress::CountFields( const char *pLine, PRInt32 maxLen, char delim)
{
    const char *pChar = pLine;
    PRInt32        len = 0;
    PRInt32        count = 0;
    char        tab = 9;

    if (delim == tab)
        tab = 0;

    while (len < maxLen) {
        while (((*pChar == ' ') || (*pChar == tab)) && (len < maxLen)) {
            pChar++;
            len++;
        }
        if ((len < maxLen) && (*pChar == '"')) {
            pChar++;
            len++;
            while ((len < maxLen) && (*pChar != '"')) {
                len++;
                pChar++;
                if (((len + 1) < maxLen) && (*pChar == '"') && (*(pChar + 1) == '"')) {
                    len += 2;
                    pChar += 2;
                }
            }
            if (len < maxLen) {
                pChar++;
                len++;
            }
        }        
        while ((len < maxLen) && (*pChar != delim)) {
            len++;
            pChar++;
        }
        
        count++;
        pChar++;
        len++;
    }

    return( count);
}

PRBool nsTextAddress::GetField( const char *pLine, PRInt32 maxLen, PRInt32 index, nsCString& field, char delim)
{
    PRBool result = PR_FALSE;
    const char *pChar = pLine;
    PRInt32        len = 0;
    char        tab = 9;

    field.Truncate();

    if (delim == tab)
        tab = 0;

    while (index && (len < maxLen)) {
        while (((*pChar == ' ') || (*pChar == tab)) && (len < maxLen)) {
            pChar++;
            len++;
        }
        if (len >= maxLen)
            break;
        if (*pChar == '"') {
            len = -1;
            do {
                len++;
                pChar++;
                if (((len + 1) < maxLen) && (*pChar == '"') && (*(pChar + 1) == '"')) {
                    len += 2;
                    pChar += 2;
                }
            } while ((len < maxLen) && (*pChar != '"'));
            if (len < maxLen) {
                pChar++;
                len++;
            }
        }
        if (len >= maxLen)
            break;
        
        while ((len < maxLen) && (*pChar != delim)) {
            len++;
            pChar++;
        }
        
        if (len >= maxLen)
            break;

        index--;
        pChar++;
        len++;
    }

    if (len >= maxLen) {
        return( result);
    }

    result = PR_TRUE;

    while ((len < maxLen) && ((*pChar == ' ') || (*pChar == tab))) {
        len++;
        pChar++;
    }

    const char *pStart = pChar;
    PRInt32        fLen = 0;
    PRBool        quoted = PR_FALSE;
    if (*pChar == '"') {
        pStart++;
        fLen = -1;
        do {
            pChar++;
            len++;
            fLen++;
            if (((len + 1) < maxLen) && (*pChar == '"') && (*(pChar + 1) == '"')) {
                quoted = PR_TRUE;
                len += 2;
                pChar += 2;
                fLen += 2;
            }
        } while ((len < maxLen) && (*pChar != '"'));
    }
    else {
        while ((len < maxLen) && (*pChar != delim)) {
            pChar++;
            len++;
            fLen++;
        }
    }

    if (!fLen) {
        return( result);
    }

    field.Append( pStart, fLen);
    field.Trim( kWhitespace);

    if (quoted) {
        field.ReplaceSubstring( "\"\"", "\"");
    }

    return( result);
}


void nsTextAddress::SanitizeSingleLine( nsCString& val)
{
    val.ReplaceSubstring( "\x0D\x0A", ", ");
    val.ReplaceChar( 13, ' ');
    val.ReplaceChar( 10, ' ');
}


nsresult nsTextAddress::DetermineDelim( nsIFileSpec *pSrc)
{
    nsresult rv = pSrc->OpenStreamForReading();
    if (NS_FAILED( rv)) {
        IMPORT_LOG0( "*** Error opening address file for reading\n");
        return( rv);
    }
    
    char *pLine = new char[kTextAddressBufferSz];
    PRBool    eof = PR_FALSE;
    rv = pSrc->Eof( &eof);
    if (NS_FAILED( rv)) {
        IMPORT_LOG0( "*** Error checking address file for eof\n");
        pSrc->CloseStream();
        return( rv);
    }
    
    PRBool    wasTruncated = PR_FALSE;
    PRInt32    lineLen = 0;
    PRInt32    lineCount = 0;
    PRInt32    tabCount = 0;
    PRInt32    commaCount = 0;
    PRInt32    tabLines = 0;
    PRInt32    commaLines = 0;

    while (!eof && NS_SUCCEEDED( rv) && (lineCount < 100)) {
        wasTruncated = PR_FALSE;
        rv = pSrc->ReadLine( &pLine, kTextAddressBufferSz, &wasTruncated);
        if (wasTruncated)
            pLine[kTextAddressBufferSz - 1] = 0;
        if (NS_SUCCEEDED( rv)) {
            lineLen = strlen( pLine);
            tabCount = CountFields( pLine, lineLen, 9);
            commaCount = CountFields( pLine, lineLen, ',');
            if (tabCount > commaCount)
                tabLines++;
            else if (commaCount)
                commaLines++;
            rv = pSrc->Eof( &eof);
        }
        lineCount++;
    }
    
    rv = pSrc->CloseStream();
    
    delete [] pLine;
    
    if (tabLines > commaLines)
        m_delim = 9;
    else
        m_delim = ',';

    return( NS_OK);
}


/*
    This is where the real work happens!
    Go through the field map and set the data in a new database row
*/
nsresult nsTextAddress::ProcessLine( const char *pLine, PRInt32 len, nsString& errors)
{
    if (!m_fieldMap) {
        IMPORT_LOG0( "*** Error, text import needs a field map\n");
        return( NS_ERROR_FAILURE);
    }

    nsresult rv;
    
    // Wait until we get our first non-empty field, then create a new row,
    // fill in the data, then add the row to the database.
        

    nsIMdbRow *    newRow = nsnull;
    nsString    uVal;
    nsCString    fieldVal;
    PRInt32        fieldNum;
    PRInt32        numFields = 0;
    PRBool        active;
    rv = m_fieldMap->GetMapSize( &numFields);
    for (PRInt32 i = 0; (i < numFields) && NS_SUCCEEDED( rv); i++) {
        active = PR_FALSE;
        rv = m_fieldMap->GetFieldMap( i, &fieldNum);
        if (NS_SUCCEEDED( rv))
            rv = m_fieldMap->GetFieldActive( i, &active);
        if (NS_SUCCEEDED( rv) && active) {
            if (GetField( pLine, len, i, fieldVal, m_delim)) {
                if (!fieldVal.IsEmpty()) {
                    if (!newRow) {
                        rv = m_database->GetNewRow( &newRow);
                        if (NS_FAILED( rv)) {
                            IMPORT_LOG0( "*** Error getting new address database row\n");
                        }
                    }
                    if (newRow) {
                        ConvertToUnicode( fieldVal.get(), uVal);
                        rv = m_fieldMap->SetFieldValue( m_database, newRow, fieldNum, uVal.get());
                    }
                }
            }
            else
                break;
            
        }
        else {
            if (active) {
                IMPORT_LOG1( "*** Error getting field map for index %ld\n", (long) i);
            }
        }

    }
    
    if (NS_SUCCEEDED( rv)) {
        if (newRow) {
            rv = m_database->AddCardRowToDB( newRow);
            // Release newRow????
        }
    }
    else {
        // Release newRow??
    }

    return( rv);
}

// Some common ldif fields, it an ldif file has NONE of these entries
// then it is most likely NOT an ldif file!
static const char *const sLDIFFields[] = {
    "objectclass",
    "sn",
    "dn",
    "cn",
    "givenName",
    "mail",
    nsnull
};
#define kMaxLDIFLen        14

// Count total number of legal ldif fields and records in the first 100 lines of the 
// file and if the average legal ldif field is 3 or higher than it's a valid ldif file.
nsresult nsTextAddress::IsLDIFFile( nsIFileSpec *pSrc, PRBool *pIsLDIF)
{
    *pIsLDIF = PR_FALSE;

    nsresult rv = pSrc->OpenStreamForReading();
    if (NS_FAILED( rv)) {
        IMPORT_LOG0( "*** Error opening address file for reading\n");
        return( rv);
    }
    
    char *pLine = new char[kTextAddressBufferSz];
    PRBool    eof = PR_FALSE;
    rv = pSrc->Eof( &eof);
    if (NS_FAILED( rv)) {
        IMPORT_LOG0( "*** Error checking address file for eof\n");
        pSrc->CloseStream();
        return( rv);
    }
    
    PRBool    wasTruncated = PR_FALSE;
    PRInt32    lineLen = 0;
    PRInt32    lineCount = 0;
    PRInt32    ldifFields = 0;  // total number of legal ldif fields.
    char    field[kMaxLDIFLen];
    PRInt32    fLen = 0;
    char *    pChar;
    PRInt32    recCount = 0;  // total number of records.
    PRInt32    i;
    PRBool    gotLDIF = PR_FALSE;

    while (!eof && NS_SUCCEEDED( rv) && (lineCount < 100)) {
        wasTruncated = PR_FALSE;
        rv = pSrc->ReadLine( &pLine, kTextAddressBufferSz, &wasTruncated);
        if (wasTruncated)
            pLine[kTextAddressBufferSz - 1] = 0;
        if (NS_SUCCEEDED( rv)) {
            lineLen = strlen( pLine);            
            pChar = pLine;
            if (!lineLen && gotLDIF) {
                recCount++;
                gotLDIF = PR_FALSE;
            }
                    
            if (lineLen && (*pChar != ' ') && (*pChar != 9)) {
                fLen = 0;
                while (lineLen && (fLen < (kMaxLDIFLen - 1)) && (*pChar != ':')) {
                    field[fLen] = *pChar;
                    pChar++;
                    fLen++;
                    lineLen--;
                }
                
                field[fLen] = 0;
                if (lineLen && (*pChar == ':') && (fLen < (kMaxLDIFLen - 1))) {
                    // see if this is an ldif field (case insensitive)?
                    i = 0;
                    while (sLDIFFields[i]) {
                        if (!nsCRT::strcasecmp( sLDIFFields[i], field)) {
                            ldifFields++;
                            gotLDIF = PR_TRUE;
                            break;
                        }
                        i++;
                    }
                    
                }
            }

            rv = pSrc->Eof( &eof);
        }
        lineCount++;
    }

    // If we just saw ldif address, increment recCount.
    if (gotLDIF)
      recCount++;
    
    rv = pSrc->CloseStream();
    
    delete [] pLine;
    
    if (recCount > 1)
      ldifFields /= recCount;

    // If the average field number >= 3 then it's a good ldif file.
    if (ldifFields >= 3)
      *pIsLDIF = PR_TRUE;

    return( NS_OK);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// This is all code from Candice Huang from the nsAddressBook.cpp.  It doesn't
// look too bad, maybe change the use of a string to use a faster buffer, and
// it's probably possible to use the field map field numbers to build a table
// of ldif fields to values rather than having the big switch statement.



#define RIGHT2            0x03
#define RIGHT4            0x0f
#define CONTINUED_LINE_MARKER    '\001'
#define IS_SPACE(VAL)                \
    (((((intn)(VAL)) & 0x7f) == ((intn)(VAL))) && isspace((intn)(VAL)) )

// XXX TODO fix me
// use the NSPR base64 library.  see plbase64.h
// see bug #145367
static unsigned char b642nib[0x80] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
    0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff
};

/*
 * str_parse_line - takes a line of the form "type:[:] value" and splits it
 * into components "type" and "value".  if a double colon separates type from
 * value, then value is encoded in base 64, and parse_line un-decodes it
 * (in place) before returning.
 * in LDIF, non-ASCII data is treated as base64 encoded UTF-8 
 */

nsresult nsTextAddress::str_parse_line(
    char    *line,
    char    **type,
    char    **value,
    int        *vlen
)
{
    char    *p, *s, *d, *byte, *stop;
    char    nib;
    int    i, b64;

    /* skip any leading space */
    while ( IS_SPACE( *line ) ) {
        line++;
    }
    *type = line;

    for ( s = line; *s && *s != ':'; s++ )
        ;    /* NULL */
    if ( *s == '\0' ) {
        return NS_ERROR_FAILURE;
    }

    /* trim any space between type and : */
    for ( p = s - 1; p > line && isspace( *p ); p-- ) {
        *p = '\0';
    }
    *s++ = '\0';

    /* check for double : - indicates base 64 encoded value */
    if ( *s == ':' ) {
        s++;
        b64 = 1;

    /* single : - normally encoded value */
    } else {
        b64 = 0;
    }

    /* skip space between : and value */
    while ( IS_SPACE( *s ) ) {
        s++;
    }

    /* if no value is present, error out */
    if ( *s == '\0' ) {
        return NS_ERROR_FAILURE;
    }

    /* check for continued line markers that should be deleted */
    for ( p = s, d = s; *p; p++ ) {
        if ( *p != CONTINUED_LINE_MARKER )
            *d++ = *p;
    }
    *d = '\0';

    *value = s;
    if ( b64 ) {
        stop = PL_strchr( s, '\0' );
        byte = s;
        for ( p = s, *vlen = 0; p < stop; p += 4, *vlen += 3 ) {
            for ( i = 0; i < 3; i++ ) {
                if ( p[i] != '=' && (p[i] & 0x80 ||
                    b642nib[ p[i] & 0x7f ] > 0x3f) ) {
                    return NS_ERROR_FAILURE;
                }
            }

            /* first digit */
            nib = b642nib[ p[0] & 0x7f ];
            byte[0] = nib << 2;
            /* second digit */
            nib = b642nib[ p[1] & 0x7f ];
            byte[0] |= nib >> 4;
            byte[1] = (nib & RIGHT4) << 4;
            /* third digit */
            if ( p[2] == '=' ) {
                *vlen += 1;
                break;
            }
            nib = b642nib[ p[2] & 0x7f ];
            byte[1] |= nib >> 2;
            byte[2] = (nib & RIGHT2) << 6;
            /* fourth digit */
            if ( p[3] == '=' ) {
                *vlen += 2;
                break;
            }
            nib = b642nib[ p[3] & 0x7f ];
            byte[2] |= nib;

            byte += 3;
        }
        s[ *vlen ] = '\0';
    } else {
        *vlen = (int) (d - s);
    }

    return NS_OK;
}

/*
 * str_getline - return the next "line" (minus newline) of input from a
 * string buffer of lines separated by newlines, terminated by \n\n
 * or \0.  this routine handles continued lines, bundling them into
 * a single big line before returning.  if a line begins with a white
 * space character, it is a continuation of the previous line. the white
 * space character (nb: only one char), and preceeding newline are changed
 * into CONTINUED_LINE_MARKER chars, to be deleted later by the
 * str_parse_line() routine above.
 *
 * it takes a pointer to a pointer to the buffer on the first call,
 * which it updates and must be supplied on subsequent calls.
 */

char * nsTextAddress::str_getline( char **next )
{
    char    *lineStr;
    char    c;

    if ( *next == nsnull || **next == '\n' || **next == '\0' ) {
        return( nsnull);
    }

    lineStr = *next;
    while ( (*next = PL_strchr( *next, '\n' )) != NULL ) {
        c = *(*next + 1);
        if ( IS_SPACE ( c ) && c != '\n' ) {
            **next = CONTINUED_LINE_MARKER;
            *(*next+1) = CONTINUED_LINE_MARKER;
        } else {
            *(*next)++ = '\0';
            break;
        }
    }

    return( lineStr );
}

/*
 * get one ldif record
 * 
 */
nsresult nsTextAddress::GetLdifStringRecord(char* buf, PRInt32 len, PRInt32& stopPos)
{
    for (; stopPos < len; stopPos++) 
    {
        char c = buf[stopPos];

        if (c == 0xA)
        {
            m_LFCount++;
        }
        else if (c == 0xD)
        {
            m_CRCount++;
        }
        else if ( c != 0xA && c != 0xD)
        {
            if (m_LFCount == 0 && m_CRCount == 0)
                 m_ldifLine.Append(c);
            else if (( m_LFCount > 1) || ( m_CRCount > 2 && m_LFCount ) ||
                ( !m_LFCount && m_CRCount > 1 ))
            {
                return NS_OK;
            }
            else if ((m_LFCount == 1 || m_CRCount == 1))
            {
                m_ldifLine.Append('\n');
                m_ldifLine.Append(c);
                m_LFCount = 0;
                m_CRCount = 0;
            }
        }
    }

    if ((stopPos == len) && (m_LFCount > 1) || (m_CRCount > 2 && m_LFCount) ||
        (!m_LFCount && m_CRCount > 1))
        return NS_OK;
    else
        return NS_ERROR_FAILURE;
}

nsresult nsTextAddress::ParseLdifFile( nsIFileSpec *pSrc, PRUint32 *pProgress)
{
    char buf[1024];
    char* pBuf = &buf[0];
    PRInt32 startPos = 0;
    PRInt32 len = 0;
    PRBool bEof = PR_FALSE;
    nsVoidArray listPosArray;   // where each list/group starts in ldif file
    nsVoidArray listSizeArray;  // size of the list/group info
    PRInt32 savedStartPos = 0;
    PRInt32 filePos = 0;

    while (NS_SUCCEEDED(pSrc->Eof(&bEof)) && !bEof)
    {
        if (NS_SUCCEEDED(pSrc->Read(&pBuf, (PRInt32)sizeof(buf), &len)) && len > 0)
        {
            startPos = 0;

            while (NS_SUCCEEDED(GetLdifStringRecord(buf, len, startPos)))
            {
                 if (m_ldifLine.Find("groupOfNames") == kNotFound)
                    AddLdifRowToDatabase(PR_FALSE);
                else
                {
                    //keep file position for mailing list
                    listPosArray.AppendElement((void*)savedStartPos);
                    listSizeArray.AppendElement((void*)(filePos + startPos-savedStartPos));
                    ClearLdifRecordBuffer();
                }
                savedStartPos = filePos + startPos;
            }
            filePos += len;
      *pProgress = (PRUint32)filePos;
        }
    }
    //last row
    if (!m_ldifLine.IsEmpty() && m_ldifLine.Find("groupOfNames") == kNotFound)
        AddLdifRowToDatabase(PR_FALSE); 

    // mail Lists
    PRInt32 i, pos, size;
    PRInt32 listTotal = listPosArray.Count();
    char *listBuf;
    ClearLdifRecordBuffer();  // make sure the buffer is clean
    for (i = 0; i < listTotal; i++)
    {
        pos  = NS_PTR_TO_INT32(listPosArray.ElementAt(i));
        size = NS_PTR_TO_INT32(listSizeArray.ElementAt(i));
        if (NS_SUCCEEDED(pSrc->Seek(pos)))
        {
            // Allocate enough space for the lists/groups as the size varies.
            listBuf = (char *) PR_Malloc(size);
            if (!listBuf)
              continue;
            if (NS_SUCCEEDED(pSrc->Read(&listBuf, size, &len)) && len > 0)
            {
                startPos = 0;

                while (NS_SUCCEEDED(GetLdifStringRecord(listBuf, len, startPos)))
                {
                    if (m_ldifLine.Find("groupOfNames") != kNotFound)
                    {
                        AddLdifRowToDatabase(PR_TRUE);
                        if (NS_SUCCEEDED(pSrc->Seek(0)))
                            break;
                    }
                }
            }
            PR_FREEIF(listBuf);
        }
    }
    return NS_OK;
}

void nsTextAddress::AddLdifRowToDatabase(PRBool bIsList)
{
    // If no data to process then reset CR/LF counters and return.
    if (m_ldifLine.IsEmpty())
    {
      m_LFCount = 0;
      m_CRCount = 0;
      return;
    }

    nsCOMPtr <nsIMdbRow> newRow;
    if (m_database)
    {
        if (bIsList)
            m_database->GetNewListRow(getter_AddRefs(newRow)); 
        else
            m_database->GetNewRow(getter_AddRefs(newRow)); 

        if (!newRow)
            return;
    }
    else
        return;

    char* cursor = ToNewCString(m_ldifLine); 
    char* saveCursor = cursor;  /* keep for deleting */ 
    char* line = 0; 
    char* typeSlot = 0; 
    char* valueSlot = 0; 
    int length = 0;  // the length  of an ldif attribute
    while ( (line = str_getline(&cursor)) != nsnull)
    {
        if ( str_parse_line(line, &typeSlot, &valueSlot, &length) == 0) {
            AddLdifColToDatabase(newRow, typeSlot, valueSlot, bIsList);
        }
        else
            continue; // parse error: continue with next loop iteration
    }
    nsMemory::Free(saveCursor);
    m_database->AddCardRowToDB(newRow);    

    if (bIsList)
        m_database->AddListDirNode(newRow);
        
    // Clear buffer for next record
    ClearLdifRecordBuffer();
}

void nsTextAddress::ClearLdifRecordBuffer()
{
  if (!m_ldifLine.IsEmpty())
  {
      m_ldifLine.Truncate();
      m_LFCount = 0;
      m_CRCount = 0;
  }
}

static void SplitCRLFAddressField(nsCString &inputAddress, nsCString &outputLine1, nsCString &outputLine2)
{
  PRInt32 crlfPos = inputAddress.Find("\r\n");
  if (crlfPos != kNotFound)
  {
    inputAddress.Left(outputLine1, crlfPos);
    inputAddress.Right(outputLine2, inputAddress.Length() - (crlfPos + 2));
  }
  else
    outputLine1.Assign(inputAddress);
}

// We have two copies of this function in the code, one here for import and 
// the other one in addrbook/src/nsAddressBook.cpp for migrating.  If ths 
// function need modification, make sure change in both places until we resolve
// this problem.
void nsTextAddress::AddLdifColToDatabase(nsIMdbRow* newRow, char* typeSlot, char* valueSlot, PRBool bIsList)
{
  nsCAutoString colType(typeSlot);
  nsCAutoString column(valueSlot);

  // 4.x exports attributes like "givenname", 
  // mozilla does "givenName" to be compliant with RFC 2798
  ToLowerCase(colType);

    mdb_u1 firstByte = (mdb_u1)(colType.get())[0];
    switch ( firstByte )
    {
    case 'b':
      if (colType.Equals("birthyear"))
        m_database->AddBirthYear(newRow, column.get());
      break; // 'b'

    case 'c':
      if (colType.Equals("cn") || colType.Equals("commonname"))
      {
        if (bIsList)
          m_database->AddListName(newRow, column.get());
        else
          m_database->AddDisplayName(newRow, column.get());
      }
      else if (colType.Equals("countryname") )
        m_database->AddWorkCountry(newRow, column.get());

      else if (colType.Equals("cellphone") )
        m_database->AddCellularNumber(newRow, column.get());

      else if (colType.Equals("carphone") )
        m_database->AddCellularNumber(newRow, column.get());
        
      else if (colType.Equals("custom1") )
        m_database->AddCustom1(newRow, column.get());
        
      else if (colType.Equals("custom2") )
        m_database->AddCustom2(newRow, column.get());
        
      else if (colType.Equals("custom3") )
        m_database->AddCustom3(newRow, column.get());
        
      else if (colType.Equals("custom4") )
        m_database->AddCustom4(newRow, column.get());
        
      else if (colType.Equals("company") )
        m_database->AddCompany(newRow, column.get());

    else if (colType.Equals("c"))
        m_database->AddWorkCountry(newRow, column.get());
      break; // 'c'

    case 'd':
      if (colType.Equals("description") )
      {
        if (bIsList)
          m_database->AddListDescription(newRow, column.get());
        else
          m_database->AddNotes(newRow, column.get());
      }

      else if (colType.Equals("department") )
        m_database->AddDepartment(newRow, column.get());

      break; // 'd'

    case 'e':

      break; // 'e'

    case 'f':

      if (colType.Equals("fax") ||
        colType.Equals("facsimiletelephonenumber") )
        m_database->AddFaxNumber(newRow, column.get());
      break; // 'f'

    case 'g':
      if (colType.Equals("givenname"))
        m_database->AddFirstName(newRow, column.get());

      break; // 'g'

    case 'h':
      if (colType.Equals("homephone") )
        m_database->AddHomePhone(newRow, column.get());
        else if (colType.Equals("homepostaladdress") )
        {
          nsCAutoString homeAddr1, homeAddr2;
          SplitCRLFAddressField(column, homeAddr1, homeAddr2);
          m_database->AddHomeAddress(newRow, homeAddr1.get());
          m_database->AddHomeAddress2(newRow, homeAddr2.get());
        }
      else if (colType.Equals("homeurl") )
        m_database->AddWebPage2(newRow, column.get());
      break; // 'h'

    case 'i':
      break; // 'i'

    case 'j':
      break; // 'j'

    case 'k':
      break; // 'k'

    case 'l':
      if (colType.Equals("l") || colType.Equals("locality") )
        m_database->AddWorkCity(newRow, column.get());

      break; // 'l'

    case 'm':
      if (colType.Equals("mail") )
        m_database->AddPrimaryEmail(newRow, column.get());
      else if ( colType.Equals("mobile") )
        m_database->AddCellularNumber(newRow, column.get());
      else if (colType.Equals("member") && bIsList )
        m_database->AddLdifListMember(newRow, column.get());
      // check if it starts with our magic prefix
      else if (colType.Find(MOZ_AB_LDIF_PREFIX) == 0)
        m_database->AddRowValue(newRow, colType, NS_ConvertUTF8toUCS2(column));
      break; // 'm'

    case 'n':
      if (colType.Equals("notes") )
        m_database->AddNotes(newRow, column.get());

      break; // 'n'

    case 'o':
      if (colType.Equals("objectclass"))
        break;

      else if (colType.Equals("ou") || colType.Equals("orgunit") )
        m_database->AddDepartment(newRow, column.get());

      else if (colType.Equals("o") ) // organization
        m_database->AddCompany(newRow, column.get());

      break; // 'o'

    case 'p':
      if (colType.Equals("postalcode") )
        m_database->AddWorkZipCode(newRow, column.get());

      else if (colType.Equals("postofficebox") || colType.Equals("postaladdress")) 
      { 
          nsCAutoString workAddr1, workAddr2;
          SplitCRLFAddressField(column, workAddr1, workAddr2);
          m_database->AddWorkAddress(newRow, workAddr1.get());
          m_database->AddWorkAddress2(newRow, workAddr2.get());
      }
      else if (colType.Equals("pager") ||
        colType.Equals("pagerphone") )
        m_database->AddPagerNumber(newRow, column.get());
                    
      break; // 'p'

    case 'r':
      if (colType.Equals("region") )
        m_database->AddWorkState(newRow, column.get());

      break; // 'r'

    case 's':
      if ( colType.Equals("sn") || colType.Equals("surname") )
        m_database->AddLastName(newRow, column.get());

      else if ( colType.Equals("streetaddress") )
      {
        nsCAutoString workAddr1, workAddr2;
        SplitCRLFAddressField(column, workAddr1, workAddr2);
        m_database->AddWorkAddress(newRow, workAddr1.get());
        m_database->AddWorkAddress2(newRow, workAddr2.get());
      }
      else if ( colType.Equals("st") )
        m_database->AddWorkState(newRow, column.get());

      break; // 's'

    case 't':
      if ( colType.Equals("title") )
        m_database->AddJobTitle(newRow, column.get());

      else if ( colType.Equals("telephonenumber") )
        m_database->AddWorkPhone(newRow, column.get());

      break; // 't'

    case 'u':

        if ( colType.Equals("uniquemember") && bIsList )
            m_database->AddLdifListMember(newRow, column.get());

      break; // 'u'

    case 'v':

      break; // 'v'

    case 'w':
      if ( colType.Equals("workurl") )
        m_database->AddWebPage1(newRow, column.get());

      break; // 'w'

    case 'x':
      if ( colType.Equals("xmozillanickname") )
      {
        if (bIsList)
          m_database->AddListNickName(newRow, column.get());
        else
          m_database->AddNickName(newRow, column.get());
      }
      else if ( colType.Equals("xmozillausehtmlmail") )
      {
        ToLowerCase(column);
        if (column.Equals("true"))
            m_database->AddPreferMailFormat(newRow, nsIAbPreferMailFormat::html);
    else if (column.Equals("false"))
            m_database->AddPreferMailFormat(newRow, nsIAbPreferMailFormat::plaintext);
        else
            m_database->AddPreferMailFormat(newRow, nsIAbPreferMailFormat::unknown);
      }

      break; // 'x'

    case 'z':
      if (colType.Equals("zip") ) // alias for postalcode
        m_database->AddWorkZipCode(newRow, column.get());

      break; // 'z'

    default:
      break; // default
    }
}

