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
 * 
 * Contributor(s): 
 *      Scott MacGregor <mscott@netscape.com>
 *   
 */

#include "nsIURI.h"
#include "nsIURL.h"
#include "nsExternalProtocolHandler.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIServiceManagerUtils.h"
#include "nsIInterfaceRequestor.h"
#include "nsIStringBundle.h"
#include "nsIPrefService.h"
#include "nsIPrompt.h"
#include "nsEventQueueUtils.h"
#include "nsIChannel.h"
#include "nsNetCID.h"
#include "netCore.h"

// used to dispatch urls to default protocol handlers
#include "nsCExternalHandlerService.h"
#include "nsIExternalProtocolService.h"

static NS_DEFINE_CID(kSimpleURICID, NS_SIMPLEURI_CID);
static const char kExternalProtocolPrefPrefix[] = "network.protocol-handler.external.";



////////////////////////////////////////////////////////////////////////
// a stub channel implemenation which will map calls to AsyncRead and OpenInputStream
// to calls in the OS for loading the url.
////////////////////////////////////////////////////////////////////////

class nsExtProtocolChannel : public nsIChannel
{
public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSICHANNEL
    NS_DECL_NSIREQUEST

    nsExtProtocolChannel();
    virtual ~nsExtProtocolChannel();

    nsresult SetURI(nsIURI*);

    nsresult OpenURL();

private:
    nsCOMPtr<nsIURI> mUrl;
    nsresult mStatus;

    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    PRBool PromptForScheme(nsIURI *aURI, nsACString& aScheme);
};

NS_IMPL_THREADSAFE_ADDREF(nsExtProtocolChannel)
NS_IMPL_THREADSAFE_RELEASE(nsExtProtocolChannel)

NS_INTERFACE_MAP_BEGIN(nsExtProtocolChannel)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIChannel)
   NS_INTERFACE_MAP_ENTRY(nsIChannel)
   NS_INTERFACE_MAP_ENTRY(nsIRequest)
NS_INTERFACE_MAP_END_THREADSAFE

nsExtProtocolChannel::nsExtProtocolChannel() : mStatus(NS_OK)
{
}

nsExtProtocolChannel::~nsExtProtocolChannel()
{}

NS_IMETHODIMP nsExtProtocolChannel::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
    *aLoadGroup = nsnull;
    return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::SetLoadGroup(nsILoadGroup * aLoadGroup)
{
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aNotificationCallbacks)
{
  NS_ENSURE_ARG_POINTER(aNotificationCallbacks);
  *aNotificationCallbacks = mCallbacks;
  NS_IF_ADDREF(*aNotificationCallbacks);
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aNotificationCallbacks)
{
  mCallbacks = aNotificationCallbacks;
  return NS_OK;       // don't fail when trying to set this
}

NS_IMETHODIMP 
nsExtProtocolChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
  *aSecurityInfo = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::GetOriginalURI(nsIURI* *aURI)
{
  *aURI = nsnull;
  return NS_OK; 
}
 
NS_IMETHODIMP nsExtProtocolChannel::SetOriginalURI(nsIURI* aURI)
{
  return NS_OK;
}
 
NS_IMETHODIMP nsExtProtocolChannel::GetURI(nsIURI* *aURI)
{
  *aURI = mUrl;
  NS_IF_ADDREF(*aURI);
  return NS_OK; 
}
 
nsresult nsExtProtocolChannel::SetURI(nsIURI* aURI)
{
  mUrl = aURI;
  return NS_OK; 
}
 
