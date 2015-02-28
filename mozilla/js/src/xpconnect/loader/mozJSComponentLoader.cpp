/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributors:
 *   Mike Shaver <shaver@zeroknowledge.com>
 *   John Bandhauer <jband@netscape.com>
 *   IBM Corp.
 *   Robert Ginda <rginda@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "prlog.h"

#include "nsCOMPtr.h"
#include "nsICategoryManager.h"
#include "nsIComponentLoader.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"
#include "nsILocalFile.h"
#include "nsIModule.h"
#include "nsIServiceManager.h"
#include "nsISupports.h"
#include "mozJSComponentLoader.h"
#include "nsIJSRuntimeService.h"
#include "nsIJSContextStack.h"
#include "nsIXPConnect.h"
#include "nsCRT.h"
#include "nsMemory.h"
#include "nsXPIDLString.h"
#include "nsIObserverService.h"
#include "nsIXPCScriptable.h"
#include "nsString.h"
#ifndef XPCONNECT_STANDALONE
#include "nsIScriptSecurityManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURL.h"
#include "nsIStandardURL.h"
#include "nsNetUtil.h"
#endif
#include "nsIComponentLoaderManager.h"
// For reporting errors with the console service
#include "nsIScriptError.h"
#include "nsIConsoleService.h"

const char mozJSComponentLoaderContractID[] = "@mozilla.org/moz/jsloader;1";
const char jsComponentTypeName[] = "text/javascript";

// same as in nsComponentManager.cpp (but without the JS)
const char JSfileSizeValueName[] = "FileSize";
const char JSlastModValueName[] = "LastModTimeStamp";
const char JSxpcomKeyName[] = "software/mozilla/XPCOM/components";

const char kJSRuntimeServiceContractID[] = "@mozilla.org/js/xpc/RuntimeService;1";
const char kXPConnectServiceContractID[] = "@mozilla.org/js/xpc/XPConnect;1";
const char kJSContextStackContractID[] =   "@mozilla.org/js/xpc/ContextStack;1";
const char kConsoleServiceContractID[] =   "@mozilla.org/consoleservice;1";
const char kScriptErrorContractID[] =      "@mozilla.org/scripterror;1";
const char kObserverServiceContractID[] = "@mozilla.org/observer-service;1";
#ifndef XPCONNECT_STANDALONE
const char kScriptSecurityManagerContractID[] = NS_SCRIPTSECURITYMANAGER_CONTRACTID;
const char kStandardURLContractID[] = "@mozilla.org/network/standard-url;1";
#endif

JS_STATIC_DLL_CALLBACK(void)
Reporter(JSContext *cx, const char *message, JSErrorReport *rep)
{
    nsresult rv;

    /* Use the console service to register the error. */
    nsCOMPtr<nsIConsoleService> consoleService =
        do_GetService(kConsoleServiceContractID);

    /*
     * Make an nsIScriptError, populate it with information from this
     * error, then log it with the console service.  The UI can then
     * poll the service to update the JavaScript console.
     */
    nsCOMPtr<nsIScriptError> errorObject = 
        do_CreateInstance(kScriptErrorContractID);
    
    if (consoleService && errorObject) {
        /*
         * Got an error object; prepare appropriate-width versions of
         * various arguments to it.
         */
        nsAutoString fileUni;
        fileUni.AssignWithConversion(rep->filename);

        PRUint32 column = rep->uctokenptr - rep->uclinebuf;

        rv = errorObject->Init(NS_REINTERPRET_CAST(const PRUnichar*,
                                                   rep->ucmessage),
                               fileUni.get(),
                               NS_REINTERPRET_CAST(const PRUnichar*,
                                                   rep->uclinebuf),
                               rep->lineno, column, rep->flags,
                               "component javascript");
        if (NS_SUCCEEDED(rv)) {
            rv = consoleService->LogMessage(errorObject);
            if (NS_SUCCEEDED(rv)) {
                // We're done!  Skip return to fall thru to stderr
                // printout, for the benefit of those invoking the
                // browser with -console
                // return;
            }
        }
    }

    /*
     * If any of the above fails for some reason, fall back to
     * printing to stderr.
     */
#ifdef DEBUG
    fprintf(stderr, "JS Component Loader: %s %s:%d\n"
            "                     %s\n",
            JSREPORT_IS_WARNING(rep->flags) ? "WARNING" : "ERROR",
            rep->filename, rep->lineno,
            message ? message : "<no message>");
#endif
}

