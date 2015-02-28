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

#include "nscore.h"
#include "msgCore.h"		// precompiled header
#include "nspr.h"

#include "nsIEventQueueService.h"

#include "nsIImapMailFolderSink.h"
#include "nsIImapMessageSink.h"
#include "nsIImapExtensionSink.h"
#include "nsIImapUrl.h"
#include "nsIImapMiscellaneousSink.h"
#include "nsImapProxyEvent.h"
#include "nsIMAPNamespace.h"
#include "nsImapCore.h"
#include "nsCOMPtr.h"

nsImapEvent::nsImapEvent()
{
    m_notifyCompletion = PR_FALSE;
}

nsImapEvent::~nsImapEvent()
{
}

void
nsImapEvent::SetNotifyCompletion(PRBool notifyCompletion)
{
    m_notifyCompletion = notifyCompletion;
}

void
nsImapEvent::InitEvent()
{
		PL_InitEvent(this, nsnull,
								 (PLHandleEventProc) imap_event_handler,
								 (PLDestroyEventProc) imap_event_destructor);
}

void
nsImapEvent::PostEvent(nsIEventQueue* aEventQ)
{
		NS_PRECONDITION(nsnull != aEventQ, "PostEvent: aEventQ is null");

		InitEvent();
		aEventQ->PostEvent(this);
}

void PR_CALLBACK
nsImapEvent::imap_event_handler(PLEvent *aEvent)
{
		nsImapEvent* ev = (nsImapEvent*) aEvent;
		ev->HandleEvent();
}

void PR_CALLBACK
nsImapEvent::imap_event_destructor(PLEvent *aEvent)
{
		nsImapEvent* ev = (nsImapEvent*) aEvent;
		delete ev;
}

nsImapProxyBase::nsImapProxyBase(nsIImapProtocol* aProtocol,
                                 nsIEventQueue* aEventQ,
                                 PRThread* aThread)
{
    NS_ASSERTION (aProtocol && aEventQ && aThread,
                  "nsImapProxy: invalid aProtocol, aEventQ, or aThread");

    m_protocol = aProtocol;
    NS_IF_ADDREF(m_protocol);
		
		m_eventQueue = aEventQ;
		NS_IF_ADDREF(m_eventQueue);

		m_thread = aThread;
}

nsImapProxyBase::~nsImapProxyBase()
{
    NS_IF_RELEASE (m_protocol);
		NS_IF_RELEASE(m_eventQueue);
}

nsImapExtensionSinkProxy::nsImapExtensionSinkProxy(nsIImapExtensionSink* aImapExtensionSink,
                                           nsIImapProtocol* aProtocol,
                                           nsIEventQueue* aEventQ,
                                           PRThread* aThread) :
    nsImapProxyBase(aProtocol, aEventQ, aThread)
{
    NS_ASSERTION (aImapExtensionSink, 
                  "nsImapExtensionSinkProxy: invalid aImapExtensionSink");
    m_realImapExtensionSink = aImapExtensionSink;
    NS_IF_ADDREF (m_realImapExtensionSink);
}

nsImapExtensionSinkProxy::~nsImapExtensionSinkProxy()
{
    NS_IF_RELEASE (m_realImapExtensionSink);
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsImapExtensionSinkProxy, nsIImapExtensionSink)

NS_IMETHODIMP
nsImapExtensionSinkProxy::ClearFolderRights(nsIImapProtocol* aProtocol,
                                        nsIMAPACLRightsInfo* aclRights)
{
    nsresult res = NS_OK;
    NS_PRECONDITION (aclRights, "Oops... null aclRights");
    if(!aclRights)
        return NS_ERROR_NULL_POINTER;
    NS_ASSERTION (m_protocol == aProtocol, "Ooh ooh, wrong protocol");

    if (PR_GetCurrentThread() == m_thread)
    {
        ClearFolderRightsProxyEvent *ev =
            new ClearFolderRightsProxyEvent(this, aclRights);
        if(nsnull == ev)
            res = NS_ERROR_OUT_OF_MEMORY;
        else
        {
            ev->SetNotifyCompletion(PR_TRUE);
            ev->PostEvent(m_eventQueue);
        }
    }
    else
    {
        res = m_realImapExtensionSink->ClearFolderRights(aProtocol, aclRights);
        aProtocol->NotifyFEEventCompletion();
    }
    return res;
}


