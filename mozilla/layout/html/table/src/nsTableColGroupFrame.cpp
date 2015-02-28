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
#include "nsTableColGroupFrame.h"
#include "nsTableColFrame.h"
#include "nsTableFrame.h"
#include "nsIHTMLTableColElement.h"
#include "nsIDOMHTMLTableColElement.h"
#include "nsReflowPath.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsHTMLParts.h"
#include "nsHTMLAtoms.h"
#include "nsCOMPtr.h"
#include "nsCSSRendering.h"
#include "nsIPresShell.h"
#include "nsLayoutAtoms.h"

#define COL_GROUP_TYPE_BITS          0xC0000000 // uses bits 31-32 from mState
#define COL_GROUP_TYPE_OFFSET        30

nsTableColGroupType 
nsTableColGroupFrame::GetColType() const 
{
  return (nsTableColGroupType)((mState & COL_GROUP_TYPE_BITS) >> COL_GROUP_TYPE_OFFSET);
}

void nsTableColGroupFrame::SetColType(nsTableColGroupType aType) 
{
  PRUint32 type = aType - eColGroupContent;
  mState |= (type << COL_GROUP_TYPE_OFFSET);
}

void nsTableColGroupFrame::ResetColIndices(nsIFrame*       aFirstColGroup,
                                           PRInt32         aFirstColIndex,
                                           nsIFrame*       aStartColFrame)
{
  nsTableColGroupFrame* colGroupFrame = (nsTableColGroupFrame*)aFirstColGroup;
  PRInt32 colIndex = aFirstColIndex;
  while (colGroupFrame) {
    if (nsLayoutAtoms::tableColGroupFrame == colGroupFrame->GetType()) {
      // reset the starting col index for the first cg only if
      // aFirstColIndex is smaller than the existing starting col index
      if ((colIndex != aFirstColIndex) ||
          (colIndex < colGroupFrame->GetStartColumnIndex())) {
        colGroupFrame->SetStartColumnIndex(colIndex);
      }
      nsIFrame* colFrame = aStartColFrame; 
      if (!colFrame || (colIndex != aFirstColIndex)) {
        colFrame = colGroupFrame->GetFirstChild(nsnull);
      }
      while (colFrame) {
        if (nsLayoutAtoms::tableColFrame == colFrame->GetType()) {
          ((nsTableColFrame*)colFrame)->SetColIndex(colIndex);
          colIndex++;
        }
        colFrame = colFrame->GetNextSibling();
      }
    }
    colGroupFrame = NS_STATIC_CAST(nsTableColGroupFrame*,
                                   colGroupFrame->GetNextSibling());
  }
}


NS_IMETHODIMP
nsTableColGroupFrame::AddColsToTable(nsIPresContext&  aPresContext,
                                     PRInt32          aFirstColIndex,
                                     PRBool           aResetSubsequentColIndices,
                                     nsIFrame*        aFirstFrame,
                                     nsIFrame*        aLastFrame)
{
  nsresult rv = NS_OK;
  nsTableFrame* tableFrame = nsnull;
  rv = nsTableFrame::GetTableFrame(this, tableFrame);
  if (!tableFrame || !aFirstFrame) return NS_ERROR_NULL_POINTER;

  // set the col indices of the col frames and and add col info to the table
  PRInt32 colIndex = aFirstColIndex;
  nsIFrame* kidFrame = aFirstFrame;
  PRBool foundLastFrame = PR_FALSE;
  while (kidFrame) {
    if (nsLayoutAtoms::tableColFrame == kidFrame->GetType()) {
      ((nsTableColFrame*)kidFrame)->SetColIndex(colIndex);
      if (!foundLastFrame) {
        mColCount++;
        tableFrame->InsertCol(aPresContext, (nsTableColFrame &)*kidFrame, colIndex);
      }
      colIndex++;
    }
    if (kidFrame == aLastFrame) {
      foundLastFrame = PR_TRUE;
    }
    kidFrame = kidFrame->GetNextSibling(); 
  }

  if (aResetSubsequentColIndices && GetNextSibling()) {
    ResetColIndices(GetNextSibling(), colIndex);
  }

  return rv;
}