PRBool nsExtProtocolChannel::PromptForScheme(nsIURI* aURI,
                                             nsACString& aScheme)
{
  // deny the load if we aren't able to ask but prefs say we should
  nsresult rv;
  if (!mCallbacks) {
    NS_ERROR("Notification Callbacks not set!");
    return PR_FALSE;
  }

  nsCOMPtr<nsIPrompt> prompt;
  mCallbacks->GetInterface(NS_GET_IID(nsIPrompt), getter_AddRefs(prompt));
  if (!prompt) {
    NS_ERROR("No prompt interface on channel");
    return PR_FALSE;
  }

  nsCOMPtr<nsIStringBundleService> sbSvc(do_GetService(NS_STRINGBUNDLE_CONTRACTID));
  if (!sbSvc) {
    NS_ERROR("Couldn't load StringBundleService");
    return PR_FALSE;
  }

  nsCOMPtr<nsIStringBundle> appstrings;
  rv = sbSvc->CreateBundle("chrome://global/locale/appstrings.properties",
                           getter_AddRefs(appstrings));
  if (NS_FAILED(rv) || !appstrings) {
    NS_ERROR("Failed to create appstrings.properties bundle");
    return PR_FALSE;
  }

  nsCAutoString spec;
  aURI->GetSpec(spec);
  NS_ConvertUTF8toUTF16 uri(spec);
  NS_ConvertUTF8toUTF16 scheme(aScheme);

  nsXPIDLString title;
  appstrings->GetStringFromName(NS_LITERAL_STRING("externalProtocolTitle").get(),
                                getter_Copies(title));
  nsXPIDLString checkMsg;
  appstrings->GetStringFromName(NS_LITERAL_STRING("externalProtocolChkMsg").get(),
                                getter_Copies(checkMsg));
  nsXPIDLString launchBtn;
  appstrings->GetStringFromName(NS_LITERAL_STRING("externalProtocolLaunchBtn").get(),
                                getter_Copies(launchBtn));

  nsXPIDLString message;
  const PRUnichar* msgArgs[] = { scheme.get(), uri.get() };
  appstrings->FormatStringFromName(NS_LITERAL_STRING("externalProtocolPrompt").get(),
                                   msgArgs,
                                   NS_ARRAY_LENGTH(msgArgs),
                                   getter_Copies(message));

  if (scheme.IsEmpty() || uri.IsEmpty() || title.IsEmpty() ||
      checkMsg.IsEmpty() || launchBtn.IsEmpty() || message.IsEmpty())
    return PR_FALSE;

  // all pieces assembled, now we can pose the dialog
  PRBool allowLoad = PR_FALSE;
  PRBool remember = PR_FALSE;
  PRInt32 choice = 1; // assume "cancel" in case of failure
  rv = prompt->ConfirmEx(title.get(), message.get(),
                         nsIPrompt::BUTTON_DELAY_ENABLE +
                         nsIPrompt::BUTTON_POS_1_DEFAULT +
                         (nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_0) +
                         (nsIPrompt::BUTTON_TITLE_CANCEL * nsIPrompt::BUTTON_POS_1),
                         launchBtn.get(), 0, 0, checkMsg.get(),
                         &remember, &choice);
  if (NS_SUCCEEDED(rv))
  {
    if (choice == 0)
      allowLoad = PR_TRUE;

    if (remember)
    {
      nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
      if (prefs)
      {
          nsCAutoString prefname(kExternalProtocolPrefPrefix);
          prefname += aScheme;
          prefs->SetBoolPref(prefname.get(), allowLoad);
      }
    }
  }

  return allowLoad;
}

