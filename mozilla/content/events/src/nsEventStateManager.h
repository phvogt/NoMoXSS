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

#ifndef nsEventStateManager_h__
#define nsEventStateManager_h__

#include "nsIEventStateManager.h"
#include "nsGUIEvent.h"
#include "nsIContent.h"
#include "nsIPrefBranchInternal.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsHashtable.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsCOMArray.h"

class nsIScrollableView;
class nsIPresShell;
class nsIFrameSelection;
class nsIDocShell;
class nsIDocShellTreeNode;
class nsIDocShellTreeItem;
class nsIFocusController;
class CurrentEventShepherd;

// mac uses click-hold context menus, a holdover from 4.x
#if defined(XP_MAC) || defined(XP_MACOSX)
#define CLICK_HOLD_CONTEXT_MENUS 1
#endif


/*
 * Event listener manager
 */

class nsEventStateManager : public nsSupportsWeakReference,
                            public nsIEventStateManager,
                            public nsIObserver
{
  // Tab focus model bit field:
  enum nsTabFocusModel {
  //eTabFocus_textControlsMask = (1<<0),  // unused - textboxes always tabbable
    eTabFocus_formElementsMask = (1<<1),  // non-text form elements
    eTabFocus_linksMask = (1<<2),         // links
    eTabFocus_any = 1 + (1<<1) + (1<<2)   // everything that can be focused
  };

  enum nsTextfieldSelectModel {
    eTextfieldSelect_unset = -1,
    eTextfieldSelect_manual = 0,
    eTextfieldSelect_auto = 1   // select textfields when focused with keyboard
  };

public:
  nsEventStateManager();
  virtual ~nsEventStateManager();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  NS_IMETHOD Init();
  nsresult Shutdown();

  /* The PreHandleEvent method is called before event dispatch to either
   * the DOM or frames.  Any processing which must not be prevented or
   * cancelled should occur here.  Any processing which is intended to
   * be conditional based on either DOM or frame processing should occur in
   * PostHandleEvent.  Any centralized event processing which must occur before
   * DOM or frame event handling should occur here as well.
   */
  NS_IMETHOD PreHandleEvent(nsIPresContext* aPresContext,
                         nsEvent *aEvent,
                         nsIFrame* aTargetFrame,
                         nsEventStatus* aStatus,
                         nsIView* aView);

  /* The PostHandleEvent method should contain all system processing which
   * should occur conditionally based on DOM or frame processing.  It should
   * also contain any centralized event processing which must occur after
   * DOM and frame processing.
   */
  NS_IMETHOD PostHandleEvent(nsIPresContext* aPresContext,
                             nsEvent *aEvent,
                             nsIFrame* aTargetFrame,
                             nsEventStatus* aStatus,
                             nsIView* aView);

  NS_IMETHOD GetCurrentEvent(nsEvent **aEvent)
               { *aEvent = mCurrentEvent; return NS_OK; }
  NS_IMETHOD SetCurrentEvent(nsEvent *aEvent)
               { mCurrentEvent = aEvent; return NS_OK; }

  NS_IMETHOD SetPresContext(nsIPresContext* aPresContext);
  NS_IMETHOD ClearFrameRefs(nsIFrame* aFrame);

  NS_IMETHOD GetEventTarget(nsIFrame **aFrame);
  NS_IMETHOD GetEventTargetContent(nsEvent* aEvent, nsIContent** aContent);
  NS_IMETHOD GetEventRelatedContent(nsIContent** aContent);

  NS_IMETHOD GetContentState(nsIContent *aContent, PRInt32& aState);
  NS_IMETHOD SetContentState(nsIContent *aContent, PRInt32 aState);
  NS_IMETHOD GetFocusedContent(nsIContent **aContent);
  NS_IMETHOD SetFocusedContent(nsIContent* aContent);
  NS_IMETHOD GetFocusedFrame(nsIFrame **aFrame);
  NS_IMETHOD ContentRemoved(nsIContent* aContent);
  NS_IMETHOD EventStatusOK(nsGUIEvent* aEvent, PRBool *aOK);

  // This is an experiement and may be temporary
  NS_IMETHOD ConsumeFocusEvents(PRBool aDoConsume) { mConsumeFocusEvents = aDoConsume; return NS_OK; }

  // Access Key Registration
  NS_IMETHOD RegisterAccessKey(nsIContent* aContent, PRUint32 aKey);
  NS_IMETHOD UnregisterAccessKey(nsIContent* aContent, PRUint32 aKey);

  NS_IMETHOD SetCursor(PRInt32 aCursor, nsIWidget* aWidget, PRBool aLockCursor);

  //Method for centralized distribution of new DOM events
  NS_IMETHOD DispatchNewEvent(nsISupports* aTarget, nsIDOMEvent* aEvent, PRBool *aPreventDefault);

  NS_IMETHOD ShiftFocus(PRBool aForward, nsIContent* aStart=nsnull);

  NS_IMETHOD GetBrowseWithCaret(PRBool *aBrowseWithCaret);
  NS_IMETHOD ResetBrowseWithCaret(PRBool *aBrowseWithCaret);

  NS_IMETHOD MoveFocusToCaret(PRBool aCanFocusDoc, PRBool *aIsSelectionWithFocus);
  NS_IMETHOD MoveCaretToFocus();

  static void StartHandlingUserInput()
  {
    ++sUserInputEventDepth;
  }

  static void StopHandlingUserInput()
  {
    --sUserInputEventDepth;
  }

  static PRBool IsHandlingUserInput()
  {
    return sUserInputEventDepth > 0;
  }

protected:
  friend class CurrentEventShepherd;

  void UpdateCursor(nsIPresContext* aPresContext, nsEvent* aEvent, nsIFrame* aTargetFrame, nsEventStatus* aStatus);
  /**
   * Turn a GUI mouse event into a mouse event targeted at the specified
   * content and frame.  This will fix the frame if it goes away during the
   * event, as well.
   */
  void DispatchMouseEvent(nsIPresContext* aPresContext,
                          nsGUIEvent* aEvent, PRUint32 aMessage,
                          nsIContent* aTargetContent,
                          nsIFrame*& aTargetFrame,
                          nsIContent* aRelatedContent);
  void MaybeDispatchMouseEventToIframe(nsIPresContext* aPresContext, nsGUIEvent* aEvent, PRUint32 aMessage);
  void GenerateMouseEnterExit(nsIPresContext* aPresContext, nsGUIEvent* aEvent);
  void GenerateDragDropEnterExit(nsIPresContext* aPresContext, nsGUIEvent* aEvent);
  nsresult SetClickCount(nsIPresContext* aPresContext, nsMouseEvent *aEvent, nsEventStatus* aStatus);
  nsresult CheckForAndDispatchClick(nsIPresContext* aPresContext, nsMouseEvent *aEvent, nsEventStatus* aStatus);
  PRBool ChangeFocus(nsIContent* aFocus, PRInt32 aFocusedWith);
  nsresult GetNextTabbableContent(nsIContent* aRootContent,
                                  nsIContent* aStartContent,
                                  nsIFrame* aStartFrame,
                                  PRBool forward, PRBool ignoreTabIndex,
                                  nsIContent** aResultNode,
                                  nsIFrame** aResultFrame);

  void TabIndexFrom(nsIContent *aFrom, PRInt32 *aOutIndex);
  PRInt32 GetNextTabIndex(nsIContent* aParent, PRBool foward);
  nsresult SendFocusBlur(nsIPresContext* aPresContext, nsIContent *aContent, PRBool aEnsureWindowHasFocus);
  PRBool CheckDisabled(nsIContent* aContent);
  void EnsureDocument(nsIPresShell* aPresShell);
  void EnsureDocument(nsIPresContext* aPresContext);
  void FlushPendingEvents(nsIPresContext* aPresContext);
  already_AddRefed<nsIFocusController> GetFocusControllerForDocument(nsIDocument* aDocument);

  typedef enum {
    eAccessKeyProcessingNormal = 0,
    eAccessKeyProcessingUp,
    eAccessKeyProcessingDown
  } ProcessingAccessKeyState;
  void HandleAccessKey(nsIPresContext* aPresContext, nsKeyEvent* aEvent, nsEventStatus* aStatus, PRInt32 aChildOffset, ProcessingAccessKeyState aAccessKeyState);

  //---------------------------------------------
  // DocShell Focus Traversal Methods
  //---------------------------------------------

  nsresult ShiftFocusInternal(PRBool aForward, nsIContent* aStart = nsnull);
  void TabIntoDocument(nsIDocShell* aDocShell, PRBool aForward);
  void ShiftFocusByDoc(PRBool forward);
  PRBool IsFrameSetDoc(nsIDocShell* aDocShell);
  PRBool IsIFrameDoc(nsIDocShell* aDocShell);
  PRBool IsShellVisible(nsIDocShell* aShell);
  void GetLastChildDocShell(nsIDocShellTreeItem* aItem,
                            nsIDocShellTreeItem** aResult);
  void GetNextDocShell(nsIDocShellTreeNode* aNode,
                       nsIDocShellTreeItem** aResult);
  void GetPrevDocShell(nsIDocShellTreeNode* aNode,
                       nsIDocShellTreeItem** aResult);

  // These functions are for mousewheel scrolling
  nsIScrollableView* GetNearestScrollingView(nsIView* aView);
  nsresult GetParentScrollingView(nsInputEvent* aEvent,
                                  nsIPresContext* aPresContext,
                                  nsIFrame* &targetOuterFrame,
                                  nsIPresContext* &presCtxOuter);
  nsresult DoScrollText(nsIPresContext* aPresContext,
                        nsIFrame* aTargetFrame,
                        nsInputEvent* aEvent,
                        PRInt32 aNumLines,
                        PRBool aScrollHorizontal,
                        PRBool aScrollPage,
                        PRBool aUseTargetFrame);
  void ForceViewUpdate(nsIView* aView);
  nsresult getPrefBranch();
  void DoScrollHistory(PRInt32 direction);
  void DoScrollTextsize(nsIFrame *aTargetFrame, PRInt32 adjustment);
  nsresult ChangeTextSize(PRInt32 change);
  // end mousewheel functions

  // routines for the d&d gesture tracking state machine
  void BeginTrackingDragGesture ( nsIPresContext* aPresContext, nsGUIEvent* inDownEvent, nsIFrame* inDownFrame ) ;
  void StopTrackingDragGesture ( ) ;
  void GenerateDragGesture ( nsIPresContext* aPresContext, nsGUIEvent *aEvent ) ;
  PRBool IsTrackingDragGesture ( ) const { return mIsTrackingDragGesture; }

  PRBool mSuppressFocusChange; // Used only for Ender text fields to suppress a focus firing on mouse down

  nsresult SetCaretEnabled(nsIPresShell *aPresShell, PRBool aVisibility);
  nsresult SetContentCaretVisible(nsIPresShell* aPresShell, nsIContent *aContent, PRBool aVisible);
  void FocusElementButNotDocument(nsIContent *aElement);

  // Return the location of the caret
  nsresult GetDocSelectionLocation(nsIContent **startContent, nsIContent **endContent, nsIFrame **startFrame, PRUint32 *startOffset);

  void GetSelection ( nsIFrame* inFrame, nsIPresContext* inPresContext, nsIFrameSelection** outSelection ) ;

  // To be called before and after you fire an event, to update booleans and
  // such
  void BeforeDispatchEvent() { ++mDOMEventLevel; }
  void AfterDispatchEvent();

  //Any frames here must be checked for validity in ClearFrameRefs
  nsIFrame* mCurrentTarget;
  nsCOMPtr<nsIContent> mCurrentTargetContent;
  nsCOMPtr<nsIContent> mCurrentRelatedContent;
  nsIFrame* mLastMouseOverFrame;
  nsCOMPtr<nsIContent> mLastMouseOverElement;
  nsIFrame* mLastDragOverFrame;

  // member variables for the d&d gesture state machine
  PRBool mIsTrackingDragGesture;
  nsPoint mGestureDownPoint;
  nsPoint mGestureDownRefPoint;
  nsIFrame* mGestureDownFrame;

  nsCOMPtr<nsIContent> mLastLeftMouseDownContent;
  nsCOMPtr<nsIContent> mLastMiddleMouseDownContent;
  nsCOMPtr<nsIContent> mLastRightMouseDownContent;

  nsCOMPtr<nsIContent> mActiveContent;
  nsCOMPtr<nsIContent> mHoverContent;
  nsCOMPtr<nsIContent> mDragOverContent;
  nsCOMPtr<nsIContent> mURLTargetContent;
  nsCOMPtr<nsIContent> mCurrentFocus;
  nsIFrame* mCurrentFocusFrame;
  PRInt32 mLastFocusedWith;
  PRInt32 mCurrentTabIndex;
  PRBool      mConsumeFocusEvents;
  PRInt32     mLockCursor;
  nsEvent    *mCurrentEvent;

  // DocShell Traversal Data Memebers
  nsCOMPtr<nsIContent> mLastContentFocus;

  //Anti-recursive stack controls
  nsCOMPtr<nsIContent> mFirstBlurEvent;
  nsCOMPtr<nsIContent> mFirstFocusEvent;
  nsCOMPtr<nsIContent> mFirstMouseOverEventElement;
  nsCOMPtr<nsIContent> mFirstMouseOutEventElement;

  nsIPresContext* mPresContext;      // Not refcnted
  nsCOMPtr<nsIDocument> mDocument;   // Doesn't necessarily need to be owner

  //Pref for dispatching middle and right clicks to content
  PRBool mLeftClickOnly;

  PRUint32 mLClickCount;
  PRUint32 mMClickCount;
  PRUint32 mRClickCount;

  PRPackedBool mNormalLMouseEventInProcess;

  //Hashtable for accesskey support
  nsSupportsHashtable *mAccessKeys;

  static PRUint32 mInstanceCount;
  static PRInt32 gGeneralAccesskeyModifier;

  // For preferences handling
  nsCOMPtr<nsIPrefBranchInternal> mPrefBranch;
  PRPackedBool m_haveShutdown;

  // To inform people that dispatched events that frames have been cleared and
  // they need to drop frame refs
  PRPackedBool mClearedFrameRefsDuringEvent;

  // The number of events we are currently nested in (currently just applies to
  // those handlers that care about clearing frame refs)
  PRInt32 mDOMEventLevel;

  // So we don't have to keep checking accessibility.browsewithcaret pref
  PRBool mBrowseWithCaret;

  // Tab focus policy (static, constant across the app):
  // Which types of elements are in the tab order?
  static PRInt8  sTextfieldSelectModel;

  // Recursion guard for tabbing
  PRBool mTabbedThroughDocument;
  nsCOMArray<nsIDocShell> mTabbingFromDocShells;

#ifdef CLICK_HOLD_CONTEXT_MENUS
  enum { kClickHoldDelay = 500 } ;        // 500ms == 1/2 second

  void CreateClickHoldTimer ( nsIPresContext* aPresContext, nsGUIEvent* inMouseDownEvent ) ;
  void KillClickHoldTimer ( ) ;
  void FireContextClick ( ) ;
  static void sClickHoldCallback ( nsITimer* aTimer, void* aESM ) ;
  
    // stash a bunch of stuff because we're going through a timer and the event
    // isn't guaranteed to be there later. We don't want to hold strong refs to
    // things because we're alerted to when they are going away in ClearFrameRefs().
  nsPoint mEventPoint, mEventRefPoint;
  nsIWidget* mEventDownWidget; // [WEAK]
  nsIPresContext* mEventPresContext; // [WEAK]
  nsCOMPtr<nsITimer> mClickHoldTimer;
#endif

  static PRInt32 sUserInputEventDepth;

};


class nsAutoHandlingUserInputStatePusher
{
public:
  nsAutoHandlingUserInputStatePusher(PRBool aIsHandlingUserInput)
    : mIsHandlingUserInput(aIsHandlingUserInput)
  {
    if (aIsHandlingUserInput) {
      nsEventStateManager::StartHandlingUserInput();
    }
  }

  ~nsAutoHandlingUserInputStatePusher()
  {
    if (mIsHandlingUserInput) {
      nsEventStateManager::StopHandlingUserInput();
    }
  }

protected:
  PRBool mIsHandlingUserInput;

private:
  // Not meant to be implemented.
  static void* operator new(size_t /*size*/) CPP_THROW_NEW;
  static void operator delete(void* /*memory*/);
};

#endif // nsEventStateManager_h__