NS_IMETHODIMP
nsImapExtensionSinkProxy::SetCopyResponseUid(nsIImapProtocol* aProtocol,
                                             nsMsgKeyArray* aKeyArray,
                                             const char* msgIdString,
                                             nsIImapUrl * aUrl)
{
    nsresult res = NS_OK;
    NS_PRECONDITION (aKeyArray, "Oops... null aKeyArray");
    if(!aKeyArray)
        return NS_ERROR_NULL_POINTER;
    NS_ASSERTION (m_protocol == aProtocol, "Ooh ooh, wrong protocol");

    if (PR_GetCurrentThread() == m_thread)
    {
        SetCopyResponseUidProxyEvent *ev =
            new SetCopyResponseUidProxyEvent(this, aKeyArray, msgIdString,
                                             aUrl);
        if(nsnull == ev)
            res = NS_ERROR_OUT_OF_MEMORY;
        else
        {
            ev->SetNotifyCompletion(PR_TRUE);
            ev->PostEvent(m_eventQueue);
        }
    }
    else
    {
        res = m_realImapExtensionSink->SetCopyResponseUid(aProtocol,
                                                          aKeyArray,
                                                          msgIdString,
                                                          aUrl);
    }
    return res;
}

NS_IMETHODIMP
nsImapExtensionSinkProxy::SetAppendMsgUid(nsIImapProtocol* aProtocol,
                                          nsMsgKey aKey,
                                          nsIImapUrl * aUrl)
{
    nsresult res = NS_OK;
    NS_ASSERTION (m_protocol == aProtocol, "Ooh ooh, wrong protocol");

    if (PR_GetCurrentThread() == m_thread)
    {
        SetAppendMsgUidProxyEvent *ev =
            new SetAppendMsgUidProxyEvent(this, aKey, aUrl);
        if(nsnull == ev)
            res = NS_ERROR_OUT_OF_MEMORY;
        else
        {
            ev->SetNotifyCompletion(PR_TRUE);
            ev->PostEvent(m_eventQueue);
        }
    }
    else
    {
        res = m_realImapExtensionSink->SetAppendMsgUid(aProtocol,
                                                       aKey,
                                                       aUrl);
    }
    return res;
}

NS_IMETHODIMP
nsImapExtensionSinkProxy::GetMessageId(nsIImapProtocol* aProtocol,
                                       nsCString* messageId,
                                       nsIImapUrl * aUrl)
{
    nsresult res = NS_OK;
    NS_ASSERTION (m_protocol == aProtocol, "Ooh ooh, wrong protocol");

    if (PR_GetCurrentThread() == m_thread)
    {
        GetMessageIdProxyEvent *ev =
            new GetMessageIdProxyEvent(this, messageId, aUrl);
        if(nsnull == ev)
            res = NS_ERROR_OUT_OF_MEMORY;
        else
        {
            ev->SetNotifyCompletion(PR_TRUE);
            ev->PostEvent(m_eventQueue);
        }
    }
    else
    {
        res = m_realImapExtensionSink->GetMessageId(aProtocol,
                                                    messageId,
                                                    aUrl);
    }
    return res;
}

nsImapMiscellaneousSinkProxy::nsImapMiscellaneousSinkProxy(
    nsIImapMiscellaneousSink* aImapMiscellaneousSink, 
    nsIImapProtocol* aProtocol,
    nsIEventQueue* aEventQ,
    PRThread* aThread) : nsImapProxyBase(aProtocol, aEventQ, aThread)
{
    NS_ASSERTION (aImapMiscellaneousSink, 
                  "nsImapMiscellaneousSinkProxy: invalid aImapMiscellaneousSink");
    m_realImapMiscellaneousSink = aImapMiscellaneousSink;
    NS_IF_ADDREF (m_realImapMiscellaneousSink);
}

nsImapMiscellaneousSinkProxy::~nsImapMiscellaneousSinkProxy()
{
    NS_IF_RELEASE (m_realImapMiscellaneousSink);
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsImapMiscellaneousSinkProxy, nsIImapMiscellaneousSink)