JS_STATIC_DLL_CALLBACK(JSBool)
Dump(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    if (!argc)
        return JS_TRUE;
    
    str = JS_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;

    char *bytes = JS_GetStringBytes(str);
    bytes = nsCRT::strdup(bytes);

#ifdef XP_MAC
    for (char *c = bytes; *c; c++)
        if (*c == '\r')
            *c = '\n';
#endif
    fputs(bytes, stderr);
    nsMemory::Free(bytes);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
Debug(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
#ifdef DEBUG
    return Dump(cx, obj, argc, argv, rval);
#else
    return JS_TRUE;
#endif
}

#ifndef XPCONNECT_STANDALONE

static JSFunctionSpec gSandboxFun[] = {
    {"dump", Dump, 1 },
    {"debug", Debug, 1 },
    {0}
};

JS_STATIC_DLL_CALLBACK(JSBool)
sandbox_enumerate(JSContext *cx, JSObject *obj)
{
    return JS_EnumerateStandardClasses(cx, obj);
}

JS_STATIC_DLL_CALLBACK(JSBool)
sandbox_resolve(JSContext *cx, JSObject *obj, jsval id)
{
    JSBool resolved;
    return JS_ResolveStandardClass(cx, obj, id, &resolved);
}

static JSClass js_SandboxClass = {
    "Sandbox", 0,
    JS_PropertyStub,   JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    sandbox_enumerate, sandbox_resolve, JS_ConvertStub,  JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JS_STATIC_DLL_CALLBACK(JSBool)
NewSandbox(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc = do_GetService(kXPConnectServiceContractID,
                                               &rv);
    if (!xpc) {
        JS_ReportError(cx, "Unable to get XPConnect service: %08lx", rv);
        return JS_FALSE;
    }
    
    /*
     * We're not likely to see much action on this context, so keep stack-arena
     * chunk size small to reduce bloat.
     */
    JSContext *sandcx = JS_NewContext(JS_GetRuntime(cx), 1024);
    if (!sandcx) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    JSBool ok = JS_FALSE;
    JSObject *sandbox = JS_NewObject(sandcx, &js_SandboxClass, nsnull, nsnull);
    if (!sandbox)
        goto out;

    JS_SetGlobalObject(sandcx, sandbox);

    ok = JS_DefineFunctions(sandcx, sandbox, gSandboxFun) &&
        NS_SUCCEEDED(xpc->InitClasses(sandcx, sandbox));
    
    *rval = OBJECT_TO_JSVAL(sandbox);
 out:
    JS_DestroyContext(sandcx);
    return ok;
}        

JS_STATIC_DLL_CALLBACK(JSBool)
EvalInSandbox(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, 
              jsval *rval)
{
    JSPrincipals* jsPrincipals;
    JSString* source;
    const jschar * URL;
    JSObject* sandbox;
    
    if (!JS_ConvertArguments(cx, argc, argv, "SoW", &source, &sandbox, &URL))
        return JS_FALSE;

    if (!JS_InstanceOf(cx, sandbox, &js_SandboxClass, NULL)) {
        JSClass *clasp = JS_GetClass(cx, sandbox);
        const char *className = clasp ? clasp->name : "<unknown!>";
        JS_ReportError(cx,
      "evalInSandbox passed object of class %s instead of Sandbox", className);
        return JS_FALSE;
    }

    NS_ConvertUCS2toUTF8 URL8((const PRUnichar *)URL);
    nsCOMPtr<nsIURL> iURL;
    nsCOMPtr<nsIStandardURL> stdUrl =
        do_CreateInstance(kStandardURLContractID);
    if (!stdUrl ||
        NS_FAILED(stdUrl->Init(nsIStandardURL::URLTYPE_STANDARD, 80,
                               URL8, nsnull, nsnull)) ||
        !(iURL = do_QueryInterface(stdUrl))) {
        JS_ReportError(cx, "Can't create URL for evalInSandbox");
        return JS_FALSE;
    }
    
    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIScriptSecurityManager> secman = 
        do_GetService(kScriptSecurityManagerContractID);
    if (!secman ||
        NS_FAILED(secman->GetCodebasePrincipal(iURL,
                                               getter_AddRefs(principal))) ||
        !principal ||
        NS_FAILED(principal->GetJSPrincipals(cx, &jsPrincipals)) ||
        !jsPrincipals) {
        JS_ReportError(cx, "Can't get principals for evalInSandbox");
        return JS_FALSE;
    }
    
    JSBool ok;
    JSContext *sandcx = JS_NewContext(JS_GetRuntime(cx), 8192);
    if (!sandcx) {
        JS_ReportError(cx, "Can't prepare context for evalInSandbox");
        ok = JS_FALSE;
    } else {
        JS_SetGlobalObject(sandcx, sandbox);
        JS_SetErrorReporter(sandcx, Reporter);

        ok = JS_EvaluateUCScriptForPrincipals(sandcx, sandbox, jsPrincipals,
                                              JS_GetStringChars(source),
                                              JS_GetStringLength(source),
                                              URL8.get(), 1, rval);
        JS_DestroyContext(sandcx);
    }
    JSPRINCIPALS_DROP(cx, jsPrincipals);
    return ok;
}        

#endif /* XPCONNECT_STANDALONE */

static JSFunctionSpec gGlobalFun[] = {
    {"dump", Dump, 1 },
    {"debug", Debug, 1 },
#ifndef XPCONNECT_STANDALONE
    {"Sandbox", NewSandbox, 0 },
    {"evalInSandbox", EvalInSandbox, 3 },
#endif
    {0}
};

#ifndef XPCONNECT_STANDALONE
class BackstagePass : public nsIScriptObjectPrincipal, public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE
  
  NS_IMETHOD GetPrincipal(nsIPrincipal **aPrincipal) {
    NS_ADDREF(*aPrincipal = mPrincipal);
    return NS_OK;
  }

  BackstagePass(nsIPrincipal *prin) :
    mPrincipal(prin)
  {
  }

  virtual ~BackstagePass() { }

private:
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(BackstagePass, nsIScriptObjectPrincipal, nsIXPCScriptable)

#else

class BackstagePass : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

  BackstagePass()
  {
  }

  virtual ~BackstagePass() { }
};

NS_IMPL_THREADSAFE_ISUPPORTS1(BackstagePass, nsIXPCScriptable)

#endif

// The nsIXPCScriptable map declaration that will generate stubs for us...
#define XPC_MAP_CLASSNAME           BackstagePass
#define XPC_MAP_QUOTED_CLASSNAME   "BackstagePass"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define XPC_MAP_FLAGS       nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY   | \
                            nsIXPCScriptable::DONT_ENUM_STATIC_PROPS       | \
                            nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE    | \
                            nsIXPCScriptable::DONT_REFLECT_INTERFACE_NAMES
#include "xpc_map_end.h" /* This will #undef the above */

/* PRBool newResolve (in nsIXPConnectWrappedNative wrapper, in JSContextPtr cx, in JSObjectPtr obj, in JSVal id, in PRUint32 flags, out JSObjectPtr objp); */
NS_IMETHODIMP
BackstagePass::NewResolve(nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx, JSObject * obj,
                          jsval id, PRUint32 flags, 
                          JSObject * *objp, PRBool *_retval)
{
    JSBool resolved;

    *_retval = JS_ResolveStandardClass(cx, obj, id, &resolved);
    if (*_retval && resolved)
        *objp = obj;
    return NS_OK;
}

mozJSComponentLoader::mozJSComponentLoader()
    : mRuntime(nsnull),
      mModules(nsnull),
      mGlobals(nsnull),
      mInitialized(PR_FALSE)
{
}

static PRIntn PR_CALLBACK
UnrootGlobals(PLHashEntry *he, PRIntn i, void *arg)
{
    JSRuntime *rt = (JSRuntime *)arg;
    JS_RemoveRootRT(rt, &he->value);
    nsCRT::free((char *)he->key);
    return HT_ENUMERATE_REMOVE;
}

static PRIntn PR_CALLBACK
UnloadAndReleaseModules(PLHashEntry *he, PRIntn i, void *arg)
{
    nsIModule *module = NS_STATIC_CAST(nsIModule *, he->value);
    nsIComponentManager *mgr = NS_STATIC_CAST(nsIComponentManager *, arg);
    PRBool canUnload;
    nsresult rv = module->CanUnload(mgr, &canUnload);
    NS_ASSERTION(NS_SUCCEEDED(rv), "module CanUnload failed");
    if (NS_SUCCEEDED(rv) && canUnload) {
        NS_RELEASE(module);
        /* XXX need to unroot the global for the module as well */
        nsCRT::free((char *)he->key);
        return HT_ENUMERATE_REMOVE;
    }
    return HT_ENUMERATE_NEXT;
}

mozJSComponentLoader::~mozJSComponentLoader()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(mozJSComponentLoader, nsIComponentLoader)

NS_IMETHODIMP
mozJSComponentLoader::GetFactory(const nsIID &aCID,
                                 const char *aLocation,
                                 const char *aType,
                                 nsIFactory **_retval)
{
    if (!_retval)
        return NS_ERROR_NULL_POINTER;

#ifdef DEBUG_shaver_off
    char *cidString = aCID.ToString();
    fprintf(stderr, "mJCL::GetFactory(%s,%s,%s)\n", cidString, aLocation, aType);
    delete [] cidString;
#endif
    
    nsIModule * module = ModuleForLocation(aLocation, 0);
    if (!module) {
#ifdef DEBUG_shaver_off
        fprintf(stderr, "ERROR: couldn't get module for %s\n", aLocation);
#endif
        return NS_ERROR_FACTORY_NOT_LOADED;
    }
    
    nsresult rv = module->GetClassObject(mCompMgr, aCID,
                                         NS_GET_IID(nsIFactory),
                                         (void **)_retval);
#ifdef DEBUG_shaver_off
    fprintf(stderr, "GetClassObject %s\n", NS_FAILED(rv) ? "FAILED" : "ok");
#endif
    return rv;
}

NS_IMETHODIMP
mozJSComponentLoader::Init(nsIComponentManager *aCompMgr, nsISupports *aReg)
{
    mCompMgr = aCompMgr;
         
    nsresult rv;
    mLoaderManager = do_QueryInterface(mCompMgr, &rv);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}

nsresult
mozJSComponentLoader::ReallyInit()
{
    nsresult rv;

    /*
     * Get the JSRuntime from the runtime svc, if possible.
     * We keep a reference around, because it's a Bad Thing if the runtime
     * service gets shut down before we're done.  Bad!
     */

    mRuntimeService = do_GetService(kJSRuntimeServiceContractID, &rv);
    if (NS_FAILED(rv) ||
        NS_FAILED(rv = mRuntimeService->GetRuntime(&mRuntime)))
        return rv;

#ifndef XPCONNECT_STANDALONE
    nsCOMPtr<nsIScriptSecurityManager> secman = 
        do_GetService(kScriptSecurityManagerContractID);
    if (!secman)
        return NS_ERROR_FAILURE;

    rv = secman->GetSystemPrincipal(getter_AddRefs(mSystemPrincipal));
    if (NS_FAILED(rv) || !mSystemPrincipal)
        return NS_ERROR_FAILURE;
#endif

    mModules = PL_NewHashTable(16, PL_HashString, PL_CompareStrings,
                               PL_CompareValues, 0, 0);
    if (!mModules)
        return NS_ERROR_OUT_OF_MEMORY;
    
    mGlobals = PL_NewHashTable(16, PL_HashString, PL_CompareStrings,
                               PL_CompareValues, 0, 0);
    if (!mGlobals)
        return NS_ERROR_OUT_OF_MEMORY;

#ifdef DEBUG_shaver_off
    fprintf(stderr, "mJCL: ReallyInit success!\n");
#endif
    mInitialized = PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP
mozJSComponentLoader::AutoRegisterComponents(PRInt32 when,
                                             nsIFile *aDirectory)
{
    return RegisterComponentsInDir(when, aDirectory);
}

nsresult
mozJSComponentLoader::RegisterComponentsInDir(PRInt32 when, nsIFile *dir)
{
    nsresult rv;
    PRBool isDir;
    
    if (NS_FAILED(rv = dir->IsDirectory(&isDir)))
        return rv;
    
    if (!isDir)
        return NS_ERROR_INVALID_ARG;

    // Create a directory iterator
    nsCOMPtr<nsISimpleEnumerator> dirIterator;
    rv = dir->GetDirectoryEntries(getter_AddRefs(dirIterator));
    
    if (NS_FAILED(rv)) return rv;
    
   // whip through the directory to register every file
    nsIFile *dirEntry = NULL;
    PRBool more = PR_FALSE;

    rv = dirIterator->HasMoreElements(&more);
    if (NS_FAILED(rv)) return rv;
    while (more == PR_TRUE)
    {
        rv = dirIterator->GetNext((nsISupports**)&dirEntry);
        if (NS_SUCCEEDED(rv))
        {
            rv = dirEntry->IsDirectory(&isDir);
            if (NS_SUCCEEDED(rv))
            {
                if (isDir == PR_TRUE)
                {
                    // This is a directory. Grovel for components into the directory.
                    rv = RegisterComponentsInDir(when, dirEntry);
                }
                else
                {
                    PRBool registered;
                    // This is a file. Try to register it.
                    rv = AutoRegisterComponent(when, dirEntry, &registered);
                }
            }
            NS_RELEASE(dirEntry);
        }
        rv = dirIterator->HasMoreElements(&more);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


nsresult
mozJSComponentLoader::SetRegistryInfo(const char *registryLocation,
                                      nsIFile *component)
{
    nsresult rv;
    if (!mLoaderManager)
        return NS_ERROR_FAILURE;
    
    PRInt64 modDate;
    rv = component->GetLastModifiedTime(&modDate);
    if (NS_FAILED(rv))
        return rv;

#ifdef DEBUG_shaver_off
    fprintf(stderr, "SetRegistryInfo(%s) => (%d,%d)\n", registryLocation,
            modDate, fileSize);
#endif
    return mLoaderManager->SaveFileInfo(component, registryLocation, modDate);                                                          
}

nsresult
mozJSComponentLoader::RemoveRegistryInfo(nsIFile *component, const char *registryLocation)
{
    if (!mLoaderManager)
        return NS_ERROR_FAILURE;
    
    return mLoaderManager->RemoveFileInfo(component, registryLocation);                                                          
}


PRBool
mozJSComponentLoader::HasChanged(const char *registryLocation,
                                 nsIFile *component)
{
    if (!mLoaderManager)
        return NS_ERROR_FAILURE;
    
    PRInt64 lastTime;
    component->GetLastModifiedTime(&lastTime);

    PRBool hasChanged = PR_TRUE;
    mLoaderManager->HasFileChanged(component, registryLocation, lastTime, &hasChanged);                                                          
    return hasChanged;
}

NS_IMETHODIMP
mozJSComponentLoader::AutoRegisterComponent(PRInt32 when,
                                            nsIFile *component,
                                            PRBool *registered)
{
    nsresult rv;
    if (!registered)
        return NS_ERROR_NULL_POINTER;

    const char jsExtension[] = ".js";
    int jsExtensionLen = 3;
    nsCAutoString leafName;

    *registered = PR_FALSE;

    /* we only do files */
    PRBool isFile = PR_FALSE;
    if (NS_FAILED(rv = component->IsFile(&isFile)) || !isFile)
        return rv;

    if (NS_FAILED(rv = component->GetNativeLeafName(leafName)))
        return rv;
    int len = leafName.Length();
    
    /* if it's not *.js, return now */
    if (len < jsExtensionLen || // too short
        PL_strcasecmp(leafName.get() + len - jsExtensionLen, jsExtension))
        return NS_OK;

#ifdef DEBUG_shaver_off
    fprintf(stderr, "mJCL: registering JS component %s\n",
            leafName.get());
#endif
      
    rv = AttemptRegistration(component, PR_FALSE);
#ifdef DEBUG_shaver_off
    if (NS_SUCCEEDED(rv))
        fprintf(stderr, "registered module %s\n", leafName.get());
    else if (rv == NS_ERROR_FACTORY_REGISTER_AGAIN) 
        fprintf(stderr, "deferred module %s\n", leafName.get());
    else
        fprintf(stderr, "failed to register %s\n", leafName.get());
#endif    
    *registered = (PRBool) NS_SUCCEEDED(rv);
    return NS_OK;
}

NS_IMETHODIMP
mozJSComponentLoader::AutoUnregisterComponent(PRInt32 when,
                                              nsIFile *component,
                                              PRBool *unregistered)
{
    nsresult rv;
    if (!unregistered)
        return NS_ERROR_NULL_POINTER;

    const char jsExtension[] = ".js";
    int jsExtensionLen = 3;
    nsCAutoString leafName;

    *unregistered = PR_FALSE;

    /* we only do files */
    PRBool isFile = PR_FALSE;
    if (NS_FAILED(rv = component->IsFile(&isFile)) || !isFile)
        return rv;

    if (NS_FAILED(rv = component->GetNativeLeafName(leafName)))
        return rv;
    int len = leafName.Length();
    
    /* if it's not *.js, return now */
    if (len < jsExtensionLen || // too short
        PL_strcasecmp(leafName.get() + len - jsExtensionLen, jsExtension))
        return NS_OK;

    rv = UnregisterComponent(component);
#ifdef DEBUG_dp
    if (NS_SUCCEEDED(rv))
        fprintf(stderr, "unregistered module %s\n", leafName.get());
    else
        fprintf(stderr, "failed to unregister %s\n", leafName.get());
#endif    
    *unregistered = (PRBool) NS_SUCCEEDED(rv);
    return NS_OK;
}

nsresult
mozJSComponentLoader::AttemptRegistration(nsIFile *component,
                                          PRBool deferred)
{
    nsXPIDLCString registryLocation;
    nsresult rv;
    nsIModule *module;
    
    // what I want to do here is QI for a Component Registration Manager.  Since this 
    // has not been invented yet, QI to the obsolete manager.  Kids, don't do this at home.
    nsCOMPtr<nsIComponentManagerObsolete> obsoleteManager = do_QueryInterface(mCompMgr, &rv);
    if (obsoleteManager)
        rv = obsoleteManager->RegistryLocationForSpec(component, 
                                                      getter_Copies(registryLocation));
    if (NS_FAILED(rv))
        return rv;
    
    /* no need to check registry data on deferred reg */
    if (!deferred && !HasChanged(registryLocation, component))
        return NS_OK;
    
    module = ModuleForLocation(registryLocation, component);
    if (!module)
        goto out;
    
    {
      // Notify observers, if any, of autoregistration work
      nsCOMPtr<nsIObserverService> observerService = 
            do_GetService(kObserverServiceContractID);
      if (observerService)
      {
        nsCOMPtr<nsIServiceManager> mgr;
        rv = NS_GetServiceManager(getter_AddRefs(mgr));
        if (NS_SUCCEEDED(rv))
        {
          // this string can't come from a string bundle, because we
          // don't have string bundles yet.
          NS_ConvertASCIItoUCS2 fileName("(no name)");

          // get the file name
          if (component)
          {
            component->GetLeafName(fileName);
          }
          
          // this string can't come from a string bundle, because we
          // don't have string bundles yet.
          (void) observerService->
            NotifyObservers(mgr,
                            NS_XPCOM_AUTOREGISTRATION_OBSERVER_ID,
                            PromiseFlatString(NS_LITERAL_STRING("Registering JS component ") +
                                              fileName).get());
        }
      }
    }
    
    rv = module->RegisterSelf(mCompMgr, component, registryLocation,
                              jsComponentTypeName);
    if (rv == NS_ERROR_FACTORY_REGISTER_AGAIN) {
        if (!deferred)
            mDeferredComponents.AppendElement(component);
        /*
         * we don't enter in the registry because we may want to
         * try again on a later autoreg, in case a dependency has
         * become available. 
         */
    } else{ 
 out:
        SetRegistryInfo(registryLocation, component);
    }

    return rv;
}

nsresult
mozJSComponentLoader::UnregisterComponent(nsIFile *component)
{
    nsXPIDLCString registryLocation;
    nsresult rv;
    nsIModule *module;
    
    // what I want to do here is QI for a Component Registration Manager.  Since this 
    // has not been invented yet, QI to the obsolete manager.  Kids, don't do this at home.
    nsCOMPtr<nsIComponentManagerObsolete> obsoleteManager = do_QueryInterface(mCompMgr, &rv);
    if (obsoleteManager)
        rv = obsoleteManager->RegistryLocationForSpec(component, 
                                                      getter_Copies(registryLocation));
    if (NS_FAILED(rv))
        return rv;
    
    module = ModuleForLocation(registryLocation, component);
    if (!module)
        return NS_ERROR_FAILURE;
    
    {
      // Notify observers, if any, of autoregistration work
      nsCOMPtr<nsIObserverService> observerService = 
            do_GetService(kObserverServiceContractID);
      if (observerService)
      {
        nsCOMPtr<nsIServiceManager> mgr;
        rv = NS_GetServiceManager(getter_AddRefs(mgr));
        if (NS_SUCCEEDED(rv))
        {
            (void) observerService->NotifyObservers(mgr,
                                                    NS_XPCOM_AUTOREGISTRATION_OBSERVER_ID,
                                                    NS_LITERAL_STRING("Unregistering JS component").get());
        }
      }
    }
    
    rv = module->UnregisterSelf(mCompMgr, component, registryLocation);

    if (NS_SUCCEEDED(rv))
    {
        // Remove any autoreg specific info. Ignore error.
        RemoveRegistryInfo(component, registryLocation);
    }
        
    return rv;
}


nsresult
mozJSComponentLoader::RegisterDeferredComponents(PRInt32 aWhen,
                                                 PRBool *aRegistered)
{
    nsresult rv;
    *aRegistered = PR_FALSE;

    PRUint32 count;
    rv = mDeferredComponents.Count(&count);
#ifdef DEBUG_shaver_off
    fprintf(stderr, "mJCL: registering deferred (%d)\n", count);
#endif
    if (NS_FAILED(rv) || !count)
        return NS_OK;
    
    for (PRUint32 i = 0; i < count; i++) {
        nsCOMPtr<nsIFile> component;
        rv = mDeferredComponents.QueryElementAt(i, NS_GET_IID(nsIFile), getter_AddRefs(component));
        if (NS_FAILED(rv))
            continue;

        rv = AttemptRegistration(component, PR_TRUE /* deferred */);
        if (rv != NS_ERROR_FACTORY_REGISTER_AGAIN) {
            if (NS_SUCCEEDED(rv))
                *aRegistered = PR_TRUE;
            mDeferredComponents.RemoveElementAt(i);
        }
    }

#ifdef DEBUG_shaver_off
    rv = mDeferredComponents.Count(&count);
    if (NS_SUCCEEDED(rv)) {
        if (*aRegistered)
            fprintf(stderr, "mJCL: registered deferred, %d left\n",
                    count);
        else
            fprintf(stderr, "mJCL: didn't register any components, %d left\n",
                    count);
    }
#endif
    /* are there any fatal errors? */
    return NS_OK;
}


nsIModule *
mozJSComponentLoader::ModuleForLocation(const char *registryLocation,
                                        nsIFile *component)
{
    nsresult rv;
    nsIModule *module = nsnull;
    if (!mInitialized &&
        NS_FAILED(ReallyInit()))
        return nsnull;

    PLHashNumber hash = PL_HashString(registryLocation);
    PLHashEntry **hep = PL_HashTableRawLookup(mModules, hash,
                                              (void *)registryLocation);
    PLHashEntry *he = *hep;
    if (he)
        return (nsIModule *)he->value;

    JSObject *global = GlobalForLocation(registryLocation, component);
    if (!global) {
#ifdef DEBUG_shaver
        fprintf(stderr, "GlobalForLocation failed!\n");
#endif
        return nsnull;
    }

    nsCOMPtr<nsIXPConnect> xpc = do_GetService(kXPConnectServiceContractID);
    if (!xpc)
        return nsnull;

    JSCLAutoContext cx(mRuntime);
    if(NS_FAILED(cx.GetError()))
        return nsnull;

    JSObject* cm_jsobj;
    nsCOMPtr<nsIXPConnectJSObjectHolder> cm_holder;
    rv = xpc->WrapNative(cx, global, mCompMgr, 
                         NS_GET_IID(nsIComponentManager),
                         getter_AddRefs(cm_holder));

    if (NS_FAILED(rv)) {
#ifdef DEBUG_shaver
        fprintf(stderr, "WrapNative(%p,%p,nsIComponentManager) failed: %x\n",
                (void *)(JSContext*)cx, (void *)mCompMgr, rv);
#endif
        return nsnull;
    }

    rv = cm_holder->GetJSObject(&cm_jsobj);
    if (NS_FAILED(rv)) {
#ifdef DEBUG_shaver
        fprintf(stderr, "GetJSObject of ComponentManager failed\n");
#endif
        return nsnull;
    }

    JSCLAutoErrorReporterSetter aers(cx, Reporter);

    jsval argv[2], retval;
    argv[0] = OBJECT_TO_JSVAL(cm_jsobj);
    argv[1] = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, registryLocation));
    if (!JS_CallFunctionName(cx, global, "NSGetModule", 2, argv,
                             &retval)) {
#ifdef DEBUG_shaver_off
        fprintf(stderr, "mJCL: NSGetModule failed for %s\n",
                registryLocation);
#endif
        return nsnull;
    }

#ifdef DEBUG_shaver_off
    JSString *s = JS_ValueToString(cx, retval);
    fprintf(stderr, "mJCL: %s::NSGetModule returned %s\n",
            registryLocation, JS_GetStringBytes(s));
#endif

    JSObject *jsModuleObj;
    if (!JS_ValueToObject(cx, retval, &jsModuleObj)) {
        /* XXX report error properly */
#ifdef DEBUG
        fprintf(stderr, "mJCL: couldn't convert %s's nsIModule to obj\n",
                registryLocation);
#endif
        return nsnull;
    }

    if (NS_FAILED(xpc->WrapJS(cx, jsModuleObj, NS_GET_IID(nsIModule),
                              (void **)&module))) {
        /* XXX report error properly */
#ifdef DEBUG
        fprintf(stderr, "mJCL: couldn't get nsIModule from jsval\n");
#endif
        return nsnull;
    }

    /* we hand our reference to the hash table, it'll be released much later */
    he = PL_HashTableRawAdd(mModules, hep, hash,
                            nsCRT::strdup(registryLocation), module);

    return module;
}

JSObject *
mozJSComponentLoader::GlobalForLocation(const char *aLocation,
                                        nsIFile *component)
{
    JSObject *global = nsnull;
    PRBool needRelease = PR_FALSE;
    JSScript *script = nsnull;
    PLHashNumber hash = PL_HashString(aLocation);
    PLHashEntry **hep = PL_HashTableRawLookup(mGlobals, hash,
                                              (void *)aLocation);
    PLHashEntry *he = *hep;
    if (he)
        return (JSObject *)he->value;

    if (!mInitialized && NS_FAILED(ReallyInit()))
        return nsnull;
    
    nsresult rv;
    JSPrincipals* jsPrincipals = nsnull;

    JSCLAutoContext cx(mRuntime);
    if (NS_FAILED(cx.GetError()))
        return nsnull;

#ifndef XPCONNECT_STANDALONE
    nsCOMPtr<nsIScriptObjectPrincipal> backstagePass =
      new BackstagePass(mSystemPrincipal);

    rv = mSystemPrincipal->GetJSPrincipals(cx, &jsPrincipals);
    if (NS_FAILED(rv) || !jsPrincipals)
        return nsnull;

#else
    nsCOMPtr<nsISupports> backstagePass = new BackstagePass();
#endif

    JSCLAutoErrorReporterSetter aers(cx, Reporter);
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

    nsCOMPtr<nsIXPConnect> xpc = do_GetService(kXPConnectServiceContractID);
    if (!xpc)
        goto out;

    rv = xpc->InitClassesWithNewWrappedGlobal(cx, backstagePass,
                                              NS_GET_IID(nsISupports),
                                              PR_FALSE,
                                              getter_AddRefs(holder));
    if (NS_FAILED(rv))
        goto out;

    rv = holder->GetJSObject(&global);
    if (NS_FAILED(rv)) {
        NS_ASSERTION(global == nsnull, "bad GetJSObject?");
        goto out;
    }

    if (!JS_DefineFunctions(cx, global, gGlobalFun)) {
        global = nsnull;
        goto out;
    }

    if (!component) {
        // what I want to do here is QI for a Component Registration Manager.  Since this 
        // has not been invented yet, QI to the obsolete manager.  Kids, don't do this at home.
        nsCOMPtr<nsIComponentManagerObsolete> obsoleteManager = do_QueryInterface(mCompMgr, &rv);
        if (!obsoleteManager) {
            global = nsnull;
            goto out;
        }

        if (NS_FAILED(obsoleteManager->SpecForRegistryLocation(aLocation, &component))) {
            global = nsnull;
            goto out;
        }
        needRelease = PR_TRUE;
    }

    {
        nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(component);
        if (!localFile) {
            global = nsnull;
            goto out;
        }

        // Quick hack to unbust XPCONNECT_STANDALONE.
        // This leaves the jsdebugger with a non-URL pathname in the 
        // XPCONNECT_STANDALONE case - but at least it builds and runs otherwise.
        // See: http://bugzilla.mozilla.org/show_bug.cgi?id=121438
        {
            nsCAutoString nativePath;
            FILE* fileHandle;
#ifdef XPCONNECT_STANDALONE
            localFile->GetNativePath(nativePath);
#else
            NS_GetURLSpecFromFile(localFile, nativePath);
#endif
            rv = localFile->OpenANSIFileDesc("r", &fileHandle);
            if (NS_FAILED(rv)) {
                global = nsnull;
                goto out;
            }

            script = 
                JS_CompileFileHandleForPrincipals(cx, global, nativePath.get(),
                                                  fileHandle, jsPrincipals);
    
            /* JS will close the filehandle after compilation is complete. */

            if (!script) {
#ifdef DEBUG_shaver_off
                fprintf(stderr, "mJCL: script compilation of %s FAILED\n",
                        nativePath.get());
#endif
                global = nsnull;
                goto out;
            }

#ifdef DEBUG_shaver_off
            fprintf(stderr, "mJCL: compiled JS component %s\n",
                    nativePath.get());
#endif

            jsval retval;
            if (!JS_ExecuteScript(cx, global, script, &retval)) {
#ifdef DEBUG_shaver_off
                fprintf(stderr, "mJCL: failed to execute %s\n",
                        nativePath.get());
#endif
                global = nsnull;
                goto out;
            }
        }
    }

    {
        /* freed when we remove from the table */
        char *location = nsCRT::strdup(aLocation);
        he = PL_HashTableRawAdd(mGlobals, hep, hash, location, global);
        JS_AddNamedRoot(cx, &he->value, location);
    }

 out:
    if (jsPrincipals)
        JSPRINCIPALS_DROP(cx, jsPrincipals);
    if (script)
        JS_DestroyScript(cx, script);
    if (needRelease)
        NS_RELEASE(component);

    return global;
}

NS_IMETHODIMP
mozJSComponentLoader::OnRegister(const nsIID &aCID, const char *aType,
                                 const char *aClassName, const char *aContractID,
                                 const char *aLocation,
                                 PRBool aReplace, PRBool aPersist)

{
#ifdef DEBUG_shaver_off
    fprintf(stderr, "mJCL: registered %s/%s in %s\n", aClassName, aContractID,
            aLocation);
#endif
    return NS_OK;
}    

NS_IMETHODIMP
mozJSComponentLoader::UnloadAll(PRInt32 aWhen)
{
    if (mInitialized) {
        mInitialized = PR_FALSE;

        // stabilize the component manager, etc.
        nsCOMPtr<nsIComponentManager> kungFuDeathGrip = mCompMgr;

        PL_HashTableEnumerateEntries(mModules, 
                                     UnloadAndReleaseModules,
                                     mCompMgr);
        PL_HashTableDestroy(mModules);
        mModules = nsnull;

        PL_HashTableEnumerateEntries(mGlobals, UnrootGlobals, mRuntime);
        PL_HashTableDestroy(mGlobals);
        mGlobals = nsnull;

        JSContext* cx = JS_NewContext(mRuntime, 256);
        if (cx) {
            JS_GC(cx);
            JS_DestroyContext(cx);
        }

        mRuntimeService = nsnull;
    }

#ifdef DEBUG_shaver_off
    fprintf(stderr, "mJCL: UnloadAll(%d)\n", aWhen);
#endif

    return NS_OK;
}

//----------------------------------------------------------------------

JSCLAutoContext::JSCLAutoContext(JSRuntime* rt)
    : mContext(nsnull), mError(NS_OK), mPopNeeded(JS_FALSE), mContextThread(0)
{
    nsCOMPtr<nsIThreadJSContextStack> cxstack = 
        do_GetService(kJSContextStackContractID, &mError);
    
    if (NS_SUCCEEDED(mError)) {
        mError = cxstack->GetSafeJSContext(&mContext);
        if (NS_SUCCEEDED(mError) && mContext) {
            mError = cxstack->Push(mContext);
            if (NS_SUCCEEDED(mError)) {
                mPopNeeded = JS_TRUE;   
            } 
        } 
    }
    
    if (mContext) {
        mContextThread = JS_GetContextThread(mContext);
        if (mContextThread) {
            JS_BeginRequest(mContext);
        } 
    } else {
        if (NS_SUCCEEDED(mError)) {
            mError = NS_ERROR_FAILURE;
        }
    }
}

JSCLAutoContext::~JSCLAutoContext()
{
    if (mContext && mContextThread) {
        JS_ClearNewbornRoots(mContext);
        JS_EndRequest(mContext);
    }

    if (mPopNeeded) {
        nsCOMPtr<nsIThreadJSContextStack> cxstack = 
            do_GetService(kJSContextStackContractID);
        if (cxstack) {
            JSContext* cx;
            nsresult rv = cxstack->Pop(&cx);
            NS_ASSERTION(NS_SUCCEEDED(rv) && cx == mContext, "push/pop mismatch");
        }        
    }        
}        

//----------------------------------------------------------------------

/* XXX this should all be data-driven, via NS_IMPL_GETMODULE_WITH_CATEGORIES */
