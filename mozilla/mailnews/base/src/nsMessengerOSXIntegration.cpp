/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-

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

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif


#include "nscore.h"

#include "nsMessengerOSXIntegration.h"
#include "nsIMsgAccountManager.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgIncomingServer.h"
#include "nsIMsgIdentity.h"
#include "nsIMsgAccount.h"
#include "nsIRDFResource.h"
#include "nsIMsgFolder.h"
#include "nsCOMPtr.h"
#include "nsMsgBaseCID.h"
#include "nsMsgFolderFlags.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIDirectoryService.h"

#include "nsIWindowMediator.h"
#include "nsIDOMChromeWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsIObserverService.h"

#include "nsIMessengerWindowService.h"
#include "prprf.h"
#include "nsIWeakReference.h"

#include "prlog.h"
#if defined(PR_LOGGING)
//
// export NSPR_LOG_MODULES=OSXIntegeration:5
//
static PRLogModuleInfo *gOSXIntegrationLog = nsnull;
#define LOG(args) PR_LOG(gOSXIntegrationLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

#include <Carbon/Carbon.h>

#define kBiffBadgeIcon "mail-biff-badge.png"

nsMessengerOSXIntegration::nsMessengerOSXIntegration()
{
#if defined(PR_LOGGING)
    if (!gOSXIntegrationLog)
        gOSXIntegrationLog = PR_NewLogModule("OSXIntegration");
#endif

  mBiffStateAtom = do_GetAtom("BiffState");

  mBiffIconVisible = PR_FALSE;
  mSuppressBiffIcon = PR_FALSE;
  mAlertInProgress = PR_FALSE;
  NS_NewISupportsArray(getter_AddRefs(mFoldersWithNewMail));
}

nsMessengerOSXIntegration::~nsMessengerOSXIntegration()
{
  if (mBiffIconVisible) 
  {
    RestoreApplicationDockTileImage();
    mBiffIconVisible = PR_FALSE;
  }
}

NS_IMPL_ADDREF(nsMessengerOSXIntegration)
NS_IMPL_RELEASE(nsMessengerOSXIntegration)

NS_INTERFACE_MAP_BEGIN(nsMessengerOSXIntegration)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMessengerOSIntegration)
   NS_INTERFACE_MAP_ENTRY(nsIMessengerOSIntegration)
   NS_INTERFACE_MAP_ENTRY(nsIFolderListener)
NS_INTERFACE_MAP_END


