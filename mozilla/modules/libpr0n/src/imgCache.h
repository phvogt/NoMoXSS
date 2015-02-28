/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.
 * All Rights Reserved.
 * 
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#include "imgICache.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#include "prtypes.h"

class imgRequest;
class nsIURI;
class nsICacheEntryDescriptor;

#define NS_IMGCACHE_CID \
{ /* fb4fd28a-1dd1-11b2-8391-e14242c59a41 */         \
     0xfb4fd28a,                                     \
     0x1dd1,                                         \
     0x11b2,                                         \
    {0x83, 0x91, 0xe1, 0x42, 0x42, 0xc5, 0x9a, 0x41} \
}

class imgCache : public imgICache, 
                 public nsIObserver,
                 public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICACHE
  NS_DECL_NSIOBSERVER

  imgCache();
  virtual ~imgCache();

  static nsresult Init();

  static void Shutdown(); // for use by the factory

  /* additional members */
  static PRBool Put(nsIURI *aKey, imgRequest *request, nsICacheEntryDescriptor **aEntry);
  static PRBool Get(nsIURI *aKey, PRBool *aHasExpired, imgRequest **aRequest, nsICacheEntryDescriptor **aEntry);
  static PRBool Remove(nsIURI *aKey);

  static nsresult ClearChromeImageCache();
  static nsresult ClearImageCache();
};

