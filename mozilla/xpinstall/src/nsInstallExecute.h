/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Daniel Veditz <dveditz@netscape.com>
 *     Douglas Turner <dougt@netscape.com>
 */


#ifndef nsInstallExecute_h__
#define nsInstallExecute_h__

#include "prtypes.h"
#include "nsString.h"

#include "nsInstallObject.h"

#include "nsInstall.h"
#include "nsIDOMInstallVersion.h"

PRInt32 xpi_PrepareProcessArguments(const char *aArgsString, char **aArgs, PRInt32 aArgsAvailable);


class nsInstallExecute : public nsInstallObject 
{
    public:
          
        nsInstallExecute( nsInstall* inInstall,
                          const nsString& inJarLocation,
                          const nsString& inArgs,
                          const PRBool inBlocking,
                          PRInt32 *error);


        virtual ~nsInstallExecute();

        PRInt32 Prepare();
        PRInt32 Complete();
        void  Abort();
        char* toString();

        PRBool CanUninstall();
        PRBool RegisterPackageNode();
	  
  
    private:
          
        nsString mJarLocation; // Location in the JAR
        nsString mArgs;        // command line arguments
        
        nsCOMPtr<nsIFile> mExecutableFile;    // temporary file location
        PRBool mBlocking;
        PRUint32* mPid;

        PRInt32 NativeComplete(void);
        void NativeAbort(void);

};

#endif /* nsInstallExecute_h__ */
