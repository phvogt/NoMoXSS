/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Chak Nanga <chak@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsViewSourceHandler.h"
#include "nsViewSourceChannel.h"
#include "nsIURL.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsXPIDLString.h"
#include "nsNetUtil.h"

static NS_DEFINE_CID(kSimpleURICID,     NS_SIMPLEURI_CID);

////////////////////////////////////////////////////////////////////////////////

nsViewSourceHandler::nsViewSourceHandler() {
}

nsViewSourceHandler::~nsViewSourceHandler() {
}

NS_IMPL_ISUPPORTS1(nsViewSourceHandler, nsIProtocolHandler)

NS_METHOD
nsViewSourceHandler::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult) {
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsViewSourceHandler* ph = new nsViewSourceHandler();
    if (ph == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(ph);

    nsresult rv = ph->QueryInterface(aIID, aResult);

    NS_RELEASE(ph);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsIProtocolHandler methods:

NS_IMETHODIMP
nsViewSourceHandler::GetScheme(nsACString &result) {
    result = "view-source";
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceHandler::GetDefaultPort(PRInt32 *result) {
    *result = -1;
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceHandler::GetProtocolFlags(PRUint32 *result) {
    *result = URI_NORELATIVE | URI_NOAUTH;
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceHandler::NewURI(const nsACString &aSpec,
                            const char *aCharset, // ignore charset info
                            nsIURI *aBaseURI,
                            nsIURI **aResult)
{
    nsresult rv;

    // Extract inner URL and normalize to ASCII.  This is done to properly
    // support IDN in cases like "view-source:http://www.szalagavató.hu/"

    PRInt32 colon = aSpec.FindChar(':');
    if (colon == kNotFound)
        return NS_ERROR_MALFORMED_URI;

    nsCOMPtr<nsIURI> innerURI;
    rv = NS_NewURI(getter_AddRefs(innerURI), Substring(aSpec, colon + 1), aCharset);
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString asciiSpec;
    rv = innerURI->GetAsciiSpec(asciiSpec);
    if (NS_FAILED(rv))
        return rv;

    // put back our scheme and construct a simple-uri wrapper

    asciiSpec.Insert("view-source:", 0);
                                                                                                           
    nsIURI *uri;
    rv = CallCreateInstance(NS_SIMPLEURI_CONTRACTID, nsnull, &uri);
    if (NS_FAILED(rv))
        return rv;

    rv = uri->SetSpec(asciiSpec);
    if (NS_FAILED(rv))
        NS_RELEASE(uri);
    else
        *aResult = uri;
    return rv;
}

NS_IMETHODIMP
nsViewSourceHandler::NewChannel(nsIURI* uri, nsIChannel* *result)
{
    nsresult rv;
    
    nsViewSourceChannel* channel;
    rv = nsViewSourceChannel::Create(nsnull, NS_GET_IID(nsIChannel), (void**)&channel);
    if (NS_FAILED(rv)) return rv;

    rv = channel->Init(uri);
    if (NS_FAILED(rv)) {
        NS_RELEASE(channel);
        return rv;
    }

    *result = NS_STATIC_CAST(nsIViewSourceChannel*, channel);
    return NS_OK;
}

NS_IMETHODIMP 
nsViewSourceHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    // don't override anything.  
    *_retval = PR_FALSE;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
