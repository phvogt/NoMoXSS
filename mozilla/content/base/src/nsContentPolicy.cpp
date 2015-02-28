/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
// vim: ft=cpp tw=78 sw=4 et ts=8
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
 * The Original Code is Mozilla code.
 *
 * The Initial Developer of the Original Code is
 * Zero-Knowledge Systems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "nsISupports.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsContentPolicyUtils.h"
#include "nsContentPolicy.h"
#include "nsICategoryManager.h"
#include "nsIURI.h"
#include "nsIDOMNode.h"
#include "nsIDOMWindow.h"

NS_IMPL_ISUPPORTS1(nsContentPolicy, nsIContentPolicy)

#ifdef XSS /* XSS */

#include "nsIPrompt.h"
#include "nsIDOMWindowInternal.h"
#include "nsIStringBundle.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIXSSHostConnectPermissionManager.h"

#define xssDialogsProperties "chrome://global/locale/commonDialogs.properties"
static NS_DEFINE_CID(kCStringBundleServiceCID,  NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kXSSHostConnectPermissionManagerCID,  NS_XSSHOSTCONNECTPERMISSIONMANAGER_CID);

#include "xsstaint.h"
#include "prenv.h"

#endif /* XSS */

#ifdef PR_LOGGING
static PRLogModuleInfo* gConPolLog;
#endif

