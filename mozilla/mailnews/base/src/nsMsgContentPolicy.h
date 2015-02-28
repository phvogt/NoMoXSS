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

/**********************************************************************************
 * nsMsgContentPolicy enforces the specified content policy on images, js, plugins, etc.
 * This is the class used to determine what elements in a message should be loaded
 ***********************************************************************************/

#ifndef _nsMsgContentPolicy_H_
#define _nsMsgContentPolicy_H_

#include "nsIContentPolicy.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsString.h"


/* DBFCFDF0-4489-4faa-8122-190FD1EFA16C */
#define NS_MSGCONTENTPOLICY_CID \
{ 0xdbfcfdf0, 0x4489, 0x4faa, { 0x81, 0x22, 0x19, 0xf, 0xd1, 0xef, 0xa1, 0x6c } }

#define NS_MSGCONTENTPOLICY_CONTRACTID "@mozilla.org/messenger/content-policy;1"

class nsIMsgDBHdr;

class nsMsgContentPolicy : public nsIContentPolicy,
                           public nsIObserver,
                           public nsSupportsWeakReference
{
public:
  nsMsgContentPolicy();
  virtual ~nsMsgContentPolicy();

  nsresult Init();
    
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPOLICY
  NS_DECL_NSIOBSERVER

protected:
  PRBool   mBlockRemoteImages;
  PRBool   mUseRemoteImageWhiteList;
  nsXPIDLCString mRemoteImageWhiteListURI;
  PRBool   mAllowPlugins;

  nsresult IsSenderInWhiteList(nsIMsgDBHdr * aMsgHdr, PRBool * aWhiteListed);
};

#endif // _nsMsgContentPolicy_H_

