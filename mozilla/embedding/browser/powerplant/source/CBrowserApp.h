/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Conrad Carlen <ccarlen@netscape.com>
 */

#pragma once

#include <PP_Prefix.h>
#include <LApplication.h>

#include "nsError.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#ifndef USE_PROFILES
class nsProfileDirServiceProvider;
#endif

class CBrowserApp : public PP_PowerPlant::LApplication

#ifdef USE_PROFILES
                      ,public nsIObserver
                      ,public nsSupportsWeakReference
#endif

{
    friend class CStartUpTask;
    
public:
                            CBrowserApp();  // constructor registers PPobs
    virtual                 ~CBrowserApp();   // stub destructor

#ifdef USE_PROFILES
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
#endif

    virtual void            AdjustCursor(const EventRecord& inMacEvent);

    virtual void            HandleAppleEvent(const AppleEvent&  inAppleEvent,
                                             AppleEvent&        outAEReply,
                                             AEDesc&            outResult,
                                             long               inAENumber);

    // this overriding method handles application commands
    virtual Boolean         ObeyCommand(PP_PowerPlant::CommandT inCommand, void* ioParam);  


    // this overriding method returns the status of menu items
    virtual void            FindCommandStatus(PP_PowerPlant::CommandT inCommand,
                                              Boolean &outEnabled, Boolean &outUsesMark,
                                              UInt16 &outMark, Str255 outName);

   virtual Boolean          AttemptQuitSelf(SInt32 inSaveOption);


protected:

    virtual nsresult        OverrideComponents();
    virtual void            MakeMenuBar();
    virtual void						Initialize();

#if TARGET_CARBON
    virtual void            InstallCarbonEventHandlers();
    static pascal OSStatus  AppEventHandler(EventHandlerCallRef myHandlerChain,
                                            EventRef event,
                                            void* userData);
#endif

    virtual nsresult        InitializePrefs();
    virtual void            OnStartUp();

    virtual Boolean         SelectFileObject(PP_PowerPlant::CommandT    inCommand,
                                             FSSpec& outSpec);

#ifdef USE_PROFILES
    Boolean                 ConfirmProfileSwitch();
#else
    nsProfileDirServiceProvider* mProfDirServiceProvider;
#endif

};
