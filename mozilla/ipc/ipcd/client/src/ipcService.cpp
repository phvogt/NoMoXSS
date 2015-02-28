/* vim:set ts=4 sw=4 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla IPC.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@meer.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdlib.h>

#include "plstr.h"

#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"
#include "nsIObserverService.h"
#include "nsICategoryManager.h"
#include "nsCategoryManagerUtils.h"
#include "nsEventQueueUtils.h"

#include "ipcConfig.h"
#include "ipcLog.h"
#include "ipcService.h"
#include "ipcMessageUtils.h"
#include "ipcm.h"

#include "ipcIMessageObserver.h"
#include "ipcIClientObserver.h"
#include "ipcIClientQueryHandler.h"

//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------

static PRBool PR_CALLBACK
ipcReleaseMessageObserver(nsHashKey *aKey, void *aData, void* aClosure)
{
    ipcIMessageObserver *obs = (ipcIMessageObserver *) aData;
    NS_RELEASE(obs);
    return PR_TRUE;
}

//----------------------------------------------------------------------------
// ipcClientQuery
//----------------------------------------------------------------------------

class ipcClientQuery
{
public:
    ipcClientQuery(PRUint32 cID, ipcIClientQueryHandler *handler)
        : mNext(nsnull)
        , mQueryID(++gLastQueryID)
        , mClientID(cID)
        , mHandler(handler)
        { }

    static PRUint32 gLastQueryID;

    void SetClientID(PRUint32 cID) { mClientID = cID; }
    void OnQueryComplete(nsresult status, const ipcmMessageClientInfo *msg);

    PRUint32 QueryID()    { return mQueryID; }
    PRBool   IsCanceled() { return mHandler.get() == NULL; }

    ipcClientQuery                  *mNext;
private:
    PRUint32                         mQueryID;
    PRUint32                         mClientID;
    nsCOMPtr<ipcIClientQueryHandler> mHandler;
};

PRUint32 ipcClientQuery::gLastQueryID = 0;

void
ipcClientQuery::OnQueryComplete(nsresult status, const ipcmMessageClientInfo *msg)
{
    NS_ASSERTION(mHandler, "no handler");

    PRUint32 nameCount = 0;
    PRUint32 targetCount = 0;
    const char **names = NULL;
    const nsID **targets = NULL;

    if (NS_SUCCEEDED(status)) {
        nameCount = msg->NameCount();
        targetCount = msg->TargetCount();
        PRUint32 i;

        names = (const char **) malloc(nameCount * sizeof(char *));
        const char *lastName = NULL;
        for (i = 0; i < nameCount; ++i) {
            lastName = msg->NextName(lastName);
            names[i] = lastName;
        }

        targets = (const nsID **) malloc(targetCount * sizeof(nsID *));
        const nsID *lastTarget = NULL;
        for (i = 0; i < targetCount; ++i) {
            lastTarget = msg->NextTarget(lastTarget);
            targets[i] = lastTarget;
        }
    }

    mHandler->OnQueryComplete(mQueryID,
                              status,
                              mClientID,
                              names, nameCount,
                              targets, targetCount);
    mHandler = NULL;

    if (names)
        free(names);
    if (targets)
        free(targets);
}

//-----------------------------------------------------------------------------
// ipcService
//-----------------------------------------------------------------------------

ipcService::ipcService()
    : mTransport(nsnull)
    , mClientID(0)
{
    IPC_InitLog(">>>");
}

ipcService::~ipcService()
{
    NS_ASSERTION(mTransport == nsnull, "no xpcom-shutdown event??");
}

nsresult
ipcService::Init()
{
    nsCOMPtr<nsIObserverService> observ(do_GetService("@mozilla.org/observer-service;1"));
    if (observ)
        observ->AddObserver(this, "xpcom-shutdown", PR_FALSE);

    nsresult rv;
    mTransport = new ipcTransport();
    if (!mTransport)
        rv = NS_ERROR_OUT_OF_MEMORY;
    else {
        rv = mTransport->Init(this, &mClientID);
        if (NS_FAILED(rv))
            mTransport = nsnull;
        else {
            //
            // broadcast IPC startup...
            //
            NS_CreateServicesFromCategory(IPC_SERVICE_STARTUP_CATEGORY,
                                          NS_STATIC_CAST(ipcIService *, this),
                                          IPC_SERVICE_STARTUP_TOPIC);
        }
    }
    return rv;
}

void
ipcService::Shutdown()
{
    //
    // broadcast IPC shutdown...
    //
    nsCOMPtr<nsIObserverService> observ(
            do_GetService("@mozilla.org/observer-service;1"));
    if (observ)
        observ->NotifyObservers(NS_STATIC_CAST(ipcIService *, this),
                                IPC_SERVICE_SHUTDOWN_TOPIC, nsnull);

    // error out any pending queries
    while (mQueryQ.First()) {
        ipcClientQuery *query = mQueryQ.First();
        query->OnQueryComplete(NS_ERROR_ABORT, NULL);
        mQueryQ.DeleteFirst();
    }

    // disconnect any message observers
    mObserverDB.Reset(ipcReleaseMessageObserver, nsnull);

    // drop daemon connection
    if (mTransport) {
        mTransport->Shutdown();
        mTransport = nsnull;
    }

    mClientID = 0;
}

void
ipcService::OnIPCMClientID(const ipcmMessageClientID *msg)
{
    LOG(("ipcService::OnIPCMClientID\n"));

    ipcClientQuery *query = mQueryQ.First();
    if (!query) {
        NS_WARNING("no pending query; ignoring message.");
        return;
    }

    PRUint32 cID = msg->ClientID();
    PRBool sync = msg->TestFlag(IPC_MSG_FLAG_SYNC_REPLY);

    //
    // (1) store client ID in query
    // (2) move query to end of queue
    // (3) issue CLIENT_INFO request
    //
    query->SetClientID(cID);

    mQueryQ.RemoveFirst();
    mQueryQ.Append(query);

    mTransport->SendMsg(new ipcmMessageQueryClientInfo(cID), sync);
}

void
ipcService::OnIPCMClientInfo(const ipcmMessageClientInfo *msg)
{
    LOG(("ipcService::OnIPCMClientInfo\n"));

    ipcClientQuery *query = mQueryQ.First();
    if (!query) {
        NS_WARNING("no pending query; ignoring message.");
        return;
    }

    if (!query->IsCanceled())
        query->OnQueryComplete(NS_OK, msg);

    mQueryQ.DeleteFirst();
}

void
ipcService::OnIPCMError(const ipcmMessageError *msg)
{
    LOG(("ipcService::OnIPCMError [reason=0x%08x]\n", msg->Reason()));

    ipcClientQuery *query = mQueryQ.First();
    if (!query) {
        NS_WARNING("no pending query; ignoring message.");
        return;
    }

    if (!query->IsCanceled())
        query->OnQueryComplete(NS_ERROR_FAILURE, NULL);

    mQueryQ.DeleteFirst();
}

//-----------------------------------------------------------------------------
// interface impl
//-----------------------------------------------------------------------------

NS_IMPL_ISUPPORTS2(ipcService, ipcIService, nsIObserver)

NS_IMETHODIMP
ipcService::GetClientID(PRUint32 *clientID)
{
    NS_ENSURE_TRUE(mClientID != 0, NS_ERROR_NOT_INITIALIZED);

    *clientID = mClientID;
    return NS_OK;
}

NS_IMETHODIMP
ipcService::AddClientName(const char *name)
{
    NS_ENSURE_TRUE(mTransport, NS_ERROR_NOT_INITIALIZED);

    ipcMessage *msg = new ipcmMessageClientAddName(name);
    if (!msg)
        return NS_ERROR_OUT_OF_MEMORY;

    return mTransport->SendMsg(msg);
}

NS_IMETHODIMP
ipcService::RemoveClientName(const char *name)
{
    NS_ENSURE_TRUE(mTransport, NS_ERROR_NOT_INITIALIZED);

    ipcMessage *msg = new ipcmMessageClientDelName(name);
    if (!msg)
        return NS_ERROR_OUT_OF_MEMORY;

    return mTransport->SendMsg(msg);
}

NS_IMETHODIMP
ipcService::QueryClientByName(const char *name,
                              ipcIClientQueryHandler *handler,
                              PRBool sync,
                              PRUint32 *queryID)
{
    if (!mTransport)
        return NS_ERROR_NOT_AVAILABLE;

    ipcMessage *msg;

    msg = new ipcmMessageQueryClientByName(name);
    if (!msg)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;
    
    rv = mTransport->SendMsg(msg, sync);
    if (NS_FAILED(rv)) return rv;

    ipcClientQuery *query = new ipcClientQuery(0, handler);
    if (queryID)
        *queryID = query->QueryID();
    mQueryQ.Append(query);
    return NS_OK;
}

NS_IMETHODIMP
ipcService::QueryClientByID(PRUint32 clientID,
                            ipcIClientQueryHandler *handler,
                            PRBool sync,
                            PRUint32 *queryID)
{
    if (!mTransport)
        return NS_ERROR_NOT_AVAILABLE;

    ipcMessage *msg;

    msg = new ipcmMessageQueryClientInfo(clientID);
    if (!msg)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;
    
    rv = mTransport->SendMsg(msg, sync);
    if (NS_FAILED(rv)) return rv;

    ipcClientQuery *query = new ipcClientQuery(clientID, handler);
    if (queryID)
        *queryID = query->QueryID();
    mQueryQ.Append(query);
    return NS_OK;
}

NS_IMETHODIMP
ipcService::CancelQuery(PRUint32 queryID)
{
    ipcClientQuery *query = mQueryQ.First();
    while (query) {
        if (query->QueryID() == queryID) {
            query->OnQueryComplete(NS_ERROR_ABORT, NULL);
            break;
        }
        query = query->mNext;
    }
    return NS_OK;
}

NS_IMETHODIMP
ipcService::SetClientObserver(ipcIClientObserver *observer)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ipcService::SetMessageObserver(const nsID &target, ipcIMessageObserver *observer)
{
    NS_ENSURE_TRUE(mTransport, NS_ERROR_NOT_INITIALIZED);

    nsIDKey key(target);
    PRBool sendAdd = PR_TRUE;

    ipcIMessageObserver *cobs = (ipcIMessageObserver *) mObserverDB.Get(&key);
    if (cobs) {
        NS_RELEASE(cobs);
        if (!observer) {
            mObserverDB.Remove(&key);
            //
            // send CLIENT_DEL_TARGET
            //
            mTransport->SendMsg(new ipcmMessageClientDelTarget(target));
            return NS_OK;
        }
        sendAdd = PR_FALSE;
    }
    if (observer) {
        NS_ADDREF(observer);
        mObserverDB.Put(&key, observer);
        if (sendAdd) {
            //
            // send CLIENT_ADD_TARGET
            //
            mTransport->SendMsg(new ipcmMessageClientAddTarget(target));
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
ipcService::SendMessage(PRUint32 clientID,
                        const nsID &target,
                        const PRUint8 *data,
                        PRUint32 dataLen,
                        PRBool sync)
{
    NS_ENSURE_TRUE(mTransport, NS_ERROR_NOT_INITIALIZED);

    if (target.Equals(IPCM_TARGET)) {
        NS_ERROR("do not try to talk to the IPCM target directly");
        return NS_ERROR_INVALID_ARG;
    }

    ipcMessage *msg;
    if (clientID)
        msg = new ipcmMessageForward(clientID, target, (const char *) data, dataLen);
    else
        msg = new ipcMessage(target, (const char *) data, dataLen);

    if (!msg)
        return NS_ERROR_OUT_OF_MEMORY;

    return mTransport->SendMsg(msg, sync);
}

//-----------------------------------------------------------------------------
// nsIObserver impl
//-----------------------------------------------------------------------------

NS_IMETHODIMP
ipcService::Observe(nsISupports *subject, const char *topic, const PRUnichar *data)
{
    if (strcmp(topic, "xpcom-shutdown") == 0)
        Shutdown();
    return NS_OK;
}

//-----------------------------------------------------------------------------
// ipcTransportObserver impl
//-----------------------------------------------------------------------------

void
ipcService::OnConnectionLost()
{
    Shutdown();
}

void
ipcService::OnMessageAvailable(const ipcMessage *msg)
{
    LOG(("ipcService::OnMessageAvailable [msg=%p]\n", msg));

    if (msg->Target().Equals(IPCM_TARGET)) {
        //
        // all IPCM messages stop here.
        //
        PRUint32 type = IPCM_GetMsgType(msg);
        switch (type) {
        case IPCM_MSG_TYPE_CLIENT_ID:
            OnIPCMClientID((const ipcmMessageClientID *) msg);
            break;
        case IPCM_MSG_TYPE_CLIENT_INFO:
            OnIPCMClientInfo((const ipcmMessageClientInfo *) msg);
            break;
        case IPCM_MSG_TYPE_ERROR:
            OnIPCMError((const ipcmMessageError *) msg);
            break;
        }
    }
    else {
        nsIDKey key(msg->Target());
        ipcIMessageObserver *observer = (ipcIMessageObserver *) mObserverDB.Get(&key);
        if (observer)
            observer->OnMessageAvailable(msg->Target(),
                                         (const PRUint8 *) msg->Data(),
                                         msg->DataLen());
    }
}
