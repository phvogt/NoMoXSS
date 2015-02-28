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
#ifndef nsGfxScrollFrame_h___
#define nsGfxScrollFrame_h___

#include "nsHTMLContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsBoxFrame.h"
#include "nsIScrollableFrame.h"

class nsISupportsArray;
class nsGfxScrollFrameInner;

/**
 * The scroll frame creates and manages the scrolling view
 *
 * It only supports having a single child frame that typically is an area
 * frame, but doesn't have to be. The child frame must have a view, though
 *
 * Scroll frames don't support incremental changes, i.e. you can't replace
 * or remove the scrolled frame
 */
class nsGfxScrollFrame : public nsBoxFrame,
                         public nsIScrollableFrame,
                         public nsIAnonymousContentCreator {
public:
  friend nsresult NS_NewGfxScrollFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame, 
                                       nsIDocument* aDocument, PRBool aIsRoot);

  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsStyleContext*  aContext,
                  nsIFrame*        aPrevInFlow);

  virtual ~nsGfxScrollFrame();

  // Called to set the child frames. We typically have three: the scroll area,
  // the vertical scrollbar, and the horizontal scrollbar.
  NS_IMETHOD SetInitialChildList(nsIPresContext* aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  NS_IMETHOD Reflow(nsIPresContext*          aPresContext,
                  nsHTMLReflowMetrics&     aDesiredSize,
                  const nsHTMLReflowState& aReflowState,
                  nsReflowStatus&          aStatus);

  // Because there can be only one child frame, these two function return
  // NS_ERROR_FAILURE
  NS_IMETHOD AppendFrames(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);

  NS_IMETHOD ReplaceFrame(nsIPresContext* aPresContext,
                     nsIPresShell&   aPresShell,
                     nsIAtom*        aListName,
                     nsIFrame*       aOldFrame,
                     nsIFrame*       aNewFrame);

  NS_IMETHOD Destroy(nsIPresContext* aPresContext);

  // This function returns NS_ERROR_NOT_IMPLEMENTED
  NS_IMETHOD RemoveFrame(nsIPresContext* aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);


  NS_IMETHOD Paint(nsIPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nsFramePaintLayer    aWhichLayer,
                   PRUint32             aFlags = 0);

  NS_IMETHOD GetContentAndOffsetsFromPoint(nsIPresContext* aCX,
                                           const nsPoint&  aPoint,
                                           nsIContent **   aNewContent,
                                           PRInt32&        aContentOffset,
                                           PRInt32&        aContentOffsetEnd,
                                           PRBool&         aBeginFrameContent);

  // nsIAnonymousContentCreator
  NS_IMETHOD CreateAnonymousContent(nsIPresContext* aPresContext,
                                    nsISupportsArray& aAnonymousItems);
  NS_IMETHOD CreateFrameFor(nsIPresContext*   aPresContext,
                            nsIContent *      aContent,
                            nsIFrame**        aFrame) { if (aFrame) *aFrame = nsnull; return NS_ERROR_FAILURE; }

  // nsIBox methods
  NS_DECL_ISUPPORTS

  NS_IMETHOD GetPrefSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMaxSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetAscent(nsBoxLayoutState& aBoxLayoutState, nscoord& aAscent);

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  NS_IMETHOD GetPadding(nsMargin& aPadding);

  // nsIScrollableFrame
  NS_IMETHOD  SetScrolledFrame(nsIPresContext* aPresContext, nsIFrame *aScrolledFrame);
  NS_IMETHOD  GetScrolledFrame(nsIPresContext* aPresContext, nsIFrame *&aScrolledFrame) const;
  NS_IMETHOD  GetScrollbarVisibility(nsIPresContext* aPresContext,
                                     PRBool *aVerticalVisible,
                                     PRBool *aHorizontalVisible) const;
  NS_IMETHOD GetScrollableView(nsIPresContext* aContext, nsIScrollableView** aResult);

  NS_IMETHOD GetScrollPosition(nsIPresContext* aContext, nscoord &aX, nscoord& aY) const;
  NS_IMETHOD ScrollTo(nsIPresContext* aContext, nscoord aX, nscoord aY, PRUint32 aFlags);

  NS_IMETHOD SetScrollbarVisibility(nsIPresContext* aPresContext,
                                    PRBool aVerticalVisible,
                                    PRBool aHorizontalVisible);

  NS_IMETHOD GetScrollbarBox(PRBool aVertical, nsIBox** aResult);

  NS_IMETHOD CurPosAttributeChanged(nsIPresContext* aPresContext,
                                    nsIContent* aChild,
                                    PRInt32 aModType);

  NS_IMETHOD  GetScrollPreference(nsIPresContext* aPresContext, nsScrollPref* aScrollPreference) const;

  virtual void ScrollToRestoredPosition();

  virtual nsMargin GetActualScrollbarSizes() const;
  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState);

  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::scrollFrame
   */
  virtual nsIAtom* GetType() const;
  
#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  virtual nsresult GetContentOf(nsIContent** aContent);

  static nsGfxScrollFrame* GetScrollFrameForPort(nsIFrame* aPort);

  struct ScrollbarStyles {
    // one of NS_STYLE_OVERFLOW_SCROLL, NS_STYLE_OVERFLOW_HIDDEN,
    // NS_STYLE_OVERFLOW_VISIBLE, NS_STYLE_OVERFLOW_AUTO
    PRInt32 mHorizontal;
    PRInt32 mVertical;
    ScrollbarStyles(PRInt32 h, PRInt32 v) : mHorizontal(h), mVertical(v) {}
  };
  virtual ScrollbarStyles GetScrollbarStyles() const;

protected:
  nsGfxScrollFrame(nsIPresShell* aShell, nsIDocument* aDocument, PRBool aIsRoot);
  virtual PRIntn GetSkipSides() const;

  // If a child frame was added or removed, reload our child frame list
  // We need this if a scrollbar frame is recreated
  void ReloadChildFrames(nsIPresContext* aPresContext);

private:
  friend class nsGfxScrollFrameInner;
  nsGfxScrollFrameInner* mInner;
  nsIPresContext*        mPresContext;  // weak reference
};

#endif /* nsGfxScrollFrame_h___ */