nsresult nsExtProtocolChannel::OpenURL()
{
  nsCOMPtr<nsIExternalProtocolService> extProtService (do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID));
  nsCAutoString urlScheme;
  mUrl->GetScheme(urlScheme);

  if (extProtService)
  {
#ifdef DEBUG
    PRBool haveHandler = PR_FALSE;
    extProtService->ExternalProtocolHandlerExists(urlScheme.get(), &haveHandler);
    NS_ASSERTION(haveHandler, "Why do we have a channel for this url if we don't support the protocol?");
#endif

    // Check that it's OK to hand this scheme off to the OS

    PRBool allowLoad    = PR_FALSE;
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));

    if (prefs)
    {
      // check whether it's explicitly approved or denied in prefs
      nsCAutoString schemePref(kExternalProtocolPrefPrefix);
      schemePref += urlScheme;

      nsresult rv = prefs->GetBoolPref(schemePref.get(), &allowLoad);

      if (NS_FAILED(rv))
      {
        // scheme not explicitly listed, what is the default action?
        const PRInt32 kExternalProtocolNever  = 0;
        const PRInt32 kExternalProtocolAlways = 1;
        const PRInt32 kExternalProtocolAsk    = 2;

        PRInt32 externalDefault = kExternalProtocolAsk;
        prefs->GetIntPref("network.protocol-handler.external-default",
                          &externalDefault);

        if (externalDefault == kExternalProtocolAlways)
        {
          // original behavior -- just do it
          allowLoad = PR_TRUE;
        }
        else if (externalDefault == kExternalProtocolAsk)
        {
          allowLoad = PromptForScheme(mUrl, urlScheme);
        }
      }
    }

    if (allowLoad)
      return extProtService->LoadUrl(mUrl);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsExtProtocolChannel::Open(nsIInputStream **_retval)
{
  OpenURL();
  return NS_ERROR_NO_CONTENT; // force caller to abort.
}

static void *PR_CALLBACK handleExtProtoEvent(PLEvent *event)
{
  nsExtProtocolChannel *channel =
      NS_STATIC_CAST(nsExtProtocolChannel*, PL_GetEventOwner(event));

  if (channel)
      channel->OpenURL();

  return nsnull;
}

static void PR_CALLBACK destroyExtProtoEvent(PLEvent *event)
{
  nsExtProtocolChannel *channel =
      NS_STATIC_CAST(nsExtProtocolChannel*, PL_GetEventOwner(event));
  NS_IF_RELEASE(channel);
  delete event;
}

NS_IMETHODIMP nsExtProtocolChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *ctxt)
{
  nsCOMPtr<nsIEventQueue> eventQ;
  nsresult rv = NS_GetCurrentEventQ(getter_AddRefs(eventQ));
  if (NS_FAILED(rv))
    return rv;

  PLEvent *event = new PLEvent;
  if (event)
  {
    NS_ADDREF_THIS();
    PL_InitEvent(event, this, handleExtProtoEvent, destroyExtProtoEvent);

    rv = eventQ->PostEvent(event);
    if (NS_FAILED(rv))
      PL_DestroyEvent(event);
  }

  return NS_ERROR_NO_CONTENT; // force caller to abort.
}