nsresult
NS_NewContentPolicy(nsIContentPolicy **aResult)
{
  *aResult = new nsContentPolicy;
  if (!*aResult)
      return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

/*
 * This constructor does far too much.  I wish there was a way to get
 * an Init method called by the service manager after the factory
 * returned the new object, so that errors could be propagated back to
 * the caller correctly.
 */
nsContentPolicy::nsContentPolicy()
{
#ifdef PR_LOGGING
    if (! gConPolLog) {
        gConPolLog = PR_NewLogModule("nsContentPolicy");
    }
#endif
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman = 
             do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return; /* log an error? */

    /*
     * I'd like to use GetCategoryContents, so that I can size the array
     * correctly on the first go and avoid the enumerator overhead, but it's
     * not yet implemented (see nsCategoryManager.cpp).  No biggie, I guess.
     */
    nsCOMPtr<nsISimpleEnumerator> catEnum;
    rv = catman->EnumerateCategory(NS_CONTENTPOLICY_CATEGORY,
                                   getter_AddRefs(catEnum));
    if (NS_FAILED(rv))
        return; /* no category, no problem */

    PRBool hasMore;
    if (NS_FAILED(catEnum->HasMoreElements(&hasMore)) || !hasMore)
       return;
    
    /* 
     * Populate mPolicies with policy services named by contractids in the
     * "content-policy" category.
     */
    nsCOMPtr<nsISupports> item;
    while (NS_SUCCEEDED(catEnum->GetNext(getter_AddRefs(item)))) {
        nsCOMPtr<nsISupportsCString> string = do_QueryInterface(item, &rv);
        if (NS_FAILED(rv))
            continue;

        nsCAutoString contractid;
        if (NS_FAILED(string->GetData(contractid)))
            continue;

        PR_LOG(gConPolLog, PR_LOG_DEBUG,
                ("POLICY: loading %s\n", contractid.get()));

        /*
         * Create this policy service and add to mPolicies.
         *
         * Should we try to parse as a CID, in case the component prefers to be
         * registered that way?
         */
        nsCOMPtr<nsIContentPolicy> policy = do_GetService(contractid.get(),
                                                          &rv);
        if (NS_SUCCEEDED(rv) && policy) {
            mPolicies.AppendObject(policy);
        }
    }
	
}

nsContentPolicy::~nsContentPolicy()
{
}

#ifdef DEBUG
#define WARN_IF_URI_UNINITIALIZED(uri,name)                         \
  PR_BEGIN_MACRO                                                    \
    if ((uri)) {                                                    \
        nsCAutoString spec;                                         \
        (uri)->GetAsciiSpec(spec);                                  \
        if (spec.IsEmpty()) {                                       \
            NS_WARNING(name " is uninitialized, fix caller");       \
        }                                                           \
    }                                                               \
  PR_END_MACRO

#else  // ! defined(DEBUG)

#define WARN_IF_URI_UNINITIALIZED(uri,name)

#endif // defined(DEBUG)

inline nsresult
nsContentPolicy::CheckPolicy(CPMethod          policyMethod,
                             PRUint32          contentType,
                             nsIURI           *contentLocation,
                             nsIURI           *requestingLocation,
                             nsISupports      *requestingContext,
                             const nsACString &mimeType,
                             nsISupports      *extra,
                             PRInt16           *decision)
{
    //sanity-check passed-through parameters
    NS_PRECONDITION(decision, "Null out pointer");
    WARN_IF_URI_UNINITIALIZED(contentLocation, "Request URI");
    WARN_IF_URI_UNINITIALIZED(requestingLocation, "Requesting URI");


#ifdef XSS /* XSS */
	// ASCII encoded URL spec
    nsCString mSpec; 
	// flag if domains are equal
    PRBool isDomainEqual = PR_FALSE; 
	// what the rule said 
	PRUint32 xssRuleResult = nsIXSSHostConnectPermissionManager::UNKNOWN_CONNECT; 
	// hosts, domains and uris
	nsCString calledURI, callerURI, calledHost, callerHost, calledDomain, callerDomain, mSpecTaintedstr;
	// the permissionmanager
	nsCOMPtr<nsIXSSHostConnectPermissionManager> xssPermissionManager;
	// prompt to interact with user
	nsCOMPtr<nsIPrompt> prompt;
	// if not 0, a error while initializing occured
	int initerror = 0;
	// flag if the contentLocation is tainted (=1) or not (=0)
	int mSpecIstainted = 0;
	// initialize string if mSpec is tainted
	mSpecTaintedstr = ToNewCString(NS_LITERAL_STRING("untainted")); 

	// initialize the variables
	nsCOMPtr<nsIDOMNode> node(do_QueryInterface(requestingContext));
	if (node) {
		// for return values
		nsresult rv;
		nsCOMPtr<nsIDOMDocument> doc;
		node->GetOwnerDocument(getter_AddRefs(doc));
		nsCOMPtr<nsIDocument> thedoc = do_QueryInterface(doc);

		// we need the doc to do something useful
		if (thedoc) {
			nsCOMPtr<nsIDOMWindowInternal> window (do_QueryInterface(thedoc->GetScriptGlobalObject()));

			if (window) {
				// get the prompt-service
				window->GetPrompter(getter_AddRefs(prompt));
				if (!prompt)
					initerror = 1;
			} else {
				initerror = 2;
			}

			if (contentLocation) {
				rv = contentLocation->GetSpec(calledURI);
				if (NS_FAILED(rv)) {
					XSS_LOG("failed to get calledURI\n", "");
					initerror = 3;
				}
				rv = contentLocation->GetHost(calledHost);
				if (NS_FAILED(rv)) {
					XSS_LOG("failed to get calledHost\n", "");
					initerror = 5;
				}
				rv = contentLocation->GetDomain(calledDomain);
				if (NS_FAILED(rv)) {
					XSS_LOG("failed to get domain of called uri\n", "");
					initerror = 7;
				}
				rv = contentLocation->GetAsciiSpec(mSpec);
				if (!NS_FAILED(rv) && mSpec.xssGetTainted()) {
					mSpecIstainted = 1;
					mSpecTaintedstr = ToNewCString(NS_LITERAL_STRING("tainted!")); 
				}
			} else {
				XSS_LOG("failed because contentLocation is 0\n", "");
				initerror = 11;
			}

			if (requestingLocation) {
				rv = requestingLocation->GetSpec(callerURI);
				if (NS_FAILED(rv)) {
					XSS_LOG("failed to get callerURI\n", "");
					initerror = 4;
				}
				rv = requestingLocation->GetHost(callerHost);
				if (NS_FAILED(rv)) {
					XSS_LOG("failed to get callerHost\n", "");
					initerror = 6;
				}
				rv = requestingLocation->GetDomain(callerDomain);
				if (NS_FAILED(rv)) {
					XSS_LOG("failed to get domain of caller\n", "");
					initerror = 8;
				}
			} else {
				XSS_LOG("failed because requestingLocation is 0\n", "");
				initerror = 12;
			}

			if (initerror == 0) {
				xssPermissionManager = do_GetService(NS_XSSHOSTCONNECTPERMISSIONMANAGER_CONTRACTID, &rv);
				if (NS_FAILED(rv)) {
					XSS_LOG("failed to get permissionmanager\n", "");
					initerror = 9;
				} else {
					// check if we already have a stored permission
					rv = xssPermissionManager->TestPermission(callerDomain, calledDomain, &xssRuleResult);
					if (NS_FAILED(rv)) {
						XSS_LOG("failed to test permission\n", "");
						initerror = 10;
					}
				}

				// check if the questioning is switched off (XSS_USERINTERACTION==XSS_ENV_USERINTERACTION_FALSE) by the environment
				char* env = PR_GetEnv(XSS_ENV_USERINTERACTION_STR);
				if (env) {
					PRInt32 num;
					if (sscanf(env, "%d", &num) > 0) {
						// XSS_USERINTERACTION must be XSS_ENV_USERINTERACTION_FALSE and not persistent settings
						if ((num == XSS_ENV_USERINTERACTION_FALSE) && (xssRuleResult == nsIXSSHostConnectPermissionManager::UNKNOWN_CONNECT)) {
							xssRuleResult = nsIXSSHostConnectPermissionManager::ALLOW_CONNECT;
							XSS_LOG("domaincheck: environment allowed %s\n", ToNewCString(NS_LITERAL_STRING("from ") + NS_ConvertUTF8toUTF16(callerURI) + NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));
						}
					}
				}

				// check if permanent rules are always used regardless of tainted status
				env = PR_GetEnv(XSS_ENV_DONTCHECKTAINT);
				if (env) {
					PRInt32 num;
					if (sscanf(env, "%d", &num) > 0) {
						// XSS_ENV_DONTCHECKTAINT must be XSS_ENV_DONTCHECKTAINT_TRUE
						if (num == XSS_ENV_DONTCHECKTAINT_TRUE) {
							xssRuleResult = nsIXSSHostConnectPermissionManager::UNKNOWN_CONNECT;
							XSS_LOG("domaincheck: environment delayed stored decision %s\n", ToNewCString(NS_LITERAL_STRING("from ") + NS_ConvertUTF8toUTF16(callerURI) + NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));
						}
					}
				}
			} // end of initerror = 0

			// to this point the variables are initialized or initerror is now != 0

			// if an error occured on initializing, we don't want to check.
			// breaking expected behaviour is bad
			if (initerror == 0) {
				nsresult xss_rv = contentLocation->DomainEquals(requestingLocation, &isDomainEqual);
				// check if this are different domains
				if (!NS_FAILED(xss_rv) &&!isDomainEqual) {
					
					// if we have a stored decision use it regardless of the taintstate
					if (xssRuleResult == nsIXSSHostConnectPermissionManager::ALLOW_CONNECT) {

						XSS_LOG("domaincheck: user allowed it always tainted: %s\n",
							ToNewCString(NS_ConvertUTF8toUTF16(mSpecTaintedstr) + NS_LITERAL_STRING(" from ") + 
							NS_ConvertUTF8toUTF16(callerURI) + NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));

					} else if (xssRuleResult == nsIXSSHostConnectPermissionManager::DENY_CONNECT) {

						XSS_LOG("domaincheck: user stopped it always tainted=%d: %s\n", 
							ToNewCString(NS_ConvertUTF8toUTF16(mSpecTaintedstr) + NS_LITERAL_STRING(" from ") + 
							NS_ConvertUTF8toUTF16(callerURI) + NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));
						*decision = nsIContentPolicy::REJECT_REQUEST;
						return NS_OK;

					// no decision, so ask if it is tainted
					} else {						
						if (mSpecIstainted) {

							// for return values
							nsresult rv;

							// check again if we already have a stored permission
							// it is possible that the env-variable overrides the initial test so
							// do it again
							rv = xssPermissionManager->TestPermission(callerDomain, calledDomain, &xssRuleResult);
							if (NS_FAILED(rv)) {
								XSS_LOG("failed to test permission\n", "");
								*decision = nsIContentPolicy::REJECT_REQUEST;
								return NS_OK;
							}

							// if there isn't a permanent rule for it, ask the user
							if (xssRuleResult == nsIXSSHostConnectPermissionManager::UNKNOWN_CONNECT) {

								XSS_LOG("domaincheck: ask %s\n", ToNewCString(NS_LITERAL_STRING("from ") + NS_ConvertUTF8toUTF16(callerURI) + NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));

								// get the question-string
								nsString xss_question_str;

								nsCOMPtr<nsIStringBundleService> stringBundleService =
									do_GetService(kCStringBundleServiceCID, &rv);

								if (NS_SUCCEEDED(rv) && stringBundleService) {
									nsCOMPtr<nsIStringBundle> stringBundle;
									rv = stringBundleService->CreateBundle(xssDialogsProperties,
										getter_AddRefs(stringBundle));

									if (stringBundle) {
										nsXPIDLString tempString;
										const PRUnichar *formatStrings[2];
										formatStrings[0] = ToNewUnicode(callerHost);
										formatStrings[1] = ToNewUnicode(calledHost);
										rv = stringBundle->FormatStringFromName(
											NS_LITERAL_STRING("ConfirmExXSS").get(),
											formatStrings, 2, getter_Copies(tempString));
										if (tempString)
											xss_question_str = tempString.get();
									}
								}

								// Just in case
								if (xss_question_str.IsEmpty()) {
									NS_WARNING("could not get ConfirmExXSS string from string bundle");
									xss_question_str.Assign(NS_LITERAL_STRING("[ConfirmExXSS] from "));
									xss_question_str.Append(NS_ConvertUTF8toUTF16(callerURI));
									xss_question_str.Append(NS_LITERAL_STRING(" to "));
									xss_question_str.Append(NS_ConvertUTF8toUTF16(calledURI));
								}
								PRUnichar *xss_question = ToNewUnicode(xss_question_str);

								// ask the question
								PRInt32 xss_choice;
								rv = prompt->ConfirmExXSS(nsnull, xss_question,
									nsIPrompt::BUTTON_TITLE_NO * nsIPrompt::BUTTON_POS_0 +
									nsIPrompt::BUTTON_TITLE_NO_ALWAYS  * nsIPrompt::BUTTON_POS_1 +
									nsIPrompt::BUTTON_TITLE_YES_ALWAYS  * nsIPrompt::BUTTON_POS_2,
									nsIPrompt::BUTTON_TITLE_YES  * nsIPrompt::BUTTON_POS_0,
									nsnull, nsnull, nsnull, nsnull, nsnull, nsnull, &xss_choice);
								if (NS_FAILED(rv)) {
									*decision = nsIContentPolicy::REJECT_REQUEST;
									return NS_OK;
								}

								// evaluate the answer
								switch (xss_choice) {
									// yes-button
									case 3:
										XSS_LOG("domaincheck: user allowed it: %s\n", 
											ToNewCString(NS_ConvertUTF8toUTF16(mSpecTaintedstr) + NS_LITERAL_STRING(" from ") 
											+ NS_ConvertUTF8toUTF16(callerURI) + NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)) );
										break;
										// yes, always-button
									case 2:
										XSS_LOG("domaincheck: user allowed it always: %s\n", 
											ToNewCString(NS_ConvertUTF8toUTF16(mSpecTaintedstr) + NS_LITERAL_STRING(" from ") 
											+ NS_ConvertUTF8toUTF16(callerURI) + NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));
										xssPermissionManager->Add(callerDomain, calledDomain, nsIXSSHostConnectPermissionManager::ALLOW_CONNECT);
										break;
										// no-always-button
									case 1:
										XSS_LOG("domaincheck: user stopped it always: %s\n", 
											ToNewCString(NS_ConvertUTF8toUTF16(mSpecTaintedstr) + NS_LITERAL_STRING(" from ") 
											+ NS_ConvertUTF8toUTF16(callerURI) + NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));
										xssPermissionManager->Add(callerDomain, calledDomain, nsIXSSHostConnectPermissionManager::DENY_CONNECT);
										*decision = nsIContentPolicy::REJECT_REQUEST;
										return NS_OK;
										break;
										// no-button
									case 0:
										XSS_LOG("domaincheck: user stopped it: %s\n", 
											ToNewCString(NS_ConvertUTF8toUTF16(mSpecTaintedstr) + NS_LITERAL_STRING(" from ") 
											+ NS_ConvertUTF8toUTF16(callerURI) + NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));
										*decision = nsIContentPolicy::REJECT_REQUEST;
										return NS_OK;
										break;
									default:
										XSS_LOG("domaincheck: error! %s\n", 
											ToNewCString(NS_LITERAL_STRING("from ") + NS_ConvertUTF8toUTF16(callerURI) 
											+ NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));
										*decision = nsIContentPolicy::REJECT_REQUEST;
										return NS_OK;
										break;
								}
							// we already have a user decision
							} else {
								if (xssRuleResult == nsIXSSHostConnectPermissionManager::DENY_CONNECT) {
									*decision = nsIContentPolicy::REJECT_REQUEST;
									XSS_LOG("domaincheck: stored always deny! %s\n", 
										ToNewCString(NS_LITERAL_STRING("from ") + NS_ConvertUTF8toUTF16(callerURI) 
										+ NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));
									return NS_OK;
								} else {
									XSS_LOG("domaincheck: stored always allow! %s\n", 
										ToNewCString(NS_LITERAL_STRING("from ") + NS_ConvertUTF8toUTF16(callerURI) 
										+ NS_LITERAL_STRING(" to ") + NS_ConvertUTF8toUTF16(calledURI)));
								}
							}
						} // end of tainted
					} // end of question
					/* If no node is found, just continue and let the other handlers do their job.
					   node could be null if the googlebar is used to search for something.
					*/
				} // end of domains are not equal
			} // end of initerror
		} // end of thedoc
	} // end of node
#endif /* XSS */
    

#ifdef DEBUG
    {
        nsCOMPtr<nsIDOMNode> node(do_QueryInterface(requestingContext));
        nsCOMPtr<nsIDOMWindow> window(do_QueryInterface(requestingContext));
        NS_ASSERTION(!requestingContext || node || window,
                     "Context should be a DOM node or a DOM window!");
    }
#endif

    PRInt32 count = mPolicies.Count();
    nsresult rv = NS_OK;

    /* 
     * Enumerate mPolicies and ask each of them, taking the logical AND of
     * their permissions.
     */
    for (PRInt32 i = 0; i < count; i++) {
        nsIContentPolicy *policy = mPolicies[i];
        if (!policy) { //shouldn't happen
            NS_ERROR("Somehow a null policy got into the list");
            continue;
        }

        /* check the appropriate policy */
        rv = (policy->*policyMethod)(contentType, contentLocation,
                                     requestingLocation, requestingContext,
                                     mimeType, extra, decision);

        if (NS_SUCCEEDED(rv) && NS_CP_REJECTED(*decision)) {
            /* policy says no, no point continuing to check */
            return NS_OK;
        }
    }

    // everyone returned failure, or no policies: sanitize result
    *decision = nsIContentPolicy::ACCEPT;
    return NS_OK;
}