NS_IMETHODIMP
nsImapMiscellaneousSinkProxy::HeaderFetchCompleted(nsIImapProtocol* aProtocol)
{
    nsresult res = NS_OK;
    NS_ASSERTION (m_protocol == aProtocol, "Ooh ooh, wrong protocol");

    if (PR_GetCurrentThread() == m_thread)
    {
        HeaderFetchCompletedProxyEvent *ev =
            new HeaderFetchCompletedProxyEvent(this);
        if(nsnull == ev)
            res = NS_ERROR_OUT_OF_MEMORY;
        else
        {
            ev->SetNotifyCompletion(PR_TRUE);
            ev->PostEvent(m_eventQueue);
        }
    }
    else
    {
        res = m_realImapMiscellaneousSink->HeaderFetchCompleted(aProtocol);
        aProtocol->NotifyFEEventCompletion();
    }
    return res;
}


NS_IMETHODIMP
nsImapMiscellaneousSinkProxy::SetBiffStateAndUpdate(nsIImapProtocol* aProtocol,
                                                nsMsgBiffState biffState)
{
    nsresult res = NS_OK;
    NS_ASSERTION (m_protocol == aProtocol, "Ooh ooh, wrong protocol");

    if (PR_GetCurrentThread() == m_thread)
    {
        SetBiffStateAndUpdateProxyEvent *ev =
            new SetBiffStateAndUpdateProxyEvent(this, biffState);
        if(nsnull == ev)
            res = NS_ERROR_OUT_OF_MEMORY;
        else
        {
            ev->SetNotifyCompletion(PR_TRUE);
            ev->PostEvent(m_eventQueue);
        }
    }
    else
    {
        res = m_realImapMiscellaneousSink->SetBiffStateAndUpdate(aProtocol, biffState);
        aProtocol->NotifyFEEventCompletion();
    }
    return res;
}

NS_IMETHODIMP
nsImapMiscellaneousSinkProxy::ProgressStatus(nsIImapProtocol* aProtocol,
                                         PRUint32 aMsgId, const PRUnichar *extraInfo)
{
    nsresult res = NS_OK;
    NS_ASSERTION (m_protocol == aProtocol, "Ooh ooh, wrong protocol");

    if (PR_GetCurrentThread() == m_thread)
    {
        ProgressStatusProxyEvent *ev =
            new ProgressStatusProxyEvent(this, aMsgId, extraInfo);
        if(nsnull == ev)
            res = NS_ERROR_OUT_OF_MEMORY;
        else
            ev->PostEvent(m_eventQueue);
    }
    else
    {
        res = m_realImapMiscellaneousSink->ProgressStatus(aProtocol, aMsgId, extraInfo);
    }
    return res;
}

NS_IMETHODIMP
nsImapMiscellaneousSinkProxy::PercentProgress(nsIImapProtocol* aProtocol,
                                          ProgressInfo* aInfo)
{
    nsresult res = NS_OK;
    NS_PRECONDITION (aInfo, "Oops... null aInfo");
    if(!aInfo)
        return NS_ERROR_NULL_POINTER;
    NS_ASSERTION (m_protocol == aProtocol, "Ooh ooh, wrong protocol");

    if (PR_GetCurrentThread() == m_thread)
    {
        PercentProgressProxyEvent *ev =
            new PercentProgressProxyEvent(this, aInfo);
        if(nsnull == ev)
            res = NS_ERROR_OUT_OF_MEMORY;
        else
            ev->PostEvent(m_eventQueue);
    }
    else
    {
        res = m_realImapMiscellaneousSink->PercentProgress(aProtocol, aInfo);
    }
    return res;
}

////
nsImapExtensionSinkProxyEvent::nsImapExtensionSinkProxyEvent(
    nsImapExtensionSinkProxy* aProxy)
{
    NS_ASSERTION (aProxy, "fatal a null imap extension proxy");
    m_proxy = aProxy;
    NS_IF_ADDREF (m_proxy);
}

nsImapExtensionSinkProxyEvent::~nsImapExtensionSinkProxyEvent()
{
    NS_IF_RELEASE (m_proxy);
}