// this is called when a col frame doesn't have an explicit col group parent.
nsTableColGroupFrame* 
nsTableColGroupFrame::FindParentForAppendedCol(nsTableFrame*  aTableFrame, 
                                               nsTableColType aColType)
{
  nsVoidArray& cols = aTableFrame->GetColCache();
  PRInt32 numCols = cols.Count();
  if (numCols == 0) return nsnull; // no columns so no colgroups
  nsIFrame* lastCol = (nsIFrame*)cols.ElementAt(numCols - 1);
  NS_ASSERTION(lastCol,"null entry in column array");
  nsIFrame* lastColGroup = lastCol->GetParent();
  if (!lastColGroup) return nsnull; // shouldn't happen
 
  nsTableColGroupFrame* relevantColGroup =
    NS_STATIC_CAST(nsTableColGroupFrame *, lastColGroup);
  nsTableColGroupType relevantColGroupType = relevantColGroup->GetColType();
  if (eColGroupAnonymousCell == relevantColGroupType) {
    if (eColAnonymousCell == aColType) {
      return relevantColGroup;
    }
    else {
      // find the next to last col group
      for (PRInt32 colX = numCols - 2; colX >= 0; colX--) {
        nsTableColFrame* colFrame = (nsTableColFrame*)cols.ElementAt(colX);
        nsTableColGroupFrame* colGroupFrame =
          NS_STATIC_CAST(nsTableColGroupFrame*, colFrame->GetParent());
        nsTableColGroupType cgType = colGroupFrame->GetColType();
        if (cgType != relevantColGroupType) {
          relevantColGroup = colGroupFrame;
          relevantColGroupType = cgType;
          break;
        }
        else if (0 == colX) {
          return nsnull;
        }
      }
    }
  }

  if (eColGroupAnonymousCol == relevantColGroupType) {
    if ((eColContent == aColType) || (eColAnonymousCol == aColType)) {
      return relevantColGroup;
    }
  }

  return nsnull;
}

PRBool
nsTableColGroupFrame::GetLastRealColGroup(nsTableFrame* aTableFrame, 
                                          nsIFrame**    aLastColGroup)
{
  *aLastColGroup = nsnull;
  nsFrameList colGroups = aTableFrame->GetColGroups();

  nsIFrame* nextToLastColGroup = nsnull;
  nsIFrame* lastColGroup       = colGroups.FirstChild();
  while(lastColGroup) {
    nsIFrame* next = lastColGroup->GetNextSibling();
    if (next) {
      nextToLastColGroup = lastColGroup;
      lastColGroup = next;
    }
    else {
      break;
    }
  }

  if (!lastColGroup) return PR_TRUE; // there are no col group frames
 
  nsTableColGroupType lastColGroupType =
    ((nsTableColGroupFrame *)lastColGroup)->GetColType();
  if (eColGroupAnonymousCell == lastColGroupType) {
    *aLastColGroup = nextToLastColGroup;
    return PR_FALSE;
  }
  else {
    *aLastColGroup = lastColGroup;
    return PR_TRUE;
  }
}

// don't set mColCount here, it is done in AddColsToTable
NS_IMETHODIMP
nsTableColGroupFrame::SetInitialChildList(nsIPresContext* aPresContext,
                                          nsIAtom*        aListName,
                                          nsIFrame*       aChildList)
{
  nsTableFrame* tableFrame;
  nsTableFrame::GetTableFrame(this, tableFrame);
  if (!tableFrame) return NS_ERROR_NULL_POINTER;

  if (!aChildList) {
    nsIFrame* firstChild;
    tableFrame->CreateAnonymousColFrames(*aPresContext, *this, GetSpan(), eColAnonymousColGroup, 
                                         PR_FALSE, nsnull, &firstChild);
    if (firstChild) {
      SetInitialChildList(aPresContext, aListName, firstChild);
    }
    return NS_OK; 
  }

  mFrames.AppendFrames(this, aChildList);
  return NS_OK;
}

NS_IMETHODIMP
nsTableColGroupFrame::AppendFrames(nsIPresContext* aPresContext,
                                   nsIPresShell&   aPresShell,
                                   nsIAtom*        aListName,
                                   nsIFrame*       aFrameList)
{
  mFrames.AppendFrames(this, aFrameList);
  InsertColsReflow(*aPresContext, aPresShell, mColCount, aFrameList);
  return NS_OK;
}

