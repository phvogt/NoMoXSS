/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * The Original Code is TransforMiiX XSLT processor.
 * 
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Peter Van der Beken are Copyright (C) 2000
 * Peter Van der Beken. All Rights Reserved.
 *
 * Contributor(s):
 * Peter Van der Beken, peter.vanderbeken@pandora.be
 *    -- original author.
 *
 */

#include "nsICategoryManager.h"
#include "nsIDOMClassInfo.h"
#include "nsIErrorService.h"
#include "nsIExceptionService.h"
#include "nsIGenericFactory.h"
#include "nsIParserService.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsXPathEvaluator.h"
#include "nsXPathException.h"
#include "nsXPathExpression.h"
#include "nsXPathNSResolver.h"
#include "nsXPathResult.h"
#include "nsXPIDLString.h"
#include "txAtoms.h"
#include "txMozillaXSLTProcessor.h"
#include "TxLog.h"
#include "nsCRT.h"
#include "nsIScriptSecurityManager.h"
#include "txURIUtils.h"
#include "txXSLTProcessor.h"
#include "nsXPath1Scheme.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPath1SchemeProcessor)

/* 1c1a3c01-14f6-11d6-a7f2-ea502af815dc */
#define TRANSFORMIIX_DOMCI_EXTENSION_CID   \
{ 0x1c1a3c01, 0x14f6, 0x11d6, {0xa7, 0xf2, 0xea, 0x50, 0x2a, 0xf8, 0x15, 0xdc} }

/* {0C351177-0159-4500-86B0-A219DFDE4258} */
#define TRANSFORMIIX_XPATH1_SCHEME_CID \
{ 0xc351177, 0x159, 0x4500, { 0x86, 0xb0, 0xa2, 0x19, 0xdf, 0xde, 0x42, 0x58 } }

#define TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID \
"@mozilla.org/transformiix-domci-extender;1"

NS_DOMCI_EXTENSION(Transformiix)
    static NS_DEFINE_CID(kXSLTProcessorCID, TRANSFORMIIX_XSLT_PROCESSOR_CID);
    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XSLTProcessor)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIXSLTProcessor)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIXSLTProcessorObsolete) // XXX DEPRECATED
    NS_DOMCI_EXTENSION_ENTRY_END(XSLTProcessor, nsIXSLTProcessor, PR_TRUE,
                                 &kXSLTProcessorCID)

    static NS_DEFINE_CID(kXPathEvaluatorCID, TRANSFORMIIX_XPATH_EVALUATOR_CID);
    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XPathEvaluator)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMXPathEvaluator)
    NS_DOMCI_EXTENSION_ENTRY_END(XPathEvaluator, nsIDOMXPathEvaluator, PR_TRUE,
                                 &kXPathEvaluatorCID)

    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XPathException)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMXPathException)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIException)
    NS_DOMCI_EXTENSION_ENTRY_END(XPathException, nsIDOMXPathException, PR_TRUE,
                                 nsnull)

    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XPathExpression)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMXPathExpression)
    NS_DOMCI_EXTENSION_ENTRY_END(XPathExpression, nsIDOMXPathExpression,
                                 PR_TRUE, nsnull)

    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XPathNSResolver)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMXPathNSResolver)
    NS_DOMCI_EXTENSION_ENTRY_END(XPathNSResolver, nsIDOMXPathNSResolver,
                                 PR_TRUE, nsnull)

    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XPathResult)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMXPathResult)
    NS_DOMCI_EXTENSION_ENTRY_END(XPathResult, nsIDOMXPathResult, PR_TRUE,
                                 nsnull)
NS_DOMCI_EXTENSION_END

// Factory Constructor
NS_GENERIC_FACTORY_CONSTRUCTOR(txMozillaXSLTProcessor)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPathEvaluator)

NS_DECL_DOM_CLASSINFO(XSLTProcessor)
NS_DECL_DOM_CLASSINFO(XPathEvaluator)
NS_DECL_DOM_CLASSINFO(XPathException)
NS_DECL_DOM_CLASSINFO(XPathExpression)
NS_DECL_DOM_CLASSINFO(XPathNSResolver)
NS_DECL_DOM_CLASSINFO(XPathResult)

static NS_METHOD 
RegisterTransformiix(nsIComponentManager *aCompMgr,
                     nsIFile *aPath,
                     const char *registryLocation,
                     const char *componentType,
                     const nsModuleComponentInfo *info)
{
    nsresult rv = NS_OK;

    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);

    if (NS_FAILED(rv))
        return rv;

    nsXPIDLCString previous;
    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XSLTProcessor",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XPathEvaluator",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XPathException",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XPathExpression",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XPathNSResolver",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XPathResult",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    char* iidString = NS_GET_IID(nsIXSLTProcessorObsolete).ToString();
    if (!iidString)
      return NS_ERROR_OUT_OF_MEMORY;
    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_INTERFACE,
                                  "nsIXSLTProcessorObsolete",
                                  iidString,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    nsCRT::free(iidString);
    NS_ENSURE_SUCCESS(rv, rv);

    iidString = NS_GET_IID(nsIXSLTProcessor).ToString();
    if (!iidString)
      return NS_ERROR_OUT_OF_MEMORY;
    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_INTERFACE,
                                  "nsIXSLTProcessor",
                                  iidString,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    nsCRT::free(iidString);

    return rv;
}

