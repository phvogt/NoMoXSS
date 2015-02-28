/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 *  Simon Fraser <sfraser@netscape.com>
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


#include "nsMemory.h"

#include "nsWindowUtils.h"
#include "nsAETokens.h"

#include "nsAEGetURLSuiteHandler.h"
#include "nsCommandLineServiceMac.h"

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIPref.h"
#include "nsIWindowMediator.h"
#include "nsIXULWindow.h"

using namespace nsWindowUtils;

PRBool AEGetURLSuiteHandler::sReuseWindowPrefInited = PR_FALSE;
PRBool AEGetURLSuiteHandler::sReuseWindow = PR_FALSE;


/*----------------------------------------------------------------------------
	AEGetURLSuiteHandler 
	
----------------------------------------------------------------------------*/
AEGetURLSuiteHandler::AEGetURLSuiteHandler()
{
  if ( !sReuseWindowPrefInited ) {
    nsCOMPtr<nsIPref> prefService ( do_GetService(NS_PREF_CONTRACTID) );
    if ( prefService ) {
      prefService->GetBoolPref("browser.always_reuse_window", &sReuseWindow);
      prefService->RegisterCallback("browser.always_reuse_window", ReuseWindowPrefCallback, nsnull);
    }
    sReuseWindowPrefInited = PR_TRUE;
  }
}

/*----------------------------------------------------------------------------
	~AEGetURLSuiteHandler 
	
----------------------------------------------------------------------------*/
AEGetURLSuiteHandler::~AEGetURLSuiteHandler()
{
}


/*----------------------------------------------------------------------------
	HandleGetURLSuiteEvent 
	
----------------------------------------------------------------------------*/
void AEGetURLSuiteHandler::HandleGetURLSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	OSErr		err = noErr;
	
	AEEventID		eventID;
	OSType		typeCode;
	Size			actualSize 	= 0L;
	
	// Get the event ID
	err = AEGetAttributePtr(appleEvent, 	keyEventIDAttr, 
									typeType, 
									&typeCode, 
									(Ptr)&eventID, 
									sizeof(eventID), 
									&actualSize);
	ThrowIfOSErr(err);
	
	try
	{
		switch (eventID)
		{
			case kGetURLEvent:
				HandleGetURLEvent(appleEvent, reply);
				break;
				
			default:
				ThrowOSErr(errAEEventNotHandled);
				break;
		}
	}
	catch (OSErr catchErr)
	{
		PutReplyErrorNumber(reply, catchErr);
		throw;
	}
	catch ( ... )
	{
		PutReplyErrorNumber(reply, paramErr);
		throw;
	}
}


/*----------------------------------------------------------------------------
	HandleGetURLEvent 
	
----------------------------------------------------------------------------*/
void AEGetURLSuiteHandler::HandleGetURLEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	StAEDesc		directParameter;
	WindowPtr		targetWindow = NULL;
	OSErr				err;
	
	// extract the direct parameter (an object specifier)
	err = ::AEGetKeyDesc(appleEvent, keyDirectObject, typeWildCard, &directParameter);
	ThrowIfOSErr(err);

	// we need to look for other parameters, to do with destination etc.
	long		dataSize = directParameter.GetDataSize();
	char*	urlString = (char *)nsMemory::Alloc(dataSize + 1);
	ThrowIfNil(urlString);	
	directParameter.GetCString(urlString, dataSize + 1);

	// get the destination window, if applicable
	StAEDesc		openInWindowDesc;
	err = ::AEGetKeyDesc(appleEvent, kInsideWindowParameter, typeObjectSpecifier, &openInWindowDesc);
	if (err != errAEDescNotFound)
	{
		// resolve the object specifier into a token record
		StAEDesc		tokenDesc;
		err = ::AEResolve(&openInWindowDesc, kAEIDoMinimum, &tokenDesc);
		ThrowIfOSErr(err);
		
		ConstAETokenDesc	tokenContainer(&tokenDesc);
		targetWindow = tokenContainer.GetWindowPtr();		
	}

  // if the AE didn't specify a target, the user may still want it to reuse
  // an existing window. Look if they have the pref specified. If they did
  // specify the window, we have its windowPtr so dispatch to that.
  PRBool dispatched = PR_FALSE;
  if ( !targetWindow )
  {
    if ( sReuseWindow ) {
      nsCOMPtr<nsIWindowMediator> mediator ( do_GetService(NS_WINDOWMEDIATOR_CONTRACTID) );
      if ( mediator ) {
        nsCOMPtr<nsISimpleEnumerator> windowEnum;
        mediator->GetZOrderXULWindowEnumerator(NS_LITERAL_STRING("navigator:browser").get(), PR_TRUE, getter_AddRefs(windowEnum));
        if ( windowEnum ) {
          nsCOMPtr<nsISupports> windowSupports;
          windowEnum->GetNext(getter_AddRefs(windowSupports));
          nsCOMPtr<nsIXULWindow> xulwindow ( do_QueryInterface(windowSupports) );
          if ( xulwindow )
            LoadURLInXULWindow(xulwindow, urlString);
          else
            LoadURLInWindow(nsnull, urlString);
        }
      }
    }
    else
      LoadURLInWindow(nsnull, urlString);
  }
  else
    LoadURLInWindow(targetWindow, urlString);

	nsMemory::Free(urlString);	
}


int
AEGetURLSuiteHandler::ReuseWindowPrefCallback ( const char* inPref, void* inClosure )
{
  nsCOMPtr<nsIPref> prefService ( do_GetService(NS_PREF_CONTRACTID) );
  if ( prefService )
    prefService->GetBoolPref("browser.always_reuse_window", &sReuseWindow);
  
  return NS_OK;
}



