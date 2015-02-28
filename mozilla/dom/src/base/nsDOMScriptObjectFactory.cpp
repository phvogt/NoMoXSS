/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date         Modified by     Description of modification
 * 03/27/2000   IBM Corp.       Added PR_CALLBACK for Optlink
 *                               use in OS2
 */

#include "nsDOMScriptObjectFactory.h"
#include "nsScriptNameSpaceManager.h"
#include "nsIObserverService.h"
#include "nsJSEnvironment.h"
#include "nsJSEventListener.h"
#include "nsGlobalWindow.h"
#include "nsIJSContextStack.h"
#include "nsDOMException.h"
#ifdef MOZ_XUL
#include "nsIXULPrototypeCache.h"
#endif

nsDOMScriptObjectFactory::nsDOMScriptObjectFactory()
{
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");

  if (observerService) {
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  }

  nsCOMPtr<nsIExceptionService> xs =
    do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);

  if (xs) {
    xs->RegisterExceptionProvider(this, NS_ERROR_MODULE_DOM);
    xs->RegisterExceptionProvider(this, NS_ERROR_MODULE_DOM_RANGE);
  }
}

NS_INTERFACE_MAP_BEGIN(nsDOMScriptObjectFactory)
  NS_INTERFACE_MAP_ENTRY(nsIDOMScriptObjectFactory)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIExceptionProvider)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMScriptObjectFactory)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDOMScriptObjectFactory)
NS_IMPL_RELEASE(nsDOMScriptObjectFactory)


NS_IMETHODIMP
nsDOMScriptObjectFactory::NewScriptContext(nsIScriptGlobalObject *aGlobal,
					   nsIScriptContext **aContext)
{
  return NS_CreateScriptContext(aGlobal, aContext);
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::NewJSEventListener(nsIScriptContext *aContext,
					     nsISupports *aObject,
					     nsIDOMEventListener **aInstancePtrResult)
{
  return NS_NewJSEventListener(aInstancePtrResult, aContext, aObject);
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::NewScriptGlobalObject(PRBool aIsChrome,
						nsIScriptGlobalObject **aGlobal)
{
  return NS_NewScriptGlobalObject(aIsChrome, aGlobal);
}

NS_IMETHODIMP_(nsISupports *)
nsDOMScriptObjectFactory::GetClassInfoInstance(nsDOMClassInfoID aID)
{
  return nsDOMClassInfo::GetClassInfoInstance(aID);
}

NS_IMETHODIMP_(nsISupports *)
nsDOMScriptObjectFactory::GetExternalClassInfoInstance(const nsAString& aName)
{
  extern nsScriptNameSpaceManager *gNameSpaceManager;

  NS_ENSURE_TRUE(gNameSpaceManager, nsnull);

  const nsGlobalNameStruct *globalStruct;
  gNameSpaceManager->LookupName(aName, &globalStruct);
  if (globalStruct) {
    if (globalStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfoCreator) {
      nsresult rv;
      nsCOMPtr<nsIDOMCIExtension> creator(do_CreateInstance(globalStruct->mCID, &rv));
      NS_ENSURE_SUCCESS(rv, nsnull);

      rv = creator->RegisterDOMCI(NS_ConvertUCS2toUTF8(aName).get(), this);
      NS_ENSURE_SUCCESS(rv, nsnull);

      rv = gNameSpaceManager->LookupName(aName, &globalStruct);
      NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && globalStruct, nsnull);

      NS_ASSERTION(globalStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo,
                   "The classinfo data for this class didn't get registered.");
    }
    if (globalStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
      return nsDOMClassInfo::GetClassInfoInstance(globalStruct->mData);
    }
  }
  return nsnull;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::Observe(nsISupports *aSubject,
				  const char *aTopic,
				  const PRUnichar *someData)
{
  if (!nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
#ifdef MOZ_XUL
    // Flush the XUL cache since it holds JS roots, and we're about to
    // start the final GC.
    nsCOMPtr<nsIXULPrototypeCache> cache =
      do_GetService("@mozilla.org/xul/xul-prototype-cache;1");

    if (cache)
      cache->Flush();
#endif

    nsCOMPtr<nsIThreadJSContextStack> stack =
      do_GetService("@mozilla.org/js/xpc/ContextStack;1");

    if (stack) {
      JSContext *cx = nsnull;

      stack->GetSafeJSContext(&cx);

      if (cx) {
        // Do one final GC to clean things up before shutdown.

        ::JS_GC(cx);
      }
    }

    GlobalWindowImpl::ShutDown();
    nsDOMClassInfo::ShutDown();
    nsJSEnvironment::ShutDown();

    nsCOMPtr<nsIExceptionService> xs =
      do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);

    if (xs) {
      xs->UnregisterExceptionProvider(this, NS_ERROR_MODULE_DOM);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::GetException(nsresult result,
				       nsIException *aDefaultException,
				       nsIException **_retval)
{
  if (NS_ERROR_GET_MODULE(result) == NS_ERROR_MODULE_DOM_RANGE) {
    return NS_NewRangeException(result, aDefaultException, _retval);
  }
  return NS_NewDOMException(result, aDefaultException, _retval);
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::RegisterDOMClassInfo(const char *aName,
					       nsDOMClassInfoExternalConstructorFnc aConstructorFptr,
					       const nsIID *aProtoChainInterface,
					       const nsIID **aInterfaces,
					       PRUint32 aScriptableFlags,
					       PRBool aHasClassInterface,
					       const nsCID *aConstructorCID)
{
  extern nsScriptNameSpaceManager *gNameSpaceManager;

  NS_ENSURE_TRUE(gNameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  return gNameSpaceManager->RegisterDOMCIData(aName,
                                              aConstructorFptr,
                                              aProtoChainInterface,
                                              aInterfaces,
                                              aScriptableFlags,
                                              aHasClassInterface,
                                              aConstructorCID);
}
