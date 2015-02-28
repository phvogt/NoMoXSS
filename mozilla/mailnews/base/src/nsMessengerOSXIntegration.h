/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
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
 * The Original Code is mozilla.org code.
 *
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Contributor(s):
 *  Scott MacGregor <mscott@mozilla.org>
 *  Jon Baumgartner <jon@bergenstreetsoftware.com>
 * ***** END LICENSE BLOCK ***** */

#ifndef __nsMessengerOSXIntegration_h
#define __nsMessengerOSXIntegration_h

#include "nsIMessengerOSIntegration.h"
#include "nsIFolderListener.h"
#include "nsIAtom.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIPref.h"
#include "nsInt64.h"
#include "nsISupportsArray.h"

#define NS_MESSENGEROSXINTEGRATION_CID \
  {0xaa83266, 0x4225, 0x4c4b, \
    {0x93, 0xf8, 0x94, 0xb1, 0x82, 0x58, 0x6f, 0x93}}

class nsMessengerOSXIntegration : public nsIMessengerOSIntegration,
                                  public nsIFolderListener
{
public:
  nsMessengerOSXIntegration();
  virtual ~nsMessengerOSXIntegration();
  virtual nsresult Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMESSENGEROSINTEGRATION
  NS_DECL_NSIFOLDERLISTENER

private:
  nsCOMPtr<nsISupportsArray> mFoldersWithNewMail;  // keep track of all the root folders with pending new mail
  nsCOMPtr<nsIAtom> mBiffStateAtom;
  PRInt32 CountNewMessages();
  nsresult OnAlertFinished(const PRUnichar * aAlertCookie);

  PRPackedBool mBiffIconVisible;
  PRPackedBool mSuppressBiffIcon;
  PRPackedBool mAlertInProgress;
};

#endif // __nsMessengerOSXIntegration_h