NS_IMETHODIMP
nsTableColGroupFrame::InsertFrames(nsIPresContext* aPresContext,
                                   nsIPresShell&   aPresShell,
                                   nsIAtom*        aListName,
                                   nsIFrame*       aPrevFrameIn,
                                   nsIFrame*       aFrameList)
{
  nsFrameList frames(aFrameList); // convience for getting last frame
  nsIFrame* lastFrame = frames.LastChild();

  mFrames.InsertFrames(this, aPrevFrameIn, aFrameList);
  nsIFrame* prevFrame = nsTableFrame::GetFrameAtOrBefore(this, aPrevFrameIn,
                                                         nsLayoutAtoms::tableColFrame);

  PRInt32 colIndex = (prevFrame) ? ((nsTableColFrame*)prevFrame)->GetColIndex() + 1 : 0;
  InsertColsReflow(*aPresContext, aPresShell, colIndex, aFrameList, lastFrame);

  return NS_OK;
}

void
nsTableColGroupFrame::InsertColsReflow(nsIPresContext& aPresContext,
                                       nsIPresShell&   aPresShell,
                                       PRInt32         aColIndex,
                                       nsIFrame*       aFirstFrame,
                                       nsIFrame*       aLastFrame)
{
  AddColsToTable(aPresContext, aColIndex, PR_TRUE, aFirstFrame, aLastFrame);

  nsTableFrame* tableFrame;
  nsTableFrame::GetTableFrame(this, tableFrame);
  if (!tableFrame) return;

  // XXX this could be optimized with much effort
  tableFrame->SetNeedStrategyInit(PR_TRUE);

  // Generate a reflow command so we reflow the table
  nsTableFrame::AppendDirtyReflowCommand(&aPresShell, tableFrame);
}

void
nsTableColGroupFrame::RemoveChild(nsIPresContext&  aPresContext,
                                  nsTableColFrame& aChild,
                                  PRBool           aResetColIndices)
{
  PRInt32 colIndex = 0;
  nsIFrame* nextChild = nsnull;
  if (aResetColIndices) {
    colIndex = aChild.GetColIndex();
    nextChild = aChild.GetNextSibling();
  }
  if (mFrames.DestroyFrame(&aPresContext, (nsIFrame*)&aChild)) {
    mColCount--;
    if (aResetColIndices) {
      ResetColIndices(this, colIndex, nextChild);
    }
  }
  nsTableFrame* tableFrame;
  nsTableFrame::GetTableFrame(this, tableFrame);
  if (!tableFrame) return;

  // XXX this could be optimized with much effort
  tableFrame->SetNeedStrategyInit(PR_TRUE);
  // Generate a reflow command so we reflow the table
  nsTableFrame::AppendDirtyReflowCommand(aPresContext.PresShell(), tableFrame);
}

// this removes children form the last col group (eColGroupAnonymousCell) in the 
// table only,so there is no need to reset col indices for subsequent col groups.
void
nsTableColGroupFrame::RemoveChildrenAtEnd(nsIPresContext& aPresContext,
                                          PRInt32         aNumChildrenToRemove)
{
  PRInt32 numToRemove = aNumChildrenToRemove;
  if (numToRemove > mColCount) {
    NS_ASSERTION(PR_FALSE, "invalid arg to RemoveChildrenAtEnd");
    numToRemove = mColCount;
  }
  PRInt32 offsetOfFirstRemoval = mColCount - numToRemove;
  PRInt32 offsetX = 0;
  nsIFrame* kidFrame = mFrames.FirstChild();
  while(kidFrame) {
    if (nsLayoutAtoms::tableColFrame == kidFrame->GetType()) {
      offsetX++;
      if (offsetX > offsetOfFirstRemoval) {
        nsIFrame* byebye = kidFrame;
        kidFrame = kidFrame->GetNextSibling();
        mFrames.DestroyFrame(&aPresContext, byebye);
        continue;
      }
    }
    kidFrame = kidFrame->GetNextSibling();
  }
}


