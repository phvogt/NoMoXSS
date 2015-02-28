/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
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


/*
   wallet.h --- prototypes for wallet functions.
*/


#ifndef _WALLET_H
#define _WALLET_H

#include "prtypes.h"
#include "nsIPresShell.h"
#include "nsString.h"
#include "nsFileSpec.h"
#include "nsIPrompt.h"

class nsIDOMWindowInternal;
class nsIDOMNode;

PR_BEGIN_EXTERN_C

#define HEADER_VERSION "#2c"

#define YES_BUTTON 0
#define NO_BUTTON 1
#define NEVER_BUTTON 2

#define pref_Crypto "wallet.crypto"
#define pref_AutoCompleteOverride "wallet.crypto.autocompleteoverride"

extern void
WLLT_ChangePassword(PRBool* status);

extern void
WLLT_DeleteAll();

extern void
WLLT_ClearUserData();

extern void
WLLT_DeletePersistentUserData();

extern void
WLLT_PreEdit(nsString& walletList);

extern void
WLLT_PostEdit(const nsString& walletList);

extern void
WLLT_PrefillReturn(const nsString& results);

extern void
WLLT_RequestToCapture(nsIPresShell* shell, nsIDOMWindowInternal * win, PRUint32* status);

extern nsresult
WLLT_PrefillOneElement
  (nsIDOMWindowInternal* win, nsIDOMNode* elementNode, nsString& compositeValue);

extern nsresult
WLLT_Prefill(nsIPresShell* shell, PRBool quick, nsIDOMWindowInternal* win);

extern void
WLLT_GetNopreviewListForViewer(nsString& aNopreviewList);

extern void
WLLT_GetNocaptureListForViewer(nsString& aNocaptureList);

extern void
WLLT_GetPrefillListForViewer(nsString& aPrefillList);

extern void
WLLT_OnSubmit(nsIContent* formNode, nsIDOMWindowInternal* window);

extern void
WLLT_ExpirePassword(PRBool* status);

extern void
WLLT_ExpirePasswordOnly(PRBool* status);

extern void
WLLT_InitReencryptCallback(nsIDOMWindowInternal* window);

extern nsresult
Wallet_Encrypt(const nsString& text, nsString& crypt);

extern nsresult
Wallet_Decrypt(const nsString& crypt, nsString& text);

extern nsresult Wallet_ProfileDirectory(nsFileSpec& dirSpec);

extern PRUnichar * Wallet_Localize(const char * genericString);

extern char* Wallet_RandomName(char* suffix);

extern PRBool Wallet_ConfirmYN(PRUnichar * szMessage, nsIDOMWindowInternal* window);

extern PRInt32 Wallet_3ButtonConfirm(PRUnichar * szMessage, nsIDOMWindowInternal* window);

extern void Wallet_GiveCaveat(nsIDOMWindowInternal* window, nsIPrompt* dialog);

extern nsresult
Wallet_Encrypt2(const nsString& text, nsString& crypt);

extern nsresult
Wallet_Decrypt2(const nsString& crypt, nsString& text);

extern void
Wallet_UTF8Put(nsOutputFileStream& strm, PRUnichar c);

extern PRUnichar
Wallet_UTF8Get(nsInputFileStream& strm);

extern void
Wallet_SignonViewerReturn(const nsString& results);

extern void
Wallet_ReleaseAllLists();

PR_END_EXTERN_C

#endif /* !_WALLET_H */
