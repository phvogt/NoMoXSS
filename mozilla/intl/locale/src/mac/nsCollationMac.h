
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
 */
#ifndef nsCollationMac_h__
#define nsCollationMac_h__


#include "nsICollation.h"
#include "nsCollation.h"  // static library
#include "plstr.h"



class nsCollationMac : public nsICollation {

protected:
  nsCollation   *mCollation;            // XP collation class
  
  short         m_scriptcode;           // Macintosh platform script code
  unsigned char m_mac_sort_tbl[256];    // Mapping table from a character code to a collation key value.

public: 
  nsCollationMac();
  ~nsCollationMac();

  // nsISupports interface
  NS_DECL_ISUPPORTS

  // nsICollation interface
  NS_DECL_NSICOLLATION

};

#endif  /* nsCollationMac_h__ */