NS_IMETHODIMP
nsTableColGroupFrame::RemoveFrame(nsIPresContext* aPresContext,
                                  nsIPresShell&   aPresShell,
                                  nsIAtom*        aListName,
                                  nsIFrame*       aOldFrame)
{
  if (!aOldFrame) return NS_OK;

  if (nsLayoutAtoms::tableColFrame == aOldFrame->GetType()) {
    nsTableColFrame* colFrame = (nsTableColFrame*)aOldFrame;
    PRInt32 colIndex = colFrame->GetColIndex();
    RemoveChild(*aPresContext, *colFrame, PR_TRUE);
    
    nsTableFrame* tableFrame;
    nsTableFrame::GetTableFrame(this, tableFrame);
    if (!tableFrame) return NS_ERROR_NULL_POINTER;

    tableFrame->RemoveCol(*aPresContext, this, colIndex, PR_TRUE, PR_TRUE);

    // XXX This could probably be optimized with much effort
    tableFrame->SetNeedStrategyInit(PR_TRUE);
    // Generate a reflow command so we reflow the table
    nsTableFrame::AppendDirtyReflowCommand(&aPresShell, tableFrame);
  }
  else {
    mFrames.DestroyFrame(aPresContext, aOldFrame);
  }

  return NS_OK;
}

NS_METHOD 
nsTableColGroupFrame::Paint(nsIPresContext*      aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect&        aDirtyRect,
                            nsFramePaintLayer    aWhichLayer,
                            PRUint32             aFlags)
{
  PRBool isVisible;
  if (NS_SUCCEEDED(IsVisibleForPainting(aPresContext, aRenderingContext, PR_FALSE, &isVisible)) && !isVisible) {
    return NS_OK;
  }

  PaintChildren(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
  return NS_OK;
}

PRIntn
nsTableColGroupFrame::GetSkipSides() const
{
  PRIntn skip = 0;
  if (nsnull != mPrevInFlow) {
    skip |= 1 << NS_SIDE_TOP;
  }
  if (nsnull != mNextInFlow) {
    skip |= 1 << NS_SIDE_BOTTOM;
  }
  return skip;
}

NS_IMETHODIMP
nsTableColGroupFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                                   const nsPoint& aPoint, 
                                   nsFramePaintLayer aWhichLayer,
                                   nsIFrame**     aFrame)
{
  // this should act like a block, so we need to override
  return GetFrameForPointUsing(aPresContext, aPoint, nsnull, aWhichLayer, PR_FALSE, aFrame);
}

NS_METHOD nsTableColGroupFrame::Reflow(nsIPresContext*          aPresContext,
                                       nsHTMLReflowMetrics&     aDesiredSize,
                                       const nsHTMLReflowState& aReflowState,
                                       nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTableColGroupFrame", aReflowState.reason);
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  NS_ASSERTION(nsnull!=mContent, "bad state -- null content for frame");
  nsresult rv=NS_OK;
  // for every content child that (is a column thingy and does not already have a frame)
  // create a frame and adjust it's style
  nsIFrame* kidFrame = nsnull;
 
  if (eReflowReason_Incremental == aReflowState.reason) {
    rv = IncrementalReflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  }

  for (kidFrame = mFrames.FirstChild(); kidFrame;
       kidFrame = kidFrame->GetNextSibling()) {
    // Give the child frame a chance to reflow, even though we know it'll have 0 size
    nsHTMLReflowMetrics kidSize(nsnull);
    // XXX Use a valid reason...
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, kidFrame,
                                     nsSize(0,0), eReflowReason_Initial);

    nsReflowStatus status;
    ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState, 0, 0, 0, status);
    FinishReflowChild(kidFrame, aPresContext, nsnull, kidSize, 0, 0, 0);
  }

  aDesiredSize.width=0;
  aDesiredSize.height=0;
  aDesiredSize.ascent=aDesiredSize.height;
  aDesiredSize.descent=0;
  if (aDesiredSize.mComputeMEW)
  {
    aDesiredSize.mMaxElementWidth=0;
  }
  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
}

