/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#ifndef nsImgManager_h__
#define nsImgManager_h__

#include "nsIImgManager.h"
#include "nsIContentPolicy.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIPermissionManager.h"

class nsIPrefBranch;

////////////////////////////////////////////////////////////////////////////////

class nsImgManager : public nsIImgManager,
                     public nsIContentPolicy,
                     public nsIObserver,
                     public nsSupportsWeakReference
{
public:

  // nsISupports
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIMGMANAGER
  NS_DECL_NSICONTENTPOLICY
  NS_DECL_NSIOBSERVER

  nsImgManager();
  virtual ~nsImgManager();
  nsresult Init();

protected:

  void PrefChanged(nsIPrefBranch *, const char *);

  nsCOMPtr<nsIPermissionManager> mPermissionManager;  
  
  PRUint8      mBehaviorPref;
  PRPackedBool mWarningPref;
  PRPackedBool mBlockInMailNewsPref;
};

// {D60B3710-166D-11d5-A542-0010A401EB10}
#define NS_IMGMANAGER_CID \
{ 0xd60b3710, 0x166d, 0x11d5, { 0xa5, 0x42, 0x0, 0x10, 0xa4, 0x1, 0xeb, 0x10 } }

#endif /* nsImgManager_h__ */
