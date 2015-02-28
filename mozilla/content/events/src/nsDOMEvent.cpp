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
 * Steve Clark (buster@netscape.com)
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

#include "nsCOMPtr.h"
#include "nsDOMEvent.h"
#include "nsIDOMNode.h"
#include "nsEventStateManager.h"
#include "nsIFrame.h"
#include "nsIContent.h"
#include "nsIRenderingContext.h"
#include "nsIWidget.h"
#include "nsIPresShell.h"
#include "nsPrivateTextRange.h"
#include "nsIDocument.h"
#include "nsIViewManager.h"
#include "nsIPrivateCompositionEvent.h"
#include "nsIScrollableView.h"
#include "nsIDOMEventTarget.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMAbstractView.h"
#include "prmem.h"
#include "nsLayoutAtoms.h"
#include "nsMutationEvent.h"
#include "nsContentUtils.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMutationEvent.h"
#include "nsIURI.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

static const char* const sEventNames[] = {
  "mousedown", "mouseup", "click", "dblclick", "mouseover",
  "mouseout", "mousemove", "contextmenu", "keydown", "keyup", "keypress",
  "focus", "blur", "load", "beforeunload", "unload", "abort", "error",
  "submit", "reset", "change", "select", "input", "paint" ,"text",
  "popupshowing", "popupshown", "popuphiding", "popuphidden", "close", "command", "broadcast", "commandupdate",
  "dragenter", "dragover", "dragexit", "dragdrop", "draggesture", "resize",
  "scroll","overflow", "underflow", "overflowchanged",
  "DOMSubtreeModified", "DOMNodeInserted", "DOMNodeRemoved", 
  "DOMNodeRemovedFromDocument", "DOMNodeInsertedIntoDocument",
  "DOMAttrModified", "DOMCharacterDataModified",
  "popupBlocked"
}; 

/** event pool used as a simple recycler for objects of this class */
static PRUint8 gEventPool[ sizeof(nsDOMEvent) ];

/* declare static class data */
PRBool nsDOMEvent::gEventPoolInUse=PR_FALSE;

#ifdef NS_DEBUG   // metrics for measuring event pool use
static PRInt32 numEvents=0;
static PRInt32 numNewEvents=0;
static PRInt32 numDelEvents=0;
static PRInt32 numAllocFromPool=0;
//#define NOISY_EVENT_LEAKS   // define NOISY_EVENT_LEAKS to get metrics printed to stdout for all nsDOMEvent allocations
#endif

static char *sPopupAllowedEvents;


// allocate the memory for the object from the recycler, if possible
// otherwise, just grab it from the heap.
void* 
nsDOMEvent::operator new(size_t aSize) CPP_THROW_NEW
{

#ifdef NS_DEBUG
  numEvents++;
#endif

  void *result = nsnull;

  if (!gEventPoolInUse && aSize <= sizeof(gEventPool)) {
#ifdef NS_DEBUG
    numAllocFromPool++;
#endif
    result = &gEventPool;
    gEventPoolInUse = PR_TRUE;
  }
  else {
#ifdef NS_DEBUG
    numNewEvents++;
#endif
    result = ::operator new(aSize);
  }
  
  if (result) {
    memset(result, 0, aSize);
  }

  return result;
}

// Overridden to prevent the global delete from being called on objects from
// the recycler.  Otherwise, just pass through to the global delete operator.
void 
nsDOMEvent::operator delete(void* aPtr)
{
  if (aPtr==&gEventPool) {
    gEventPoolInUse = PR_FALSE;
  }
  else {
#ifdef NS_DEBUG
    numDelEvents++;
#endif
    ::operator delete(aPtr);
  }
#if defined(NS_DEBUG) && defined(NOISY_EVENT_LEAKS)
  printf("total events =%d, from pool = %d, concurrent live events = %d\n", 
          numEvents, numAllocFromPool, numNewEvents-numDelEvents);
#endif
}



nsDOMEvent::nsDOMEvent(nsIPresContext* aPresContext, nsEvent* aEvent,
                       const nsAString& aEventType) 
{

  mPresContext = aPresContext;

  if (aEvent) {
    mEvent = aEvent;
  }
  else {
    mEventIsInternal = PR_TRUE;
    AllocateEvent(aEventType);
  }

  // Get the explicit original target (if it's anonymous make it null)
  {
    mExplicitOriginalTarget = GetTargetFromFrame();
    mTmpRealOriginalTarget = mExplicitOriginalTarget;
    nsCOMPtr<nsIContent> content = do_QueryInterface(mExplicitOriginalTarget);
    if (content) {
      if (content->IsNativeAnonymous()) {
        mExplicitOriginalTarget = nsnull;
      }
      if (content->GetBindingParent()) {
        mExplicitOriginalTarget = nsnull;
      }
    }
  }
  mText = nsnull;
  mButton = -1;
  if (aEvent) {
    mScreenPoint.x = aEvent->refPoint.x;
    mScreenPoint.y = aEvent->refPoint.y;
    mClientPoint.x = aEvent->point.x;
    mClientPoint.y = aEvent->point.y;
  } else
    mScreenPoint.x = mScreenPoint.y = mClientPoint.x = mClientPoint.y = 0;

  if (aEvent && aEvent->eventStructType == NS_TEXT_EVENT) {
	  //
	  // extract the IME composition string
	  //

    nsTextEvent *te = (nsTextEvent*)aEvent;

	  mText = new nsString(te->theText);

	  //
	  // build the range list -- ranges need to be DOM-ified since the
	  // IME transaction will hold a ref, the widget representation
	  // isn't persistent
	  //
	  nsIPrivateTextRange** tempTextRangeList =
      new nsIPrivateTextRange*[te->rangeCount];

	  if (tempTextRangeList) {
      PRUint16 i;

      for(i = 0; i < te->rangeCount; i++) {
        nsPrivateTextRange* tempPrivateTextRange = new
          nsPrivateTextRange(te->rangeArray[i].mStartOffset,
                             te->rangeArray[i].mEndOffset,
                             te->rangeArray[i].mRangeType);

        if (tempPrivateTextRange) {
          NS_ADDREF(tempPrivateTextRange);

          tempTextRangeList[i] = (nsIPrivateTextRange*)tempPrivateTextRange;
        }
      }
	  }

	  // We need to create mTextRange even rangeCount is 0. 
	  // If rangeCount is 0, mac carbon will return 0 for new and
	  // tempTextRangeList will be null. but we should still create
	  // mTextRange, otherwise, we will crash it later when some code
	  // call GetInputRange and AddRef to the result

    mTextRange = new nsPrivateTextRangeList(te->rangeCount ,tempTextRangeList);
  }
}

nsDOMEvent::~nsDOMEvent() 
{
  NS_ASSERT_OWNINGTHREAD(nsDOMEvent);

  if (mEventIsInternal) {
    if (mEvent->userType) {
      delete mEvent->userType;
    }
    if (mEvent->eventStructType == NS_POPUPBLOCKED_EVENT) {
      nsPopupBlockedEvent* event = NS_STATIC_CAST(nsPopupBlockedEvent*, mEvent);
      NS_IF_RELEASE(event->mRequestingWindowURI);
      NS_IF_RELEASE(event->mPopupWindowURI);
    }
    delete mEvent;
  }

  delete mText;
}