nsresult
nsMessengerOSXIntegration::Init()
{
  nsresult rv;
    
  nsCOMPtr <nsIMsgAccountManager> accountManager = 
    do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  // because we care if the default server changes
  rv = accountManager->AddRootFolderListener(this);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIMsgMailSession> mailSession = do_GetService(NS_MSGMAILSESSION_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  // because we care if the unread total count changes
  rv = mailSession->AddFolderListener(this, nsIFolderListener::boolPropertyChanged | nsIFolderListener::intPropertyChanged);
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemPropertyChanged(nsISupports *, nsIAtom *, char const *, char const *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemUnicharPropertyChanged(nsISupports *, nsIAtom *, const PRUnichar *, const PRUnichar *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemRemoved(nsISupports *, nsISupports *, const char *)
{
  return NS_OK;
}

PRInt32 nsMessengerOSXIntegration::CountNewMessages()
{

  // iterate over all the folders in mFoldersWithNewMail
  nsCOMPtr<nsISupports> supports;
  nsCOMPtr<nsIMsgFolder> folder;
  nsCOMPtr<nsIWeakReference> weakReference;
  PRInt32 numNewMessages = 0;
  PRInt32 totalNewMessages = 0;
  
  PRUint32 count = 0;
  mFoldersWithNewMail->Count(&count);

  for (PRUint32 index = 0; index < count; index++)
  {
    supports = getter_AddRefs(mFoldersWithNewMail->ElementAt(index));
    weakReference = do_QueryInterface(supports);
    folder = do_QueryReferent(weakReference);
    if (folder)
    {

      numNewMessages = 0;   
      folder->GetNumNewMessages(PR_TRUE, &numNewMessages);
      totalNewMessages += numNewMessages;
    } // if we got a folder
  } // for each folder

  return totalNewMessages;
  
}

nsresult nsMessengerOSXIntegration::OnAlertFinished(const PRUnichar * aAlertCookie)
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIPref> prefService;
  prefService = do_GetService(NS_PREF_CONTRACTID, &rv);  
  PRBool bounceDockIcon = PR_FALSE; 
  
  if (prefService)
    prefService->GetBoolPref("mail.biff.animate_dock_icon", &bounceDockIcon);

   // This will call GetAttention(), which will bounce the dock icon.
  if (!mSuppressBiffIcon)
  {
    nsCOMPtr<nsIWindowMediator> mediator (do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
    if (bounceDockIcon && mediator)
    {
        nsCOMPtr<nsIDOMWindowInternal> domWindow;
        mediator->GetMostRecentWindow(NS_LITERAL_STRING("mail:3pane").get(), getter_AddRefs(domWindow));
        if (domWindow)
        {         
            nsCOMPtr<nsIDOMChromeWindow> chromeWindow(do_QueryInterface(domWindow));
            chromeWindow->GetAttention();
        }
    }

    // This will change the dock icon.     
    // If we want to overlay the number of new messages on top of
    // the icon ...
    
    // use OverlayApplicationDockTileImage
    // -- you'll have to pass it a CGImage, and somehow we have to
    // create the CGImage with the numbers. tricky    
    PRInt32 totalNewMessages = CountNewMessages();
    CGContextRef context = ::BeginCGContextForApplicationDockTile();
    
    // Draw a circle.

    ::CGContextBeginPath(context);
    ::CGContextAddArc(context, 95.0, 95.0, 25.0, 0.0, 2 * M_PI, true);
    ::CGContextClosePath(context);

    // use #2fc600 for the color.
    ::CGContextSetRGBFillColor(context, 0.184, 0.776, 0.0, 1);

    //::CGContextSetRGBFillColor(context, 1.0, 1.0, 1.0, 0.7);
    ::CGContextFillPath(context);

    // Draw the number.
    nsAutoString total;
    total.AppendInt(totalNewMessages);

    // Use a system font (kThemeUtilityWindowTitleFont)
    ScriptCode sysScript = ::GetScriptManagerVariable(smSysScript);

    Str255 fontName;
    SInt16 fontSize;
    Style fontStyle;
    ::GetThemeFont(kThemeSmallEmphasizedSystemFont, sysScript, fontName,
                   &fontSize, &fontStyle);

    FMFontFamily family = ::FMGetFontFamilyFromName(fontName);
    FMFont fmFont;
    OSStatus err = ::FMGetFontFromFontFamilyInstance(family, fontStyle, &fmFont,
                                                     nsnull);
    if (err != noErr) {
        NS_WARNING("FMGetFontFromFontFamilyInstance failed");
        ::EndCGContextForApplicationDockTile(context);
        return NS_ERROR_FAILURE;
    }

    ATSUStyle style;
    err = ::ATSUCreateStyle(&style);
    if (err != noErr) {
        NS_WARNING("ATSUCreateStyle failed");
        ::EndCGContextForApplicationDockTile(context);

        return NS_ERROR_FAILURE;
        }
        
    Fixed size = Long2Fix(24);
    RGBColor black = { 0x0, 0x0, 0x0 };
    RGBColor white = { 0xFFFF, 0xFFFF, 0xFFFF };
    
    ATSUAttributeTag tags[3] = { kATSUFontTag, kATSUSizeTag, kATSUColorTag };
    ByteCount valueSizes[3] = { sizeof(ATSUFontID), sizeof(Fixed),
                                sizeof(RGBColor) };
    ATSUAttributeValuePtr values[3] = { &fmFont, &size, &white };

    err = ::ATSUSetAttributes(style, 3, tags, valueSizes, values);
    if (err != noErr) {
        NS_WARNING("ATSUSetAttributes failed");
        ::ATSUDisposeStyle(style);
        ::EndCGContextForApplicationDockTile(context);

        return NS_ERROR_FAILURE;
      }

    UniCharCount runLengths = kATSUToTextEnd;
    ATSUTextLayout textLayout;
    err = ::ATSUCreateTextLayoutWithTextPtr(total.get(),
                                            kATSUFromTextBeginning,
                                            kATSUToTextEnd, total.Length(), 1,
                                            &runLengths, &style, &textLayout);
    if (err != noErr) {
        NS_WARNING("ATSUCreateTextLayoutWithTextPtr failed");
        ::ATSUDisposeStyle(style);
        ::EndCGContextForApplicationDockTile(context);

        return NS_ERROR_FAILURE;
    }

    ATSUAttributeTag layoutTags[1] = { kATSUCGContextTag };
    ByteCount layoutValueSizes[1] = { sizeof(CGContextRef) };
    ATSUAttributeValuePtr layoutValues[1] = { &context };

    err = ::ATSUSetLayoutControls(textLayout, 1, layoutTags, layoutValueSizes,
                                  layoutValues);
    if (err != noErr) {
        NS_WARNING("ATSUSetLayoutControls failed");
        ::ATSUDisposeStyle(style);
        ::EndCGContextForApplicationDockTile(context);

        return NS_ERROR_FAILURE;
    }

    Rect boundingBox;
    err = ::ATSUMeasureTextImage(textLayout, kATSUFromTextBeginning,
                                 kATSUToTextEnd, Long2Fix(0), Long2Fix(0),
                                 &boundingBox);
    if (err != noErr) {
        NS_WARNING("ATSUMeasureTextImage failed");
        ::ATSUDisposeStyle(style);
        ::EndCGContextForApplicationDockTile(context);

        return NS_ERROR_FAILURE;
    }

    // Center text inside circle
    err = ::ATSUDrawText(textLayout, kATSUFromTextBeginning, kATSUToTextEnd,
                         Long2Fix(90 - (boundingBox.right - boundingBox.left) / 2),
                         Long2Fix(95 - (boundingBox.bottom - boundingBox.top) / 2));

    ::ATSUDisposeStyle(style);
    ::ATSUDisposeTextLayout(textLayout);

    ::CGContextFlush(context);

    ::EndCGContextForApplicationDockTile(context);
    mBiffIconVisible = PR_TRUE;
  }

  mSuppressBiffIcon = PR_FALSE;
  mAlertInProgress = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemPropertyFlagChanged(nsISupports *item, nsIAtom *property, PRUint32 oldFlag, PRUint32 newFlag)
{
  nsresult rv = NS_OK;
   
  // if we got new mail bounce the Dock icon and/or apply badge to Dock icon
	if (mBiffStateAtom == property && mFoldersWithNewMail)
	{
    nsCOMPtr<nsIMsgFolder> folder = do_QueryInterface(item);
    NS_ENSURE_TRUE(folder, NS_OK);

		if (newFlag == nsIMsgFolder::nsMsgBiffState_NewMail) 
    {
      // if the icon is not already visible, only show a system tray icon iff 
      // we are performing biff (as opposed to the user getting new mail)
      if (!mBiffIconVisible)
      {
        PRBool performingBiff = PR_FALSE;
        nsCOMPtr<nsIMsgIncomingServer> server;
        folder->GetServer(getter_AddRefs(server));
        if (server)
          server->GetPerformingBiff(&performingBiff);
        if (!performingBiff) 
          return NS_OK; // kick out right now...
      }
      nsCOMPtr<nsIWeakReference> weakFolder = do_GetWeakReference(folder); 

      // remove the element if it is already in the array....
      PRUint32 count = 0;
      PRUint32 index = 0; 
      mFoldersWithNewMail->Count(&count);
      nsCOMPtr<nsISupports> supports;
      nsCOMPtr<nsIMsgFolder> oldFolder;
      nsCOMPtr<nsIWeakReference> weakReference;
      for (index = 0; index < count; index++)
      {
        supports = getter_AddRefs(mFoldersWithNewMail->ElementAt(index));
        weakReference = do_QueryInterface(supports);
        oldFolder = do_QueryReferent(weakReference);
        if (oldFolder == folder) // if they point to the same folder
          break;
        oldFolder = nsnull;
      }

      if (oldFolder)
        mFoldersWithNewMail->ReplaceElementAt(weakFolder, index);
      else
        mFoldersWithNewMail->AppendElement(weakFolder);
      // now regenerate the tooltip
      OnAlertFinished(nsnull);  
    }
    else if (newFlag == nsIMsgFolder::nsMsgBiffState_NoMail)
    {
      // we are always going to remove the icon whenever we get our first no mail
      // notification. 

      mFoldersWithNewMail->Clear(); 
      if (mBiffIconVisible) 
      {
        RestoreApplicationDockTileImage();
        mBiffIconVisible = PR_FALSE;
      }
    }
  } // if the biff property changed
  
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemAdded(nsISupports *, nsISupports *, const char *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemBoolPropertyChanged(nsISupports *aItem,
                                                         nsIAtom *aProperty,
                                                         PRBool aOldValue,
                                                         PRBool aNewValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemEvent(nsIMsgFolder *, nsIAtom *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemIntPropertyChanged(nsISupports *aItem, nsIAtom *aProperty, PRInt32 aOldValue, PRInt32 aNewValue)
{
  nsresult rv;
  return NS_OK;
}
