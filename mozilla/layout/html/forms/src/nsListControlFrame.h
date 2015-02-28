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
#ifndef nsListControlFrame_h___
#define nsListControlFrame_h___

#ifdef DEBUG_evaughan
//#define DEBUG_rods
#endif

#ifdef DEBUG_rods
//#define DO_REFLOW_DEBUG
//#define DO_REFLOW_COUNTER
//#define DO_UNCONSTRAINED_CHECK
//#define DO_PIXELS
#endif

#include "nsGfxScrollFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIListControlFrame.h"
#include "nsISelectControlFrame.h"
#include "nsIStatefulFrame.h"
#include "nsIDOMEventListener.h"
#include "nsIPresState.h"
#include "nsIContent.h"
#include "nsAutoPtr.h"

class nsIDOMHTMLSelectElement;
class nsIDOMHTMLOptionsCollection;
class nsIDOMHTMLOptionElement;
class nsIComboboxControlFrame;
class nsIViewManager;
class nsIPresContext;
class nsVoidArray;
class nsIScrollableView;

class nsSelectUpdateTimer;
class nsVoidArray;
class nsListEventListener;

/**
 * Frame-based listbox.
 */

class nsListControlFrame : public nsGfxScrollFrame,
                           public nsIFormControlFrame, 
                           public nsIListControlFrame,
                           public nsIStatefulFrame,
                           public nsISelectControlFrame
{
public:
  friend nsresult NS_NewListControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
  friend class nsSelectUpdateTimer;

   // nsISupports
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

    // nsIFrame
  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);
  
  NS_IMETHOD SetInitialChildList(nsIPresContext* aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  NS_IMETHOD Reflow(nsIPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsStyleContext*  aContext,
                   nsIFrame*        aPrevInFlow);

  NS_IMETHOD DidReflow(nsIPresContext*           aPresContext, 
                       const nsHTMLReflowState*  aReflowState, 
                       nsDidReflowStatus         aStatus);
  NS_IMETHOD Destroy(nsIPresContext *aPresContext);

  NS_IMETHOD Paint(nsIPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nsFramePaintLayer    aWhichLayer,
                   PRUint32             aFlags = 0);

  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::scrollFrame
   */
  virtual nsIAtom* GetType() const;

#ifdef DEBUG
    // nsIFrameDebug
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

    // nsIFormControlFrame
  NS_IMETHOD_(PRInt32) GetFormControlType() const;
  NS_IMETHOD GetName(nsAString* aName);
  NS_IMETHOD SetProperty(nsIPresContext* aPresContext, nsIAtom* aName, const nsAString& aValue);
  NS_IMETHOD GetProperty(nsIAtom* aName, nsAString& aValue); 
  NS_IMETHOD GetMultiple(PRBool* aResult, nsIDOMHTMLSelectElement* aSelect = nsnull);
  NS_IMETHOD GetFont(nsIPresContext* aPresContext, 
                     const nsFont*&  aFont);
  NS_IMETHOD GetFormContent(nsIContent*& aContent) const;
  NS_IMETHOD OnContentReset();

  virtual void SetFocus(PRBool aOn = PR_TRUE, PRBool aRepaint = PR_FALSE);
  virtual void ScrollIntoView(nsIPresContext* aPresContext);
  virtual void MouseClicked(nsIPresContext* aPresContext);
  virtual nscoord GetVerticalInsidePadding(nsIPresContext* aPresContext,
                                           float aPixToTwip,
                                           nscoord aInnerHeight) const;
  virtual nscoord GetHorizontalInsidePadding(nsIPresContext* aPresContext,
                                             float aPixToTwip, 
                                             nscoord aInnerWidth,
                                             nscoord aCharWidth) const;
  virtual nsresult RequiresWidget(PRBool &aRequiresWidget);

  virtual ScrollbarStyles GetScrollbarStyles() const;

    // for accessibility purposes
#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

    // nsHTMLContainerFrame
  virtual PRIntn GetSkipSides() const;

    // nsIListControlFrame
  NS_IMETHOD SetComboboxFrame(nsIFrame* aComboboxFrame);
  NS_IMETHOD GetSelectedIndex(PRInt32* aIndex); 
  NS_IMETHOD GetOptionText(PRInt32 aIndex, nsAString & aStr);
  NS_IMETHOD CaptureMouseEvents(nsIPresContext* aPresContext, PRBool aGrabMouseEvents);
  NS_IMETHOD GetMaximumSize(nsSize &aSize);
  NS_IMETHOD SetSuggestedSize(nscoord aWidth, nscoord aHeight);
  NS_IMETHOD GetNumberOfOptions(PRInt32* aNumOptions);  
  NS_IMETHOD SyncViewWithFrame(nsIPresContext* aPresContext);
  NS_IMETHOD AboutToDropDown();
  NS_IMETHOD AboutToRollup();
  NS_IMETHOD UpdateSelection();
  NS_IMETHOD SetOverrideReflowOptimization(PRBool aValue) { mOverrideReflowOpt = aValue; return NS_OK; }
  NS_IMETHOD GetOptionsContainer(nsIPresContext* aPresContext, nsIFrame** aFrame);
  NS_IMETHOD FireOnChange();
  NS_IMETHOD ComboboxFinish(PRInt32 aIndex);

  // nsISelectControlFrame
  NS_IMETHOD AddOption(nsIPresContext* aPresContext, PRInt32 index);
  NS_IMETHOD RemoveOption(nsIPresContext* aPresContext, PRInt32 index);
  NS_IMETHOD GetOptionSelected(PRInt32 aIndex, PRBool* aValue);
  NS_IMETHOD DoneAddingChildren(PRBool aIsDone);
  NS_IMETHOD OnOptionSelected(nsIPresContext* aPresContext,
                              PRInt32 aIndex,
                              PRBool aSelected);
  NS_IMETHOD OnOptionTextChanged(nsIDOMHTMLOptionElement* option);
  NS_IMETHOD GetDummyFrame(nsIFrame** aFrame);
  NS_IMETHOD SetDummyFrame(nsIFrame* aFrame);

  //nsIStatefulFrame
  NS_IMETHOD SaveState(nsIPresContext* aPresContext, nsIPresState** aState);
  NS_IMETHOD RestoreState(nsIPresContext* aPresContext, nsIPresState* aState);

  // mouse event listeners
  nsresult MouseDown(nsIDOMEvent* aMouseEvent);
  nsresult MouseUp(nsIDOMEvent* aMouseEvent);

  // mouse motion listeners
  nsresult MouseMove(nsIDOMEvent* aMouseEvent);
  nsresult DragMove(nsIDOMEvent* aMouseEvent);

  // key listeners
  nsresult KeyPress(nsIDOMEvent* aKeyEvent);

  // Static Methods
  static nsIDOMHTMLSelectElement* GetSelect(nsIContent * aContent);
  static nsIDOMHTMLOptionsCollection* GetOptions(nsIContent * aContent, nsIDOMHTMLSelectElement* aSelect = nsnull);
  static nsIDOMHTMLOptionElement* GetOption(nsIDOMHTMLOptionsCollection* aOptions, PRInt32 aIndex);
  static nsIContent* GetOptionAsContent(nsIDOMHTMLOptionsCollection* aCollection,PRInt32 aIndex);

  static void ComboboxFocusSet();

  // Helper
  void SetPassId(PRInt16 aId)  { mPassId = aId; }

  void PaintFocus(nsIRenderingContext& aRC, nsFramePaintLayer aWhichLayer);

#ifdef ACCESSIBILITY
  void FireMenuItemActiveEvent(); // Inform assistive tech what got focused
#endif

protected:

  void       DropDownToggleKey(nsIDOMEvent* aKeyEvent);
  nsresult   IsOptionDisabled(PRInt32 anIndex, PRBool &aIsDisabled);
  nsresult   ScrollToFrame(nsIContent * aOptElement);
  nsresult   ScrollToIndex(PRInt32 anIndex);
  PRBool     IsClickingInCombobox(nsIDOMEvent* aMouseEvent);
  void       AdjustIndexForDisabledOpt(PRInt32 aStartIndex, PRInt32 &anNewIndex,
                                       PRInt32 aNumOptions, PRInt32 aDoAdjustInc, PRInt32 aDoAdjustIncNext);
  virtual void ResetList(nsIPresContext* aPresContext, nsVoidArray * aInxList = nsnull);

  nsListControlFrame(nsIPresShell* aShell, nsIDocument* aDocument);
  virtual ~nsListControlFrame();

  // Utility methods
  nsresult GetSizeAttribute(PRInt32 *aSize);
  nsIContent* GetOptionFromContent(nsIContent *aContent);
  nsresult GetIndexFromDOMEvent(nsIDOMEvent* aMouseEvent, PRInt32& aCurIndex);
  nsIContent* GetOptionContent(PRInt32 aIndex);
  PRBool   IsContentSelected(nsIContent* aContent);
  PRBool   IsContentSelectedByIndex(PRInt32 aIndex);
  void     GetViewOffset(nsIViewManager* aManager, nsIView* aView, nsPoint& aPoint);
  PRBool   IsOptionElement(nsIContent* aContent);
  PRBool   CheckIfAllFramesHere();
  PRInt32  GetIndexFromContent(nsIContent *aContent);
  PRBool   IsLeftButton(nsIDOMEvent* aMouseEvent);

  // Dropped down stuff
  void     SetComboboxItem(PRInt32 aIndex);
  PRBool   IsInDropDownMode() const;

  // Selection
  PRBool   SetOptionsSelectedFromFrame(PRInt32 aStartIndex,
                                       PRInt32 aEndIndex,
                                       PRBool aValue,
                                       PRBool aClearAll);
  PRBool   ToggleOptionSelectedFromFrame(PRInt32 aIndex);
  PRBool   SingleSelection(PRInt32 aClickedIndex, PRBool aDoToggle);
  PRBool   ExtendedSelection(PRInt32 aStartIndex, PRInt32 aEndIndex,
                             PRBool aClearAll);
  PRBool   PerformSelection(PRInt32 aClickedIndex, PRBool aIsShift,
                            PRBool aIsControl);
  PRBool   HandleListSelection(nsIDOMEvent * aDOMEvent, PRInt32 selectedIndex);
  void     InitSelectionRange(PRInt32 aClickedIndex);

  // Timer Methods
  nsresult StartUpdateTimer(nsIPresContext * aPresContext);
  void     StopUpdateTimer();
  void     ItemsHaveBeenRemoved(nsIPresContext * aPresContext);

  // Data Members
  PRInt32      mStartSelectionIndex;
  PRInt32      mEndSelectionIndex;
  PRPackedBool mChangesSinceDragStart;

  PRInt32      mSelectedIndexWhenPoppedDown;
  nsIComboboxControlFrame *mComboboxFrame;
  PRPackedBool mButtonDown;
  nscoord      mMaxWidth;
  nscoord      mMaxHeight;
  PRInt32      mNumDisplayRows;

  PRBool       mIsAllContentHere;
  PRPackedBool mIsAllFramesHere;
  PRPackedBool mHasBeenInitialized;
  PRPackedBool mDoneWithInitialReflow;

  PRPackedBool mOverrideReflowOpt;

  nsIPresContext* mPresContext;             // XXX: Remove the need to cache the pres context.

  nsRefPtr<nsListEventListener> mEventListener;

  // XXX temprary only until full system mouse capture works
  PRPackedBool mIsScrollbarVisible;

  PRInt16 mPassId;
  nscoord mCachedDesiredMEW;

  // Update timer
  nsSelectUpdateTimer * mUpdateTimer;

  nsIFrame* mDummyFrame;

  //Resize Reflow OpitmizationSize;
  nsSize       mCacheSize;
  nscoord      mCachedAscent;
  nscoord      mCachedMaxElementWidth;
  nsSize       mCachedUnconstrainedSize;
  nsSize       mCachedAvailableSize;

  static nsListControlFrame * mFocused;

#ifdef DO_REFLOW_COUNTER
  PRInt32 mReflowId;
#endif

private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }

  // for incremental typing navigation
  static nsAString& GetIncrementalString ();
  static DOMTimeStamp gLastKeyTime;
};

#endif /* nsListControlFrame_h___ */

