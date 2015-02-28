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
 * The Original Code is the mozilla.org LDAP XPCOM SDK.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): Dan Mosedale <dmose@mozilla.org>
 *                 Leif Hedstrom <leif@netscape.com>
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#include "nsLDAPURL.h"
#include "nsIGenericFactory.h"
#include "nsLDAPConnection.h"
#include "nsLDAPOperation.h"
#include "nsLDAPMessage.h"
#include "nsLDAPServer.h"
#include "nsLDAPService.h"
#include "nsLDAPBERValue.h"
#include "ldappr.h"

#ifdef MOZ_LDAP_XPCOM_EXPERIMENTAL
#include "nsLDAPProtocolHandler.h"
#include "nsLDAPChannel.h"
#endif

// use the default constructor
//
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLDAPConnection)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLDAPOperation)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLDAPMessage)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLDAPServer)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsLDAPURL, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsLDAPService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLDAPBERValue)

#ifdef MOZ_LDAP_XPCOM_EXPERIMENTAL
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLDAPProtocolHandler)
#endif

// a table of the CIDs implemented by this module
//
static const nsModuleComponentInfo components[] =
{
    { "LDAP Connection", NS_LDAPCONNECTION_CID,
          "@mozilla.org/network/ldap-connection;1", 
          nsLDAPConnectionConstructor },
    { "LDAP Operation", NS_LDAPOPERATION_CID,
          "@mozilla.org/network/ldap-operation;1", 
          nsLDAPOperationConstructor },
    { "LDAP Message", NS_LDAPMESSAGE_CID,
          "@mozilla.org/network/ldap-message;1", nsLDAPMessageConstructor },
    { "LDAP Server", NS_LDAPSERVER_CID,
          "@mozilla.org/network/ldap-server;1", nsLDAPServerConstructor },
    { "LDAP Service", NS_LDAPSERVICE_CID,
          "@mozilla.org/network/ldap-service;1", nsLDAPServiceConstructor },
#ifdef MOZ_LDAP_XPCOM_EXPERIMENTAL    
    { "LDAP Protocol Handler", NS_LDAPPROTOCOLHANDLER_CID, 
          NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "ldap", 
          nsLDAPProtocolHandlerConstructor },   
#endif
    { "LDAP URL", NS_LDAPURL_CID,
          "@mozilla.org/network/ldap-url;1", nsLDAPURLConstructor },
    { "LDAP BER Value", NS_LDAPBERVALUE_CID,
          "@mozilla.org/network/ldap-ber-value;1", nsLDAPBERValueConstructor }
};

PR_STATIC_CALLBACK(nsresult)
nsLDAPInitialize(nsIModule *aSelf)
{
#ifdef PR_LOGGING
    gLDAPLogModule = PR_NewLogModule("ldap");
    if (!gLDAPLogModule) {
        PR_fprintf(PR_STDERR, 
                   "nsLDAP_Initialize(): PR_NewLogModule() failed\n");
        return NS_ERROR_NOT_AVAILABLE;
    }
#endif

    // use NSPR under the hood for all networking
    //
    int rv = prldap_install_routines( NULL, 1 /* shared */ );

    if (rv != LDAP_SUCCESS) {
        PR_LOG(gLDAPLogModule, PR_LOG_ERROR,
               ("nsLDAPInitialize(): pr_ldap_install_routines() failed: %s\n",
               ldap_err2string(rv)));
        return NS_ERROR_FAILURE;
    }

    // Never block for more than 10000 milliseconds (ie 10 seconds) doing any 
    // sort of I/O operation.
    //
    rv = prldap_set_session_option(0, 0, PRLDAP_OPT_IO_MAX_TIMEOUT, 
                                   10000);
    if (rv != LDAP_SUCCESS) {
        PR_LOG(gLDAPLogModule, PR_LOG_ERROR,
               ("nsLDAPInitialize(): error setting PRLDAP_OPT_IO_MAX_TIMEOUT:"
                " %s\n", ldap_err2string(rv)));
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

// implement the NSGetModule() exported function
//
NS_IMPL_NSGETMODULE_WITH_CTOR(nsLDAPProtocolModule, components, 
                              nsLDAPInitialize)

#ifdef PR_LOGGING
PRLogModuleInfo *gLDAPLogModule = 0;
#endif