NS_IMETHODIMP nsExtProtocolChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  *aLoadFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::GetContentType(nsACString &aContentType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::SetContentType(const nsACString &aContentType)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsExtProtocolChannel::GetContentCharset(nsACString &aContentCharset)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::SetContentCharset(const nsACString &aContentCharset)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::GetContentLength(PRInt32 * aContentLength)
{
  *aContentLength = -1;
  return NS_OK;
}

NS_IMETHODIMP
nsExtProtocolChannel::SetContentLength(PRInt32 aContentLength)
{
  NS_NOTREACHED("SetContentLength");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::GetOwner(nsISupports * *aPrincipal)
{
  NS_NOTREACHED("GetOwner");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::SetOwner(nsISupports * aPrincipal)
{
  NS_NOTREACHED("SetOwner");
  return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// From nsIRequest
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsExtProtocolChannel::GetName(nsACString &result)
{
  NS_NOTREACHED("nsExtProtocolChannel::GetName");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::IsPending(PRBool *result)
{
  *result = PR_TRUE;
  return NS_OK; 
}

NS_IMETHODIMP nsExtProtocolChannel::GetStatus(nsresult *status)
{
  *status = mStatus;
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::Cancel(nsresult status)
{
  mStatus = status;
  return NS_OK;
}

NS_IMETHODIMP nsExtProtocolChannel::Suspend()
{
  NS_NOTREACHED("Suspend");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsExtProtocolChannel::Resume()
{
  NS_NOTREACHED("Resume");
  return NS_ERROR_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////
// the default protocol handler implementation
//////////////////////////////////////////////////////////////////////

nsExternalProtocolHandler::nsExternalProtocolHandler()
{
  m_schemeName = "default";
  m_extProtService = do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID);
}


nsExternalProtocolHandler::~nsExternalProtocolHandler()
{}

NS_IMPL_THREADSAFE_ADDREF(nsExternalProtocolHandler)
NS_IMPL_THREADSAFE_RELEASE(nsExternalProtocolHandler)

NS_INTERFACE_MAP_BEGIN(nsExternalProtocolHandler)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIProtocolHandler)
   NS_INTERFACE_MAP_ENTRY(nsIProtocolHandler)
   NS_INTERFACE_MAP_ENTRY(nsIExternalProtocolHandler)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMETHODIMP nsExternalProtocolHandler::GetScheme(nsACString &aScheme)
{
  aScheme = m_schemeName;
  return NS_OK;
}

NS_IMETHODIMP nsExternalProtocolHandler::GetDefaultPort(PRInt32 *aDefaultPort)
{
  *aDefaultPort = 0;
    return NS_OK;
}

NS_IMETHODIMP 
nsExternalProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    // don't override anything.  
    *_retval = PR_FALSE;
    return NS_OK;
}
// returns TRUE if the OS can handle this protocol scheme and false otherwise.
PRBool nsExternalProtocolHandler::HaveProtocolHandler(nsIURI * aURI)
{
  PRBool haveHandler = PR_FALSE;
  if (aURI)
  {
    nsCAutoString scheme;
    aURI->GetScheme(scheme);
    if (m_extProtService)
      m_extProtService->ExternalProtocolHandlerExists(scheme.get(), &haveHandler);
  }

  return haveHandler;
}

NS_IMETHODIMP nsExternalProtocolHandler::GetProtocolFlags(PRUint32 *aUritype)
{
    // Make it norelative since it is a simple uri
    *aUritype = URI_NORELATIVE;
    return NS_OK;
}

NS_IMETHODIMP nsExternalProtocolHandler::NewURI(const nsACString &aSpec,
                                                const char *aCharset, // ignore charset info
                                                nsIURI *aBaseURI,
                                                nsIURI **_retval)
{
  nsresult rv;
  nsCOMPtr<nsIURI> uri = do_CreateInstance(kSimpleURICID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = uri->SetSpec(aSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = uri);
  return NS_OK;
}

NS_IMETHODIMP nsExternalProtocolHandler::NewChannel(nsIURI *aURI, nsIChannel **_retval)
{
  // only try to return a channel if we have a protocol handler for the url

  PRBool haveHandler = HaveProtocolHandler(aURI);
  if (haveHandler)
  {
    nsCOMPtr<nsIChannel> channel;
    NS_NEWXPCOM(channel, nsExtProtocolChannel);
    if (!channel) return NS_ERROR_OUT_OF_MEMORY;

    ((nsExtProtocolChannel*) channel.get())->SetURI(aURI);

    if (_retval)
    {
      *_retval = channel;
      NS_IF_ADDREF(*_retval);
      return NS_OK;
    }
  }

  return NS_ERROR_UNKNOWN_PROTOCOL;
}

///////////////////////////////////////////////////////////////////////
// External protocol handler interface implementation
//////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsExternalProtocolHandler::ExternalAppExistsForScheme(const nsACString& aScheme, PRBool *_retval)
{
  if (m_extProtService)
    return m_extProtService->ExternalProtocolHandlerExists(PromiseFlatCString(aScheme).get(), _retval);

  // In case we don't have external protocol service.
  *_retval = PR_FALSE;
  return NS_OK;
}
