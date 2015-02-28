/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code,
 * released March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *     Daniel Veditz <dveditz@netscape.com>
 */

#include "nscore.h"
#include "nsXPITriggerInfo.h"
#include "nsDebug.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsIJSContextStack.h"

static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);


//
// nsXPITriggerItem
//
MOZ_DECL_CTOR_COUNTER(nsXPITriggerItem)

nsXPITriggerItem::nsXPITriggerItem( const PRUnichar* aName,
                                    const PRUnichar* aURL,
                                    const PRUnichar* aIconURL,
                                    PRInt32 aFlags)
  : mName(aName), mURL(aURL), mIconURL(aIconURL), mFlags(aFlags)
{
    MOZ_COUNT_CTOR(nsXPITriggerItem);

    // check for arguments
    PRInt32 qmark = mURL.FindChar('?');
    if ( qmark != kNotFound )
    {
        mArguments = Substring( mURL, qmark+1, mURL.Length() );
    }

    // construct name if not passed in
    if ( mName.IsEmpty() )
    {
        // Use the filename as the display name by starting after the last
        // slash in the URL, looking backwards from the arguments delimiter if
        // we found one. By good fortune using kNotFound as the offset for
        // RFindChar() starts at the end, so we can use qmark in all cases.

        PRInt32 namestart = mURL.RFindChar( '/', qmark );

        // the real start is after the slash (or 0 if not found)
        namestart = ( namestart==kNotFound ) ? 0 : namestart + 1;

        PRInt32 length;
        if (qmark == kNotFound)
            length =  mURL.Length();      // no '?', slurp up rest of URL
        else
            length = (qmark - namestart); // filename stops at the '?'

        mName = Substring( mURL, namestart, length );
    }
}

nsXPITriggerItem::~nsXPITriggerItem()
{
    MOZ_COUNT_DTOR(nsXPITriggerItem);
}

PRBool nsXPITriggerItem::IsRelativeURL()
{
    PRInt32 cpos = mURL.FindChar(':');
    if (cpos == kNotFound)
        return PR_TRUE;

    PRInt32 spos = mURL.FindChar('/');
    return (cpos > spos);
}


void
nsXPITriggerItem::SetPrincipal(nsIPrincipal* aPrincipal)
{
    mPrincipal = aPrincipal;

    // aPrincipal can be null for various failure cases.
    // see bug 213894 for an example.
    // nsXPInstallManager::OnCertAvailable can be called with a null principal
    // and it can also force a null principal.
    if (!aPrincipal)
        return;

    PRBool hasCert;
    aPrincipal->GetHasCertificate(&hasCert);
    if (hasCert) {
        nsXPIDLCString cName;
        aPrincipal->GetCommonName(getter_Copies(cName));
        mCertName = NS_ConvertUTF8toUCS2(cName);
    }
}

//
// nsXPITriggerInfo
//

MOZ_DECL_CTOR_COUNTER(nsXPITriggerInfo)

nsXPITriggerInfo::nsXPITriggerInfo()
  : mCx(0), mCbval(JSVAL_NULL)
{
    MOZ_COUNT_CTOR(nsXPITriggerInfo);
}

nsXPITriggerInfo::~nsXPITriggerInfo()
{
    nsXPITriggerItem* item;

    for(PRUint32 i=0; i < Size(); i++)
    {
        item = Get(i);
        if (item)
            delete item;
    }
    mItems.Clear();

    if ( mCx && !JSVAL_IS_NULL(mCbval) )
        JS_RemoveRoot( mCx, &mCbval );

    MOZ_COUNT_DTOR(nsXPITriggerInfo);
}

void nsXPITriggerInfo::SaveCallback( JSContext *aCx, jsval aVal )
{
    NS_ASSERTION( mCx == 0, "callback set twice, memory leak" );
    mCx = aCx;
    JSObject *obj = JS_GetGlobalObject( mCx );

    JSClass* clazz;

#ifdef JS_THREADSAFE
    clazz = ::JS_GetClass(aCx, obj);
#else
    clazz = ::JS_GetClass(obj);
#endif

    if (clazz &&
        (clazz->flags & JSCLASS_HAS_PRIVATE) &&
        (clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS)) {
      mGlobalWrapper =
        do_QueryInterface((nsISupports*)::JS_GetPrivate(aCx, obj));
    }

    mCbval = aVal;
    mThread = PR_GetCurrentThread();

    if ( !JSVAL_IS_NULL(mCbval) )
        JS_AddRoot( mCx, &mCbval );
}

static void  destroyTriggerEvent(XPITriggerEvent* event)
{
    JS_RemoveRoot( event->cx, &event->cbval );
    delete event;
}

static void* handleTriggerEvent(XPITriggerEvent* event)
{
    jsval  ret;
    void*  mark;
    jsval* args;

    args = JS_PushArguments( event->cx, &mark, "Wi",
                             event->URL.get(),
                             event->status );
    if ( args )
    {
        nsCOMPtr<nsIJSContextStack> stack =
            do_GetService("@mozilla.org/js/xpc/ContextStack;1");
        if (stack)
            stack->Push(event->cx);

        JS_CallFunctionValue( event->cx,
                              JSVAL_TO_OBJECT(event->global),
                              event->cbval,
                              2,
                              args,
                              &ret );

        if (stack)
            stack->Pop(nsnull);

        JS_PopArguments( event->cx, mark );
    }

    return 0;
}


void nsXPITriggerInfo::SendStatus(const PRUnichar* URL, PRInt32 status)
{
    nsCOMPtr<nsIEventQueue> eq;
    nsresult rv;

    if ( mCx && mGlobalWrapper && mCbval )
    {
        nsCOMPtr<nsIEventQueueService> EQService =
                 do_GetService(kEventQueueServiceCID, &rv);
        if ( NS_SUCCEEDED( rv ) )
        {
            rv = EQService->GetThreadEventQueue(mThread, getter_AddRefs(eq));
            if ( NS_SUCCEEDED(rv) )
            {
                // create event and post it
                XPITriggerEvent* event = new XPITriggerEvent();
                if (event)
                {
                    PL_InitEvent(&event->e, 0,
                        (PLHandleEventProc)handleTriggerEvent,
                        (PLDestroyEventProc)destroyTriggerEvent);

                    event->URL      = URL;
                    event->status   = status;
                    event->cx       = mCx;

                    JSObject *obj = nsnull;

                    mGlobalWrapper->GetJSObject(&obj);

                    event->global   = OBJECT_TO_JSVAL(obj);

                    event->cbval    = mCbval;
                    JS_AddNamedRoot( event->cx, &event->cbval,
                                     "XPITriggerEvent::cbval" );

                    // Hold a strong reference to keep the underlying
                    // JSContext from dying before we handle this event.
                    event->ref      = mGlobalWrapper;

                    eq->PostEvent(&event->e);
                }
                else
                    rv = NS_ERROR_OUT_OF_MEMORY;
            }
        }

        if ( NS_FAILED( rv ) )
        {
            // couldn't get event queue -- maybe window is gone or
            // some similarly catastrophic occurrance
        }
    }
}

