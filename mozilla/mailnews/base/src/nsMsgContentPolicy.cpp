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
 * Contributor(s):
 *   Scott MacGregor <scott@scott-macgregor.org>
 *
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

#include "nsMsgContentPolicy.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranchInternal.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsIContentPolicy.h"
#include "nsIRDFService.h"
#include "nsIRDFResource.h"
#include "nsIMsgHeaderParser.h"
#include "nsIAbMDBDirectory.h"

#include "nsIMsgMailNewsUrl.h"
#include "nsIMsgWindow.h"
#include "nsIMimeMiscStatus.h"
#include "nsIMsgMessageService.h"
#include "nsIMsgIncomingServer.h"
#include "nsIRssIncomingServer.h"
#include "nsIMsgHdr.h"
#include "nsMsgUtils.h"

static const char kBlockRemoteImages[] = "mailnews.message_display.disable_remote_image";
static const char kRemoteImagesUseWhiteList[] = "mailnews.message_display.disable_remote_images.useWhitelist";
static const char kRemoteImagesWhiteListURI[] = "mailnews.message_display.disable_remote_images.whiteListAbURI";
static const char kAllowPlugins[] = "mailnews.message_display.allow.plugins";

// Per message headder flags to keep track of whether the user is allowing remote
// content for a particular message. 
// if you change or add more values to these constants, be sure to modify
// the corresponding definitions in mailWindowOverlay.js
#define kNoRemoteContentPolicy 0
#define kBlockRemoteContent 1
#define kAllowRemoteContent 2

NS_IMPL_ADDREF(nsMsgContentPolicy)
NS_IMPL_RELEASE(nsMsgContentPolicy)

NS_INTERFACE_MAP_BEGIN(nsMsgContentPolicy)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContentPolicy)
   NS_INTERFACE_MAP_ENTRY(nsIContentPolicy)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsSupportsWeakReference)
NS_INTERFACE_MAP_END

nsMsgContentPolicy::nsMsgContentPolicy()
{
  mAllowPlugins = PR_FALSE;
  mUseRemoteImageWhiteList = PR_TRUE;
  mBlockRemoteImages = PR_TRUE;
}

nsMsgContentPolicy::~nsMsgContentPolicy()
{
  // hey, we are going away...clean up after ourself....unregister our observer
  nsresult rv = NS_OK;
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIPrefBranchInternal> prefInternal = do_QueryInterface(prefBranch, &rv);
    if (NS_SUCCEEDED(rv))
    {
      prefInternal->RemoveObserver(kBlockRemoteImages, this);
      prefInternal->RemoveObserver(kRemoteImagesUseWhiteList, this);
      prefInternal->RemoveObserver(kRemoteImagesWhiteListURI, this);
      prefInternal->RemoveObserver(kAllowPlugins, this);
    }
  }
}

