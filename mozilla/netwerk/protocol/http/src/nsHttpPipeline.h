/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is Mozilla.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications.  Portions created by Netscape Communications are
 * Copyright (C) 2001 by Netscape Communications.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Darin Fisher <darin@netscape.com> (original author)
 */

#ifndef nsHttpPipeline_h__
#define nsHttpPipeline_h__

#include "nsHttp.h"
#include "nsAHttpConnection.h"
#include "nsAHttpTransaction.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsVoidArray.h"
#include "nsCOMPtr.h"

class nsHttpPipeline : public nsAHttpConnection
                     , public nsAHttpTransaction
                     , public nsAHttpSegmentReader
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSAHTTPCONNECTION
    NS_DECL_NSAHTTPTRANSACTION
    NS_DECL_NSAHTTPSEGMENTREADER

    nsHttpPipeline();
    virtual ~nsHttpPipeline();

    nsresult AddTransaction(nsAHttpTransaction *);

private:
    nsresult FillSendBuf();
    
    static NS_METHOD ReadFromPipe(nsIInputStream *, void *, const char *,
                                  PRUint32, PRUint32, PRUint32 *);

    // convenience functions
    nsAHttpTransaction *Request(PRInt32 i)
    {
        if (mRequestQ.Count() == 0)
            return nsnull;

        return (nsAHttpTransaction *) mRequestQ[i];
    }
    nsAHttpTransaction *Response(PRInt32 i)
    {
        if (mResponseQ.Count() == 0)
            return nsnull;

        return (nsAHttpTransaction *) mResponseQ[i];
    }

    nsAHttpConnection *mConnection;
    nsVoidArray        mRequestQ;  // array of transactions
    nsVoidArray        mResponseQ; // array of transactions
    nsresult           mStatus;

    // these flags indicate whether or not the first request or response
    // is partial.  a partial request means that Request(0) has been 
    // partially written out to the socket.  a partial response means
    // that Response(0) has been partially read in from the socket.
    PRPackedBool mRequestIsPartial;
    PRPackedBool mResponseIsPartial;

    // indicates whether or not the pipeline has been explicitly closed.
    PRPackedBool mClosed;

    // used when calling ReadSegments/WriteSegments on a transaction.
    nsAHttpSegmentReader *mReader;
    nsAHttpSegmentWriter *mWriter;

    // send buffer
    nsCOMPtr<nsIInputStream>  mSendBufIn;
    nsCOMPtr<nsIOutputStream> mSendBufOut;

    // the push back buffer.  not exceeding NS_HTTP_SEGMENT_SIZE bytes.
    char     *mPushBackBuf;
    PRUint32  mPushBackLen;
    PRUint32  mPushBackMax;
};

#endif // nsHttpPipeline_h__
