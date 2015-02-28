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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *   Dean Tessman <dean_tessman@hotmail.com>
 *   Mark Hammond <markh@ActiveState.com>
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

#include "nsMenuListener.h"
#include "nsMenuBarFrame.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsGUIEvent.h"

// Drag & Drop, Clipboard
#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsCOMPtr.h"
#include "nsIDOMKeyEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsXULAtoms.h"

#include "nsIEventStateManager.h"

#include "nsIViewManager.h"
#include "nsIView.h"
#include "nsISupportsArray.h"

/*
 * nsMenuListener implementation
 */

NS_IMPL_ADDREF(nsMenuListener)
NS_IMPL_RELEASE(nsMenuListener)
NS_IMPL_QUERY_INTERFACE3(nsMenuListener, nsIDOMKeyListener, nsIDOMFocusListener, nsIDOMMouseListener)


////////////////////////////////////////////////////////////////////////

nsMenuListener::nsMenuListener(nsIMenuParent* aMenuParent) 
{
  mMenuParent = aMenuParent;
}

////////////////////////////////////////////////////////////////////////
nsMenuListener::~nsMenuListener() 
{
}



////////////////////////////////////////////////////////////////////////
nsresult
nsMenuListener::KeyUp(nsIDOMEvent* aKeyEvent)
{
  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aKeyEvent));

  if (nsevent) {
    nsevent->PreventBubble();
    nsevent->PreventCapture();
  }

  aKeyEvent->PreventDefault();

  return NS_ERROR_BASE; // I am consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult
nsMenuListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  PRInt32 menuAccessKey = -1;
  
  // If the key just pressed is the access key (usually Alt),
  // dismiss and unfocus the menu.

  nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);
  if (menuAccessKey) {
    PRUint32 theChar;
    nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
    keyEvent->GetKeyCode(&theChar);

    if (theChar == (PRUint32)menuAccessKey) {
      // No other modifiers can be down.
      // Especially CTRL.  CTRL+ALT == AltGR, and
      // we'll fuck up on non-US enhanced 102-key
      // keyboards if we don't check this.
      PRBool ctrl = PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_CONTROL)
        keyEvent->GetCtrlKey(&ctrl);
      PRBool alt=PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_ALT)
        keyEvent->GetAltKey(&alt);
      PRBool shift=PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_SHIFT)
        keyEvent->GetShiftKey(&shift);
      PRBool meta=PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_META)
        keyEvent->GetMetaKey(&meta);
      if (!(ctrl || alt || shift || meta)) {
        // The access key just went down and no other
        // modifiers are already down.
        mMenuParent->DismissChain();
      }
    }
  }

  // Since a menu was open, eat the event to keep other event
  // listeners from becoming confused.

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aKeyEvent));

  if (nsevent) {
    nsevent->PreventBubble();
    nsevent->PreventCapture();
  }

  aKeyEvent->PreventDefault();

  return NS_ERROR_BASE; // I am consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult
nsMenuListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  // if event has already been handled, bail
  nsCOMPtr<nsIDOMNSUIEvent> uiEvent ( do_QueryInterface(aKeyEvent) );
  if ( uiEvent ) {
    PRBool eventHandled = PR_FALSE;
    uiEvent->GetPreventDefault ( &eventHandled );
    if ( eventHandled )
      return NS_OK;       // don't consume event
  }
  
  //handlers shouldn't be triggered by non-trusted events.
  if (aKeyEvent) {
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(aKeyEvent);
    if (privateEvent) {
      PRBool trustedEvent;
      privateEvent->IsTrustedEvent(&trustedEvent);
      if (!trustedEvent)
        return NS_OK;
    }
  }

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  PRUint32 theChar;
	keyEvent->GetKeyCode(&theChar);
  PRBool handled = PR_FALSE;

  if (theChar == NS_VK_LEFT ||
      theChar == NS_VK_RIGHT ||
      theChar == NS_VK_UP ||
      theChar == NS_VK_DOWN ||
      theChar == NS_VK_HOME ||
      theChar == NS_VK_END) {
    // The navigation keys were pressed. User is moving around within
    // the menus.
	  mMenuParent->KeyboardNavigation(theChar, handled);
  }
  else if (theChar == NS_VK_ESCAPE) {
    // Close one level.
	  mMenuParent->Escape(handled);
    if (!handled)
      mMenuParent->DismissChain();
  }
  else if (theChar == NS_VK_TAB)
    mMenuParent->DismissChain();
  else if (theChar == NS_VK_ENTER ||
           theChar == NS_VK_RETURN) {
    // Open one level.
    mMenuParent->Enter();
  }
#if !defined(XP_MAC) && !defined(XP_MACOSX)
  else if (theChar == NS_VK_F10) {
    // doesn't matter what modifier keys are down in Non-Mac platform
    // if the menu bar is active and F10 is pressed - deactivate it
    mMenuParent->DismissChain();
  }
#endif // !XP_MAC && !XP_MACOSX
  else {
    PRInt32 menuAccessKey = -1;
    nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);
    if (menuAccessKey) {
      // Do shortcut navigation.
      // A letter was pressed. We want to see if a shortcut gets matched. If
      // so, we'll know the menu got activated.
      keyEvent->GetCharCode(&theChar);
      mMenuParent->ShortcutNavigation(keyEvent, handled);
    }
  }

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aKeyEvent));

  if (nsevent) {
    nsevent->PreventBubble();
    nsevent->PreventCapture();
  }

  aKeyEvent->PreventDefault();

  return NS_ERROR_BASE; // I am consuming event
}

////////////////////////////////////////////////////////////////////////

nsresult
nsMenuListener::Focus(nsIDOMEvent* aEvent)
{
  return NS_OK; // means I am NOT consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult
nsMenuListener::Blur(nsIDOMEvent* aEvent)
{
  
  return NS_OK; // means I am NOT consuming event
}
  
////////////////////////////////////////////////////////////////////////
nsresult 
nsMenuListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult 
nsMenuListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}

nsresult 
nsMenuListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult 
nsMenuListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult 
nsMenuListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult 
nsMenuListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult
nsMenuListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}


