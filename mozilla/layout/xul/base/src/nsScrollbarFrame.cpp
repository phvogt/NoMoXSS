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

//
// Eric Vaughan
// Netscape Communications
//
// See documentation in associated header file
//

#include "nsScrollbarFrame.h"
#include "nsScrollbarButtonFrame.h"
#include "nsXULAtoms.h"
#include "nsIScrollableFrame.h"
#include "nsIView.h"
#include "nsIViewManager.h"


//
// NS_NewToolbarFrame
//
// Creates a new Toolbar frame and returns it in |aNewFrame|
//
nsresult
NS_NewScrollbarFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame )
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsScrollbarFrame* it = new (aPresShell) nsScrollbarFrame (aPresShell);
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewScrollbarFrame

//
// QueryInterface
//
NS_INTERFACE_MAP_BEGIN(nsScrollbarFrame)
  NS_INTERFACE_MAP_ENTRY(nsIScrollbarFrame)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)


NS_IMETHODIMP
nsScrollbarFrame::Init(nsIPresContext*  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsStyleContext*  aContext,
              nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  CreateViewForFrame(aPresContext,this,aContext,PR_TRUE);
  nsIView* view = GetView();
  view->GetViewManager()->SetViewContentTransparency(view, PR_TRUE);

  // We want to be a reflow root since we use reflows to move the
  // slider.  Any reflow inside the scrollbar frame will be a reflow to
  // move the slider and will thus not change anything outside of the
  // scrollbar or change the size of the scrollbar frame.
  mState |= NS_FRAME_REFLOW_ROOT;

  return rv;
}

NS_IMETHODIMP
nsScrollbarFrame::Reflow(nsIPresContext*          aPresContext,
                         nsHTMLReflowMetrics&     aDesiredSize,
                         const nsHTMLReflowState& aReflowState,
                         nsReflowStatus&          aStatus)
{
  nsresult rv = nsBoxFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  // nsGfxScrollFrame may have told us to shrink to nothing. If so, make sure our
  // desired size agrees.
  if (aReflowState.availableWidth == 0) {
    aDesiredSize.width = 0;
  }
  if (aReflowState.availableHeight == 0) {
    aDesiredSize.height = 0;
  }

  return NS_OK;
}

/* virtual */ PRBool
nsScrollbarFrame::IsContainingBlock() const
{
  // Return true so that the nsHTMLReflowState code is happy with us
  // being a reflow root.
  return PR_TRUE;
}

NS_IMETHODIMP
nsScrollbarFrame::AttributeChanged(nsIPresContext* aPresContext,
                                   nsIContent* aChild,
                                   PRInt32 aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32 aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aPresContext, aChild,
                                             aNameSpaceID, aAttribute,
                                             aModType);

  // if the current position changes, notify any nsGfxScrollFrame
  // parent we may have
  if (aAttribute != nsXULAtoms::curpos)
    return rv;

  nsIFrame* parent = GetParent();
  if (!parent)
    return rv;

  nsIScrollableFrame* scrollable = nsnull;
  parent->QueryInterface(NS_GET_IID(nsIScrollableFrame), (void**)&scrollable);
  if (!scrollable)
    return rv;

  scrollable->CurPosAttributeChanged(aPresContext, aChild, aModType);
  return rv;
}

NS_IMETHODIMP
nsScrollbarFrame::HandlePress(nsIPresContext* aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
nsScrollbarFrame::HandleMultiplePress(nsIPresContext* aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsScrollbarFrame::HandleDrag(nsIPresContext* aPresContext, 
                              nsGUIEvent*     aEvent,
                              nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsScrollbarFrame::HandleRelease(nsIPresContext* aPresContext, 
                                 nsGUIEvent*     aEvent,
                                 nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

