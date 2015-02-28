/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Conrad Carlen <conrad@ingress.com>
 */

#include "nsIDirectoryService.h"
#include "nsILocalFile.h"

class nsIFile;

//*****************************************************************************
// class nsAppFileLocationProvider
//*****************************************************************************   

class nsAppFileLocationProvider : public nsIDirectoryServiceProvider2
{
public:
                        nsAppFileLocationProvider();

   NS_DECL_ISUPPORTS
   NS_DECL_NSIDIRECTORYSERVICEPROVIDER
   NS_DECL_NSIDIRECTORYSERVICEPROVIDER2

private:
                        ~nsAppFileLocationProvider() {}

protected:
   NS_METHOD            CloneMozBinDirectory(nsILocalFile **aLocalFile);   
   NS_METHOD            GetProductDirectory(nsILocalFile **aLocalFile);
   NS_METHOD            GetDefaultUserProfileRoot(nsILocalFile **aLocalFile);

   nsCOMPtr<nsILocalFile> mMozBinDirectory;
};