NS_IMPL_ADDREF(nsDOMEvent)
NS_IMPL_RELEASE(nsDOMEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEvent, nsIDOMMouseEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMUIEvent, nsIDOMMouseEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSUIEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPopupBlockedEvent)
  NS_INTERFACE_MAP_ENTRY(nsIPrivateDOMEvent)
  NS_INTERFACE_MAP_ENTRY(nsIPrivateTextEvent)
  NS_INTERFACE_MAP_ENTRY(nsIPrivateCompositionEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMouseEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(Event)
NS_INTERFACE_MAP_END

// nsIDOMEventInterface
NS_METHOD nsDOMEvent::GetType(nsAString& aType)
{
  const char* name = GetEventName(mEvent->message);

  if (name) {
    CopyASCIItoUTF16(name, aType);
    return NS_OK;
  }
  else {
    if (mEvent->message == NS_USER_DEFINED_EVENT && mEvent->userType) {
      aType.Assign(NS_STATIC_CAST(nsStringKey*, mEvent->userType)->GetString());
      return NS_OK;
    }
  }
  
  return NS_ERROR_FAILURE;
}

NS_METHOD nsDOMEvent::GetTarget(nsIDOMEventTarget** aTarget)
{
  if (nsnull != mTarget) {
    *aTarget = mTarget;
    NS_ADDREF(*aTarget);
    return NS_OK;
  }
  
	*aTarget = nsnull;

  nsCOMPtr<nsIContent> targetContent;  

  if (mPresContext) {
    mPresContext->EventStateManager()->
      GetEventTargetContent(mEvent, getter_AddRefs(targetContent));
  }
  
  if (targetContent) {
    mTarget = do_QueryInterface(targetContent);
    if (mTarget) {
      *aTarget = mTarget;
      NS_ADDREF(*aTarget);
    }
  }
  else {
    //Always want a target.  Use document if nothing else.
    nsIPresShell *presShell;
    if (mPresContext && (presShell = mPresContext->GetPresShell())) {
      nsCOMPtr<nsIDocument> doc;
      if (NS_SUCCEEDED(presShell->GetDocument(getter_AddRefs(doc))) && doc) {
        mTarget = do_QueryInterface(doc);
        if (mTarget) {
          *aTarget = mTarget;
          NS_ADDREF(*aTarget);
        }      
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::GetCurrentTarget(nsIDOMEventTarget** aCurrentTarget)
{
  *aCurrentTarget = mCurrentTarget;
  NS_IF_ADDREF(*aCurrentTarget);
  return NS_OK;
}

//
// Get the actual event target node (may have been retargeted for mouse events)
//
already_AddRefed<nsIDOMEventTarget>
nsDOMEvent::GetTargetFromFrame()
{
  if (!mPresContext) { return nsnull; }

  // Get the target frame (have to get the ESM first)
  nsIFrame* targetFrame = nsnull;
  mPresContext->EventStateManager()->GetEventTarget(&targetFrame);
  if (!targetFrame) { return nsnull; }

  // get the real content
  nsCOMPtr<nsIContent> realEventContent;
  targetFrame->GetContentForEvent(mPresContext, mEvent, getter_AddRefs(realEventContent));
  if (!realEventContent) { return nsnull; }

  // Finally, we have the real content.  QI it and return.
  nsIDOMEventTarget* target = nsnull;
  CallQueryInterface(realEventContent, &target);
  return target;
}

NS_IMETHODIMP
nsDOMEvent::GetExplicitOriginalTarget(nsIDOMEventTarget** aRealEventTarget)
{
  if (mExplicitOriginalTarget) {
    *aRealEventTarget = mExplicitOriginalTarget;
    NS_ADDREF(*aRealEventTarget);
    return NS_OK;
  }

  return GetTarget(aRealEventTarget);
}

NS_IMETHODIMP
nsDOMEvent::GetTmpRealOriginalTarget(nsIDOMEventTarget** aRealEventTarget)
{
  if (mTmpRealOriginalTarget) {
    *aRealEventTarget = mTmpRealOriginalTarget;
    NS_ADDREF(*aRealEventTarget);
    return NS_OK;
  }

  return GetOriginalTarget(aRealEventTarget);
}

NS_IMETHODIMP
nsDOMEvent::GetOriginalTarget(nsIDOMEventTarget** aOriginalTarget)
{
  if (!mOriginalTarget)
    return GetTarget(aOriginalTarget);

  *aOriginalTarget = mOriginalTarget;
  NS_IF_ADDREF(*aOriginalTarget);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::HasOriginalTarget(PRBool* aResult)
{
  *aResult = (mOriginalTarget != nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::IsTrustedEvent(PRBool* aResult)
{
  return GetIsTrusted(aResult);
}

NS_IMETHODIMP
nsDOMEvent::SetTrusted(PRBool aTrusted)
{
  if (aTrusted) {
    mEvent->internalAppFlags |= NS_APP_EVENT_FLAG_TRUSTED;
  } else {
    mEvent->internalAppFlags &= ~NS_APP_EVENT_FLAG_TRUSTED;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::GetEventPhase(PRUint16* aEventPhase)
{
  if (mEvent->flags & NS_EVENT_FLAG_INIT) {
    *aEventPhase = nsIDOMMouseEvent::AT_TARGET;
  }
  else if (mEvent->flags & NS_EVENT_FLAG_CAPTURE) {
    *aEventPhase = nsIDOMMouseEvent::CAPTURING_PHASE;
  }
  else if (mEvent->flags & NS_EVENT_FLAG_BUBBLE) {
    *aEventPhase = nsIDOMMouseEvent::BUBBLING_PHASE;
  }
  else {
    *aEventPhase = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::GetBubbles(PRBool* aBubbles)
{
  *aBubbles = !(mEvent->flags & NS_EVENT_FLAG_CANT_BUBBLE);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::GetCancelable(PRBool* aCancelable)
{
  *aCancelable = !(mEvent->flags & NS_EVENT_FLAG_CANT_CANCEL);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::GetTimeStamp(PRUint64* aTimeStamp)
{
  LL_UI2L(*aTimeStamp, mEvent->time);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::StopPropagation()
{
  mEvent->flags |= NS_EVENT_FLAG_STOP_DISPATCH;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::PreventBubble()
{
  if (mEvent->flags & NS_EVENT_FLAG_BUBBLE || mEvent->flags & NS_EVENT_FLAG_INIT) {
    mEvent->flags |= NS_EVENT_FLAG_STOP_DISPATCH;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::PreventCapture()
{
  if (mEvent->flags & NS_EVENT_FLAG_CAPTURE) {
    mEvent->flags |= NS_EVENT_FLAG_STOP_DISPATCH;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::GetIsTrusted(PRBool *aIsTrusted)
{
  *aIsTrusted = mEvent->internalAppFlags & NS_APP_EVENT_FLAG_TRUSTED;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::PreventDefault()
{
  if (!(mEvent->flags & NS_EVENT_FLAG_CANT_CANCEL)) {
    mEvent->flags |= NS_EVENT_FLAG_NO_DEFAULT;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsDOMEvent::GetView(nsIDOMAbstractView** aView)
{
  NS_ENSURE_ARG_POINTER(aView);
  *aView = nsnull;
  nsresult rv = NS_OK;

  if (mPresContext) {
    nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
    NS_ENSURE_TRUE(container, NS_OK);
    
    nsCOMPtr<nsIDOMWindowInternal> window = do_GetInterface(container);
    NS_ENSURE_TRUE(window, NS_OK);

    CallQueryInterface(window, aView);
  }

  return rv;
}

NS_IMETHODIMP
nsDOMEvent::GetDetail(PRInt32* aDetail)
{
  //detail is valid for more than just mouseevents but we don't
  //use it for anything else right now

  if (!mEvent) {
    *aDetail = 0;
    return NS_OK;
  }

  switch(mEvent->eventStructType)
  {
    case NS_SCROLLPORT_EVENT:
    {
      nsScrollPortEvent* scrollEvent = (nsScrollPortEvent*)mEvent;
      *aDetail = (PRInt32)scrollEvent->orient;
      return NS_OK;
    }

    case NS_MOUSE_EVENT:
    {
      switch (mEvent->message) {
      case NS_MOUSE_LEFT_BUTTON_UP:
      case NS_MOUSE_LEFT_BUTTON_DOWN:
      case NS_MOUSE_LEFT_CLICK:
      case NS_MOUSE_LEFT_DOUBLECLICK:
      case NS_MOUSE_MIDDLE_BUTTON_UP:
      case NS_MOUSE_MIDDLE_BUTTON_DOWN:
      case NS_MOUSE_MIDDLE_CLICK:
      case NS_MOUSE_MIDDLE_DOUBLECLICK:
      case NS_MOUSE_RIGHT_BUTTON_UP:
      case NS_MOUSE_RIGHT_BUTTON_DOWN:
      case NS_MOUSE_RIGHT_CLICK:
      case NS_MOUSE_RIGHT_DOUBLECLICK:
      case NS_USER_DEFINED_EVENT:
        *aDetail = ((nsMouseEvent*)mEvent)->clickCount;
        break;
      default:
        break;
      }
      return NS_OK;
    }

    case NS_MOUSE_SCROLL_EVENT:
    {
      *aDetail = ((nsMouseScrollEvent*)mEvent)->delta;
      break;
    }

    default:
      *aDetail = 0;
      return NS_OK;
  }

  return NS_OK;
}

NS_METHOD nsDOMEvent::GetText(nsString& aText)
{
  if (mEvent->message == NS_TEXT_TEXT) {
    aText = *mText;
    return NS_OK;
  }
  aText.Truncate();
  return NS_ERROR_FAILURE;
}

NS_METHOD nsDOMEvent::GetInputRange(nsIPrivateTextRangeList** aInputRange)
{
  NS_ENSURE_ARG_POINTER(aInputRange);
  if (mEvent->message == NS_TEXT_TEXT) {
    *aInputRange = mTextRange;
    return NS_OK;
  }
  *aInputRange = 0;
  return NS_ERROR_FAILURE;
}

NS_METHOD nsDOMEvent::GetEventReply(nsTextEventReply** aReply)
{
  NS_ENSURE_ARG_POINTER(aReply);
  if (mEvent->message==NS_TEXT_TEXT) {
    *aReply = &(((nsTextEvent*)mEvent)->theReply);
    return NS_OK;
  }
  *aReply = 0;
  return NS_ERROR_FAILURE;
}

NS_METHOD nsDOMEvent::GetCompositionReply(nsTextEventReply** aReply)
{
  NS_ENSURE_ARG_POINTER(aReply);
  if((mEvent->message==NS_COMPOSITION_START) ||
     (mEvent->message==NS_COMPOSITION_QUERY)) {
    *aReply = &(((nsCompositionEvent*)mEvent)->theReply);
    return NS_OK;
  }
  *aReply = 0;
  return NS_ERROR_FAILURE;
}

NS_METHOD
nsDOMEvent::GetReconversionReply(nsReconversionEventReply** aReply)
{
  NS_ENSURE_ARG_POINTER(aReply);
  *aReply = &(((nsReconversionEvent*)mEvent)->theReply);
  return NS_OK;
}


NS_METHOD nsDOMEvent::GetScreenX(PRInt32* aScreenX)
{
  NS_ENSURE_ARG_POINTER(aScreenX);
  if (!mEvent || 
       (mEvent->eventStructType != NS_MOUSE_EVENT &&
        mEvent->eventStructType != NS_POPUP_EVENT &&
        !NS_IS_DRAG_EVENT(mEvent))) {
    *aScreenX = 0;
    return NS_OK;
  }

  if (!((nsGUIEvent*)mEvent)->widget ) {
    *aScreenX = mScreenPoint.x;
    return NS_OK;
  }
    
  nsRect bounds, offset;
  bounds.x = mEvent->refPoint.x;
  
  ((nsGUIEvent*)mEvent)->widget->WidgetToScreen ( bounds, offset );
  *aScreenX = offset.x;
  
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetScreenY(PRInt32* aScreenY)
{
  NS_ENSURE_ARG_POINTER(aScreenY);
  if (!mEvent || 
       (mEvent->eventStructType != NS_MOUSE_EVENT &&
        mEvent->eventStructType != NS_POPUP_EVENT &&
        !NS_IS_DRAG_EVENT(mEvent))) {
    *aScreenY = 0;
    return NS_OK;
  }

  if (!((nsGUIEvent*)mEvent)->widget ) {
    *aScreenY = mScreenPoint.y;    
    return NS_OK;
  }

  nsRect bounds, offset;
  bounds.y = mEvent->refPoint.y;
  
  ((nsGUIEvent*)mEvent)->widget->WidgetToScreen ( bounds, offset );
  *aScreenY = offset.y;
  
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetClientX(PRInt32* aClientX)
{
  NS_ENSURE_ARG_POINTER(aClientX);
  if (!mEvent || 
       (mEvent->eventStructType != NS_MOUSE_EVENT &&
        mEvent->eventStructType != NS_POPUP_EVENT &&
        !NS_IS_DRAG_EVENT(mEvent)) ||
      !mPresContext) {
    *aClientX = 0;
    return NS_OK;
  }

  if (!((nsGUIEvent*)mEvent)->widget ) {
    *aClientX = mClientPoint.x;
    return NS_OK;
  }

  //My god, man, there *must* be a better way to do this.
  nsIWidget* rootWidget = nsnull;
  nsIPresShell *presShell = mPresContext->GetPresShell();
  if (presShell) {
    nsIViewManager* vm = presShell->GetViewManager();
    if (vm) {
      vm->GetWidget(&rootWidget);
    }
  }


  nsRect bounds, offset;
  offset.x = 0;

  nsIWidget* parent = ((nsGUIEvent*)mEvent)->widget;
  //Add extra ref since loop will free one.
  NS_IF_ADDREF(parent);
  nsIWidget* tmp;
  while (rootWidget != parent && nsnull != parent) {
    nsWindowType windowType;
    parent->GetWindowType(windowType);
    if (windowType == eWindowType_popup)
      break;

    parent->GetBounds(bounds);
    offset.x += bounds.x;
    tmp = parent;
    parent = tmp->GetParent();
    NS_RELEASE(tmp);
  }
  NS_IF_RELEASE(parent);
  NS_IF_RELEASE(rootWidget);
  
  *aClientX = mEvent->refPoint.x + offset.x;
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetClientY(PRInt32* aClientY)
{
  NS_ENSURE_ARG_POINTER(aClientY);
  if (!mEvent || 
       (mEvent->eventStructType != NS_MOUSE_EVENT &&
        mEvent->eventStructType != NS_POPUP_EVENT &&
        !NS_IS_DRAG_EVENT(mEvent)) ||
      !mPresContext) {
    *aClientY = 0;
    return NS_OK;
  }

  if (!((nsGUIEvent*)mEvent)->widget ) {
    *aClientY = mClientPoint.y;
    return NS_OK;
  }

  //My god, man, there *must* be a better way to do this.
  nsIWidget* rootWidget = nsnull;
  nsIPresShell *presShell = mPresContext->GetPresShell();
  if (presShell) {
    nsIViewManager* vm = presShell->GetViewManager();
		if (vm) {
      vm->GetWidget(&rootWidget);
    }
  }


  nsRect bounds, offset;
  offset.y = 0;

  nsIWidget* parent = ((nsGUIEvent*)mEvent)->widget;
  //Add extra ref since loop will free one.
  NS_IF_ADDREF(parent);
  nsIWidget* tmp;
  while (rootWidget != parent && nsnull != parent) {
    nsWindowType windowType;
    parent->GetWindowType(windowType);
    if (windowType == eWindowType_popup)
      break;

    parent->GetBounds(bounds);
    offset.y += bounds.y;
    tmp = parent;
    parent = tmp->GetParent();
    NS_RELEASE(tmp);
  }
  NS_IF_RELEASE(parent);
  NS_IF_RELEASE(rootWidget);
  
  *aClientY = mEvent->refPoint.y + offset.y;
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetAltKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isAlt;
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetCtrlKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isControl;
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetShiftKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isShift;
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetMetaKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isMeta;
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetCharCode(PRUint32* aCharCode)
{
  NS_ENSURE_ARG_POINTER(aCharCode);
  if (!mEvent || mEvent->eventStructType != NS_KEY_EVENT) {
    *aCharCode = 0;
    return NS_OK;
  }

  switch (mEvent->message) {
  case NS_KEY_UP:
  case NS_KEY_DOWN:
#if defined(NS_DEBUG) && defined(DEBUG_brade)
    printf("GetCharCode used for wrong key event; should use onkeypress.\n");
#endif
    *aCharCode = 0;
    break;
  case NS_KEY_PRESS:
    *aCharCode = ((nsKeyEvent*)mEvent)->charCode;
    break;
  default:
    break;
  }
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetKeyCode(PRUint32* aKeyCode)
{
  NS_ENSURE_ARG_POINTER(aKeyCode);
  if (!mEvent || mEvent->eventStructType != NS_KEY_EVENT) {
    *aKeyCode = 0;
    return NS_OK;
  }

  switch (mEvent->message) {
  case NS_KEY_UP:
  case NS_KEY_PRESS:
  case NS_KEY_DOWN:
    *aKeyCode = ((nsKeyEvent*)mEvent)->keyCode;
    break;
  default:
    break;
  }
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetButton(PRUint16* aButton)
{
  NS_ENSURE_ARG_POINTER(aButton);
  if (!mEvent || mEvent->eventStructType != NS_MOUSE_EVENT) {
    NS_WARNING("Tried to get mouse button for null or non-mouse event!");
    *aButton = (PRUint16)-1;
    return NS_OK;
  }

  // If button has been set then use that instead.
  if (mButton > 0) {
    *aButton = (PRUint16)mButton;
  }
  else {
    switch (mEvent->message) {
    case NS_MOUSE_LEFT_BUTTON_UP:
    case NS_MOUSE_LEFT_BUTTON_DOWN:
    case NS_MOUSE_LEFT_CLICK:
    case NS_MOUSE_LEFT_DOUBLECLICK:
      *aButton = 0;
      break;
    case NS_MOUSE_MIDDLE_BUTTON_UP:
    case NS_MOUSE_MIDDLE_BUTTON_DOWN:
    case NS_MOUSE_MIDDLE_CLICK:
    case NS_MOUSE_MIDDLE_DOUBLECLICK:
      *aButton = 1;
      break;
    case NS_MOUSE_RIGHT_BUTTON_UP:
    case NS_MOUSE_RIGHT_BUTTON_DOWN:
    case NS_MOUSE_RIGHT_CLICK:
    case NS_MOUSE_RIGHT_DOUBLECLICK:
      *aButton = 2;
      break;
    default:
      break;
    }
  }
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetRelatedTarget(nsIDOMEventTarget** aRelatedTarget)
{
  NS_ENSURE_ARG_POINTER(aRelatedTarget);
  *aRelatedTarget = nsnull;

  if (!mPresContext) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> relatedContent;
  mPresContext->EventStateManager()->
    GetEventRelatedContent(getter_AddRefs(relatedContent));
  if (!relatedContent) {
    return NS_OK;
  }

  return CallQueryInterface(relatedContent, aRelatedTarget);
}

// nsINSEventInterface
NS_METHOD nsDOMEvent::GetLayerX(PRInt32* aLayerX)
{
  NS_ENSURE_ARG_POINTER(aLayerX);
  if (!mEvent || (mEvent->eventStructType != NS_MOUSE_EVENT) ||
      !mPresContext) {
    *aLayerX = 0;
    return NS_OK;
  }

  float t2p;
  t2p = mPresContext->TwipsToPixels();
  *aLayerX = NSTwipsToIntPixels(mEvent->point.x, t2p);
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetLayerY(PRInt32* aLayerY)
{
  NS_ENSURE_ARG_POINTER(aLayerY);
  if (!mEvent || (mEvent->eventStructType != NS_MOUSE_EVENT) ||
      !mPresContext) {
    *aLayerY = 0;
    return NS_OK;
  }

  float t2p;
  t2p = mPresContext->TwipsToPixels();
  *aLayerY = NSTwipsToIntPixels(mEvent->point.y, t2p);
  return NS_OK;
}

nsresult nsDOMEvent::GetScrollInfo(nsIScrollableView** aScrollableView,
   float* aP2T, float* aT2P)
{
  NS_ENSURE_ARG_POINTER(aScrollableView);
  NS_ENSURE_ARG_POINTER(aP2T);
  NS_ENSURE_ARG_POINTER(aT2P);
  if (!mPresContext) {
    *aScrollableView = nsnull;
    return NS_ERROR_FAILURE;
  }

  *aP2T = mPresContext->PixelsToTwips();
  *aT2P = mPresContext->TwipsToPixels();

  nsIPresShell *presShell = mPresContext->GetPresShell();
  if (presShell) {
    nsIViewManager* vm = presShell->GetViewManager();
    if(vm) {
      return vm->GetRootScrollableView(aScrollableView);
    }
  }
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetPageX(PRInt32* aPageX)
{
  NS_ENSURE_ARG_POINTER(aPageX);
  nsresult ret = NS_OK;
  PRInt32 scrollX = 0;
  nsIScrollableView* view = nsnull;
  float p2t, t2p;

  GetScrollInfo(&view, &p2t, &t2p);
  if(view) {
    nscoord xPos, yPos;
    ret = view->GetScrollPosition(xPos, yPos);
    scrollX = NSTwipsToIntPixels(xPos, t2p);
  }

  if (NS_SUCCEEDED(ret)) {
    ret = GetClientX(aPageX);
  }

  if (NS_SUCCEEDED(ret)) {
    *aPageX += scrollX;
  }

  return ret;
}

NS_METHOD nsDOMEvent::GetPageY(PRInt32* aPageY)
{
  NS_ENSURE_ARG_POINTER(aPageY);
  nsresult ret = NS_OK;
  PRInt32 scrollY = 0;
  nsIScrollableView* view = nsnull;
  float p2t, t2p;

  GetScrollInfo(&view, &p2t, &t2p);
  if(view) {
    nscoord xPos, yPos;
    ret = view->GetScrollPosition(xPos, yPos);
    scrollY = NSTwipsToIntPixels(yPos, t2p);
  }

  if (NS_SUCCEEDED(ret)) {
    ret = GetClientY(aPageY);
  }

  if (NS_SUCCEEDED(ret)) {
    *aPageY += scrollY;
  }

  return ret;
}

NS_METHOD nsDOMEvent::GetWhich(PRUint32* aWhich)
{
  NS_ENSURE_ARG_POINTER(aWhich);
  switch (mEvent->eventStructType) {
  case NS_KEY_EVENT:
	  switch (mEvent->message) {
			case NS_KEY_UP:
			case NS_KEY_DOWN:
				return GetKeyCode(aWhich);
			case NS_KEY_PRESS:
        //Special case for 4xp bug 62878.  Try to make value of which
        //more closely mirror the values that 4.x gave for RETURN and BACKSPACE
        {
          PRUint32 keyCode = ((nsKeyEvent*)mEvent)->keyCode;
          if (keyCode == NS_VK_RETURN || keyCode == NS_VK_BACK) {
            *aWhich = keyCode;
            return NS_OK;
          }
				  return GetCharCode(aWhich);
        }
			default:
				break;
		}
  case NS_MOUSE_EVENT:
    {
    PRUint16 button;
    (void) GetButton(&button);
    *aWhich = button + 1;
    break;
    }
  default:
    *aWhich = 0;
    break;
  }
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetRangeParent(nsIDOMNode** aRangeParent)
{
  NS_ENSURE_ARG_POINTER(aRangeParent);
  nsIFrame* targetFrame = nsnull;

  if (mPresContext) {
    mPresContext->EventStateManager()->GetEventTarget(&targetFrame);
  }

  *aRangeParent = nsnull;

  if (targetFrame) {
    nsCOMPtr<nsIContent> parent;
    PRInt32 offset, endOffset;
    PRBool beginOfContent;
    if (NS_SUCCEEDED(targetFrame->GetContentAndOffsetsFromPoint(mPresContext, 
                                              mEvent->point,
                                              getter_AddRefs(parent),
                                              offset,
                                              endOffset,
                                              beginOfContent))) {
      if (parent) {
        return CallQueryInterface(parent, aRangeParent);
      }
    }
  }

  return NS_OK;
}

NS_METHOD nsDOMEvent::GetRangeOffset(PRInt32* aRangeOffset)
{
  NS_ENSURE_ARG_POINTER(aRangeOffset);
  nsIFrame* targetFrame = nsnull;

  if (mPresContext) {
    mPresContext->EventStateManager()->GetEventTarget(&targetFrame);
  }

  if (targetFrame) {
    nsIContent* parent = nsnull;
    PRInt32 endOffset;
    PRBool beginOfContent;
    if (NS_SUCCEEDED(targetFrame->GetContentAndOffsetsFromPoint(mPresContext, 
                                              mEvent->point,
                                              &parent,
                                              *aRangeOffset,
                                              endOffset,
                                              beginOfContent))) {
      NS_IF_RELEASE(parent);
      return NS_OK;
    }
  }
  *aRangeOffset = 0;
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetCancelBubble(PRBool* aCancelBubble)
{
  NS_ENSURE_ARG_POINTER(aCancelBubble);
  if (mEvent->flags & NS_EVENT_FLAG_BUBBLE || mEvent->flags & NS_EVENT_FLAG_INIT) {
    *aCancelBubble = (mEvent->flags &= NS_EVENT_FLAG_STOP_DISPATCH) ? PR_TRUE : PR_FALSE;
  }
  else {
    *aCancelBubble = PR_FALSE;
  }
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetCancelBubble(PRBool aCancelBubble)
{
  if (mEvent->flags & NS_EVENT_FLAG_BUBBLE || mEvent->flags & NS_EVENT_FLAG_INIT) {
    if (aCancelBubble) {
      mEvent->flags |= NS_EVENT_FLAG_STOP_DISPATCH;
    }
    else {
      mEvent->flags &= ~NS_EVENT_FLAG_STOP_DISPATCH;
    }
  }
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetIsChar(PRBool* aIsChar)
{
  NS_ENSURE_ARG_POINTER(aIsChar);
  if (!mEvent) {
    *aIsChar = PR_FALSE;
    return NS_OK;
  }
  if (mEvent->eventStructType == NS_KEY_EVENT) {
    *aIsChar = ((nsKeyEvent*)mEvent)->isChar;
    return NS_OK;
  }
  if (mEvent->eventStructType == NS_TEXT_EVENT) {
    *aIsChar = ((nsTextEvent*)mEvent)->isChar;
    return NS_OK;
  }

  *aIsChar = PR_FALSE;
  return NS_OK;
}

NS_METHOD nsDOMEvent::GetPreventDefault(PRBool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  if (!mEvent) {
    *aReturn = PR_FALSE; 
  }
  else {
    *aReturn = (mEvent->flags & NS_EVENT_FLAG_NO_DEFAULT) ? PR_TRUE : PR_FALSE;
  }

  return NS_OK;
}

nsresult
nsDOMEvent::SetEventType(const nsAString& aEventTypeArg)
{
  nsCOMPtr<nsIAtom> atom= do_GetAtom(NS_LITERAL_STRING("on") + aEventTypeArg);
  mEvent->message = NS_USER_DEFINED_EVENT;

  if (mEvent->eventStructType == NS_MOUSE_EVENT) {
    if (atom == nsLayoutAtoms::onmousedown)
      mEvent->message = NS_MOUSE_LEFT_BUTTON_DOWN;
    else if (atom == nsLayoutAtoms::onmouseup)
      mEvent->message = NS_MOUSE_LEFT_BUTTON_UP;
    else if (atom == nsLayoutAtoms::onclick)
      mEvent->message = NS_MOUSE_LEFT_CLICK;
    else if (atom == nsLayoutAtoms::ondblclick)
      mEvent->message = NS_MOUSE_LEFT_DOUBLECLICK;
    else if (atom == nsLayoutAtoms::onmouseover)
      mEvent->message = NS_MOUSE_ENTER_SYNTH;
    else if (atom == nsLayoutAtoms::onmouseout)
      mEvent->message = NS_MOUSE_EXIT_SYNTH;
    else if (atom == nsLayoutAtoms::onmousemove)
      mEvent->message = NS_MOUSE_MOVE;
    else if (atom == nsLayoutAtoms::oncontextmenu)
      mEvent->message = NS_CONTEXTMENU;
  } else if (mEvent->eventStructType == NS_KEY_EVENT) {
    if (atom == nsLayoutAtoms::onkeydown)
      mEvent->message = NS_KEY_DOWN;
    else if (atom == nsLayoutAtoms::onkeyup)
      mEvent->message = NS_KEY_UP;
    else if (atom == nsLayoutAtoms::onkeypress)
      mEvent->message = NS_KEY_PRESS;
  } else if (mEvent->eventStructType == NS_EVENT) {
    if (atom == nsLayoutAtoms::onfocus)
      mEvent->message = NS_FOCUS_CONTENT;
    else if (atom == nsLayoutAtoms::onblur)
      mEvent->message = NS_BLUR_CONTENT;
    else if (atom == nsLayoutAtoms::onsubmit)
      mEvent->message = NS_FORM_SUBMIT;
    else if (atom == nsLayoutAtoms::onreset)
      mEvent->message = NS_FORM_RESET;
    else if (atom == nsLayoutAtoms::onchange)
      mEvent->message = NS_FORM_CHANGE;
    else if (atom == nsLayoutAtoms::onselect)
      mEvent->message = NS_FORM_SELECTED;
    else if (atom == nsLayoutAtoms::onload)
      mEvent->message = NS_PAGE_LOAD;
    else if (atom == nsLayoutAtoms::onunload)
      mEvent->message = NS_PAGE_UNLOAD;
    else if (atom == nsLayoutAtoms::onabort)
      mEvent->message = NS_IMAGE_ABORT;
    else if (atom == nsLayoutAtoms::onerror)
      mEvent->message = NS_IMAGE_ERROR;
  } else if (mEvent->eventStructType == NS_MUTATION_EVENT) {
    if (atom == nsLayoutAtoms::onDOMAttrModified)
      mEvent->message = NS_MUTATION_ATTRMODIFIED;
    else if (atom == nsLayoutAtoms::onDOMCharacterDataModified)
      mEvent->message = NS_MUTATION_CHARACTERDATAMODIFIED;
    else if (atom == nsLayoutAtoms::onDOMNodeInserted)
      mEvent->message = NS_MUTATION_NODEINSERTED;
    else if (atom == nsLayoutAtoms::onDOMNodeRemoved)
      mEvent->message = NS_MUTATION_NODEREMOVED;
    else if (atom == nsLayoutAtoms::onDOMNodeInsertedIntoDocument)
      mEvent->message = NS_MUTATION_NODEINSERTEDINTODOCUMENT;
    else if (atom == nsLayoutAtoms::onDOMNodeRemovedFromDocument)
      mEvent->message = NS_MUTATION_NODEREMOVEDFROMDOCUMENT;
    else if (atom == nsLayoutAtoms::onDOMSubtreeModified)
      mEvent->message = NS_MUTATION_SUBTREEMODIFIED;
  }

  if (mEvent->message == NS_USER_DEFINED_EVENT)
    mEvent->userType = new nsStringKey(aEventTypeArg);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::InitEvent(const nsAString& aEventTypeArg, PRBool aCanBubbleArg, PRBool aCancelableArg)
{
  NS_ENSURE_SUCCESS(SetEventType(aEventTypeArg), NS_ERROR_FAILURE);
  mEvent->flags |= aCanBubbleArg ? NS_EVENT_FLAG_NONE : NS_EVENT_FLAG_CANT_BUBBLE;
  mEvent->flags |= aCancelableArg ? NS_EVENT_FLAG_NONE : NS_EVENT_FLAG_CANT_CANCEL;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::InitUIEvent(const nsAString& aTypeArg, PRBool aCanBubbleArg, PRBool aCancelableArg, 
                        nsIDOMAbstractView* aViewArg, PRInt32 aDetailArg)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDOMEvent::InitMouseEvent(const nsAString & aTypeArg, PRBool aCanBubbleArg, PRBool aCancelableArg, 
                           nsIDOMAbstractView *aViewArg, PRInt32 aDetailArg, PRInt32 aScreenXArg, 
                           PRInt32 aScreenYArg, PRInt32 aClientXArg, PRInt32 aClientYArg, 
                           PRBool aCtrlKeyArg, PRBool aAltKeyArg, PRBool aShiftKeyArg, 
                           PRBool aMetaKeyArg, PRUint16 aButtonArg, nsIDOMEventTarget *aRelatedTargetArg)
{
  NS_ENSURE_SUCCESS(SetEventType(aTypeArg), NS_ERROR_FAILURE);
  mEvent->flags |= aCanBubbleArg ? NS_EVENT_FLAG_NONE : NS_EVENT_FLAG_CANT_BUBBLE;
  mEvent->flags |= aCancelableArg ? NS_EVENT_FLAG_NONE : NS_EVENT_FLAG_CANT_CANCEL;

  NS_ASSERTION(mEvent->eventStructType == NS_MOUSE_EVENT || mEvent->eventStructType == NS_MOUSE_SCROLL_EVENT, "event type mismatch");
  if (mEvent->eventStructType == NS_MOUSE_EVENT || mEvent->eventStructType == NS_MOUSE_SCROLL_EVENT) {
    nsInputEvent* inputEvent = NS_STATIC_CAST(nsInputEvent*, mEvent);
    inputEvent->isControl = aCtrlKeyArg;
    inputEvent->isAlt = aAltKeyArg;
    inputEvent->isShift = aShiftKeyArg;
    inputEvent->isMeta = aMetaKeyArg;
    inputEvent->point.x = aClientXArg;
    inputEvent->point.y = aClientYArg;
    inputEvent->refPoint.x = aScreenXArg;
    inputEvent->refPoint.y = aScreenYArg;
    mScreenPoint.x = aScreenXArg;
    mScreenPoint.y = aScreenYArg;
    mClientPoint.x = aClientXArg;
    mClientPoint.y = aClientYArg;
    mButton = aButtonArg;
    if (mEvent->eventStructType == NS_MOUSE_SCROLL_EVENT) {
      nsMouseScrollEvent* scrollEvent = NS_STATIC_CAST(nsMouseScrollEvent*, mEvent);
      scrollEvent->delta = aDetailArg;
    } else {
      nsMouseEvent* mouseEvent = NS_STATIC_CAST(nsMouseEvent*, mEvent);
      mouseEvent->clickCount = aDetailArg;
    }
    return NS_OK;
  }
  //include a way to set view once we have more than one

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDOMEvent::InitKeyEvent(const nsAString& aTypeArg, PRBool aCanBubbleArg, PRBool aCancelableArg, 
                         nsIDOMAbstractView* aViewArg, PRBool aCtrlKeyArg, PRBool aAltKeyArg, 
                         PRBool aShiftKeyArg, PRBool aMetaKeyArg, 
                         PRUint32 aKeyCodeArg, PRUint32 aCharCodeArg)
{
  NS_ENSURE_SUCCESS(SetEventType(aTypeArg), NS_ERROR_FAILURE);
  mEvent->flags |= aCanBubbleArg ? NS_EVENT_FLAG_NONE : NS_EVENT_FLAG_CANT_BUBBLE;
  mEvent->flags |= aCancelableArg ? NS_EVENT_FLAG_NONE : NS_EVENT_FLAG_CANT_CANCEL;
  mEvent->internalAppFlags |= NS_APP_EVENT_FLAG_NONE;

  NS_ASSERTION(mEvent->eventStructType == NS_KEY_EVENT, "event type mismatch");
  if (mEvent->eventStructType == NS_KEY_EVENT) {
    nsKeyEvent* keyEvent = NS_STATIC_CAST(nsKeyEvent*, mEvent);
    keyEvent->isControl = aCtrlKeyArg;
    keyEvent->isAlt = aAltKeyArg;
    keyEvent->isShift = aShiftKeyArg;
    keyEvent->isMeta = aMetaKeyArg;
    keyEvent->keyCode = aKeyCodeArg;
    keyEvent->charCode = aCharCodeArg;
    return NS_OK;
  }
  //include a way to set view once we have more than one

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsDOMEvent::InitPopupBlockedEvent(const nsAString & aTypeArg,
                            PRBool aCanBubbleArg, PRBool aCancelableArg,
                            nsIURI *aRequestingWindowURI,
                            nsIURI *aPopupWindowURI,
                            const nsAString & aPopupWindowFeatures)
{
  NS_ENSURE_SUCCESS(SetEventType(aTypeArg), NS_ERROR_FAILURE);
  mEvent->flags |= aCanBubbleArg ? NS_EVENT_FLAG_NONE : NS_EVENT_FLAG_CANT_BUBBLE;
  mEvent->flags |= aCancelableArg ? NS_EVENT_FLAG_NONE : NS_EVENT_FLAG_CANT_CANCEL;
  mEvent->internalAppFlags |= NS_APP_EVENT_FLAG_NONE;

  NS_ASSERTION(mEvent->eventStructType == NS_POPUPBLOCKED_EVENT, "event type mismatch");
  if (mEvent->eventStructType == NS_POPUPBLOCKED_EVENT) {
    nsPopupBlockedEvent* event = NS_STATIC_CAST(nsPopupBlockedEvent*, mEvent);
    event->mRequestingWindowURI = aRequestingWindowURI;
    event->mPopupWindowURI = aPopupWindowURI;
    NS_IF_ADDREF(event->mRequestingWindowURI);
    NS_IF_ADDREF(event->mPopupWindowURI);
    event->mPopupWindowFeatures = aPopupWindowFeatures;
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

/* readonly attribute nsIURI requestingWindowURI; */
NS_IMETHODIMP nsDOMEvent::GetRequestingWindowURI(nsIURI **aRequestingWindowURI)
{
  NS_ENSURE_ARG_POINTER(aRequestingWindowURI);
  if (mEvent->eventStructType == NS_POPUPBLOCKED_EVENT) {
    nsPopupBlockedEvent* event = NS_STATIC_CAST(nsPopupBlockedEvent*, mEvent);
    *aRequestingWindowURI = event->mRequestingWindowURI;
    NS_IF_ADDREF(*aRequestingWindowURI);
    return NS_OK;
  }
  *aRequestingWindowURI = 0;
  return NS_OK;  // Don't throw an exception
}

/* readonly attribute nsIURI popupWindowURI; */
NS_IMETHODIMP nsDOMEvent::GetPopupWindowURI(nsIURI **aPopupWindowURI)
{
  NS_ENSURE_ARG_POINTER(aPopupWindowURI);
  if (mEvent->eventStructType == NS_POPUPBLOCKED_EVENT) {
    nsPopupBlockedEvent* event = NS_STATIC_CAST(nsPopupBlockedEvent*, mEvent);
    *aPopupWindowURI = event->mPopupWindowURI;
    NS_IF_ADDREF(*aPopupWindowURI);
    return NS_OK;
  }
  *aPopupWindowURI = 0;
  return NS_OK;  // Don't throw an exception
}

/* readonly attribute DOMString popupFeatures; */
NS_IMETHODIMP nsDOMEvent::GetPopupWindowFeatures(nsAString &aPopupWindowFeatures)
{
  if (mEvent->eventStructType == NS_POPUPBLOCKED_EVENT) {
    nsPopupBlockedEvent* event = NS_STATIC_CAST(nsPopupBlockedEvent*, mEvent);
    aPopupWindowFeatures = event->mPopupWindowFeatures;
    return NS_OK;
  }
  aPopupWindowFeatures.Truncate();
  return NS_OK;  // Don't throw an exception
}

NS_METHOD nsDOMEvent::DuplicatePrivateData()
{
  //XXX Write me!

  //XXX And when you do, make sure to copy over the event target here, too!
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetTarget(nsIDOMEventTarget* aTarget)
{
  mTarget = aTarget;
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetCurrentTarget(nsIDOMEventTarget* aCurrentTarget)
{
  mCurrentTarget = aCurrentTarget;
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetOriginalTarget(nsIDOMEventTarget* aOriginalTarget)
{
  mOriginalTarget = aOriginalTarget;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::IsDispatchStopped(PRBool* aIsDispatchStopped)
{
  if (mEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) {
    *aIsDispatchStopped = PR_TRUE;
  } else {
    *aIsDispatchStopped = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::IsHandled(PRBool* aIsHandled)
{
  if (mEvent->internalAppFlags | NS_APP_EVENT_FLAG_HANDLED) {
    *aIsHandled = PR_TRUE;
  } else {
    *aIsHandled = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::SetHandled(PRBool aHandled)
{
  if(aHandled)
    mEvent->internalAppFlags |= NS_APP_EVENT_FLAG_HANDLED;
  else
    mEvent->internalAppFlags &= ~NS_APP_EVENT_FLAG_HANDLED;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMEvent::GetInternalNSEvent(nsEvent** aNSEvent)
{
  NS_ENSURE_ARG_POINTER(aNSEvent);
  *aNSEvent = mEvent;
  return NS_OK;
}

// return true if eventName is contained within events, delimited by
// spaces
static PRBool
PopupAllowedForEvent(const char *eventName)
{
  if (!sPopupAllowedEvents) {
    nsDOMEvent::PopupAllowedEventsChanged();

    if (!sPopupAllowedEvents) {
      return PR_FALSE;
    }
  }

  nsDependentCString events(sPopupAllowedEvents);

  nsAFlatCString::const_iterator start, end;
  nsAFlatCString::const_iterator startiter(events.BeginReading(start));
  events.EndReading(end);

  while (startiter != end) {
    nsAFlatCString::const_iterator enditer(end);

    if (!FindInReadable(nsDependentCString(eventName), startiter, enditer))
      return PR_FALSE;

    // the match is surrounded by spaces, or at a string boundary
    if ((startiter == start || *--startiter == ' ') &&
        (enditer == end || *enditer == ' ')) {
      return PR_TRUE;
    }

    // Move on and see if there are other matches. (The delimitation
    // requirement makes it pointless to begin the next search before
    // the end of the invalid match just found.)
    startiter = enditer;
  }

  return PR_FALSE;
}

// static
PopupControlState
nsDOMEvent::GetEventPopupControlState(nsEvent *aEvent)
{
  // generally if an event handler is running, new windows are disallowed.
  // check for exceptions:
  PopupControlState abuse = openAbused;

  switch(aEvent->eventStructType) {
  case NS_EVENT :
    // For these following events only allow popups if they're
    // triggered while handling user input. See
    // nsPresShell::HandleEventInternal() for details.
    if (nsEventStateManager::IsHandlingUserInput()) {
      switch(aEvent->message) {
      case NS_FORM_SELECTED :
        if (::PopupAllowedForEvent("select"))
          abuse = openControlled;
      case NS_FORM_CHANGE :
        if (::PopupAllowedForEvent("change"))
          abuse = openControlled;
        break;
      }
    }
    break;
  case NS_GUI_EVENT :
    // For this following event only allow popups if it's triggered
    // while handling user input. See
    // nsPresShell::HandleEventInternal() for details.
    if (nsEventStateManager::IsHandlingUserInput()) {
      switch(aEvent->message) {
      case NS_FORM_INPUT :
        if (::PopupAllowedForEvent("input"))
          abuse = openControlled;
        break;
      }
    }
    break;
  case NS_INPUT_EVENT :
    // For this following event only allow popups if it's triggered
    // while handling user input. See
    // nsPresShell::HandleEventInternal() for details.
    if (nsEventStateManager::IsHandlingUserInput()) {
      switch(aEvent->message) {
      case NS_FORM_CHANGE :
        if (::PopupAllowedForEvent("change"))
          abuse = openControlled;
        break;
      }
    }
    break;
  case NS_KEY_EVENT :
    if (aEvent->internalAppFlags & NS_APP_EVENT_FLAG_TRUSTED) {
      PRUint32 key = NS_STATIC_CAST(nsKeyEvent *, aEvent)->keyCode;
      switch(aEvent->message) {
      case NS_KEY_PRESS :
        // return key on focused button. see note at NS_MOUSE_LEFT_CLICK.
        if (key == nsIDOMKeyEvent::DOM_VK_RETURN)
          abuse = openAllowed;
        else if (::PopupAllowedForEvent("keypress"))
          abuse = openControlled;
        break;
      case NS_KEY_UP :
        // space key on focused button. see note at NS_MOUSE_LEFT_CLICK.
        if (key == nsIDOMKeyEvent::DOM_VK_SPACE)
          abuse = openAllowed;
        else if (::PopupAllowedForEvent("keyup"))
          abuse = openControlled;
        break;
      case NS_KEY_DOWN :
        if (::PopupAllowedForEvent("keydown"))
          abuse = openControlled;
        break;
      }
    }
    break;
  case NS_MOUSE_EVENT :
    if (aEvent->internalAppFlags & NS_APP_EVENT_FLAG_TRUSTED) {
      switch(aEvent->message) {
      case NS_MOUSE_LEFT_BUTTON_UP :
        if (::PopupAllowedForEvent("mouseup"))
          abuse = openControlled;
        break;
      case NS_MOUSE_LEFT_BUTTON_DOWN :
        if (::PopupAllowedForEvent("mousedown"))
          abuse = openControlled;
        break;
      case NS_MOUSE_LEFT_CLICK :
        /* Click events get special treatment because of their
           historical status as a more legitimate event handler. If
           click popups are enabled in the prefs, clear the popup
           status completely. */
        if (::PopupAllowedForEvent("click"))
          abuse = openAllowed;
        break;
      case NS_MOUSE_LEFT_DOUBLECLICK :
        if (::PopupAllowedForEvent("dblclick"))
          abuse = openControlled;
        break;
      }
    }
    break;
  case NS_SCRIPT_ERROR_EVENT :
    switch(aEvent->message) {
    case NS_SCRIPT_ERROR :
      // Any error event will allow popups, if enabled in the pref.
      if (::PopupAllowedForEvent("error"))
        abuse = openControlled;
      break;
    }
    break;
  case NS_FORM_EVENT :
    // For these following events only allow popups if they're
    // triggered while handling user input. See
    // nsPresShell::HandleEventInternal() for details.
    if (nsEventStateManager::IsHandlingUserInput()) {
      switch(aEvent->message) {
      case NS_FORM_SUBMIT :
        if (::PopupAllowedForEvent("submit"))
          abuse = openControlled;
        break;
      case NS_FORM_RESET :
        if (::PopupAllowedForEvent("reset"))
          abuse = openControlled;
        break;
      }
    }
    break;
  }

  return abuse;
}

// static
void
nsDOMEvent::PopupAllowedEventsChanged()
{
  if (sPopupAllowedEvents) {
    nsMemory::Free(sPopupAllowedEvents);
  }

  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID);

  if (prefBranch) {
    prefBranch->GetCharPref("dom.popup_allowed_events", &sPopupAllowedEvents);
  }
}

// static
void
nsDOMEvent::Shutdown()
{
  if (sPopupAllowedEvents) {
    nsMemory::Free(sPopupAllowedEvents);
  }
}

// static
const char* nsDOMEvent::GetEventName(PRUint32 aEventType)
{
  switch(aEventType) {
  case NS_MOUSE_LEFT_BUTTON_DOWN:
  case NS_MOUSE_MIDDLE_BUTTON_DOWN:
  case NS_MOUSE_RIGHT_BUTTON_DOWN:
    return sEventNames[eDOMEvents_mousedown];
  case NS_MOUSE_LEFT_BUTTON_UP:
  case NS_MOUSE_MIDDLE_BUTTON_UP:
  case NS_MOUSE_RIGHT_BUTTON_UP:
    return sEventNames[eDOMEvents_mouseup];
  case NS_MOUSE_LEFT_CLICK:
  case NS_MOUSE_MIDDLE_CLICK:
  case NS_MOUSE_RIGHT_CLICK:
    return sEventNames[eDOMEvents_click];
  case NS_MOUSE_LEFT_DOUBLECLICK:
  case NS_MOUSE_MIDDLE_DOUBLECLICK:
  case NS_MOUSE_RIGHT_DOUBLECLICK:
    return sEventNames[eDOMEvents_dblclick];
  case NS_MOUSE_ENTER_SYNTH:
    return sEventNames[eDOMEvents_mouseover];
  case NS_MOUSE_EXIT_SYNTH:
    return sEventNames[eDOMEvents_mouseout];
  case NS_MOUSE_MOVE:
    return sEventNames[eDOMEvents_mousemove];
  case NS_KEY_UP:
    return sEventNames[eDOMEvents_keyup];
  case NS_KEY_DOWN:
    return sEventNames[eDOMEvents_keydown];
  case NS_KEY_PRESS:
    return sEventNames[eDOMEvents_keypress];
  case NS_FOCUS_CONTENT:
    return sEventNames[eDOMEvents_focus];
  case NS_BLUR_CONTENT:
    return sEventNames[eDOMEvents_blur];
  case NS_XUL_CLOSE:
    return sEventNames[eDOMEvents_close];
  case NS_PAGE_LOAD:
  case NS_IMAGE_LOAD:
  case NS_SCRIPT_LOAD:
    return sEventNames[eDOMEvents_load];
  case NS_BEFORE_PAGE_UNLOAD:
    return sEventNames[eDOMEvents_beforeunload];
  case NS_PAGE_UNLOAD:
    return sEventNames[eDOMEvents_unload];
  case NS_IMAGE_ABORT:
    return sEventNames[eDOMEvents_abort];
  case NS_IMAGE_ERROR:
  case NS_SCRIPT_ERROR:
    return sEventNames[eDOMEvents_error];
  case NS_FORM_SUBMIT:
    return sEventNames[eDOMEvents_submit];
  case NS_FORM_RESET:
    return sEventNames[eDOMEvents_reset];
  case NS_FORM_CHANGE:
    return sEventNames[eDOMEvents_change];
  case NS_FORM_SELECTED:
    return sEventNames[eDOMEvents_select];
  case NS_FORM_INPUT:
    return sEventNames[eDOMEvents_input];
  case NS_PAINT:
    return sEventNames[eDOMEvents_paint];
  case NS_RESIZE_EVENT:
    return sEventNames[eDOMEvents_resize];
  case NS_SCROLL_EVENT:
    return sEventNames[eDOMEvents_scroll];
  case NS_TEXT_TEXT:
    return sEventNames[eDOMEvents_text];
  case NS_XUL_POPUP_SHOWING:
    return sEventNames[eDOMEvents_popupShowing];
  case NS_XUL_POPUP_SHOWN:
    return sEventNames[eDOMEvents_popupShown];
  case NS_XUL_POPUP_HIDING:
    return sEventNames[eDOMEvents_popupHiding];
  case NS_XUL_POPUP_HIDDEN:
    return sEventNames[eDOMEvents_popupHidden];
  case NS_XUL_COMMAND:
    return sEventNames[eDOMEvents_command];
  case NS_XUL_BROADCAST:
    return sEventNames[eDOMEvents_broadcast];
  case NS_XUL_COMMAND_UPDATE:
    return sEventNames[eDOMEvents_commandupdate];
  case NS_DRAGDROP_ENTER:
    return sEventNames[eDOMEvents_dragenter];
  case NS_DRAGDROP_OVER_SYNTH:
    return sEventNames[eDOMEvents_dragover];
  case NS_DRAGDROP_EXIT_SYNTH:
    return sEventNames[eDOMEvents_dragexit];
  case NS_DRAGDROP_DROP:
    return sEventNames[eDOMEvents_dragdrop];
  case NS_DRAGDROP_GESTURE:
    return sEventNames[eDOMEvents_draggesture];
  case NS_SCROLLPORT_OVERFLOW:
    return sEventNames[eDOMEvents_overflow];
  case NS_SCROLLPORT_UNDERFLOW:
    return sEventNames[eDOMEvents_underflow];
  case NS_SCROLLPORT_OVERFLOWCHANGED:
    return sEventNames[eDOMEvents_overflowchanged];
  case NS_MUTATION_SUBTREEMODIFIED:
    return sEventNames[eDOMEvents_subtreemodified];
  case NS_MUTATION_NODEINSERTED:
    return sEventNames[eDOMEvents_nodeinserted];
  case NS_MUTATION_NODEREMOVED:
    return sEventNames[eDOMEvents_noderemoved];
  case NS_MUTATION_NODEREMOVEDFROMDOCUMENT:
    return sEventNames[eDOMEvents_noderemovedfromdocument];
  case NS_MUTATION_NODEINSERTEDINTODOCUMENT:
    return sEventNames[eDOMEvents_nodeinsertedintodocument];
  case NS_MUTATION_ATTRMODIFIED:
    return sEventNames[eDOMEvents_attrmodified];
  case NS_MUTATION_CHARACTERDATAMODIFIED:
    return sEventNames[eDOMEvents_characterdatamodified];
  case NS_CONTEXTMENU:
  case NS_CONTEXTMENU_KEY:
    return sEventNames[eDOMEvents_contextmenu];
  default:
    break;
  }
  return nsnull;
}

nsresult
NS_NewDOMUIEvent(nsIDOMEvent** aInstancePtrResult,
                 nsIPresContext* aPresContext, const nsAString& aEventType,
                 nsEvent *aEvent)
{
  nsDOMEvent* it = new nsDOMEvent(aPresContext, aEvent, aEventType);

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}

void
nsDOMEvent::AllocateEvent(const nsAString& aEventType)
{
  //Allocate internal event
  nsAutoString eventType(aEventType);
  if (eventType.EqualsIgnoreCase("MouseEvents")) {
    mEvent = new nsMouseEvent();
  }
  else if (eventType.EqualsIgnoreCase("MouseScrollEvents")) {
    mEvent = new nsMouseScrollEvent();
  }
  else if (eventType.EqualsIgnoreCase("KeyEvents")) {
    mEvent = new nsKeyEvent();
  }
  else if (eventType.EqualsIgnoreCase("MutationEvents")) {
    mEvent = new nsMutationEvent();
  }
  else if (eventType.EqualsIgnoreCase("PopupEvents")) {
    mEvent = new nsGUIEvent();
  }
  else if (eventType.EqualsIgnoreCase("PopupBlockedEvents")) {
    mEvent = new nsPopupBlockedEvent();
  }
  else {
    mEvent = new nsEvent();
  }
  mEvent->time = PR_Now();
}
  
nsresult
NS_NewDOMEvent(nsIDOMEvent** aInstancePtrResult, nsIPresContext* aPresContext,
               nsEvent *aEvent) 
{
  return NS_ERROR_FAILURE;
}
