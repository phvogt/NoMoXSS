/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Brian Ryner.
 * Portions created by Brian Ryner are Copyright (C) 2000 Brian Ryner.
 * All Rights Reserved.
 *
 * Contributor(s): 
 *  Scott MacGregor <mscott@netscape.com>
 */

#include "nsIconChannel.h"
#include "nsIconURI.h"
#include "nsIconProtocolHandler.h"
#include "nsIURL.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsNetCID.h"

////////////////////////////////////////////////////////////////////////////////

nsIconProtocolHandler::nsIconProtocolHandler() 
{
}

nsIconProtocolHandler::~nsIconProtocolHandler() 
{}

NS_IMPL_ISUPPORTS2(nsIconProtocolHandler, nsIProtocolHandler, nsISupportsWeakReference)

    
////////////////////////////////////////////////////////////////////////////////
// nsIProtocolHandler methods:

NS_IMETHODIMP nsIconProtocolHandler::GetScheme(nsACString &result) 
{
  result = "moz-icon";
  return NS_OK;
}

NS_IMETHODIMP nsIconProtocolHandler::GetDefaultPort(PRInt32 *result) 
{
  *result = 0;
  return NS_OK;
}

NS_IMETHODIMP nsIconProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    // don't override anything.  
    *_retval = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP nsIconProtocolHandler::GetProtocolFlags(PRUint32 *result) 
{
  *result = URI_NORELATIVE | URI_NOAUTH;
  return NS_OK;
}

NS_IMETHODIMP nsIconProtocolHandler::NewURI(const nsACString &aSpec,
                                            const char *aOriginCharset, // ignored
                                            nsIURI *aBaseURI,
                                            nsIURI **result) 
{
  
  nsCOMPtr<nsIURI> uri;
  NS_NEWXPCOM(uri, nsMozIconURI);
  if (!uri) return NS_ERROR_FAILURE;

  nsresult rv = uri->SetSpec(aSpec);
  if (NS_FAILED(rv)) return rv;

  NS_ADDREF(*result = uri);
  return NS_OK;
}

NS_IMETHODIMP nsIconProtocolHandler::NewChannel(nsIURI* url, nsIChannel* *result)
{
  nsCOMPtr<nsIChannel> channel;
  NS_NEWXPCOM(channel, nsIconChannel);

  if (channel)
    NS_STATIC_CAST(nsIconChannel*,NS_STATIC_CAST(nsIChannel*, channel))->Init(url);

  *result = channel;
  NS_IF_ADDREF(*result);

  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