ClearFolderRightsProxyEvent::ClearFolderRightsProxyEvent(
    nsImapExtensionSinkProxy* aProxy, nsIMAPACLRightsInfo* aclRights) :
    nsImapExtensionSinkProxyEvent(aProxy)
{
    NS_ASSERTION (aclRights, "Oops... a null acl rights info");
    if (aclRights)
    {
        m_aclRightsInfo.hostName = PL_strdup(aclRights->hostName);
        m_aclRightsInfo.mailboxName = PL_strdup(aclRights->mailboxName);
        m_aclRightsInfo.userName = PL_strdup(aclRights->userName);
        m_aclRightsInfo.rights = PL_strdup(aclRights->rights);
    }
    else
    {
        m_aclRightsInfo.hostName = nsnull;
        m_aclRightsInfo.mailboxName = nsnull;
        m_aclRightsInfo.userName = nsnull;
        m_aclRightsInfo.rights = nsnull;
    }
}

ClearFolderRightsProxyEvent::~ClearFolderRightsProxyEvent()
{
    if (m_aclRightsInfo.hostName)
        PL_strfree(m_aclRightsInfo.hostName);
    if (m_aclRightsInfo.mailboxName)
        PL_strfree(m_aclRightsInfo.mailboxName);
    if (m_aclRightsInfo.userName)
        PL_strfree(m_aclRightsInfo.userName);
    if (m_aclRightsInfo.rights)
        PL_strfree(m_aclRightsInfo.rights);
}

NS_IMETHODIMP
ClearFolderRightsProxyEvent::HandleEvent()
{
    nsresult res = m_proxy->m_realImapExtensionSink->ClearFolderRights(
        m_proxy->m_protocol, &m_aclRightsInfo); 
    if (m_notifyCompletion)
        m_proxy->m_protocol->NotifyFEEventCompletion();
    return res;
}

SetCopyResponseUidProxyEvent::SetCopyResponseUidProxyEvent(
    nsImapExtensionSinkProxy* aProxy, nsMsgKeyArray* aKeyArray,
    const char* msgIdString, nsIImapUrl * aUrl) :
    nsImapExtensionSinkProxyEvent(aProxy), m_msgIdString(msgIdString)
{
    NS_ASSERTION (aKeyArray, "Oops... a null key array");
    if (aKeyArray)
    {
        m_copyKeyArray.CopyArray(aKeyArray);
    }
    m_Url = aUrl;
}

SetCopyResponseUidProxyEvent::~SetCopyResponseUidProxyEvent()
{
}

NS_IMETHODIMP
SetCopyResponseUidProxyEvent::HandleEvent()
{
    nsresult res = m_proxy->m_realImapExtensionSink->SetCopyResponseUid(
        m_proxy->m_protocol, &m_copyKeyArray, m_msgIdString.get(),
        m_Url);
    if (m_notifyCompletion)
        m_proxy->m_protocol->NotifyFEEventCompletion();
    return res;
}

SetAppendMsgUidProxyEvent::SetAppendMsgUidProxyEvent(
    nsImapExtensionSinkProxy* aProxy, nsMsgKey aKey, nsIImapUrl * aUrl) :
    nsImapExtensionSinkProxyEvent(aProxy), m_key(aKey)
{
  m_Url = aUrl;
}

SetAppendMsgUidProxyEvent::~SetAppendMsgUidProxyEvent()
{
}

NS_IMETHODIMP
SetAppendMsgUidProxyEvent::HandleEvent()
{
    nsresult res = m_proxy->m_realImapExtensionSink->SetAppendMsgUid(
        m_proxy->m_protocol, m_key, m_Url);
    if (m_notifyCompletion)
        m_proxy->m_protocol->NotifyFEEventCompletion();
    return res;
}

GetMessageIdProxyEvent::GetMessageIdProxyEvent(
    nsImapExtensionSinkProxy* aProxy, nsCString* messageId, 
    nsIImapUrl * aUrl) :
    nsImapExtensionSinkProxyEvent(aProxy), m_messageId(messageId)
{
  m_Url = aUrl;
}

GetMessageIdProxyEvent::~GetMessageIdProxyEvent()
{
}

NS_IMETHODIMP
GetMessageIdProxyEvent::HandleEvent()
{
    nsresult res = m_proxy->m_realImapExtensionSink->GetMessageId(
        m_proxy->m_protocol, m_messageId, m_Url);
    if (m_notifyCompletion)
        m_proxy->m_protocol->NotifyFEEventCompletion();
    return res;
}

