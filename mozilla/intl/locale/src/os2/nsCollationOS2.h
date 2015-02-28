/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef _nscollationos2_h_
#define _nscollationos2_h_


#include "nsICollation.h"
#include "nsCollation.h"  // static library
#include "nsCRT.h"



class nsCollationOS2 : public nsICollation {

protected:
  nsCollation   *mCollation;
  nsString      mLocale;
  nsString      mSavedLocale;

public:
  nsCollationOS2();
  ~nsCollationOS2();

  // nsISupports interface
  NS_DECL_ISUPPORTS

  // nsICollation interface
  NS_DECL_NSICOLLATION

};

#endif /* nsCollationOS2_h__ */ 