nsresult nsMsgContentPolicy::Init()
{
  nsresult rv;

  // register ourself as an observer on the mail preference to block remote images
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranchInternal> prefInternal = do_QueryInterface(prefBranch, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  prefInternal->AddObserver(kBlockRemoteImages, this, PR_TRUE);
  prefInternal->AddObserver(kRemoteImagesUseWhiteList, this, PR_TRUE);
  prefInternal->AddObserver(kRemoteImagesWhiteListURI, this, PR_TRUE);
  prefInternal->AddObserver(kAllowPlugins, this, PR_TRUE);

  prefBranch->GetBoolPref(kAllowPlugins, &mAllowPlugins);
  prefBranch->GetBoolPref(kRemoteImagesUseWhiteList, &mUseRemoteImageWhiteList);
  prefBranch->GetCharPref(kRemoteImagesWhiteListURI, getter_Copies(mRemoteImageWhiteListURI));
  return prefBranch->GetBoolPref(kBlockRemoteImages, &mBlockRemoteImages);
}

nsresult nsMsgContentPolicy::IsSenderInWhiteList(nsIMsgDBHdr * aMsgHdr, PRBool * aWhiteListed)
{
  *aWhiteListed = PR_FALSE;
  NS_ENSURE_ARG_POINTER(aMsgHdr); 
  nsresult rv = NS_OK;

  if (mBlockRemoteImages && mUseRemoteImageWhiteList && !mRemoteImageWhiteListURI.IsEmpty())
  {
    nsXPIDLCString author;
    rv = aMsgHdr->GetAuthor(getter_Copies(author));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr <nsIRDFResource> resource;
    rv = rdfService->GetResource(mRemoteImageWhiteListURI, getter_AddRefs(resource));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr <nsIAbMDBDirectory> addressBook = do_QueryInterface(resource, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIMsgHeaderParser> headerParser = do_GetService("@mozilla.org/messenger/headerparser;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsXPIDLCString emailAddress; 
    rv = headerParser->ExtractHeaderAddressMailboxes(nsnull, author, getter_Copies(emailAddress));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = addressBook->HasCardForEmailAddress(emailAddress, aWhiteListed);
  }

  return rv;
}

NS_IMETHODIMP
nsMsgContentPolicy::ShouldLoad(PRUint32          aContentType,
                               nsIURI           *aContentLocation,
                               nsIURI           *aRequestingLocation,
                               nsISupports      *aRequestingContext,
                               const nsACString &aMimeGuess,
                               nsISupports      *aExtra,
                               PRInt16          *aDecision)
{
  nsresult rv = NS_OK;
  *aDecision = nsIContentPolicy::ACCEPT;

  if (!aContentLocation) 
      return rv;

  if (aContentType == nsIContentPolicy::TYPE_OBJECT)
  {
      // only allow the plugin to load if the allow plugins pref has been set
      *aDecision = mAllowPlugins ? nsIContentPolicy::ACCEPT :
                                   nsIContentPolicy::REJECT_TYPE;
  }
  else if (aContentType == nsIContentPolicy::TYPE_IMAGE)
  {
    PRBool isFtp = PR_FALSE;
    rv = aContentLocation->SchemeIs("ftp", &isFtp);

    if (isFtp) 
    {
      // never allow ftp for mail messages, 
      // because we don't want to send the users email address
      // as the anonymous password
      *aDecision = nsIContentPolicy::REJECT_REQUEST;
    }
    else 
    {
      PRBool needToCheck = PR_FALSE;
      rv = aContentLocation->SchemeIs("http", &needToCheck);
      NS_ENSURE_SUCCESS(rv,rv);

      if (!needToCheck) {
        rv = aContentLocation->SchemeIs("https", &needToCheck);
        NS_ENSURE_SUCCESS(rv,rv);
      }

      // Consider blocking remote image requests if the image url is http or https
      if (needToCheck) 
      {
        // default to blocking remote content 
        *aDecision = mBlockRemoteImages ? nsIContentPolicy::REJECT_REQUEST : nsIContentPolicy::ACCEPT;

        // now do some more detective work to better refine our decision...
        // (1) examine the msg hdr value for the remote content policy on this particular message to
        //     see if this particular message has special rights to bypass the remote content check
        // (2) special case RSS urls, always allow them to load remote images since the user explicitly
        //     subscribed to the feed.
        // (3) Check the personal address book and use it as a white list for senders 
        //     who are allowed to send us remote images 

        // get the msg hdr for the message URI we are actually loading
        NS_ENSURE_TRUE(aRequestingLocation, NS_OK);
        nsCOMPtr<nsIMsgMessageUrl> msgUrl = do_QueryInterface(aRequestingLocation, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsXPIDLCString resourceURI;
        msgUrl->GetUri(getter_Copies(resourceURI));
        
        // get the msg service for this URI
        nsCOMPtr<nsIMsgMessageService> msgService;
        rv = GetMessageServiceFromURI(resourceURI.get(), getter_AddRefs(msgService));
        NS_ENSURE_SUCCESS(rv, rv);
        
        nsCOMPtr<nsIMsgDBHdr> msgHdr;
        rv = msgService->MessageURIToMsgHdr(resourceURI, getter_AddRefs(msgHdr));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIMsgMailNewsUrl> mailnewsUrl = do_QueryInterface(aRequestingLocation, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        // Case #1, check the db hdr for the remote content policy on this particular message
        PRUint32 remoteContentPolicy = kNoRemoteContentPolicy;
        msgHdr->GetUint32Property("remoteContentPolicy", &remoteContentPolicy); 

        // get the folder and the server from the msghdr
        // an error trying to get the folder shouldn't make us quit...
        nsCOMPtr<nsIRssIncomingServer> rssServer;
        nsCOMPtr<nsIMsgFolder> folder;
        rv = msgHdr->GetFolder(getter_AddRefs(folder));
        if (NS_SUCCEEDED(rv))
        {
          nsCOMPtr<nsIMsgIncomingServer> server;
          folder->GetServer(getter_AddRefs(server));
          rssServer = do_QueryInterface(server);
        }

        // Case #3, author is in our white list..
        PRBool authorInWhiteList = PR_FALSE;
        IsSenderInWhiteList(msgHdr, &authorInWhiteList);
        
        // Case #1 and #2: special case RSS. Allow urls that are RSS feeds to show remote image (Bug #250246)
        // Honor the message specific remote content policy
        if (rssServer || remoteContentPolicy == kAllowRemoteContent || authorInWhiteList)
          *aDecision = nsIContentPolicy::ACCEPT;
        else if (mBlockRemoteImages) 
        {
          if (!remoteContentPolicy) // kNoRemoteContentPolicy means we have never set a value on the message
            msgHdr->SetUint32Property("remoteContentPolicy", kBlockRemoteContent); 

          // now we need to call out the msg sink informing it that this message has remote content
          nsCOMPtr<nsIMsgWindow> msgWindow;
          rv = mailnewsUrl->GetMsgWindow(getter_AddRefs(msgWindow)); // it's not an error for the msg window to be null
          NS_ENSURE_TRUE(msgWindow, NS_ERROR_FAILURE);

          nsCOMPtr<nsIMsgHeaderSink> msgHdrSink;
          rv = msgWindow->GetMsgHeaderSink(getter_AddRefs(msgHdrSink));
          NS_ENSURE_SUCCESS(rv, rv);

          msgHdrSink->OnMsgHasRemoteContent(msgHdr); // notify the UI to show the remote content hdr bar so the user can overide
        } // if mBlockRemoteImages
      } // if need to check the url for a remote image policy
        }
  } // if aContentType == nsIContentPolicy::TYPE_IMAGE

  return rv;
}

NS_IMETHODIMP
nsMsgContentPolicy::ShouldProcess(PRUint32          aContentType,
                                  nsIURI           *aContentLocation,
                                  nsIURI           *aRequestingLocation,
                                  nsISupports      *aRequestingContext,
                                  const nsACString &aMimeGuess,
                                  nsISupports      *aExtra,
                                  PRInt16          *aDecision)
{
  *aDecision = nsIContentPolicy::ACCEPT;
  return NS_OK;
}

NS_IMETHODIMP nsMsgContentPolicy::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
  nsresult rv;
   
  if (!nsCRT::strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic)) 
  {
    nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_LossyConvertUCS2toASCII pref(aData);

    if (pref.Equals(kBlockRemoteImages))
      rv = prefBranch->GetBoolPref(kBlockRemoteImages, &mBlockRemoteImages);
    else if (pref.Equals(kRemoteImagesUseWhiteList))
      prefBranch->GetBoolPref(kRemoteImagesUseWhiteList, &mUseRemoteImageWhiteList);
    else if (pref.Equals(kRemoteImagesWhiteListURI))
      prefBranch->GetCharPref(kRemoteImagesWhiteListURI, getter_Copies(mRemoteImageWhiteListURI));
  }
  
  return NS_OK;
}