nsImapMiscellaneousSinkProxyEvent::nsImapMiscellaneousSinkProxyEvent(
    nsImapMiscellaneousSinkProxy* aProxy)
{
    NS_ASSERTION (aProxy, "fatal: a null imap miscellaneous proxy");
    m_proxy = aProxy;
    NS_IF_ADDREF (m_proxy);
}

nsImapMiscellaneousSinkProxyEvent::~nsImapMiscellaneousSinkProxyEvent()
{
    NS_IF_RELEASE (m_proxy);
}

HeaderFetchCompletedProxyEvent::HeaderFetchCompletedProxyEvent(
    nsImapMiscellaneousSinkProxy* aProxy) :
    nsImapMiscellaneousSinkProxyEvent(aProxy)
{
}

HeaderFetchCompletedProxyEvent::~HeaderFetchCompletedProxyEvent()
{
}

NS_IMETHODIMP
HeaderFetchCompletedProxyEvent::HandleEvent()
{
    nsresult res = m_proxy->m_realImapMiscellaneousSink->HeaderFetchCompleted(
        m_proxy->m_protocol);
    if (m_notifyCompletion)
        m_proxy->m_protocol->NotifyFEEventCompletion();
    return res;
}

SetBiffStateAndUpdateProxyEvent::SetBiffStateAndUpdateProxyEvent(
    nsImapMiscellaneousSinkProxy* aProxy, nsMsgBiffState biffState) :
    nsImapMiscellaneousSinkProxyEvent(aProxy)
{
    m_biffState = biffState;
}

SetBiffStateAndUpdateProxyEvent::~SetBiffStateAndUpdateProxyEvent()
{
}

NS_IMETHODIMP
SetBiffStateAndUpdateProxyEvent::HandleEvent()
{
    nsresult res = m_proxy->m_realImapMiscellaneousSink->SetBiffStateAndUpdate(
        m_proxy->m_protocol, m_biffState);
    if (m_notifyCompletion)
        m_proxy->m_protocol->NotifyFEEventCompletion();
    return res;
}


ProgressStatusProxyEvent::ProgressStatusProxyEvent(
    nsImapMiscellaneousSinkProxy* aProxy, PRUint32 aMsgId, const PRUnichar *extraInfo) :
    nsImapMiscellaneousSinkProxyEvent(aProxy)
{
	m_statusMsgId = aMsgId;
	m_extraInfo = (extraInfo) ? nsCRT::strdup(extraInfo) : nsnull;
}

ProgressStatusProxyEvent::~ProgressStatusProxyEvent()
{
	if (m_extraInfo)
		nsCRT::free(m_extraInfo);
}

NS_IMETHODIMP
ProgressStatusProxyEvent::HandleEvent()
{
    nsresult res = m_proxy->m_realImapMiscellaneousSink->ProgressStatus(
        m_proxy->m_protocol, m_statusMsgId, m_extraInfo);
    if (m_notifyCompletion)
        m_proxy->m_protocol->NotifyFEEventCompletion();
    return res;
}

PercentProgressProxyEvent::PercentProgressProxyEvent(
    nsImapMiscellaneousSinkProxy* aProxy, ProgressInfo* aInfo) :
    nsImapMiscellaneousSinkProxyEvent(aProxy)
{
    NS_ASSERTION (aInfo, "Oops... a null progress info");
    if (aInfo)
    {
        m_progressInfo.message = (aInfo->message) ? nsCRT::strdup(aInfo->message) : nsnull;
        m_progressInfo.currentProgress = aInfo->currentProgress;
        m_progressInfo.maxProgress = aInfo->maxProgress;
    }
    else
    {
        m_progressInfo.message = nsnull;
        m_progressInfo.maxProgress = -1;
        m_progressInfo.currentProgress = 0;

    }
}

PercentProgressProxyEvent::~PercentProgressProxyEvent()
{
    if (m_progressInfo.message)
        PR_Free(m_progressInfo.message);
}

NS_IMETHODIMP
PercentProgressProxyEvent::HandleEvent()
{
    nsresult res = m_proxy->m_realImapMiscellaneousSink->PercentProgress(
        m_proxy->m_protocol, &m_progressInfo);
    if (m_notifyCompletion)
        m_proxy->m_protocol->NotifyFEEventCompletion();
    return res;
}