#ifdef PR_LOGGING

//uses the parameters from ShouldXYZ to produce and log a message
//logType must be a literal string constant
#define LOG_CHECK(logType)                                                    \
  PR_BEGIN_MACRO                                                              \
    /* skip all this nonsense if the call failed */                           \
    if (NS_SUCCEEDED(rv)) {                                                   \
      const char *resultName;                                                 \
      if (decision) {                                                         \
        resultName = NS_CP_ResponseName(*decision);                           \
      } else {                                                                \
        resultName = "(null ptr)";                                            \
      }                                                                       \
      nsCAutoString spec("None");                                             \
      if (contentLocation) {                                                  \
          contentLocation->GetSpec(spec);                                     \
      }                                                                       \
      nsCAutoString refSpec("None");                                          \
      if (requestingLocation) {                                               \
          requestingLocation->GetSpec(refSpec);                               \
      }                                                                       \
      PR_LOG(gConPolLog, PR_LOG_DEBUG,                                        \
             ("Content Policy: " logType ": <%s> <Ref:%s> result=%s",         \
              spec.get(), refSpec.get(), resultName)                          \
             );                                                               \
    }                                                                         \
  PR_END_MACRO

#else //!defined(PR_LOGGING)

#define LOG_CHECK(logType)

#endif //!defined(PR_LOGGING)