NS_METHOD nsTableColGroupFrame::IncrementalReflow(nsIPresContext*          aPresContext,
                                                  nsHTMLReflowMetrics&     aDesiredSize,
                                                  const nsHTMLReflowState& aReflowState,
                                                  nsReflowStatus&          aStatus)
{

  // the col group is a target if its path has a reflow command
  nsHTMLReflowCommand* command = aReflowState.path->mReflowCommand;
  if (command)
    IR_TargetIsMe(aPresContext, aDesiredSize, aReflowState, aStatus);

  // see if the chidren are targets as well
  nsReflowPath::iterator iter = aReflowState.path->FirstChild();
  nsReflowPath::iterator end  = aReflowState.path->EndChildren();
  for (; iter != end; ++iter)
    IR_TargetIsChild(aPresContext, aDesiredSize, aReflowState, aStatus, *iter);

  return NS_OK;
}

NS_METHOD nsTableColGroupFrame::IR_TargetIsMe(nsIPresContext*          aPresContext,
                                              nsHTMLReflowMetrics&     aDesiredSize,
                                              const nsHTMLReflowState& aReflowState,
                                              nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;
  aStatus = NS_FRAME_COMPLETE;
  nsReflowType type;
  aReflowState.path->mReflowCommand->GetType(type);
  nsIFrame *objectFrame;
  aReflowState.path->mReflowCommand->GetChildFrame(objectFrame); 
  switch (type)
  {
  case eReflowType_StyleChanged :
    rv = IR_StyleChanged(aPresContext, aDesiredSize, aReflowState, aStatus);
    break;

  case eReflowType_ContentChanged :
    NS_ASSERTION(PR_FALSE, "illegal reflow type: ContentChanged");
    rv = NS_ERROR_ILLEGAL_VALUE;
    break;
  
  default:
    NS_NOTYETIMPLEMENTED("unexpected reflow command type");
    rv = NS_ERROR_NOT_IMPLEMENTED;
    break;
  }

  return rv;
}

NS_METHOD nsTableColGroupFrame::IR_StyleChanged(nsIPresContext*          aPresContext,
                                                nsHTMLReflowMetrics&     aDesiredSize,
                                                const nsHTMLReflowState& aReflowState,
                                                nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;
  // we presume that all the easy optimizations were done in the nsHTMLStyleSheet before we were called here
  // XXX: we can optimize this when we know which style attribute changed
  nsTableFrame* tableFrame = nsnull;
  rv = nsTableFrame::GetTableFrame(this, tableFrame);
  if (tableFrame)  {
    tableFrame->SetNeedStrategyInit(PR_TRUE);
  }
  return rv;
}

NS_METHOD nsTableColGroupFrame::IR_TargetIsChild(nsIPresContext*          aPresContext,
                                                 nsHTMLReflowMetrics&     aDesiredSize,
                                                 const nsHTMLReflowState& aReflowState,
                                                 nsReflowStatus&          aStatus,
                                                 nsIFrame *               aNextFrame)
{
  nsresult rv;
 
  // Pass along the reflow command
  nsHTMLReflowMetrics desiredSize(nsnull);
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, aNextFrame,
                                   nsSize(aReflowState.availableWidth,
                                          aReflowState.availableHeight));
  rv = ReflowChild(aNextFrame, aPresContext, desiredSize, kidReflowState, 0, 0, 0, aStatus);
  aNextFrame->DidReflow(aPresContext, nsnull, NS_FRAME_REFLOW_FINISHED);
  if (NS_FAILED(rv))
    return rv;

  nsTableFrame *tableFrame=nsnull;
  rv = nsTableFrame::GetTableFrame(this, tableFrame);
  if (tableFrame) {
    // compare the new col count to the old col count.  
    // If they are the same, we just need to rebalance column widths
    // If they differ, we need to fix up other column groups and the column cache
    // XXX for now assume the worse
    tableFrame->SetNeedStrategyInit(PR_TRUE);
  }
  return rv;
}

nsTableColFrame * nsTableColGroupFrame::GetFirstColumn()
{
  return GetNextColumn(nsnull);
}

nsTableColFrame * nsTableColGroupFrame::GetNextColumn(nsIFrame *aChildFrame)
{
  nsTableColFrame *result = nsnull;
  nsIFrame *childFrame = aChildFrame;
  if (nsnull==childFrame)
    childFrame = mFrames.FirstChild();
  while (nsnull!=childFrame)
  {
    if (NS_STYLE_DISPLAY_TABLE_COLUMN ==
        childFrame->GetStyleDisplay()->mDisplay)
    {
      result = (nsTableColFrame *)childFrame;
      break;
    }
    childFrame = childFrame->GetNextSibling();
  }
  return result;
}


