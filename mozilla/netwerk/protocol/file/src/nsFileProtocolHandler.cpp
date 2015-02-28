/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include "nsFileProtocolHandler.h"
#include "nsFileChannel.h"
#include "nsInputStreamChannel.h"
#include "nsStandardURL.h"
#include "nsURLHelper.h"
#include "nsNetCID.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsIDirectoryListing.h"

//-----------------------------------------------------------------------------

nsFileProtocolHandler::nsFileProtocolHandler()
    : mGenerateHTMLDirs(PR_FALSE)
{
}

nsresult
nsFileProtocolHandler::Init()
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
        PRInt32 sFormat;
        nsresult rv = prefs->GetIntPref("network.dir.format", &sFormat);
        if (NS_SUCCEEDED(rv) && sFormat == nsIDirectoryListing::FORMAT_HTML)
            mGenerateHTMLDirs = PR_TRUE;
    }
    return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsFileProtocolHandler,
                              nsIFileProtocolHandler,
                              nsIProtocolHandler,
                              nsISupportsWeakReference)

//-----------------------------------------------------------------------------
// nsIProtocolHandler methods:

NS_IMETHODIMP
nsFileProtocolHandler::GetScheme(nsACString &result)
{
    result = NS_LITERAL_CSTRING("file");
    return NS_OK;
}

NS_IMETHODIMP
nsFileProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        // no port for file: URLs
    return NS_OK;
}

NS_IMETHODIMP
nsFileProtocolHandler::GetProtocolFlags(PRUint32 *result)
{
    *result = URI_NOAUTH;
    return NS_OK;
}

NS_IMETHODIMP
nsFileProtocolHandler::NewURI(const nsACString &spec,
                              const char *charset,
                              nsIURI *baseURI,
                              nsIURI **result)
{
    nsCOMPtr<nsIStandardURL> url = new nsStandardURL(PR_TRUE);
    if (!url)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = url->Init(nsIStandardURL::URLTYPE_NO_AUTHORITY, -1,
                            spec, charset, baseURI);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(url, result);
}

NS_IMETHODIMP
nsFileProtocolHandler::NewChannel(nsIURI *uri, nsIChannel **result)
{
    nsFileChannel *chan = new nsFileChannel();
    if (!chan)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(chan);

    nsresult rv = chan->Init(uri, mGenerateHTMLDirs);
    if (NS_FAILED(rv)) {
        NS_RELEASE(chan);
        return rv;
    }

    *result = chan;
    return NS_OK;
}

NS_IMETHODIMP 
nsFileProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *result)
{
    // don't override anything.  
    *result = PR_FALSE;
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsIFileProtocolHandler methods:

NS_IMETHODIMP
nsFileProtocolHandler::NewFileURI(nsIFile *file, nsIURI **result)
{
    nsresult rv;

    nsCOMPtr<nsIFileURL> url = new nsStandardURL(PR_TRUE);
    if (!url)
        return NS_ERROR_OUT_OF_MEMORY;

    // XXX shouldn't we set nsIURI::originCharset ??

    rv = url->SetFile(file);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(url, result);
}

NS_IMETHODIMP
nsFileProtocolHandler::GetURLSpecFromFile(nsIFile *file, nsACString &result)
{
    return net_GetURLSpecFromFile(file, result);
}

NS_IMETHODIMP
nsFileProtocolHandler::GetFileFromURLSpec(const nsACString &spec, nsIFile **result)
{
    return net_GetFileFromURLSpec(spec, result);
}