static PRBool gInitialized = PR_FALSE;
static nsIExceptionProvider *gXPathExceptionProvider = 0;
nsINameSpaceManager *gTxNameSpaceManager = 0;
nsIParserService *gTxParserService = 0;

// Perform our one-time intialization for this module
PR_STATIC_CALLBACK(nsresult)
Initialize(nsIModule* aSelf)
{
    NS_PRECONDITION(!gInitialized, "module already initialized");
    if (gInitialized)
        return NS_OK;

    gInitialized = PR_TRUE;

    gXPathExceptionProvider = new nsXPathExceptionProvider();
    if (!gXPathExceptionProvider)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(gXPathExceptionProvider);
    nsCOMPtr<nsIExceptionService> xs =
        do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);
    if (xs)
        xs->RegisterExceptionProvider(gXPathExceptionProvider,
                                      NS_ERROR_MODULE_DOM_XPATH);

    if (!txXSLTProcessor::init()) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    nsresult rv = CallGetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID,
                                 &gTxSecurityManager);
    if (NS_FAILED(rv)) {
        gTxSecurityManager = nsnull;
        return rv;
    }

    rv = CallGetService(NS_NAMESPACEMANAGER_CONTRACTID, &gTxNameSpaceManager);
    if (NS_FAILED(rv)) {
        gTxNameSpaceManager = nsnull;
        return rv;
    }

    rv = CallGetService("@mozilla.org/parser/parser-service;1",
                        &gTxParserService);
    if (NS_FAILED(rv)) {
        gTxParserService = nsnull;
        return rv;
    }

    nsCOMPtr<nsIErrorService> errorService =
        do_GetService(NS_ERRORSERVICE_CONTRACTID);
    if (errorService) {
        errorService->RegisterErrorStringBundle(NS_ERROR_MODULE_XSLT,
                                                XSLT_MSGS_URL);
    }

    return NS_OK;
}

// Shutdown this module, releasing all of the module resources
PR_STATIC_CALLBACK(void)
Shutdown(nsIModule* aSelf)
{
    NS_PRECONDITION(gInitialized, "module not initialized");
    if (!gInitialized)
        return;

    gInitialized = PR_FALSE;
    if (gXPathExceptionProvider) {
        nsCOMPtr<nsIExceptionService> xs =
            do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);
        if (xs)
            xs->UnregisterExceptionProvider(gXPathExceptionProvider,
                                            NS_ERROR_MODULE_DOM_XPATH);
        NS_RELEASE(gXPathExceptionProvider);
    }

    NS_IF_RELEASE(NS_CLASSINFO_NAME(XSLTProcessor));
    NS_IF_RELEASE(NS_CLASSINFO_NAME(XPathEvaluator));
    NS_IF_RELEASE(NS_CLASSINFO_NAME(XPathException));
    NS_IF_RELEASE(NS_CLASSINFO_NAME(XPathExpression));
    NS_IF_RELEASE(NS_CLASSINFO_NAME(XPathNSResolver));
    NS_IF_RELEASE(NS_CLASSINFO_NAME(XPathResult));

    txXSLTProcessor::shutdown();

    NS_IF_RELEASE(gTxSecurityManager);
    NS_IF_RELEASE(gTxNameSpaceManager);
    NS_IF_RELEASE(gTxParserService);
}

// Component Table
static const nsModuleComponentInfo gComponents[] = {
    { "XSLTProcessor",
      TRANSFORMIIX_XSLT_PROCESSOR_CID,
      TRANSFORMIIX_XSLT_PROCESSOR_CONTRACTID,
      txMozillaXSLTProcessorConstructor,
      RegisterTransformiix },
    { "XPathEvaluator",
      TRANSFORMIIX_XPATH_EVALUATOR_CID,
      NS_XPATH_EVALUATOR_CONTRACTID,
      nsXPathEvaluatorConstructor },
    { "Transformiix DOMCI Extender",
      TRANSFORMIIX_DOMCI_EXTENSION_CID,
      TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
      NS_DOMCI_EXTENSION_CONSTRUCTOR(Transformiix) },
    { "XPath1 XPointer Scheme Processor",
      TRANSFORMIIX_XPATH1_SCHEME_CID,
      NS_XPOINTER_SCHEME_PROCESSOR_BASE "xpath1",
      nsXPath1SchemeProcessorConstructor }
};

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(TransformiixModule, gComponents,
                                   Initialize, Shutdown)
