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

#ifndef nsIPrivateDOMEvent_h__
#define nsIPrivateDOMEvent_h__

#include "nsEvent.h"
#include "nsISupports.h"

class nsIPresContext;

/*
 * Event listener manager interface.
 */
#define NS_IPRIVATEDOMEVENT_IID \
{ /* 80a98c80-2036-11d2-bd89-00805f8ae3f4 */ \
0x80a98c80, 0x2036, 0x11d2, \
{0xbd, 0x89, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf4} }

class nsIDOMEventTarget;
class nsIDOMEvent;

class nsIPrivateDOMEvent : public nsISupports {

public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IPRIVATEDOMEVENT_IID)

  NS_IMETHOD DuplicatePrivateData() = 0;
  NS_IMETHOD SetTarget(nsIDOMEventTarget* aTarget) = 0;
  NS_IMETHOD SetCurrentTarget(nsIDOMEventTarget* aTarget) = 0;
  NS_IMETHOD SetOriginalTarget(nsIDOMEventTarget* aTarget) = 0;
  NS_IMETHOD IsDispatchStopped(PRBool* aIsDispatchPrevented) = 0;
  NS_IMETHOD GetInternalNSEvent(nsEvent** aNSEvent) = 0;
  NS_IMETHOD HasOriginalTarget(PRBool* aResult)=0;
  NS_IMETHOD IsTrustedEvent(PRBool* aResult)=0;
  NS_IMETHOD SetTrusted(PRBool aTrusted)=0;
};

nsresult
NS_NewDOMEvent(nsIDOMEvent** aInstancePtrResult, nsIPresContext* aPresContext,
               nsEvent *aEvent);
nsresult
NS_NewDOMUIEvent(nsIDOMEvent** aInstancePtrResult,
                 nsIPresContext* aPresContext, const nsAString& aEventType,
                 nsEvent *aEvent);
nsresult
NS_NewDOMMutationEvent(nsIDOMEvent** aResult, nsIPresContext* aPresContext, 
                       nsEvent* aEvent);

#endif // nsIPrivateDOMEvent_h__