nsTableColFrame * nsTableColGroupFrame::GetColumnAt (PRInt32 aColIndex)
{
  nsTableColFrame *result = nsnull;
  PRInt32 count = 0;
  nsIFrame *childFrame = mFrames.FirstChild();

  while (nsnull!=childFrame) {
    if (NS_STYLE_DISPLAY_TABLE_COLUMN ==
        childFrame->GetStyleDisplay()->mDisplay) {
      nsTableColFrame *col = (nsTableColFrame *)childFrame;
      count++;
      if (aColIndex<=count) {
        result = col;
      }
    }
    childFrame = childFrame->GetNextSibling();
  }

  return result;
}

PRInt32 nsTableColGroupFrame::GetSpan()
{
  PRInt32 span = 1;
  nsIContent* iContent = GetContent();
  if (!iContent) return NS_OK;

  // col group element derives from col element
  nsIDOMHTMLTableColElement* cgContent = nsnull;
  nsresult rv = iContent->QueryInterface(NS_GET_IID(nsIDOMHTMLTableColElement), 
                                         (void **)&cgContent);
  if (cgContent && NS_SUCCEEDED(rv)) { 
    cgContent->GetSpan(&span);
    // XXX why does this work!!
    if (span == -1) {
      span = 1;
    }
    NS_RELEASE(cgContent);
  }
  return span;
}

/** returns colcount because it is frequently used in the context of 
  * shuffling relative colgroup order, and it's convenient to not have to
  * call GetColumnCount redundantly.
  */
PRInt32 nsTableColGroupFrame::SetStartColumnIndex (int aIndex)
{
  PRInt32 result = mColCount;
  if (aIndex != mStartColIndex) {  
    mStartColIndex = aIndex;
    result = GetColCount(); 
  }
  return result;
}

void nsTableColGroupFrame::DeleteColFrame(nsIPresContext* aPresContext, nsTableColFrame* aColFrame)
{
  mFrames.DestroyFrame(aPresContext, aColFrame);
}


void nsTableColGroupFrame::SetContinuousBCBorderWidth(PRUint8     aForSide,
                                                      BCPixelSize aPixelValue)
{
  switch (aForSide) {
    case NS_SIDE_TOP:
      mTopContBorderWidth = aPixelValue;
      return;
    case NS_SIDE_BOTTOM:
      mBottomContBorderWidth = aPixelValue;
      return;
    default:
      NS_ERROR("invalid side arg");
  }
}

void nsTableColGroupFrame::GetContinuousBCBorderWidth(float     aPixelsToTwips,
                                                      nsMargin& aBorder)
{
  nsTableFrame* table;
  nsTableFrame::GetTableFrame(this, table);
  nsTableColFrame* col = table->GetColFrame(mStartColIndex + mColCount - 1);
  col->GetContinuousBCBorderWidth(aPixelsToTwips, aBorder);
  aBorder.top = BC_BORDER_BOTTOM_HALF_COORD(aPixelsToTwips,
                                            mTopContBorderWidth);
  aBorder.bottom = BC_BORDER_TOP_HALF_COORD(aPixelsToTwips,
                                            mBottomContBorderWidth);
  return;
}

/* ----- global methods ----- */

nsresult 
NS_NewTableColGroupFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTableColGroupFrame* it = new (aPresShell) nsTableColGroupFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

NS_IMETHODIMP
nsTableColGroupFrame::Init(nsIPresContext*  aPresContext,
                           nsIContent*      aContent,
                           nsIFrame*        aParent,
                           nsStyleContext*  aContext,
                           nsIFrame*        aPrevInFlow)
{
  nsresult  rv;

  // Let the base class do its processing
  rv = nsHTMLContainerFrame::Init(aPresContext, aContent, aParent, aContext,
                                  aPrevInFlow);

  // record that children that are ignorable whitespace should be excluded 
  mState |= NS_FRAME_EXCLUDE_IGNORABLE_WHITESPACE;

  return rv;
}

nsIAtom*
nsTableColGroupFrame::GetType() const
{
  return nsLayoutAtoms::tableColGroupFrame;
}

#ifdef DEBUG
NS_IMETHODIMP
nsTableColGroupFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableColGroup"), aResult);
}
#endif
