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
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Navin Gupta <naving@netscape.com> (Original Author)
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
#include "nsMsgQuickSearchDBView.h"
#include "nsIMsgHdr.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgImapMailFolder.h"
#include "nsImapCore.h"

nsMsgQuickSearchDBView::nsMsgQuickSearchDBView()
{
}

nsMsgQuickSearchDBView::~nsMsgQuickSearchDBView()
{	
 /* destructor code */
}

NS_IMPL_ISUPPORTS_INHERITED2(nsMsgQuickSearchDBView, nsMsgDBView, nsIMsgDBView, nsIMsgSearchNotify)

NS_IMETHODIMP nsMsgQuickSearchDBView::DoCommand(nsMsgViewCommandTypeValue aCommand)
{
  if (aCommand == nsMsgViewCommandType::markAllRead)
  {
    nsresult rv = NS_OK;
    m_folder->EnableNotifications(nsIMsgFolder::allMessageCountNotifications, PR_FALSE, PR_TRUE /*dbBatching*/);

    for (PRInt32 i=0;NS_SUCCEEDED(rv) && i < GetSize();i++)
    {
      nsCOMPtr<nsIMsgDBHdr> msgHdr;
      m_db->GetMsgHdrForKey(m_keys[i],getter_AddRefs(msgHdr)); 
      rv = m_db->MarkHdrRead(msgHdr, PR_TRUE, nsnull);
    }

    m_folder->EnableNotifications(nsIMsgFolder::allMessageCountNotifications, PR_TRUE, PR_TRUE /*dbBatching*/);

    nsCOMPtr<nsIMsgImapMailFolder> imapFolder = do_QueryInterface(m_folder);
    if (NS_SUCCEEDED(rv) && imapFolder)
      rv = imapFolder->StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, m_keys.GetArray(), m_keys.GetSize());

    m_db->SetSummaryValid(PR_TRUE);
    m_db->Commit(nsMsgDBCommitType::kLargeCommit);
    return rv;
  }
  else
    return nsMsgDBView::DoCommand(aCommand);
}

NS_IMETHODIMP nsMsgQuickSearchDBView::GetViewType(nsMsgViewTypeValue *aViewType)
{
    NS_ENSURE_ARG_POINTER(aViewType);
    *aViewType = nsMsgViewType::eShowQuickSearchResults; 
    return NS_OK;
}

nsresult nsMsgQuickSearchDBView::OnNewHeader(nsMsgKey newKey, nsMsgKey aParentKey, PRBool ensureListed)
{
  nsresult	rv;
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  rv = m_db->GetMsgHdrForKey(newKey, getter_AddRefs(msgHdr));
  if (NS_SUCCEEDED(rv) && msgHdr != nsnull)
  {
    PRBool match=PR_FALSE;
    nsCOMPtr <nsIMsgSearchSession> searchSession = do_QueryReferent(m_searchSession);
    if (searchSession)
      searchSession->MatchHdr(msgHdr, m_db, &match);
    if (match)
      AddHdr(msgHdr); // do not add a new message if there isn't a match.
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgQuickSearchDBView::OnKeyChange(nsMsgKey aKeyChanged, PRUint32 aOldFlags, 
                                       PRUint32 aNewFlags, nsIDBChangeListener *aInstigator)
{
  nsresult rv = nsMsgDBView::OnKeyChange(aKeyChanged, aOldFlags, aNewFlags, aInstigator);
  // flags haven't really changed - check if the message is newly classified as junk 
  if ((aOldFlags == aNewFlags) && (aOldFlags & MSG_FLAG_NEW)) 
  {
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    rv = m_db->GetMsgHdrForKey(aKeyChanged, getter_AddRefs(msgHdr));
    if (NS_SUCCEEDED(rv) && msgHdr != nsnull)
    {
      nsXPIDLCString junkScoreStr;
      (void) msgHdr->GetStringProperty("junkscore", getter_Copies(junkScoreStr));
      if (atoi(junkScoreStr.get()) > 50)
      {
        nsXPIDLCString originStr;
        (void) msgHdr->GetStringProperty("junkscoreorigin", 
                                       getter_Copies(originStr));

        // if this was classified by the plugin, see if we're supposed to
        // show junk mail
        if (originStr.get()[0] == 'p') 
        {
          PRBool match=PR_FALSE;
          nsCOMPtr <nsIMsgSearchSession> searchSession = do_QueryReferent(m_searchSession);
          if (searchSession)
            searchSession->MatchHdr(msgHdr, m_db, &match);
          if (!match)
          {
            // remove hdr from view
            nsMsgViewIndex deletedIndex = m_keys.FindIndex(aKeyChanged);
            if (deletedIndex != nsMsgViewIndex_None)
              RemoveByIndex(deletedIndex);
          }
        }
      }
    }
  }
  return rv;
}

NS_IMETHODIMP
nsMsgQuickSearchDBView::GetSearchSession(nsIMsgSearchSession* *aSession)
{
  NS_ASSERTION(PR_FALSE, "GetSearchSession method is not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsMsgQuickSearchDBView::SetSearchSession(nsIMsgSearchSession *aSession)
{
  m_searchSession = do_GetWeakReference(aSession);
  return NS_OK;
}

NS_IMETHODIMP
nsMsgQuickSearchDBView::OnSearchHit(nsIMsgDBHdr* aMsgHdr, nsIMsgFolder *folder)
{
  NS_ENSURE_ARG(aMsgHdr);
  NS_ENSURE_ARG(folder);
  nsMsgKey msgKey;
  PRUint32 msgFlags;
  aMsgHdr->GetMessageKey(&msgKey);
  aMsgHdr->GetFlags(&msgFlags);
  m_keys.Add(msgKey);
  m_levels.Add(0);
  m_flags.Add(msgFlags);

  // this needs to happen after we add the key, as RowCountChanged() will call our GetRowCount()
  if (mTree)
    mTree->RowCountChanged(GetSize() - 1, 1);

  return NS_OK;
}

NS_IMETHODIMP
nsMsgQuickSearchDBView::OnSearchDone(nsresult status)
{
  if (m_sortType != nsMsgViewSortType::byThread)//we do not find levels for the results.
  {
    m_sortValid = PR_FALSE;       //sort the results 
    Sort(m_sortType, m_sortOrder);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsMsgQuickSearchDBView::OnNewSearch()
{
  PRInt32 oldSize = GetSize();

  m_keys.RemoveAll();
  m_levels.RemoveAll();
  m_flags.RemoveAll();

  // this needs to happen after we remove all the keys, since RowCountChanged() will call our GetRowCount()
  if (mTree)
    mTree->RowCountChanged(0, -oldSize);

  return NS_OK;
}

NS_IMETHODIMP
nsMsgQuickSearchDBView::Sort(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder)
{
  nsMsgKeyArray preservedSelection;
  SaveAndClearSelection(&preservedSelection);
  nsMsgDBView::Sort(sortType, sortOrder);
  RestoreSelection(&preservedSelection);
  if (mTree) 
    mTree->Invalidate();
  return NS_OK;
}