NS_IMETHODIMP
nsContentPolicy::ShouldLoad(PRUint32          contentType,
                            nsIURI           *contentLocation,
                            nsIURI           *requestingLocation,
                            nsISupports      *requestingContext,
                            const nsACString &mimeType,
                            nsISupports      *extra,
                            PRInt16          *decision)
{
    // ShouldProcess does not need a content location, but we do
    NS_PRECONDITION(contentLocation, "Must provide request location");
    nsresult rv = CheckPolicy(&nsIContentPolicy::ShouldLoad, contentType,
                              contentLocation, requestingLocation,
                              requestingContext, mimeType, extra, decision);
    LOG_CHECK("ShouldLoad");

    return rv;
}

NS_IMETHODIMP
nsContentPolicy::ShouldProcess(PRUint32          contentType,
                               nsIURI           *contentLocation,
                               nsIURI           *requestingLocation,
                               nsISupports      *requestingContext,
                               const nsACString &mimeType,
                               nsISupports      *extra,
                               PRInt16          *decision)
{
    nsresult rv = CheckPolicy(&nsIContentPolicy::ShouldProcess, contentType,
                              contentLocation, requestingLocation,
                              requestingContext, mimeType, extra, decision);
    LOG_CHECK("ShouldProcess");

    return rv;
}
