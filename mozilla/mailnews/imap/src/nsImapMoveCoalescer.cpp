/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 1999
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

#include "msgCore.h"
#include "nsMsgImapCID.h"
#include "nsImapMailFolder.h"
#include "nsImapMoveCoalescer.h"
#include "nsMsgKeyArray.h"
#include "nsImapService.h"
#include "nsIMsgCopyService.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgFolder.h" // TO include biffState enum. Change to bool later...
#include "nsMsgFolderFlags.h"


NS_IMPL_ISUPPORTS1(nsImapMoveCoalescer, nsISupports)

nsImapMoveCoalescer::nsImapMoveCoalescer(nsIMsgFolder *sourceFolder, nsIMsgWindow *msgWindow)
{
  m_sourceFolder = sourceFolder; 
  m_msgWindow = msgWindow;
}

nsImapMoveCoalescer::~nsImapMoveCoalescer()
{
  for (PRInt32 i = 0; i < m_sourceKeyArrays.Count(); i++)
  {
    nsMsgKeyArray *keys = (nsMsgKeyArray *) m_sourceKeyArrays.ElementAt(i);
    delete keys;
  }
  for (PRInt32 index = 0; index < m_keyBuckets.Count(); index++)
  {
    nsMsgKeyArray *keys = (nsMsgKeyArray *) m_keyBuckets.ElementAt(index);
    delete keys;
  }
}

nsresult nsImapMoveCoalescer::AddMove(nsIMsgFolder *folder, nsMsgKey key)
{
  if (!m_destFolders)
    NS_NewISupportsArray(getter_AddRefs(m_destFolders));
  if (m_destFolders)
  {
    nsCOMPtr <nsISupports> supports = do_QueryInterface(folder);
    if (supports)
    {
      PRInt32 folderIndex = m_destFolders->IndexOf(supports);
      nsMsgKeyArray *keysToAdd=nsnull;
      if (folderIndex >= 0)
      {
        keysToAdd = (nsMsgKeyArray *) m_sourceKeyArrays.ElementAt(folderIndex);
      }
      else
      {
        m_destFolders->AppendElement(supports);
        keysToAdd = new nsMsgKeyArray;
        if (!keysToAdd)
          return NS_ERROR_OUT_OF_MEMORY;
        
        m_sourceKeyArrays.AppendElement(keysToAdd);
      }
      if (keysToAdd)
        keysToAdd->Add(key);
      return NS_OK;
    }
    else
      return NS_ERROR_NULL_POINTER;
  }
  else
    return NS_ERROR_OUT_OF_MEMORY;
  
}

nsresult nsImapMoveCoalescer::PlaybackMoves()
{
  PRUint32 numFolders;
  nsresult rv = NS_OK;
  
  if (!m_destFolders)
    return NS_OK;	// nothing to do.

  m_destFolders->Count(&numFolders);
  for (PRUint32 i = 0; i < numFolders; i++)
  {
    // XXX TODO
    // JUNK MAIL RELATED
    // is this the right place to make sure dest folder exists
    // (and has proper flags?), before we start copying?
    nsCOMPtr <nsIMsgFolder> destFolder(do_QueryElementAt(m_destFolders, i));
    nsCOMPtr<nsIImapService> imapService = 
      do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
    {
      nsMsgKeyArray *keysToAdd = (nsMsgKeyArray *) m_sourceKeyArrays.ElementAt(i);
      if (keysToAdd)
      {
        nsCAutoString messageIds;
        
        nsImapMailFolder::AllocateUidStringFromKeys(keysToAdd->GetArray(), keysToAdd->GetSize(), messageIds);
        PRInt32 numKeysToAdd = keysToAdd->GetSize();
        if (numKeysToAdd == 0)
          continue;

        PRUint32 destFlags;
        destFolder->GetFlags(&destFlags);
        if (! (destFlags & MSG_FOLDER_FLAG_JUNK)) // don't set has new on junk folder
        {
          destFolder->SetNumNewMessages(numKeysToAdd);
          destFolder->SetHasNewMessages(PR_TRUE);
        }
        // adjust the new message count on the source folder
        PRInt32 oldNewMessageCount = 0;
        m_sourceFolder->GetNumNewMessages(PR_FALSE, &oldNewMessageCount);
        if (oldNewMessageCount >= numKeysToAdd)
          oldNewMessageCount -= numKeysToAdd;
        else
          oldNewMessageCount = 0;

        m_sourceFolder->SetNumNewMessages(oldNewMessageCount);
        
        nsCOMPtr <nsISupports> sourceSupports = do_QueryInterface(m_sourceFolder, &rv);
        nsCOMPtr <nsIUrlListener> urlListener(do_QueryInterface(sourceSupports));
        
        nsCOMPtr<nsISupportsArray> messages;
        NS_NewISupportsArray(getter_AddRefs(messages));
        for (PRUint32 keyIndex = 0; keyIndex < keysToAdd->GetSize(); keyIndex++)
        {
          nsCOMPtr<nsIMsgDBHdr> mailHdr = nsnull;
          rv = m_sourceFolder->GetMessageHeader(keysToAdd->ElementAt(keyIndex), getter_AddRefs(mailHdr));
          if (NS_SUCCEEDED(rv) && mailHdr)
          {
            nsCOMPtr<nsISupports> iSupports = do_QueryInterface(mailHdr);
            messages->AppendElement(iSupports);
          }
        }
        keysToAdd->RemoveAll();
        nsCOMPtr<nsIMsgCopyService> copySvc = do_GetService(NS_MSGCOPYSERVICE_CONTRACTID);
        if (copySvc)
          rv = copySvc->CopyMessages(m_sourceFolder, messages, destFolder, PR_TRUE,
          /*nsIMsgCopyServiceListener* listener*/ nsnull, m_msgWindow, PR_FALSE /*allowUndo*/);
      }
    }
  }
  return rv;
}

nsMsgKeyArray *nsImapMoveCoalescer::GetKeyBucket(PRInt32 keyArrayIndex)
{
  PRInt32 bucketCount = m_keyBuckets.Count();
  if (bucketCount < keyArrayIndex + 1)
  {
    for (PRInt32 i = 0; i < keyArrayIndex + 1 - bucketCount; i++)
    {
        nsMsgKeyArray *keysToAdd = new nsMsgKeyArray;
        if (!keysToAdd)
          return nsnull;
        
        m_keyBuckets.AppendElement(keysToAdd);
    }
  }
  return (nsMsgKeyArray *) m_keyBuckets.SafeElementAt(keyArrayIndex);
}
