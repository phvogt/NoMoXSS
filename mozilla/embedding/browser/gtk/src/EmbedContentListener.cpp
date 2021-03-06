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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Christopher Blizzard.
 * Portions created by Christopher Blizzard are Copyright (C)
 * Christopher Blizzard.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Christopher Blizzard <blizzard@mozilla.org>
 */

#include <strings.h>
#include <nsXPIDLString.h>

#include "nsIURI.h"

#include "EmbedContentListener.h"
#include "EmbedPrivate.h"

#include "nsICategoryManager.h"
#include "nsIServiceManagerUtils.h"

EmbedContentListener::EmbedContentListener(void)
{
  mOwner = nsnull;
}

EmbedContentListener::~EmbedContentListener()
{
}

NS_IMPL_ISUPPORTS2(EmbedContentListener,
                   nsIURIContentListener,
                   nsISupportsWeakReference)

nsresult
EmbedContentListener::Init(EmbedPrivate *aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
EmbedContentListener::OnStartURIOpen(nsIURI     *aURI,
				     PRBool     *aAbortOpen)
{
  nsresult rv;

  nsCAutoString specString;
  rv = aURI->GetSpec(specString);

  if (NS_FAILED(rv))
    return rv;

  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
		  moz_embed_signals[OPEN_URI],
		  specString.get(), &return_val);

  *aAbortOpen = return_val;

  return NS_OK;
}

NS_IMETHODIMP
EmbedContentListener::DoContent(const char         *aContentType,
				PRBool             aIsContentPreferred,
				nsIRequest         *aRequest,
				nsIStreamListener **aContentHandler,
				PRBool             *aAbortProcess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::IsPreferred(const char        *aContentType,
				  char             **aDesiredContentType,
				  PRBool            *aCanHandleContent)
{
  return CanHandleContent(aContentType, PR_TRUE, aDesiredContentType,
			  aCanHandleContent);
}

NS_IMETHODIMP
EmbedContentListener::CanHandleContent(const char        *aContentType,
				       PRBool           aIsContentPreferred,
				       char             **aDesiredContentType,
				       PRBool            *_retval)
{
  *_retval = PR_FALSE;
  
  if (aContentType) {
    nsCOMPtr<nsICategoryManager> catMgr;
    nsresult rv;
    catMgr = do_GetService("@mozilla.org/categorymanager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsXPIDLCString value;
    rv = catMgr->GetCategoryEntry("Gecko-Content-Viewers",
				  aContentType, 
				  getter_Copies(value));

    // If the category manager can't find what we're looking for
    // it returns NS_ERROR_NOT_AVAILABLE, we don't want to propagate
    // that to the caller since it's really not a failure

    if (NS_FAILED(rv) && rv != NS_ERROR_NOT_AVAILABLE)
      return rv;

    if (value && *value)
      *_retval = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedContentListener::GetLoadCookie(nsISupports **aLoadCookie)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::SetLoadCookie(nsISupports *aLoadCookie)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::GetParentContentListener(nsIURIContentListener **aParent)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::SetParentContentListener(nsIURIContentListener *aParent)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

