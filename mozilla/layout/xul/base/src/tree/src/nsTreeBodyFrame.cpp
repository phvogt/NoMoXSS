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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 * Contributor(s):
 *  Ben Goodger <ben@netscape.com>
 *  Joe Hewitt <hewitt@netscape.com>
 *  Jan Varga <varga@nixcorp.com>
 *  Dean Tessman <dean_tessman@hotmail.com>
 *  Brian Ryner <bryner@brianryner.com>
 *  Blake Ross <blaker@netscape.com>
 *  Pierre Chanial <pierrechanial@netscape.net>
 *  Rene Pronk <r.pronk@its.tudelft.nl>
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
#include "nsISupportsArray.h"
#include "nsIPresContext.h"
#include "nsINameSpaceManager.h"
#include "nsIScrollbarFrame.h"

#include "nsTreeBodyFrame.h"
#include "nsTreeSelection.h"

#include "nsXULAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsHTMLAtoms.h"

#include "nsIContent.h"
#include "nsStyleContext.h"
#include "nsIBoxObject.h"
#include "nsGUIEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMXULElement.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsICSSStyleRule.h"
#include "nsCSSRendering.h"
#include "nsIFontMetrics.h"
#include "nsIDeviceContext.h"
#include "nsIXULTemplateBuilder.h"
#include "nsXPIDLString.h"
#include "nsHTMLContainerFrame.h"
#include "nsIView.h"
#include "nsWidgetsCID.h"
#include "nsBoxFrame.h"
#include "nsBoxObject.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsBoxLayoutState.h"
#include "nsIDragService.h"
#include "nsTreeContentView.h"
#include "nsTreeUtils.h"
#include "nsChildIterator.h"
#include "nsIScrollableView.h"
#include "nsITheme.h"
#include "nsITimelineService.h"
#include "nsITimerInternal.h"
#include "imgIRequest.h"
#include "imgIContainer.h"
#include "imgIContainerObserver.h"
#include "imgILoader.h"
#include "nsStyleSet.h"

#ifdef IBMBIDI
#include "nsBidiPresUtils.h"
#endif

#define ELLIPSIS "..."

// Enumeration function that cancels all the image requests in our cache
PR_STATIC_CALLBACK(PRBool)
CancelImageRequest(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsISupports* supports = NS_STATIC_CAST(nsISupports*, aData);
  nsCOMPtr<imgIRequest> request = do_QueryInterface(supports);
  nsCOMPtr<imgIDecoderObserver> observer;
  request->GetDecoderObserver(getter_AddRefs(observer));
  NS_ASSERTION(observer, "No observer?  We're leaking!");
  request->Cancel(NS_ERROR_FAILURE);
  imgIDecoderObserver* observer2 = observer;
  NS_RELEASE(observer2);  // Balance out the addref from GetImage()
  return PR_TRUE;
}

// The style context cache impl
nsStyleContext*
nsTreeStyleCache::GetStyleContext(nsICSSPseudoComparator* aComparator,
                                  nsIPresContext* aPresContext,
                                  nsIContent* aContent, 
                                  nsStyleContext* aContext,
                                  nsIAtom* aPseudoElement,
                                  nsISupportsArray* aInputWord)
{
  PRUint32 count;
  aInputWord->Count(&count);
  nsDFAState startState(0);
  nsDFAState* currState = &startState;

  // Go ahead and init the transition table.
  if (!mTransitionTable) {
    // Automatic miss. Build the table
    mTransitionTable =
      new nsObjectHashtable(nsnull, nsnull, DeleteDFAState, nsnull);
    if (!mTransitionTable)
      return nsnull;
  }

  // The first transition is always made off the supplied pseudo-element.
  nsTransitionKey key(currState->GetStateID(), aPseudoElement);
  currState = NS_STATIC_CAST(nsDFAState*, mTransitionTable->Get(&key));

  if (!currState) {
    // We had a miss. Make a new state and add it to our hash.
    currState = new nsDFAState(mNextState);
    if (!currState)
      return nsnull;
    mNextState++;
    mTransitionTable->Put(&key, currState);
  }

  for (PRUint32 i = 0; i < count; i++)
  {
    nsCOMPtr<nsIAtom> pseudo = getter_AddRefs(NS_STATIC_CAST(nsIAtom*, aInputWord->ElementAt(i)));
    nsTransitionKey key(currState->GetStateID(), pseudo);
    currState = NS_STATIC_CAST(nsDFAState*, mTransitionTable->Get(&key));

    if (!currState) {
      // We had a miss. Make a new state and add it to our hash.
      currState = new nsDFAState(mNextState);
      if (!currState)
        return nsnull;

      mNextState++;
      mTransitionTable->Put(&key, currState);
    }
  }

  // We're in a final state.
  // Look up our style context for this state.
  nsStyleContext* result = nsnull;
  if (mCache)
    result = NS_STATIC_CAST(nsStyleContext*, mCache->Get(currState));
  if (!result) {
    // We missed the cache. Resolve this pseudo-style.
    result = aPresContext->StyleSet()->
      ResolvePseudoStyleFor(aContent, aPseudoElement,
                            aContext, aComparator).get();

    // Put the style context in our table, transferring the owning reference to the table.
    if (!mCache) {
      mCache = new nsObjectHashtable(nsnull, nsnull, ReleaseStyleContext, nsnull);
      if (!mCache)
        return nsnull;
    }
    mCache->Put(currState, result);
  }

  return result;
}

/* static */ PRBool PR_CALLBACK
nsTreeStyleCache::DeleteDFAState(nsHashKey *aKey,
                                     void *aData,
                                     void *closure)
{
  nsDFAState* entry = NS_STATIC_CAST(nsDFAState*, aData);
  delete entry;
  return PR_TRUE;
}

/* static */ PRBool PR_CALLBACK
nsTreeStyleCache::ReleaseStyleContext(nsHashKey *aKey,
                                      void *aData,
                                      void *closure)
{
  nsStyleContext* context = NS_STATIC_CAST(nsStyleContext*, aData);
  context->Release();
  return PR_TRUE;
}

// Column class that caches all the info about our column.
nsTreeColumn::nsTreeColumn(nsIContent* aColElement, nsIFrame* aFrame)
:mNext(nsnull)
{
  mColFrame = aFrame;
  mColElement = aColElement; 

  // Fetch the ID.
  mColElement->GetAttr(kNameSpaceID_None, nsHTMLAtoms::id, mID);

  // If we have an ID, cache the ID as an atom.
  if (!mID.IsEmpty()) {
    mIDAtom = do_GetAtom(mID);
  }

  const nsStyleVisibility* vis = aFrame->GetStyleVisibility();

  // Fetch the crop style.
  mCropStyle = 0;
  nsAutoString crop;
  mColElement->GetAttr(kNameSpaceID_None, nsXULAtoms::crop, crop);
  if (crop.Equals(NS_LITERAL_STRING("center")))
    mCropStyle = 1;
  else if (crop.Equals(NS_LITERAL_STRING("left")) ||
           crop.Equals(NS_LITERAL_STRING("start")))
    mCropStyle = 2;

  // Cache our text alignment policy.
  const nsStyleText* textStyle = aFrame->GetStyleText();

  mTextAlignment = textStyle->mTextAlign;
  if (mTextAlignment == 0 || mTextAlignment == 2) { // Left or Right
    if (vis->mDirection == NS_STYLE_DIRECTION_RTL)
      mTextAlignment = 2 - mTextAlignment; // Right becomes left, left becomes right.
  }

  // Figure out if we're the primary column (that has to have indentation
  // and twisties drawn.
  mIsPrimaryCol = PR_FALSE;
  nsAutoString primary;
  mColElement->GetAttr(kNameSpaceID_None, nsXULAtoms::primary, primary);
  if (primary.Equals(NS_LITERAL_STRING("true")))
    mIsPrimaryCol = PR_TRUE;

  // Figure out if we're a cycling column (one that doesn't cause a selection
  // to happen).
  mIsCyclerCol = PR_FALSE;
  nsAutoString cycler;
  mColElement->GetAttr(kNameSpaceID_None, nsXULAtoms::cycler, cycler);
  if (cycler.Equals(NS_LITERAL_STRING("true")))
    mIsCyclerCol = PR_TRUE;

  // Figure out our column type. Default type is text.
  mType = eText;
  nsAutoString type;
  mColElement->GetAttr(kNameSpaceID_None, nsHTMLAtoms::type, type);
  if (type.Equals(NS_LITERAL_STRING("checkbox")))
    mType = eCheckbox;
  else if (type.Equals(NS_LITERAL_STRING("progressmeter")))
    mType = eProgressMeter;

  // Cache our index.
  mColIndex = -1;
  nsIContent* parent = mColElement->GetParent();
  PRUint32 count = parent->GetChildCount();
  PRInt32 j = 0;
  for (PRUint32 i = 0; i < count; i++) {
    nsIContent *child = parent->GetChildAt(i);
    nsINodeInfo *ni = child->GetNodeInfo();

    if (ni && ni->Equals(nsXULAtoms::treecol, kNameSpaceID_XUL)) {
      if (child == mColElement) {
        mColIndex = j;
        break;
      }
      j++;
    }
  }
}

inline nscoord nsTreeColumn::GetWidth()
{
  if (mColFrame) {
    return mColFrame->GetSize().width;
  }
  return 0;
}

//
// NS_NewTreeFrame
//
// Creates a new tree frame
//
nsresult
NS_NewTreeBodyFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTreeBodyFrame* it = new (aPresShell) nsTreeBodyFrame(aPresShell);
  if (!it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewTreeFrame


//
// QueryInterface
//
NS_INTERFACE_MAP_BEGIN(nsTreeBodyFrame)
  NS_INTERFACE_MAP_ENTRY(nsITreeBoxObject)
  NS_INTERFACE_MAP_ENTRY(nsICSSPseudoComparator)
  NS_INTERFACE_MAP_ENTRY(nsIScrollbarMediator)
  NS_INTERFACE_MAP_ENTRY(nsIReflowCallback)
NS_INTERFACE_MAP_END_INHERITING(nsLeafBoxFrame)



// Constructor
nsTreeBodyFrame::nsTreeBodyFrame(nsIPresShell* aPresShell)
:nsLeafBoxFrame(aPresShell), mPresContext(nsnull), mTreeBoxObject(nsnull), mImageCache(nsnull),
 mColumns(nsnull), mScrollbar(nsnull), mTopRowIndex(0), mRowHeight(0), mIndentation(0), mStringWidth(-1),
 mFocused(PR_FALSE), mColumnsDirty(PR_TRUE), mDropAllowed(PR_FALSE), mHasFixedRowCount(PR_FALSE),
 mVerticalOverflow(PR_FALSE), mImageGuard(PR_FALSE), mReflowCallbackPosted(PR_FALSE),
 mDropRow(-1), mDropOrient(-1), mScrollLines(0), mTimer(nsnull), mValueArray(~PRInt32(0)),
 mUpdateBatchNest(0), mRowCount(0)
{
  NS_NewISupportsArray(getter_AddRefs(mScratchArray));
}

// Destructor
nsTreeBodyFrame::~nsTreeBodyFrame()
{
  if (mImageCache) {
    mImageCache->Enumerate(CancelImageRequest);
    delete mImageCache;
  }
  if (mTimer)
    mTimer->Cancel();
}

NS_IMETHODIMP_(nsrefcnt) 
nsTreeBodyFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsTreeBodyFrame::Release(void)
{
  return NS_OK;
}

static nsIFrame* InitScrollbarFrame(nsIPresContext* aPresContext, nsIFrame* aCurrFrame, nsIScrollbarMediator* aSM)
{
  // Check ourselves
  nsCOMPtr<nsIScrollbarFrame> sf(do_QueryInterface(aCurrFrame));
  if (sf) {
    sf->SetScrollbarMediator(aSM);
    return aCurrFrame;
  }

  nsIFrame* child = aCurrFrame->GetFirstChild(nsnull);
  while (child) {
    nsIFrame* result = InitScrollbarFrame(aPresContext, child, aSM);
    if (result)
      return result;
    child = child->GetNextSibling();
  }

  return nsnull;
}

static void
GetBorderPadding(nsStyleContext* aContext, nsMargin& aMargin)
{
  nsStyleBorderPadding  borderPaddingStyle;
  aContext->GetBorderPaddingFor(borderPaddingStyle);
  borderPaddingStyle.GetBorderPadding(aMargin);
}

static void
AdjustForBorderPadding(nsStyleContext* aContext, nsRect& aRect)
{
  nsMargin borderPadding(0, 0, 0, 0);
  GetBorderPadding(aContext, borderPadding);
  aRect.Deflate(borderPadding);
}

NS_IMETHODIMP
nsTreeBodyFrame::Init(nsIPresContext* aPresContext, nsIContent* aContent,
                          nsIFrame* aParent, nsStyleContext* aContext, nsIFrame* aPrevInFlow)
{
  mPresContext = aPresContext;
  nsresult rv = nsLeafBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  nsBoxFrame::CreateViewForFrame(aPresContext, this, aContext, PR_TRUE);
  nsIView* ourView = nsLeafBoxFrame::GetView();

  static NS_DEFINE_CID(kWidgetCID, NS_CHILD_CID);

  ourView->CreateWidget(kWidgetCID);
  mTreeWidget = ourView->GetWidget();
  mIndentation = GetIndentation();
  mRowHeight = GetRowHeight();
  return rv;
}

NS_IMETHODIMP
nsTreeBodyFrame::GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  EnsureView();

  nsCOMPtr<nsIContent> baseElement;
  GetBaseElement(getter_AddRefs(baseElement));

  PRInt32 desiredRows;
  if (baseElement->Tag() == nsHTMLAtoms::select &&
      baseElement->IsContentOfType(nsIContent::eHTML)) {
    aSize.width = CalcMaxRowWidth(aBoxLayoutState);
    nsAutoString size;
    baseElement->GetAttr(kNameSpaceID_None, nsHTMLAtoms::size, size);
    if (!size.IsEmpty()) {
      PRInt32 err;
      desiredRows = size.ToInteger(&err);
      mHasFixedRowCount = PR_TRUE;
      mPageCount = desiredRows;
    }
    else {
      desiredRows = 1;
    }
  } else {
    // tree
    aSize.width = 0;
    nsAutoString rows;
    baseElement->GetAttr(kNameSpaceID_None, nsXULAtoms::rows, rows);
    if (!rows.IsEmpty()) {
      PRInt32 err;
      desiredRows = rows.ToInteger(&err);
      mPageCount = desiredRows;
    }
    else {
      desiredRows = 0;
    }
  }

  aSize.height = mRowHeight * desiredRows;

  AddBorderAndPadding(aSize);
  AddInset(aSize);
  nsIBox::AddCSSMinSize(aBoxLayoutState, this, aSize);

  return NS_OK;
}

nscoord
nsTreeBodyFrame::CalcMaxRowWidth(nsBoxLayoutState& aState)
{
  if (mStringWidth != -1)
    return mStringWidth;

  if (!mView)
    return 0;

  nsStyleContext* rowContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreerow);
  nsMargin rowMargin(0,0,0,0);
  GetBorderPadding(rowContext, rowMargin);

  nscoord rowWidth;
  nsTreeColumn* col;
  EnsureColumns();

  nsCOMPtr<nsIRenderingContext> rc;
  mPresContext->PresShell()->CreateRenderingContext(this, getter_AddRefs(rc));

  for (PRInt32 row = 0; row < mRowCount; ++row) {
    rowWidth = 0;
    col = mColumns;

    while (col) {
      nscoord desiredWidth, currentWidth;
      GetCellWidth(row, col->GetID(), rc, desiredWidth, currentWidth);
      rowWidth += desiredWidth;
      col = col->GetNext();
    }

    if (rowWidth > mStringWidth)
      mStringWidth = rowWidth;
  }

  mStringWidth += rowMargin.left + rowMargin.right;
  return mStringWidth;
}

NS_IMETHODIMP
nsTreeBodyFrame::Destroy(nsIPresContext* aPresContext)
{
  // Make sure we cancel any posted callbacks. 
  if (mReflowCallbackPosted) {
    aPresContext->PresShell()->CancelReflowCallback(this);
    mReflowCallbackPosted = PR_FALSE;
  }

  // Delete our column structures.
  delete mColumns;
  mColumns = nsnull;
  
  // Save off our info into the box object.
  EnsureBoxObject();
  if (mTreeBoxObject) {
    nsCOMPtr<nsIBoxObject> box(do_QueryInterface(mTreeBoxObject));
    if (mTopRowIndex > 0) {
      nsAutoString topRowStr; topRowStr.Assign(NS_LITERAL_STRING("topRow"));
      nsAutoString topRow;
      topRow.AppendInt(mTopRowIndex);
      box->SetProperty(topRowStr.get(), topRow.get());
    }

    // Always null out the cached tree body frame.
    nsAutoString treeBody(NS_LITERAL_STRING("treebody"));
    box->RemoveProperty(treeBody.get());

    mTreeBoxObject = nsnull; // Drop our ref here.
  }

  if (mView) {
    nsCOMPtr<nsITreeSelection> sel;
    mView->GetSelection(getter_AddRefs(sel));
    if (sel)
      sel->SetTree(nsnull);
    mView->SetTree(nsnull);
    mView = nsnull;
  }

  return nsLeafBoxFrame::Destroy(aPresContext);
}

void
nsTreeBodyFrame::EnsureBoxObject()
{
  if (!mTreeBoxObject) {
    nsCOMPtr<nsIContent> parent;
    GetBaseElement(getter_AddRefs(parent));

    if (parent) {
      nsCOMPtr<nsIDOMNSDocument> nsDoc = do_QueryInterface(parent->GetDocument());
      if (!nsDoc) // there may be no document, if we're called from Destroy()
        return;
      nsCOMPtr<nsIBoxObject> box;
      nsCOMPtr<nsIDOMElement> domElem = do_QueryInterface(parent);
      nsDoc->GetBoxObjectFor(domElem, getter_AddRefs(box));
      
      if (box) {
        nsCOMPtr<nsITreeBoxObject> treeBox(do_QueryInterface(box));
        SetBoxObject(treeBox);
      }
    }
  }
}

void
nsTreeBodyFrame::EnsureView()
{
  if (!mView) {
    EnsureBoxObject();
    nsCOMPtr<nsIBoxObject> box = do_QueryInterface(mTreeBoxObject);
    if (box) {
      nsCOMPtr<nsISupports> suppView;
      box->GetPropertyAsSupports(NS_LITERAL_STRING("view").get(), getter_AddRefs(suppView));
      nsCOMPtr<nsITreeView> treeView(do_QueryInterface(suppView));

      if (treeView) {
        nsXPIDLString rowStr;
        box->GetProperty(NS_LITERAL_STRING("topRow").get(), getter_Copies(rowStr));
        nsAutoString rowStr2(rowStr);
        PRInt32 error;
        PRInt32 rowIndex = rowStr2.ToInteger(&error);

        // Set our view.
        SetView(treeView);

        // Scroll to the given row.
        // XXX is this optimal if we haven't laid out yet?
        ScrollToRow(rowIndex);

        // Clear out the property info for the top row, but we always keep the
        // view current.
        box->RemoveProperty(NS_LITERAL_STRING("topRow").get());
      }
    }

    if (!mView) {
      // If we don't have a box object yet, or no view was set on it,
      // look for a XULTreeBuilder or create a content view.
      
      nsCOMPtr<nsIDOMXULElement> xulele = do_QueryInterface(mContent->GetParent());
      if (xulele) {
        nsCOMPtr<nsITreeView> view;

        // See if there is a XUL tree builder associated with
        // the parent element.
        nsCOMPtr<nsIXULTemplateBuilder> builder;
        xulele->GetBuilder(getter_AddRefs(builder));
        if (builder)
          view = do_QueryInterface(builder);

        if (!view) {
          // No tree builder, create an tree content view.
          nsCOMPtr<nsITreeContentView> contentView;
          NS_NewTreeContentView(getter_AddRefs(contentView));
          if (contentView)
            view = do_QueryInterface(contentView);
        }

        // Hook up the view.
        if (view)
          SetView(view);
      }
    }
  }
}

NS_IMETHODIMP
nsTreeBodyFrame::SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect)
{
  if (aRect != mRect && !mReflowCallbackPosted) {
    mReflowCallbackPosted = PR_TRUE;
    mPresContext->PresShell()->PostReflowCallback(this);
  }

  return nsLeafBoxFrame::SetBounds(aBoxLayoutState, aRect);
}


NS_IMETHODIMP
nsTreeBodyFrame::ReflowFinished(nsIPresShell* aPresShell, PRBool* aFlushFlag)
{
  if (mView) {
    CalcInnerBox();
    if (!mHasFixedRowCount)
      mPageCount = mInnerBox.height / mRowHeight;

    PRInt32 lastPageTopRow = PR_MAX(0, mRowCount - mPageCount);
    if (mTopRowIndex > lastPageTopRow)
      ScrollToRow(lastPageTopRow);

    // make sure that the current selected item is still
    // visible after the tree changes size.
    nsCOMPtr<nsITreeSelection> sel;
    mView->GetSelection(getter_AddRefs(sel));
    if (sel) {
      PRInt32 currentIndex;
      sel->GetCurrentIndex(&currentIndex);
      if (currentIndex != -1)
        EnsureRowIsVisible(currentIndex);
    }

    InvalidateScrollbar();
    CheckVerticalOverflow();
  }

  mReflowCallbackPosted = PR_FALSE;
  *aFlushFlag = PR_FALSE;

  return NS_OK;
}


NS_IMETHODIMP nsTreeBodyFrame::GetView(nsITreeView * *aView)
{
  EnsureView();
  *aView = mView;
  NS_IF_ADDREF(*aView);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::SetView(nsITreeView * aView)
{
  // First clear out the old view.
  EnsureBoxObject();
  nsCOMPtr<nsIBoxObject> box = do_QueryInterface(mTreeBoxObject);
  
  nsAutoString view(NS_LITERAL_STRING("view"));
  
  if (mView) {
    nsCOMPtr<nsITreeSelection> sel;
    mView->GetSelection(getter_AddRefs(sel));
    if (sel)
      sel->SetTree(nsnull);
    mView->SetTree(nsnull);
    mView = nsnull;
    box->RemoveProperty(view.get());

    // Only reset the top row index and delete the columns if we had an old non-null view.
    mTopRowIndex = 0;
    delete mColumns;
    mColumns = nsnull;
  }

  // Tree, meet the view.
  mView = aView;
 
  // Changing the view causes us to refetch our data.  This will
  // necessarily entail a full invalidation of the tree.
  Invalidate();
 
  if (mView) {
    // Give the view a new empty selection object to play with, but only if it
    // doesn't have one already.
    nsCOMPtr<nsITreeSelection> sel;
    mView->GetSelection(getter_AddRefs(sel));
    if (sel) {
      sel->SetTree(mTreeBoxObject);
    } else {
      NS_NewTreeSelection(mTreeBoxObject, getter_AddRefs(sel));
      mView->SetSelection(sel);
    }

    // View, meet the tree.
    mView->SetTree(mTreeBoxObject);
    mView->GetRowCount(&mRowCount);
 
    box->SetPropertyAsSupports(view.get(), mView);

    // The scrollbar will need to be updated.
    InvalidateScrollbar();

    // Reset scrollbar position.
    UpdateScrollbar();

    CheckVerticalOverflow();
  }
 
  return NS_OK;
}

NS_IMETHODIMP 
nsTreeBodyFrame::GetFocused(PRBool* aFocused)
{
  *aFocused = mFocused;
  return NS_OK;
}

NS_IMETHODIMP 
nsTreeBodyFrame::SetFocused(PRBool aFocused)
{
  if (mFocused != aFocused) {
    mFocused = aFocused;
    if (mView) {
      nsCOMPtr<nsITreeSelection> sel;
      mView->GetSelection(getter_AddRefs(sel));
      if (sel)
        sel->InvalidateSelection();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsTreeBodyFrame::GetTreeBody(nsIDOMElement** aElement)
{
  //NS_ASSERTION(mContent, "no content, see bug #104878");
  if (!mContent)
    return NS_ERROR_NULL_POINTER;

  return mContent->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
}

NS_IMETHODIMP nsTreeBodyFrame::GetSelection(nsITreeSelection** aSelection)
{
  if (mView)
    return mView->GetSelection(aSelection);

  *aSelection = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::GetRowHeight(PRInt32* _retval)
{
  float t2p;
  t2p = mPresContext->TwipsToPixels();
  *_retval = NSToCoordRound((float) mRowHeight * t2p);

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::GetColumnIndex(const PRUnichar *aColID, PRInt32 *_retval)
{
  *_retval = -1;
  for (nsTreeColumn* currCol = mColumns; currCol; currCol = currCol->GetNext()) {
    if (currCol->GetID().Equals(aColID)) {
      *_retval = currCol->GetColIndex();
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::GetColumnID(PRInt32 colIndex, nsAString & _retval)
{
  _retval.Truncate();
  for (nsTreeColumn* currCol = mColumns; currCol; currCol = currCol->GetNext()) {
    if (currCol->GetColIndex() == colIndex) {
      _retval = currCol->GetID();
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::GetKeyColumnIndex(PRInt32 *_retval)
{
  nsAutoString attr;
  PRInt32 first, primary, sorted;

  EnsureColumns();
  first = primary = sorted = -1;
  for (nsTreeColumn* currCol = mColumns; currCol; currCol = currCol->GetNext()) {
    // Skip hidden column
    currCol->GetElement()->GetAttr(kNameSpaceID_None, nsHTMLAtoms::hidden, attr);
    if (attr.Equals(NS_LITERAL_STRING("true")))
      continue;

    // Skip non-text column
    if (currCol->GetType() != nsTreeColumn::eText)
      continue;

    if (first == -1)
      first = currCol->GetColIndex();
    
    currCol->GetElement()->GetAttr(kNameSpaceID_None, nsXULAtoms::sortDirection, attr);
    if (!attr.IsEmpty()) { // Use sorted column as the primary
      sorted = currCol->GetColIndex();
      break;
    }

    if (currCol->IsPrimary())
      if (primary == -1)
        primary = currCol->GetColIndex();
  }

  if (sorted >= 0)
    *_retval = sorted;
  else if (primary >= 0)
    *_retval = primary;
  else
    *_retval = first;

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::GetFirstVisibleRow(PRInt32 *_retval)
{
  *_retval = mTopRowIndex;
  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::GetLastVisibleRow(PRInt32 *_retval)
{
  *_retval = mTopRowIndex + mPageCount;
  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::GetPageCount(PRInt32 *_retval)
{
  *_retval = mPageCount;
  return NS_OK;
}


NS_IMETHODIMP nsTreeBodyFrame::Invalidate()
{
  if (mUpdateBatchNest)
    return NS_OK;
  nsIFrame::Invalidate(GetOutlineRect(), PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::InvalidateColumn(const PRUnichar *aColID)
{
  if (mUpdateBatchNest)
    return NS_OK;
  nscoord currX = mInnerBox.x;
  for (nsTreeColumn* currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width; 
       currCol = currCol->GetNext()) {
    if (currCol->GetID().Equals(aColID)) {
      nsRect columnRect(currX, mInnerBox.y, currCol->GetWidth(), mInnerBox.height);
      nsIFrame::Invalidate(columnRect, PR_FALSE);
      break;
    }
    currX += currCol->GetWidth();
  }

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::InvalidateRow(PRInt32 aIndex)
{
  if (mUpdateBatchNest)
    return NS_OK;
  if (aIndex < mTopRowIndex || aIndex > mTopRowIndex + mPageCount + 1)
    return NS_OK;

  nsRect rowRect(mInnerBox.x, mInnerBox.y+mRowHeight*(aIndex-mTopRowIndex), mInnerBox.width, mRowHeight);
  nsIFrame::Invalidate(rowRect, PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::InvalidateCell(PRInt32 aIndex, const PRUnichar *aColID)
{
  if (mUpdateBatchNest)
    return NS_OK;
  if (aIndex < mTopRowIndex || aIndex > mTopRowIndex + mPageCount + 1)
    return NS_OK;

  if (mImageGuard)
    return NS_OK;

  nscoord currX = mInnerBox.x;
  nscoord yPos = mInnerBox.y+mRowHeight*(aIndex-mTopRowIndex);
  for (nsTreeColumn* currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width; 
       currCol = currCol->GetNext()) {

    if (currCol->GetID().Equals(aColID)) {
      nsRect cellRect(currX, yPos, currCol->GetWidth(), mRowHeight);
      nsIFrame::Invalidate(cellRect, PR_FALSE);
      break;
    }
    currX += currCol->GetWidth();
  }
  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::InvalidatePrimaryCell(PRInt32 aIndex)
{
  if (mUpdateBatchNest)
    return NS_OK;
  if (aIndex < mTopRowIndex || aIndex > mTopRowIndex + mPageCount + 1)
    return NS_OK;

  nscoord currX = mInnerBox.x;
  nscoord yPos = mInnerBox.y+mRowHeight*(aIndex-mTopRowIndex);
  for (nsTreeColumn* currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width; 
       currCol = currCol->GetNext()) {

    if (currCol->IsPrimary()) {
      nsRect cellRect(currX, yPos, currCol->GetWidth(), mRowHeight);
#if defined(XP_MAC) || defined(XP_MACOSX)
      // Mac can't process the event loop during a drag, so if we're dragging,
      // invalidate synchronously.
      nsIFrame::Invalidate(cellRect, mDragSession != nsnull);
#else
      nsIFrame::Invalidate(cellRect, PR_FALSE);
#endif
      break;
    }
    currX += currCol->GetWidth();
  }
  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::InvalidateRange(PRInt32 aStart, PRInt32 aEnd)
{
  if (mUpdateBatchNest)
    return NS_OK;
  if (aStart == aEnd)
    return InvalidateRow(aStart);

  PRInt32 last;
  GetLastVisibleRow(&last);
  if (aEnd < mTopRowIndex || aStart > last)
    return NS_OK;

  if (aStart < mTopRowIndex)
    aStart = mTopRowIndex;

  if (aEnd > last)
    aEnd = last;

  nsRect rangeRect(mInnerBox.x, mInnerBox.y+mRowHeight*(aStart-mTopRowIndex), mInnerBox.width, mRowHeight*(aEnd-aStart+1));
  nsIFrame::Invalidate(rangeRect, PR_FALSE);

  return NS_OK;
}

nsIFrame*
nsTreeBodyFrame::EnsureScrollbar()
{
  if (!mScrollbar) {
    // Try to find it.
    nsCOMPtr<nsIContent> parContent;
    GetBaseElement(getter_AddRefs(parContent));
    nsIFrame* treeFrame;
    mPresContext->PresShell()->GetPrimaryFrameFor(parContent, &treeFrame);
    if (treeFrame)
      mScrollbar = InitScrollbarFrame(mPresContext, treeFrame, this);
  }

  NS_ASSERTION(mScrollbar, "no scroll bar");
  return mScrollbar;
}

void
nsTreeBodyFrame::UpdateScrollbar()
{
  // Update the scrollbar.
  if (!EnsureScrollbar())
    return;
  float t2p;
  t2p = mPresContext->TwipsToPixels();
  nscoord rowHeightAsPixels = NSToCoordRound((float)mRowHeight*t2p);

  nsAutoString curPos;
  curPos.AppendInt(mTopRowIndex*rowHeightAsPixels);
  mScrollbar->GetContent()->SetAttr(kNameSpaceID_None, nsXULAtoms::curpos, curPos, PR_TRUE);
}

nsresult nsTreeBodyFrame::CheckVerticalOverflow()
{
  PRBool verticalOverflowChanged = PR_FALSE;

  if (!mVerticalOverflow && mRowCount > mPageCount) {
    mVerticalOverflow = PR_TRUE;
    verticalOverflowChanged = PR_TRUE;
  }
  else if (mVerticalOverflow && mRowCount <= mPageCount) {
    mVerticalOverflow = PR_FALSE;
    verticalOverflowChanged = PR_TRUE;
  }
 
  if (verticalOverflowChanged) {
    nsScrollPortEvent event(mVerticalOverflow ? NS_SCROLLPORT_OVERFLOW
                            : NS_SCROLLPORT_UNDERFLOW);
    event.orient = nsScrollPortEvent::vertical;

    nsEventStatus status = nsEventStatus_eIgnore;
    mContent->HandleDOMEvent(mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
  }

  return NS_OK;
}

void nsTreeBodyFrame::InvalidateScrollbar()
{
  if (mUpdateBatchNest || !mView || mRowCount <= mPageCount || !EnsureScrollbar())
    return;

  nsIContent* scrollbar = mScrollbar->GetContent();

  nsAutoString maxposStr;

  float t2p;
  t2p = mPresContext->TwipsToPixels();
  nscoord rowHeightAsPixels = NSToCoordRound((float)mRowHeight*t2p);

  PRInt32 size = rowHeightAsPixels*(mRowCount-mPageCount);
  maxposStr.AppendInt(size);
  scrollbar->SetAttr(kNameSpaceID_None, nsXULAtoms::maxpos, maxposStr, PR_TRUE);

  // Also set our page increment and decrement.
  nscoord pageincrement = mPageCount*rowHeightAsPixels;
  nsAutoString pageStr;
  pageStr.AppendInt(pageincrement);
  scrollbar->SetAttr(kNameSpaceID_None, nsXULAtoms::pageincrement, pageStr, PR_TRUE);
}


// Takes client x/y in pixels, converts them to twips, and massages them to be
// in our coordinate system.
void
nsTreeBodyFrame::AdjustEventCoordsToBoxCoordSpace (PRInt32 aX, PRInt32 aY, PRInt32* aResultX, PRInt32* aResultY)
{
  // Convert our x and y coords to twips.
  float pixelsToTwips = 0.0;
  pixelsToTwips = mPresContext->PixelsToTwips();
  aX = NSToIntRound(aX * pixelsToTwips);
  aY = NSToIntRound(aY * pixelsToTwips);
  
  // Get our box object.
  nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(mContent->GetDocument()));
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(mContent));
  
  nsCOMPtr<nsIBoxObject> boxObject;
  nsDoc->GetBoxObjectFor(elt, getter_AddRefs(boxObject));
  
  PRInt32 x;
  PRInt32 y;
  boxObject->GetX(&x);
  boxObject->GetY(&y);

  x = NSToIntRound(x * pixelsToTwips);
  y = NSToIntRound(y * pixelsToTwips);

  // Take into account the parent's scroll offset, since clientX and clientY
  // are relative to the viewport.

  nsIView* parentView = nsLeafBoxFrame::GetView()->GetParent()->GetParent();

  if (parentView) {
    nsIScrollableView* scrollView = nsnull;
    CallQueryInterface(parentView, &scrollView);
    if (scrollView) {
      nscoord scrollX = 0, scrollY = 0;
      scrollView->GetScrollPosition(scrollX, scrollY);
      x -= scrollX;
      y -= scrollY;
    }
  }

  // Adjust into our coordinate space.
  x = aX-x;
  y = aY-y;

  // Adjust y by the inner box y, so that we're in the inner box's
  // coordinate space.
  y += mInnerBox.y;
  
  *aResultX = x;
  *aResultY = y;
} // AdjustEventCoordsToBoxCoordSpace

NS_IMETHODIMP nsTreeBodyFrame::GetRowAt(PRInt32 aX, PRInt32 aY, PRInt32* _retval)
{
  if (!mView)
    return NS_OK;

  PRInt32 x;
  PRInt32 y;
  AdjustEventCoordsToBoxCoordSpace (aX, aY, &x, &y);

  // Check if the coordinates are above our visible space.
  if (y < 0) {
    *_retval = -1;
    return NS_OK;
  }

  // Now just mod by our total inner box height and add to our top row index.
  *_retval = (y/mRowHeight)+mTopRowIndex;

  // Check if the coordinates are below our visible space (or within 
  // our visible space but below any row).
  if (*_retval > PR_MIN(mTopRowIndex+mPageCount, mRowCount - 1))
    *_retval = -1;

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::GetCellAt(PRInt32 aX, PRInt32 aY, PRInt32* aRow, PRUnichar** aColID,
                                             PRUnichar** aChildElt)
{
  if (!mView)
    return NS_OK;

  PRInt32 x;
  PRInt32 y;
  AdjustEventCoordsToBoxCoordSpace (aX, aY, &x, &y);

  // Check if the coordinates are above our visible space.
  if (y < 0) {
    *aRow = -1;
    return NS_OK;
  }

  // Now just mod by our total inner box height and add to our top row index.
  *aRow = (y/mRowHeight)+mTopRowIndex;

  // Check if the coordinates are below our visible space (or within 
  // our visible space but below any row).
  if (*aRow > PR_MIN(mTopRowIndex+mPageCount, mRowCount - 1)) {
    *aRow = -1;
    return NS_OK;
  }

  // Determine the column hit.
  nscoord currX = mInnerBox.x;
  for (nsTreeColumn* currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width; 
       currCol = currCol->GetNext()) {
    nsRect cellRect(currX, mInnerBox.y+mRowHeight*(*aRow-mTopRowIndex), currCol->GetWidth(), mRowHeight);
    PRInt32 overflow = cellRect.x+cellRect.width-(mInnerBox.x+mInnerBox.width);
    if (overflow > 0)
      cellRect.width -= overflow;

    if (x >= cellRect.x && x < cellRect.x + cellRect.width) {
      // We know the column hit now.
      *aColID = ToNewUnicode(currCol->GetID());

      if (currCol->IsCycler())
        // Cyclers contain only images.  Fill this in immediately and return.
        *aChildElt = ToNewUnicode(NS_LITERAL_STRING("image"));
      else
        GetItemWithinCellAt(x, cellRect, *aRow, currCol, aChildElt);
      break;
    }

    currX += cellRect.width;
  }

  return NS_OK;
}


//
// GetCoordsForCellItem
//
// Find the x/y location and width/height (all in PIXELS) of the given object
// in the given column. 
//
// XXX IMPORTANT XXX:
// Hyatt says in the bug for this, that the following needs to be done:
// (1) You need to deal with overflow when computing cell rects.  See other column 
// iteration examples... if you don't deal with this, you'll mistakenly extend the 
// cell into the scrollbar's rect.
//
// (2) You are adjusting the cell rect by the *row" border padding.  That's 
// wrong.  You need to first adjust a row rect by its border/padding, and then the 
// cell rect fits inside the adjusted row rect.  It also can have border/padding 
// as well as margins.  The vertical direction isn't that important, but you need 
// to get the horizontal direction right.
//
// (3) GetImageSize() does not include margins (but it does include border/padding).  
// You need to make sure to add in the image's margins as well.
//
NS_IMETHODIMP
nsTreeBodyFrame::GetCoordsForCellItem(PRInt32 aRow, const PRUnichar *aColID, const PRUnichar *aCellItem, 
                                          PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
  *aX = 0;
  *aY = 0;
  *aWidth = 0;
  *aHeight = 0;

  nscoord currX = mInnerBox.x;

  // The Rect for the requested item. 
  nsRect theRect;

  for (nsTreeColumn* currCol = mColumns; currCol && currX < mInnerBox.x + mInnerBox.width;
       currCol = currCol->GetNext()) {

    // The Rect for the current cell. 
    nsRect cellRect(currX, mInnerBox.y + mRowHeight * (aRow - mTopRowIndex), currCol->GetWidth(), mRowHeight);

    // Check the ID of the current column to see if it matches. If it doesn't 
    // increment the current X value and continue to the next column.
    if (!currCol->GetID().Equals(aColID)) {
      currX += cellRect.width;
      continue;
    }

    // Now obtain the properties for our cell.
    PrefillPropertyArray(aRow, currCol);
    mView->GetCellProperties(aRow, currCol->GetID().get(), mScratchArray);

    nsStyleContext* rowContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreerow);

    // We don't want to consider any of the decorations that may be present
    // on the current row, so we have to deflate the rect by the border and 
    // padding and offset its left and top coordinates appropriately. 
    AdjustForBorderPadding(rowContext, cellRect);

    nsStyleContext* cellContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecell);

    NS_NAMED_LITERAL_STRING(cell, "cell");
    if (currCol->IsCycler() || cell.Equals(aCellItem)) {
      // If the current Column is a Cycler, then the Rect is just the cell - the margins. 
      // Similarly, if we're just being asked for the cell rect, provide it. 

      theRect = cellRect;
      nsMargin cellMargin;
      cellContext->GetStyleMargin()->GetMargin(cellMargin);
      theRect.Deflate(cellMargin);
      break;
    }

    // Since we're not looking for the cell, and since the cell isn't a cycler,
    // we're looking for some subcomponent, and now we need to subtract the 
    // borders and padding of the cell from cellRect so this does not 
    // interfere with our computations.
    AdjustForBorderPadding(cellContext, cellRect);

    // Now we'll start making our way across the cell, starting at the edge of 
    // the cell and proceeding until we hit the right edge. |cellX| is the 
    // working X value that we will increment as we crawl from left to right.
    nscoord cellX = cellRect.x;
    nscoord remainWidth = cellRect.width;

    if (currCol->IsPrimary()) {
      // If the current Column is a Primary, then we need to take into account the indentation
      // and possibly a twisty. 

      // The amount of indentation is the indentation width (|mIndentation|) by the level. 
      PRInt32 level;
      mView->GetLevel(aRow, &level);
      cellX += mIndentation * level;
      remainWidth -= mIndentation * level;

      PRBool hasTwisty = PR_FALSE;
      PRBool isContainer = PR_FALSE;
      mView->IsContainer(aRow, &isContainer);
      if (isContainer) {
        PRBool isContainerEmpty = PR_FALSE;
        mView->IsContainerEmpty(aRow, &isContainerEmpty);
        if (!isContainerEmpty)
          hasTwisty = PR_TRUE;
      }

      // Find the twisty rect by computing its size. 
      nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);

      // |GetImageSize| returns the rect of the twisty image, including the 
      // borders and padding. 
      nsRect twistyImageRect = GetImageSize(aRow, currCol->GetID().get(), PR_TRUE, twistyContext);
      if (NS_LITERAL_STRING("twisty").Equals(aCellItem)) {
        // If we're looking for the twisty Rect, just return the result of |GetImageSize|
        theRect = twistyImageRect;
        break;
      }
      
      // Now we need to add in the margins of the twisty element, so that we 
      // can find the offset of the next element in the cell. 
      nsMargin twistyMargin;
      twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
      twistyImageRect.Inflate(twistyMargin);

      // Adjust our working X value with the twisty width (image size, margins,
      // borders, padding. 
      cellX += twistyImageRect.width;
    }

    // Cell Image
    nsStyleContext* imageContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeimage);

    nsRect imageSize = GetImageSize(aRow, currCol->GetID().get(), PR_FALSE, imageContext);
    if (NS_LITERAL_STRING("image").Equals(aCellItem)) {
      theRect = imageSize;
      theRect.x = cellX;
      theRect.y = cellRect.y;
      break;
    }

    // Add in the margins of the cell image.
    nsMargin imageMargin;
    imageContext->GetStyleMargin()->GetMargin(imageMargin);
    imageSize.Inflate(imageMargin);

    // Increment cellX by the image width
    cellX += imageSize.width;
    
    // Cell Text 
    nsAutoString cellText;
    mView->GetCellText(aRow, currCol->GetID().get(), cellText);

    // Create a scratch rect to represent the text rectangle, with the current 
    // X and Y coords, and a guess at the width and height. The width is the 
    // remaining width we have left to traverse in the cell, which will be the
    // widest possible value for the text rect, and the row height. 
    nsRect textRect(cellX, cellRect.y, remainWidth, mRowHeight);

    // Measure the width of the text. If the width of the text is greater than 
    // the remaining width available, then we just assume that the text has 
    // been cropped and use the remaining rect as the text Rect. Otherwise,
    // we add in borders and padding to the text dimension and give that back. 
    nsStyleContext* textContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecelltext);

    nsCOMPtr<nsIFontMetrics> fm;
    mPresContext->DeviceContext()->
      GetMetricsFor(textContext->GetStyleFont()->mFont, *getter_AddRefs(fm));
    nscoord height;
    fm->GetHeight(height);

    nsMargin bp(0,0,0,0);
    GetBorderPadding(textContext, bp);
    
    textRect.height = height + bp.top + bp.bottom;

    nsCOMPtr<nsIRenderingContext> rc;
    mPresContext->PresShell()->CreateRenderingContext(this,
                                                      getter_AddRefs(rc));
    rc->SetFont(fm);
    nscoord width;
    rc->GetWidth(cellText, width);

    nscoord totalTextWidth = width + bp.left + bp.right;
    if (totalTextWidth < remainWidth) {
      // If the text is not cropped, the text is smaller than the available 
      // space and we set the text rect to be that width. 
      textRect.width = totalTextWidth;
    }

    theRect = textRect;
  }
  
  float t2p = 0.0;
  t2p = mPresContext->TwipsToPixels();
  
  *aX = NSToIntRound(theRect.x * t2p);
  *aY = NSToIntRound(theRect.y * t2p);
  *aWidth = NSToIntRound(theRect.width * t2p);
  *aHeight = NSToIntRound(theRect.height * t2p);
 
  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetItemWithinCellAt(PRInt32 aX, const nsRect& aCellRect, 
                                         PRInt32 aRowIndex,
                                         nsTreeColumn* aColumn, PRUnichar** aChildElt)
{
  // Obtain the properties for our cell.
  PrefillPropertyArray(aRowIndex, aColumn);
  mView->GetCellProperties(aRowIndex, aColumn->GetID().get(), mScratchArray);

  // Resolve style for the cell.
  nsStyleContext* cellContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecell);

  // Obtain the margins for the cell and then deflate our rect by that 
  // amount.  The cell is assumed to be contained within the deflated rect.
  nsRect cellRect(aCellRect);
  nsMargin cellMargin;
  cellContext->GetStyleMargin()->GetMargin(cellMargin);
  cellRect.Deflate(cellMargin);

  // Adjust the rect for its border and padding.
  AdjustForBorderPadding(cellContext, cellRect);

  if (aX < cellRect.x || aX >= cellRect.x + cellRect.width) {
    // The user clicked within the cell's margins/borders/padding.  This constitutes a click on the cell.
    *aChildElt = ToNewUnicode(NS_LITERAL_STRING("cell"));
    return NS_OK;
  }

  nscoord currX = cellRect.x;
  nscoord remainingWidth = cellRect.width;

  // XXX Handle right alignment hit testing.

  if (aColumn->IsPrimary()) {
    // If we're the primary column, we have indentation and a twisty.
    PRInt32 level;
    mView->GetLevel(aRowIndex, &level);

    currX += mIndentation*level;
    remainingWidth -= mIndentation*level;

    if (aX < currX) {
      // The user clicked within the indentation.
      *aChildElt = ToNewUnicode(NS_LITERAL_STRING("cell"));
      return NS_OK;
    }

    // Always leave space for the twisty.
    nsRect twistyRect(currX, cellRect.y, remainingWidth, cellRect.height);
    PRBool hasTwisty = PR_FALSE;
    PRBool isContainer = PR_FALSE;
    mView->IsContainer(aRowIndex, &isContainer);
    if (isContainer) {
      PRBool isContainerEmpty = PR_FALSE;
      mView->IsContainerEmpty(aRowIndex, &isContainerEmpty);
      if (!isContainerEmpty)
        hasTwisty = PR_TRUE;
    }

    // Resolve style for the twisty.
    nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);

    // We will treat a click as hitting the twisty if it happens on the margins, borders, padding,
    // or content of the twisty object.  By allowing a "slop" into the margin, we make it a little
    // bit easier for a user to hit the twisty.  (We don't want to be too picky here.)
    nsRect imageSize = GetImageSize(aRowIndex, aColumn->GetID().get(), PR_TRUE, twistyContext);
    nsMargin twistyMargin;
    twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
    imageSize.Inflate(twistyMargin);
    twistyRect.width = imageSize.width;

    // Now we test to see if aX is actually within the twistyRect.  If it is, and if the item should
    // have a twisty, then we return "twisty".  If it is within the rect but we shouldn't have a twisty,
    // then we return "cell".
    if (aX >= twistyRect.x && aX < twistyRect.x + twistyRect.width) {
      if (hasTwisty)
        *aChildElt = ToNewUnicode(NS_LITERAL_STRING("twisty"));
      else
        *aChildElt = ToNewUnicode(NS_LITERAL_STRING("cell"));
      return NS_OK;
    }

    currX += twistyRect.width;
    remainingWidth -= twistyRect.width;    
  }
  
  // Now test to see if the user hit the icon for the cell.
  nsRect iconRect(currX, cellRect.y, remainingWidth, cellRect.height);
  
  // Resolve style for the image.
  nsStyleContext* imageContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeimage);

  nsRect iconSize = GetImageSize(aRowIndex, aColumn->GetID().get(), PR_FALSE, imageContext);
  nsMargin imageMargin;
  imageContext->GetStyleMargin()->GetMargin(imageMargin);
  iconSize.Inflate(imageMargin);
  iconRect.width = iconSize.width;

  if (aX >= iconRect.x && aX < iconRect.x + iconRect.width) {
    // The user clicked on the image.
    *aChildElt = ToNewUnicode(NS_LITERAL_STRING("image"));
    return NS_OK;
  }

  // Just assume "text".
  // XXX For marquee selection, we'll have to make this more precise and do text measurement.
  *aChildElt = ToNewUnicode(NS_LITERAL_STRING("text"));
  return NS_OK;
}

void
nsTreeBodyFrame::GetCellWidth(PRInt32 aRow, const nsAString& aColID,
                              nsIRenderingContext* aRenderingContext,
                              nscoord& aDesiredSize, nscoord& aCurrentSize)
{
  nsTreeColumn* currCol = nsnull;
  // Keep looping until we find a column with a matching Id.
  for (currCol = mColumns; currCol; currCol = currCol->GetNext()) {
    if (currCol->GetID().Equals(aColID))
      break;
  }
  
  if (currCol) {
    // The rect for the current cell.
    nsRect cellRect(0, 0, currCol->GetWidth(), mRowHeight);
    PRInt32 overflow = cellRect.x+cellRect.width-(mInnerBox.x+mInnerBox.width);
    if (overflow > 0)
      cellRect.width -= overflow;

    // Adjust borders and padding for the cell.
    nsStyleContext* cellContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecell);
    nsMargin bp(0,0,0,0);
    GetBorderPadding(cellContext, bp);

    aCurrentSize = cellRect.width;
    aDesiredSize = bp.left + bp.right;

    if (currCol->IsPrimary()) {
      // If the current Column is a Primary, then we need to take into account 
      // the indentation and possibly a twisty. 

      // The amount of indentation is the indentation width (|mIndentation|) by the level.
      PRInt32 level;
      mView->GetLevel(aRow, &level);
      aDesiredSize += mIndentation * level;
      
      // Find the twisty rect by computing its size.
      nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);

      // |GetImageSize| returns the rect of the twisty image, including the 
      // borders and padding.
      nsRect twistyImageRect = GetImageSize(aRow, currCol->GetID().get(), PR_TRUE, twistyContext);
      
      // Add in the margins of the twisty element.
      nsMargin twistyMargin;
      twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
      twistyImageRect.Inflate(twistyMargin);

      aDesiredSize += twistyImageRect.width;
    }

    nsStyleContext* imageContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeimage);

    // Account for the width of the cell image.
    nsRect imageSize = GetImageSize(aRow, currCol->GetID().get(), PR_FALSE, imageContext);
    // Add in the margins of the cell image.
    nsMargin imageMargin;
    imageContext->GetStyleMargin()->GetMargin(imageMargin);
    imageSize.Inflate(imageMargin);

    aDesiredSize += imageSize.width;
    
    // Get the cell text.
    nsAutoString cellText;
    mView->GetCellText(aRow, currCol->GetID().get(), cellText);

    nsStyleContext* textContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecelltext);

    // Get the borders and padding for the text.
    GetBorderPadding(textContext, bp);
    
    // Get the font style for the text and pass it to the rendering context.
    aRenderingContext->SetFont(textContext->GetStyleFont()->mFont, nsnull);

    // Get the width of the text itself
    nscoord width;
    aRenderingContext->GetWidth(cellText, width);
    nscoord totalTextWidth = width + bp.left + bp.right;
    aDesiredSize += totalTextWidth;
  }
}

NS_IMETHODIMP
nsTreeBodyFrame::IsCellCropped(PRInt32 aRow, const nsAString& aColID, PRBool *_retval)
{  
  nscoord currentSize, desiredSize;
  nsCOMPtr<nsIRenderingContext> rc;
  mPresContext->PresShell()->CreateRenderingContext(this, getter_AddRefs(rc));

  GetCellWidth(aRow, aColID, rc, desiredSize, currentSize);
  *_retval = desiredSize > currentSize;
  
  return NS_OK;
}

void
nsTreeBodyFrame::MarkDirtyIfSelect()
{
  nsCOMPtr<nsIContent> baseElement;
  GetBaseElement(getter_AddRefs(baseElement));

  if (baseElement->Tag() == nsHTMLAtoms::select &&
      baseElement->IsContentOfType(nsIContent::eHTML)) {
    // If we are an intrinsically sized select widget, we may need to
    // resize, if the widest item was removed or a new item was added.
    // XXX optimize this more

    mStringWidth = -1;
    nsBoxLayoutState state(mPresContext);
    MarkDirty(state);
  }
}

nsresult
nsTreeBodyFrame::CreateTimer(const nsILookAndFeel::nsMetricID aID,
                             nsTimerCallbackFunc aFunc, PRInt32 aType,
                             nsITimer** aTimer)
{
  // Get the delay from the look and feel service.
  PRInt32 delay = 0;
  nsCOMPtr<nsILookAndFeel> lookAndFeel;
  mPresContext->LookAndFeel()->GetMetric(aID, delay);

  nsCOMPtr<nsITimer> timer;

  // Create a new timer only if the delay is greater than zero.
  // Zero value means that this feature is completely disabled.
  if (delay > 0) {
    timer = do_CreateInstance("@mozilla.org/timer;1");
    if (timer) {
      nsCOMPtr<nsITimerInternal> timerInternal = do_QueryInterface(timer);
      if (timerInternal) {
        timerInternal->SetIdle(PR_FALSE);
      }
      timer->InitWithFuncCallback(aFunc, this, delay, aType);
    }
  }

  NS_IF_ADDREF(*aTimer = timer);

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::RowCountChanged(PRInt32 aIndex, PRInt32 aCount)
{
  if (aCount == 0 || !mView)
    return NS_OK; // Nothing to do.

  // Adjust our selection.
  nsCOMPtr<nsITreeSelection> sel;
  mView->GetSelection(getter_AddRefs(sel));
  if (sel)
    sel->AdjustSelection(aIndex, aCount);

  if (mUpdateBatchNest)
    return NS_OK;

  mRowCount += aCount;
#ifdef DEBUG
  PRInt32 rowCount = mRowCount;
  mView->GetRowCount(&rowCount);
  NS_ASSERTION(rowCount == mRowCount, "row count did not change by the amount suggested, check caller");
#endif

  PRInt32 count = PR_ABS(aCount);
  PRInt32 last;
  GetLastVisibleRow(&last);
  if (aIndex >= mTopRowIndex && aIndex <= last)
    InvalidateRange(aIndex, last);

  if (mTopRowIndex == 0) {    
    // Just update the scrollbar and return.
    InvalidateScrollbar();
    CheckVerticalOverflow();
    MarkDirtyIfSelect();
    return NS_OK;
  }

  // Adjust our top row index.
  if (aCount > 0) {
    if (mTopRowIndex > aIndex) {
      // Rows came in above us.  Augment the top row index.
      mTopRowIndex += aCount;
      UpdateScrollbar();
    }
  }
  else if (aCount < 0) {
    if (mTopRowIndex > aIndex+count-1) {
      // No need to invalidate. The remove happened
      // completely above us (offscreen).
      mTopRowIndex -= count;
      UpdateScrollbar();
    }
    else if (mTopRowIndex >= aIndex) {
      // This is a full-blown invalidate.
      if (mTopRowIndex + mPageCount > mRowCount - 1) {
        mTopRowIndex = PR_MAX(0, mRowCount - 1 - mPageCount);
        UpdateScrollbar();
      }
      Invalidate();
    }
  }

  InvalidateScrollbar();
  CheckVerticalOverflow();
  MarkDirtyIfSelect();

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::BeginUpdateBatch()
{
  ++mUpdateBatchNest;

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::EndUpdateBatch()
{
  NS_ASSERTION(mUpdateBatchNest > 0, "badly nested update batch");

  if (--mUpdateBatchNest == 0) {
    if (mView) {
      Invalidate();
      PRInt32 countBeforeUpdate = mRowCount;
      mView->GetRowCount(&mRowCount);
      if (countBeforeUpdate != mRowCount) {
        if (mTopRowIndex + mPageCount > mRowCount - 1) {
          mTopRowIndex = PR_MAX(0, mRowCount - 1 - mPageCount);
          UpdateScrollbar();
        }
        InvalidateScrollbar();
        CheckVerticalOverflow();
      }
    }
  }

  return NS_OK;
}

void
nsTreeBodyFrame::PrefillPropertyArray(PRInt32 aRowIndex, nsTreeColumn* aCol)
{
  mScratchArray->Clear();
  
  // focus
  if (mFocused)
    mScratchArray->AppendElement(nsXULAtoms::focus);

  // sort
  PRBool sorted = PR_FALSE;
  mView->IsSorted(&sorted);
  if (sorted)
    mScratchArray->AppendElement(nsXULAtoms::sorted);

  // drag session
  if (mDragSession)
    mScratchArray->AppendElement(nsXULAtoms::dragSession);

  if (aRowIndex != -1) {
    nsCOMPtr<nsITreeSelection> selection;
    mView->GetSelection(getter_AddRefs(selection));
  
    if (selection) {
      // selected
      PRBool isSelected;
      selection->IsSelected(aRowIndex, &isSelected);
      if (isSelected)
        mScratchArray->AppendElement(nsHTMLAtoms::selected);

      // current
      PRInt32 currentIndex;
      selection->GetCurrentIndex(&currentIndex);
      if (aRowIndex == currentIndex)
        mScratchArray->AppendElement(nsXULAtoms::current);
    }

    // container or leaf
    PRBool isContainer = PR_FALSE;
    mView->IsContainer(aRowIndex, &isContainer);
    if (isContainer) {
      mScratchArray->AppendElement(nsXULAtoms::container);

      // open or closed
      PRBool isOpen = PR_FALSE;
      mView->IsContainerOpen(aRowIndex, &isOpen);
      if (isOpen)
        mScratchArray->AppendElement(nsXULAtoms::open);
      else
        mScratchArray->AppendElement(nsXULAtoms::closed);
    }
    else {
      mScratchArray->AppendElement(nsXULAtoms::leaf);
    }

    // drop orientation
    if (mDropAllowed && mDropRow == aRowIndex) {
      if (mDropOrient == nsITreeView::inDropBefore)
        mScratchArray->AppendElement(nsXULAtoms::dropBefore);
      else if (mDropOrient == nsITreeView::inDropOn)
        mScratchArray->AppendElement(nsXULAtoms::dropOn);
      else if (mDropOrient == nsITreeView::inDropAfter)
        mScratchArray->AppendElement(nsXULAtoms::dropAfter);
    }

    // odd or even
    if (aRowIndex % 2)
      mScratchArray->AppendElement(nsXULAtoms::odd);
    else
      mScratchArray->AppendElement(nsXULAtoms::even);
  }

  if (aCol) {
    nsCOMPtr<nsIAtom> colID;
    aCol->GetIDAtom(getter_AddRefs(colID));
    mScratchArray->AppendElement(colID);

    if (aCol->IsPrimary())
      mScratchArray->AppendElement(nsXULAtoms::primary);

    if (aCol->GetType() == nsTreeColumn::eProgressMeter) {
      mScratchArray->AppendElement(nsXULAtoms::progressmeter);

      PRInt32 state = nsITreeView::progressNone;
      if (aRowIndex != -1)
        mView->GetProgressMode(aRowIndex, aCol->GetID().get(), &state);
      if (state == nsITreeView::progressNormal)
        mScratchArray->AppendElement(nsXULAtoms::progressNormal);
      else if (state == nsITreeView::progressUndetermined)
        mScratchArray->AppendElement(nsXULAtoms::progressUndetermined);
      else if (state == nsITreeView::progressNone)
        mScratchArray->AppendElement(nsXULAtoms::progressNone);
    }
  }
}

nsresult
nsTreeBodyFrame::GetImage(PRInt32 aRowIndex, const PRUnichar* aColID, PRBool aUseContext,
                          nsStyleContext* aStyleContext, PRBool& aAllowImageRegions, imgIContainer** aResult)
{
  *aResult = nsnull;

  nsAutoString imageSrc;
  mView->GetImageSrc(aRowIndex, aColID, imageSrc);
  if (!aUseContext && !imageSrc.IsEmpty()) {
    aAllowImageRegions = PR_FALSE;
  }
  else {
    // Obtain the URL from the style context.
    aAllowImageRegions = PR_TRUE;
    nsIURI* uri = aStyleContext->GetStyleList()->mListStyleImage;
    if (!uri)
      return NS_OK;
    nsCAutoString spec;
    uri->GetSpec(spec);
    CopyUTF8toUTF16(spec, imageSrc);
  }
  nsStringKey key(imageSrc);

  if (mImageCache) {
    // Look the image up in our cache.
    nsCOMPtr<imgIRequest> imgReq = getter_AddRefs(NS_STATIC_CAST(imgIRequest*, mImageCache->Get(&key)));
    if (imgReq) {
      // Find out if the image has loaded.
      PRUint32 status;
      imgReq->GetImageStatus(&status);
      imgReq->GetImage(aResult); // We hand back the image here.  The GetImage call addrefs *aResult.
      PRUint32 numFrames = 1;
      if (*aResult)
        (*aResult)->GetNumFrames(&numFrames);

      if ((!(status & imgIRequest::STATUS_LOAD_COMPLETE)) || numFrames > 1) {
        // We either aren't done loading, or we're animating. Add our row as a listener for invalidations.
        nsCOMPtr<imgIDecoderObserver> obs;
        imgReq->GetDecoderObserver(getter_AddRefs(obs));
        nsCOMPtr<nsITreeImageListener> listener(do_QueryInterface(obs));
        if (listener)
          listener->AddRow(aRowIndex);
        return NS_OK;
      }
    }
  }

  if (!*aResult) {
    // Create a new nsTreeImageListener object and pass it our row and column
    // information.
    nsTreeImageListener* listener = new nsTreeImageListener(mTreeBoxObject, aColID);
    if (!listener)
      return NS_ERROR_OUT_OF_MEMORY;

    listener->AddRow(aRowIndex);
    nsCOMPtr<imgIDecoderObserver> imgDecoderObserver = listener;

    nsCOMPtr<nsIURI> baseURI;
    nsCOMPtr<nsIDocument> doc = mContent->GetDocument();
    if (!doc)
      // The page is currently being torn down.  Why bother.
      return NS_ERROR_FAILURE;

    baseURI = mContent->GetBaseURI();

    nsCOMPtr<nsIURI> srcURI;
    // XXX origin charset needed
    NS_NewURI(getter_AddRefs(srcURI), imageSrc, nsnull, baseURI);
    if (!srcURI)
      return NS_ERROR_FAILURE;
    nsCOMPtr<imgIRequest> imageRequest;

    nsresult rv;
    nsCOMPtr<imgILoader> il(do_GetService("@mozilla.org/image/loader;1", &rv));
    if (NS_FAILED(rv))
      return rv;

    mImageGuard = PR_TRUE;
    // XXX: initialDocumentURI is NULL!
    rv = il->LoadImage(srcURI, nsnull, doc->GetDocumentURI(), nsnull,
                       imgDecoderObserver, doc, nsIRequest::LOAD_NORMAL,
                       nsnull, nsnull, getter_AddRefs(imageRequest));
    mImageGuard = PR_FALSE;

    if (!imageRequest)
      return NS_ERROR_FAILURE;

    // In a case it was already cached.
    imageRequest->GetImage(aResult);

    if (!mImageCache) {
      mImageCache = new nsSupportsHashtable(16);
      if (!mImageCache)
        return NS_ERROR_OUT_OF_MEMORY;
    }
    mImageCache->Put(&key, imageRequest);
    imgIDecoderObserver* decoderObserverPtr = imgDecoderObserver;
    NS_ADDREF(decoderObserverPtr);  // Will get released when we remove the cache entry.
  }
  return NS_OK;
}

nsRect nsTreeBodyFrame::GetImageSize(PRInt32 aRowIndex, const PRUnichar* aColID, PRBool aUseContext,
                                     nsStyleContext* aStyleContext)
{
  // XXX We should respond to visibility rules for collapsed vs. hidden.

  // This method returns the width of the twisty INCLUDING borders and padding.
  // It first checks the style context for a width.  If none is found, it tries to
  // use the default image width for the twisty.  If no image is found, it defaults
  // to border+padding.
  nsRect r(0,0,0,0);
  nsMargin bp(0,0,0,0);
  GetBorderPadding(aStyleContext, bp);
  r.Inflate(bp);

  // Now r contains our border+padding info.  We now need to get our width and
  // height.
  PRBool needWidth = PR_FALSE;
  PRBool needHeight = PR_FALSE;

  // We have to load image even though we already have a size.
  // Don't change this, otherwise things start to go crazy.
  PRBool useImageRegion = PR_TRUE;
  nsCOMPtr<imgIContainer> image;
  GetImage(aRowIndex, aColID, aUseContext, aStyleContext, useImageRegion, getter_AddRefs(image));

  const nsStylePosition* myPosition = aStyleContext->GetStylePosition();
  const nsStyleList* myList = aStyleContext->GetStyleList();

  if (useImageRegion) {
    r.x += myList->mImageRegion.x;
    r.y += myList->mImageRegion.y;
  }

  if (myPosition->mWidth.GetUnit() == eStyleUnit_Coord)  {
    PRInt32 val = myPosition->mWidth.GetCoordValue();
    r.width += val;
  }
  else if (useImageRegion && myList->mImageRegion.width > 0)
    r.width += myList->mImageRegion.width;
  else 
    needWidth = PR_TRUE;

  if (myPosition->mHeight.GetUnit() == eStyleUnit_Coord)  {
    PRInt32 val = myPosition->mHeight.GetCoordValue();
    r.height += val;
  }
  else if (useImageRegion && myList->mImageRegion.height > 0)
    r.height += myList->mImageRegion.height;
  else 
    needHeight = PR_TRUE;

  if (image) {
    if (needWidth || needHeight) {
      // Get the natural image size.
      float p2t;
      p2t = mPresContext->PixelsToTwips();

      if (needWidth) {
        // Get the size from the image.
        nscoord width;
        image->GetWidth(&width);
        r.width += NSIntPixelsToTwips(width, p2t); 
      }
    
      if (needHeight) {
        nscoord height;
        image->GetHeight(&height);
        r.height += NSIntPixelsToTwips(height, p2t); 
      }
    }
  }

  return r;
}

PRInt32 nsTreeBodyFrame::GetRowHeight()
{
  // Look up the correct height.  It is equal to the specified height
  // + the specified margins.
  mScratchArray->Clear();
  nsStyleContext* rowContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreerow);
  if (rowContext) {
    const nsStylePosition* myPosition = rowContext->GetStylePosition();

    nscoord minHeight = 0;
    if (myPosition->mMinHeight.GetUnit() == eStyleUnit_Coord)
      minHeight = myPosition->mMinHeight.GetCoordValue();

    nscoord height = 0;
    if (myPosition->mHeight.GetUnit() == eStyleUnit_Coord)
      height = myPosition->mHeight.GetCoordValue();

    if (height < minHeight)
      height = minHeight;

    if (height > 0) {
      float t2p;
      t2p = mPresContext->TwipsToPixels();
      height = NSTwipsToIntPixels(height, t2p);
      height += height % 2;
      float p2t;
      p2t = mPresContext->PixelsToTwips();
      height = NSIntPixelsToTwips(height, p2t);

      // XXX Check box-sizing to determine if border/padding should augment the height
      // Inflate the height by our margins.
      nsRect rowRect(0,0,0,height);
      nsMargin rowMargin;
      rowContext->GetStyleMargin()->GetMargin(rowMargin);
      rowRect.Inflate(rowMargin);
      height = rowRect.height;
      return height;
    }
  }

  float p2t;
  p2t = mPresContext->PixelsToTwips();
  return NSIntPixelsToTwips(18, p2t); // As good a default as any.
}

PRInt32 nsTreeBodyFrame::GetIndentation()
{
  // Look up the correct indentation.  It is equal to the specified indentation width.
  mScratchArray->Clear();
  nsStyleContext* indentContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeindentation);
  if (indentContext) {
    const nsStylePosition* myPosition = indentContext->GetStylePosition();
    if (myPosition->mWidth.GetUnit() == eStyleUnit_Coord)  {
      nscoord val = myPosition->mWidth.GetCoordValue();
      return val;
    }
  }
  float p2t;
  p2t = mPresContext->PixelsToTwips();
  return NSIntPixelsToTwips(16, p2t); // As good a default as any.
}

void nsTreeBodyFrame::CalcInnerBox()
{
  mInnerBox.SetRect(0, 0, mRect.width, mRect.height);
  AdjustForBorderPadding(mStyleContext, mInnerBox);
}

nsLineStyle nsTreeBodyFrame::ConvertBorderStyleToLineStyle(PRUint8 aBorderStyle)
{
  switch (aBorderStyle) {
    case NS_STYLE_BORDER_STYLE_DOTTED:
      return nsLineStyle_kDotted;
    case NS_STYLE_BORDER_STYLE_DASHED:
      return nsLineStyle_kDashed;
    default:
      return nsLineStyle_kSolid;
  }
}

// Painting routines
NS_IMETHODIMP
nsTreeBodyFrame::Paint(nsIPresContext*      aPresContext,
                       nsIRenderingContext& aRenderingContext,
                       const nsRect&        aDirtyRect,
                       nsFramePaintLayer    aWhichLayer,
                       PRUint32             aFlags)
{
  if (aWhichLayer != NS_FRAME_PAINT_LAYER_BACKGROUND &&
      aWhichLayer != NS_FRAME_PAINT_LAYER_FOREGROUND)
    return NS_OK;

  if (!GetStyleVisibility()->IsVisibleOrCollapsed())
    return NS_OK; // We're invisible.  Don't paint.

  // Handles painting our background, border, and outline.
  nsresult rv = nsLeafFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
  if (NS_FAILED(rv)) return rv;

  if (aWhichLayer == NS_FRAME_PAINT_LAYER_FOREGROUND) {
    if (!mView)
      return NS_OK;

    PRBool clipState = PR_FALSE;
  
    // Update our available height and our page count.
    CalcInnerBox();
    PRInt32 oldPageCount = mPageCount;
    if (!mHasFixedRowCount)
      mPageCount = mInnerBox.height/mRowHeight;

    if (oldPageCount != mPageCount) {
      // Schedule a ResizeReflow that will update our page count properly.
      nsBoxLayoutState state(mPresContext);
      MarkDirty(state);
    }

#ifdef DEBUG
    PRInt32 rowCount = mRowCount;
    mView->GetRowCount(&mRowCount);
    NS_ASSERTION(mRowCount == rowCount, "row count changed unexpectedly");
#endif

    // Ensure our column info is built.
    EnsureColumns();

    // Loop through our columns and paint them (e.g., for sorting).  This is only
    // relevant when painting backgrounds, since columns contain no content.  Content
    // is contained in the rows.
    nscoord currX = mInnerBox.x;
    for (nsTreeColumn* currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width; 
         currCol = currCol->GetNext()) {
      nsRect colRect(currX, mInnerBox.y, currCol->GetWidth(), mInnerBox.height);
      PRInt32 overflow = colRect.x+colRect.width-(mInnerBox.x+mInnerBox.width);
      if (overflow > 0)
        colRect.width -= overflow;
      nsRect dirtyRect;
      if (dirtyRect.IntersectRect(aDirtyRect, colRect)) {
        PaintColumn(currCol, colRect, aPresContext, aRenderingContext, aDirtyRect); 
      }
      currX += currCol->GetWidth();
    }

    // Loop through our on-screen rows.
    for (PRInt32 i = mTopRowIndex; i < mRowCount && i < mTopRowIndex+mPageCount+1; i++) {
      nsRect rowRect(mInnerBox.x, mInnerBox.y+mRowHeight*(i-mTopRowIndex), mInnerBox.width, mRowHeight);
      nsRect dirtyRect;
      if (dirtyRect.IntersectRect(aDirtyRect, rowRect) && rowRect.y < (mInnerBox.y+mInnerBox.height)) {
        PRBool clip = (rowRect.y + rowRect.height > mInnerBox.y + mInnerBox.height);
        if (clip) {
          // We need to clip the last row, since it extends outside our inner box.  Push
          // a clip rect down.
          PRInt32 overflow = (rowRect.y+rowRect.height) - (mInnerBox.y+mInnerBox.height);
          nsRect clipRect(rowRect.x, rowRect.y, mInnerBox.width, mRowHeight-overflow);
          aRenderingContext.PushState();
          aRenderingContext.SetClipRect(clipRect, nsClipCombine_kReplace, clipState);
        }

        PaintRow(i, rowRect, aPresContext, aRenderingContext, aDirtyRect);

        if (clip)
          aRenderingContext.PopState(clipState);
      }
    }

    if (mDropAllowed && (mDropOrient == nsITreeView::inDropBefore ||
        mDropOrient == nsITreeView::inDropAfter)) {
      nsRect feedbackRect(mInnerBox.x, mInnerBox.y+mRowHeight*(mDropRow-mTopRowIndex), mInnerBox.width, mRowHeight);
      if (mDropOrient == nsITreeView::inDropAfter)
        feedbackRect.y += mRowHeight;

      nsRect dirtyRect;
      if (dirtyRect.IntersectRect(aDirtyRect, feedbackRect)) {
        PaintDropFeedback(feedbackRect, aPresContext, aRenderingContext, aDirtyRect);
      }
    }
  }

  return NS_OK;
}

nsresult
nsTreeBodyFrame::PaintColumn(nsTreeColumn*        aColumn,
                             const nsRect&        aColumnRect,
                             nsIPresContext*      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect)
{
  if (aColumnRect.width == 0)
    return NS_OK; // Don't paint hidden columns.

  // Now obtain the properties for our cell.
  // XXX Automatically fill in the following props: open, closed, container, leaf, selected, focused, and the col ID.
  PrefillPropertyArray(-1, aColumn);
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(aColumn->GetElement()));
  mView->GetColumnProperties(aColumn->GetID().get(), elt, mScratchArray);

  // Read special properties from attributes on the column content node
  nsAutoString attr;
  aColumn->GetElement()->GetAttr(kNameSpaceID_None, nsXULAtoms::insertbefore, attr);
  if (attr.Equals(NS_LITERAL_STRING("true")))
    mScratchArray->AppendElement(nsXULAtoms::insertbefore);
  attr.Truncate();
  aColumn->GetElement()->GetAttr(kNameSpaceID_None, nsXULAtoms::insertafter, attr);
  if (attr.Equals(NS_LITERAL_STRING("true")))
    mScratchArray->AppendElement(nsXULAtoms::insertafter);
  
  // Resolve style for the column.  It contains all the info we need to lay ourselves
  // out and to paint.
  nsStyleContext* colContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecolumn);

  // Obtain the margins for the cell and then deflate our rect by that 
  // amount.  The cell is assumed to be contained within the deflated rect.
  nsRect colRect(aColumnRect);
  nsMargin colMargin;
  colContext->GetStyleMargin()->GetMargin(colMargin);
  colRect.Deflate(colMargin);

  PaintBackgroundLayer(colContext, aPresContext, aRenderingContext, colRect, aDirtyRect);

  return NS_OK;
}

nsresult
nsTreeBodyFrame::PaintRow(PRInt32              aRowIndex,
                          const nsRect&        aRowRect,
                          nsIPresContext*      aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          const nsRect&        aDirtyRect)
{
  // We have been given a rect for our row.  We treat this row like a full-blown
  // frame, meaning that it can have borders, margins, padding, and a background.
  
  // Without a view, we have no data. Check for this up front.
  if (!mView)
    return NS_OK;

  // Now obtain the properties for our row.
  // XXX Automatically fill in the following props: open, closed, container, leaf, selected, focused
  PrefillPropertyArray(aRowIndex, nsnull);
  mView->GetRowProperties(aRowIndex, mScratchArray);

  // Resolve style for the row.  It contains all the info we need to lay ourselves
  // out and to paint.
  nsStyleContext* rowContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreerow);

  // Obtain the margins for the row and then deflate our rect by that 
  // amount.  The row is assumed to be contained within the deflated rect.
  nsRect rowRect(aRowRect);
  nsMargin rowMargin;
  rowContext->GetStyleMargin()->GetMargin(rowMargin);
  rowRect.Deflate(rowMargin);

  // Paint our borders and background for our row rect.
  // If a -moz-appearance is provided, use theme drawing only if the current row
  // is not selected (since we draw the selection as part of drawing the background).
  PRBool useTheme = PR_FALSE;
  nsCOMPtr<nsITheme> theme;
  const nsStyleDisplay* displayData = rowContext->GetStyleDisplay();
  if ( displayData->mAppearance ) {
    aPresContext->GetTheme(getter_AddRefs(theme));
    if (theme && theme->ThemeSupportsWidget(aPresContext, nsnull, displayData->mAppearance))
      useTheme = PR_TRUE;
  }
  PRBool isSelected = PR_FALSE;
  nsCOMPtr<nsITreeSelection> selection;
  GetSelection(getter_AddRefs(selection));
  if ( selection ) 
    selection->IsSelected(aRowIndex, &isSelected);
  if ( useTheme && !isSelected )
    theme->DrawWidgetBackground(&aRenderingContext, this, 
                                displayData->mAppearance, rowRect, aDirtyRect);     
  else
    PaintBackgroundLayer(rowContext, aPresContext, aRenderingContext, rowRect, aDirtyRect);
  
  // Adjust the rect for its border and padding.
  AdjustForBorderPadding(rowContext, rowRect);

  PRBool isSeparator = PR_FALSE;
  mView->IsSeparator(aRowIndex, &isSeparator);
  if (isSeparator) {
    // The row is a separator. Paint only a double horizontal line.

    // Find the primary cell.
    nscoord currX, primaryX1, primaryX2;
    currX = primaryX1 = primaryX2 = rowRect.x;
    for (nsTreeColumn* currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width;
         currCol = currCol->GetNext()) {
      if (currCol->IsPrimary()) {
        nsRect cellRect(currX, rowRect.y, currCol->GetWidth(), rowRect.height);
        PRInt32 overflow = cellRect.x+cellRect.width-(mInnerBox.x+mInnerBox.width);
        if (overflow > 0)
          cellRect.width -= overflow;
        nsRect dirtyRect;
        if (dirtyRect.IntersectRect(aDirtyRect, cellRect))
          PaintCell(aRowIndex, currCol, cellRect, aPresContext, aRenderingContext, aDirtyRect, primaryX2); 

        primaryX1 = currX;
        PRInt32 level;
        mView->GetLevel(aRowIndex, &level);
        if (level == 0) {
          primaryX1 += mIndentation;
        }

        break;
      }
      currX += currCol->GetWidth();
    }

    // Resolve style for the separator.
    nsStyleContext* separatorContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeseparator);
    PRBool useTheme = PR_FALSE;
    nsCOMPtr<nsITheme> theme;
    const nsStyleDisplay* displayData = separatorContext->GetStyleDisplay();
    if ( displayData->mAppearance ) {
      aPresContext->GetTheme(getter_AddRefs(theme));
      if (theme && theme->ThemeSupportsWidget(aPresContext, nsnull, displayData->mAppearance))
        useTheme = PR_TRUE;
    }

    // use -moz-appearance if provided.
    if (useTheme) {
      if (primaryX1 > rowRect.x) {
        // XXX We should draw the left side of the native separator here.
        // The problem is that Mac native theme implementation is too smart
        // and draws a vertical line if the separator is too narrow.
      }
      rowRect.width -= primaryX2 - rowRect.x;
      rowRect.x = primaryX2;
      theme->DrawWidgetBackground(&aRenderingContext, this,
                                  displayData->mAppearance, rowRect, aDirtyRect); 
    }
    else {
      // Get border style
      const nsStyleBorder* borderStyle = separatorContext->GetStyleBorder();

      aRenderingContext.PushState();

      PRUint8 side = NS_SIDE_TOP;
      nscoord currY = rowRect.y + rowRect.height / 2;
      for (PRInt32 i = 0; i < 2; i++) {
        nscolor color;
        PRBool transparent; PRBool foreground;
        borderStyle->GetBorderColor(side, color, transparent, foreground);
        aRenderingContext.SetColor(color);
        PRUint8 style;
        style = borderStyle->GetBorderStyle(side);
        aRenderingContext.SetLineStyle(ConvertBorderStyleToLineStyle(style));

        if (primaryX1 > rowRect.x) {
          aRenderingContext.DrawLine(rowRect.x, currY, primaryX1, currY);
        }
        aRenderingContext.DrawLine(primaryX2, currY, rowRect.x + rowRect.width, currY);

        side = NS_SIDE_BOTTOM;
        currY += 16;
      }

      PRBool clipState;
      aRenderingContext.PopState(clipState);
    }
  }
  else {
    // Now loop over our cells. Only paint a cell if it intersects with our dirty rect.
    nscoord currX = rowRect.x;
    for (nsTreeColumn* currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width; 
         currCol = currCol->GetNext()) {
      nsRect cellRect(currX, rowRect.y, currCol->GetWidth(), rowRect.height);
      PRInt32 overflow = cellRect.x+cellRect.width-(mInnerBox.x+mInnerBox.width);
      if (overflow > 0)
        cellRect.width -= overflow;
      nsRect dirtyRect;
      nscoord dummy;
      if (dirtyRect.IntersectRect(aDirtyRect, cellRect))
        PaintCell(aRowIndex, currCol, cellRect, aPresContext, aRenderingContext, aDirtyRect, dummy); 
      currX += currCol->GetWidth();
    }
  }

  return NS_OK;
}

nsresult
nsTreeBodyFrame::PaintCell(PRInt32              aRowIndex,
                           nsTreeColumn*        aColumn,
                           const nsRect&        aCellRect,
                           nsIPresContext*      aPresContext,
                           nsIRenderingContext& aRenderingContext,
                           const nsRect&        aDirtyRect,
                           nscoord&             aCurrX)
{
  if (aCellRect.width == 0)
    return NS_OK; // Don't paint cells in hidden columns.

  // Now obtain the properties for our cell.
  // XXX Automatically fill in the following props: open, closed, container, leaf, selected, focused, and the col ID.
  PrefillPropertyArray(aRowIndex, aColumn);
  mView->GetCellProperties(aRowIndex, aColumn->GetID().get(), mScratchArray);

  // Resolve style for the cell.  It contains all the info we need to lay ourselves
  // out and to paint.
  nsStyleContext* cellContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecell);

  // Obtain the margins for the cell and then deflate our rect by that 
  // amount.  The cell is assumed to be contained within the deflated rect.
  nsRect cellRect(aCellRect);
  nsMargin cellMargin;
  cellContext->GetStyleMargin()->GetMargin(cellMargin);
  cellRect.Deflate(cellMargin);

  // Paint our borders and background for our row rect.
  PaintBackgroundLayer(cellContext, aPresContext, aRenderingContext, cellRect, aDirtyRect);

  // Adjust the rect for its border and padding.
  AdjustForBorderPadding(cellContext, cellRect);

  nscoord currX = cellRect.x;
  nscoord remainingWidth = cellRect.width;

  // Now we paint the contents of the cells.
  // Text alignment determines the order in which we paint.  
  // LEFT means paint from left to right.
  // RIGHT means paint from right to left.
  // XXX Implement RIGHT alignment!

  if (aColumn->IsPrimary()) {
    // If we're the primary column, we need to indent and paint the twisty and any connecting lines
    // between siblings.

    PRInt32 level;
    mView->GetLevel(aRowIndex, &level);

    currX += mIndentation * level;
    remainingWidth -= mIndentation * level;

    // Resolve the style to use for the connecting lines.
    nsStyleContext* lineContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeline);
    
    if (lineContext->GetStyleVisibility()->IsVisibleOrCollapsed() && level) {
      // Paint the connecting lines.

      // Get the size of the twisty. We don't want to paint the twisty
      // before painting of connecting lines since it would paint lines over
      // the twisty. But we need to leave a place for it.
      nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);

      nsRect twistySize = GetImageSize(aRowIndex, aColumn->GetID().get(), PR_TRUE, twistyContext);

      nsMargin twistyMargin;
      twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
      twistySize.Inflate(twistyMargin);

      aRenderingContext.PushState();

      const nsStyleBorder* borderStyle = lineContext->GetStyleBorder();
      nscolor color;
      PRBool transparent; PRBool foreground;
      borderStyle->GetBorderColor(NS_SIDE_LEFT, color, transparent, foreground);

      aRenderingContext.SetColor(color);
      PRUint8 style;
      style = borderStyle->GetBorderStyle(NS_SIDE_LEFT);
      aRenderingContext.SetLineStyle(ConvertBorderStyleToLineStyle(style));

      nscoord lineX = currX;
      nscoord lineY = (aRowIndex - mTopRowIndex) * mRowHeight;

      // Compute the maximal level to paint.
      PRInt32 maxLevel = level;
      if (maxLevel > cellRect.width / mIndentation)
        maxLevel = cellRect.width / mIndentation;

      PRInt32 currentParent = aRowIndex;
      for (PRInt32 i = level; i > 0; i--) {
        if (i <= maxLevel) {
          lineX = currX + twistySize.width + mIndentation / 2;

          nscoord srcX = lineX - (level - i + 1) * mIndentation;
          if (srcX <= cellRect.x + cellRect.width) {
            // Paint full vertical line only if we have next sibling.
            PRBool hasNextSibling;
            mView->HasNextSibling(currentParent, aRowIndex, &hasNextSibling);
            if (hasNextSibling)
              aRenderingContext.DrawLine(srcX, lineY, srcX, lineY + mRowHeight);
            else if (i == level)
              aRenderingContext.DrawLine(srcX, lineY, srcX, lineY + mRowHeight / 2);
          }
        }

        PRInt32 parent;
        if (NS_FAILED(mView->GetParentIndex(currentParent, &parent)) || parent < 0)
          break;
        currentParent = parent;
      }

      // Don't paint off our cell.
      if (level == maxLevel) {
        nscoord srcX = lineX - mIndentation + 16;
        if (srcX <= cellRect.x + cellRect.width) {
          nscoord destX = lineX - mIndentation / 2;
          if (destX > cellRect.x + cellRect.width)
            destX = cellRect.x + cellRect.width;
          aRenderingContext.DrawLine(srcX, lineY + mRowHeight / 2, destX, lineY + mRowHeight / 2);
        }
      }

      PRBool clipState;
      aRenderingContext.PopState(clipState);
    }

    // Always leave space for the twisty.
    nsRect twistyRect(currX, cellRect.y, remainingWidth, cellRect.height);
    PaintTwisty(aRowIndex, aColumn, twistyRect, aPresContext, aRenderingContext, aDirtyRect,
                remainingWidth, currX);
  }
  
  // Now paint the icon for our cell.
  nsRect iconRect(currX, cellRect.y, remainingWidth, cellRect.height);
  nsRect dirtyRect;
  if (dirtyRect.IntersectRect(aDirtyRect, iconRect))
    PaintImage(aRowIndex, aColumn, iconRect, aPresContext, aRenderingContext, aDirtyRect,
               remainingWidth, currX);

  // Now paint our element, but only if we aren't a cycler column.
  // XXX until we have the ability to load images, allow the view to 
  // insert text into cycler columns...
  if (!aColumn->IsCycler()) {
    nsRect elementRect(currX, cellRect.y, remainingWidth, cellRect.height);
    nsRect dirtyRect;
    if (dirtyRect.IntersectRect(aDirtyRect, elementRect)) {
      switch (aColumn->GetType()) {
        case nsTreeColumn::eText:
          PaintText(aRowIndex, aColumn, elementRect, aPresContext, aRenderingContext, aDirtyRect, currX);
          break;
        case nsTreeColumn::eCheckbox:
          PaintCheckbox(aRowIndex, aColumn, elementRect, aPresContext, aRenderingContext, aDirtyRect);
          break;
        case nsTreeColumn::eProgressMeter:
          PRInt32 state;
          mView->GetProgressMode(aRowIndex, aColumn->GetID().get(), &state);
          switch (state) {
            case nsITreeView::progressNormal:
            case nsITreeView::progressUndetermined:
              PaintProgressMeter(aRowIndex, aColumn, elementRect, aPresContext, aRenderingContext, aDirtyRect);
              break;
            case nsITreeView::progressNone:
            default:
              PaintText(aRowIndex, aColumn, elementRect, aPresContext, aRenderingContext, aDirtyRect, currX);
              break;
          }
          break;
      }
    }
  }

  aCurrX = currX;

  return NS_OK;
}

nsresult
nsTreeBodyFrame::PaintTwisty(PRInt32              aRowIndex,
                             nsTreeColumn*        aColumn,
                             const nsRect&        aTwistyRect,
                             nsIPresContext*      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect,
                             nscoord&             aRemainingWidth,
                             nscoord&             aCurrX)
{
  // Paint the twisty, but only if we are a non-empty container.
  PRBool shouldPaint = PR_FALSE;
  PRBool isContainer = PR_FALSE;
  mView->IsContainer(aRowIndex, &isContainer);
  if (isContainer) {
    PRBool isContainerEmpty = PR_FALSE;
    mView->IsContainerEmpty(aRowIndex, &isContainerEmpty);
    if (!isContainerEmpty)
      shouldPaint = PR_TRUE;
  }

  // Resolve style for the twisty.
  nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);

  PRBool useTheme = PR_FALSE;
  nsCOMPtr<nsITheme> theme;
  const nsStyleDisplay* twistyDisplayData = twistyContext->GetStyleDisplay();
  if ( twistyDisplayData->mAppearance ) {
    aPresContext->GetTheme(getter_AddRefs(theme));
    if (theme && theme->ThemeSupportsWidget(aPresContext, nsnull, twistyDisplayData->mAppearance))
      useTheme = PR_TRUE;
  }
  
  // Obtain the margins for the twisty and then deflate our rect by that 
  // amount.  The twisty is assumed to be contained within the deflated rect.
  nsRect twistyRect(aTwistyRect);
  nsMargin twistyMargin;
  twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
  twistyRect.Deflate(twistyMargin);

  // The twisty rect extends all the way to the end of the cell.  This is incorrect.  We need to
  // determine the twisty rect's true width.  This is done by examining the style context for
  // a width first.  If it has one, we use that.  If it doesn't, we use the image's natural width.
  // If the image hasn't loaded and if no width is specified, then we just bail. If there is
  // a -moz-apperance involved, adjust the rect by the minimum widget size provided by
  // the theme implementation.
  nsRect imageSize = GetImageSize(aRowIndex, aColumn->GetID().get(), PR_TRUE, twistyContext);
  if (imageSize.height > twistyRect.height)
    imageSize.height = twistyRect.height;
  if (imageSize.width > twistyRect.width)
    imageSize.width = twistyRect.width;
  else
    twistyRect.width = imageSize.width;
  if ( useTheme ) {
    nsSize minTwistySize(0,0);
    PRBool canOverride = PR_TRUE;
    theme->GetMinimumWidgetSize(&aRenderingContext, this, twistyDisplayData->mAppearance, &minTwistySize, &canOverride);
    
    // GMWS() returns size in pixels, we need to convert it back to twips
    float p2t;
    aPresContext->GetScaledPixelsToTwips(&p2t);
    minTwistySize.width = NSIntPixelsToTwips(minTwistySize.width, p2t);
    minTwistySize.height = NSIntPixelsToTwips(minTwistySize.height, p2t);

    if ( twistyRect.width < minTwistySize.width || !canOverride )
      twistyRect.width = minTwistySize.width;
  }
  
  // Subtract out the remaining width.  This is done even when we don't actually paint a twisty in 
  // this cell, so that cells in different rows still line up.
  nsRect copyRect(twistyRect);
  copyRect.Inflate(twistyMargin);
  aRemainingWidth -= copyRect.width;
  aCurrX += copyRect.width;

  if (shouldPaint) {
    // Paint our borders and background for our image rect.
    PaintBackgroundLayer(twistyContext, aPresContext, aRenderingContext, twistyRect, aDirtyRect);

    if ( useTheme ) {
      // yeah, i know it says we're drawing a background, but a twisty is really a fg
      // object since it doesn't have anything that gecko would want to draw over it. Besides,
      // we have to prevent imagelib from drawing it.
      theme->DrawWidgetBackground(&aRenderingContext, this, 
                                  twistyDisplayData->mAppearance, twistyRect, aDirtyRect);
    }
    else {
      // Time to paint the twisty.
      // Adjust the rect for its border and padding.
      nsMargin bp(0,0,0,0);
      GetBorderPadding(twistyContext, bp);
      twistyRect.Deflate(bp);
      imageSize.Deflate(bp);

      // Get the image for drawing.
      nsCOMPtr<imgIContainer> image;
      PRBool useImageRegion = PR_TRUE;
      GetImage(aRowIndex, aColumn->GetID().get(), PR_TRUE, twistyContext, useImageRegion, getter_AddRefs(image));
      if (image) {
        nsPoint p(twistyRect.x, twistyRect.y);

        // Center the image. XXX Obey vertical-align style prop?
        if (imageSize.height < twistyRect.height) {
          p.y += (twistyRect.height - imageSize.height)/2;
        }
          
        // Paint the image.
        aRenderingContext.DrawImage(image, &imageSize, &p);
      }
    }        
  }

  return NS_OK;
}

nsresult
nsTreeBodyFrame::PaintImage(PRInt32              aRowIndex,
                            nsTreeColumn*        aColumn,
                            const nsRect&        aImageRect,
                            nsIPresContext*      aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect&        aDirtyRect,
                            nscoord&             aRemainingWidth,
                            nscoord&             aCurrX)
{
  // Resolve style for the image.
  nsStyleContext* imageContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeimage);

  // Obtain the margins for the twisty and then deflate our rect by that 
  // amount.  The twisty is assumed to be contained within the deflated rect.
  nsRect imageRect(aImageRect);
  nsMargin imageMargin;
  imageContext->GetStyleMargin()->GetMargin(imageMargin);
  imageRect.Deflate(imageMargin);

  // If the column isn't a cycler, the image rect extends all the way to the end of the cell.  
  // This is incorrect.  We need to determine the image rect's true width.  This is done by 
  // examining the style context for a width first.  If it has one, we use that.  If it doesn't, 
  // we use the image's natural width.
  // If the image hasn't loaded and if no width is specified, then we just bail.
  nsRect imageSize = GetImageSize(aRowIndex, aColumn->GetID().get(), PR_FALSE, imageContext);

  if (imageSize.height > imageRect.height)
    imageSize.height = imageRect.height;
  if (imageSize.width > imageRect.width)
    imageSize.width = imageRect.width;
  else if (!aColumn->IsCycler())
    imageRect.width = imageSize.width;

  // Subtract out the remaining width.
  nsRect copyRect(imageRect);
  copyRect.Inflate(imageMargin);
  aRemainingWidth -= copyRect.width;
  aCurrX += copyRect.width;

  // Get the image for drawing.
  PRBool useImageRegion = PR_TRUE;
  nsCOMPtr<imgIContainer> image; 
  GetImage(aRowIndex, aColumn->GetID().get(), PR_FALSE, imageContext, useImageRegion, getter_AddRefs(image));
  if (image) {
    // Paint our borders and background for our image rect
    PaintBackgroundLayer(imageContext, aPresContext, aRenderingContext, imageRect, aDirtyRect);
 
    // Time to paint the twisty.
    // Adjust the rect for its border and padding.
    nsMargin bp(0,0,0,0);
    GetBorderPadding(imageContext, bp);
    imageRect.Deflate(bp);
    imageSize.Deflate(bp);
 
    nsPoint p(imageRect.x, imageRect.y);
      
    // Center the image. XXX Obey vertical-align style prop?

    float t2p, p2t;
    t2p = mPresContext->TwipsToPixels();
    p2t = mPresContext->PixelsToTwips();

    if (imageSize.height < imageRect.height) {
      p.y += (imageRect.height - imageSize.height)/2;
    }

    // For cyclers, we also want to center the image in the column.
    if (aColumn->IsCycler() && imageSize.width < imageRect.width) {
      p.x += (imageRect.width - imageSize.width)/2;
    }

    // Paint the image.
    aRenderingContext.DrawImage(image, &imageSize, &p);
  }

  return NS_OK;
}

nsresult
nsTreeBodyFrame::PaintText(PRInt32              aRowIndex,
                           nsTreeColumn*        aColumn,
                           const nsRect&        aTextRect,
                           nsIPresContext*      aPresContext,
                           nsIRenderingContext& aRenderingContext,
                           const nsRect&        aDirtyRect,
                           nscoord&             aCurrX)
{
  // Now obtain the text for our cell.
  nsAutoString text;
  mView->GetCellText(aRowIndex, aColumn->GetID().get(), text);

  if (text.Length() == 0)
    return NS_OK; // Don't paint an empty string. XXX What about background/borders? Still paint?

  // Resolve style for the text.  It contains all the info we need to lay ourselves
  // out and to paint.
  nsStyleContext* textContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecelltext);

  // Obtain the margins for the text and then deflate our rect by that 
  // amount.  The text is assumed to be contained within the deflated rect.
  nsRect textRect(aTextRect);
  nsMargin textMargin;
  textContext->GetStyleMargin()->GetMargin(textMargin);
  textRect.Deflate(textMargin);

  // Adjust the rect for its border and padding.
  nsMargin bp(0,0,0,0);
  GetBorderPadding(textContext, bp);
  textRect.Deflate(bp);

  // Compute our text size.
  nsCOMPtr<nsIFontMetrics> fontMet;
  aPresContext->DeviceContext()->
    GetMetricsFor(textContext->GetStyleFont()->mFont,
                  *getter_AddRefs(fontMet));

  nscoord height, baseline;
  fontMet->GetHeight(height);
  fontMet->GetMaxAscent(baseline);

  // Center the text. XXX Obey vertical-align style prop?
  if (height < textRect.height) {
    textRect.y += (textRect.height - height)/2;
    textRect.height = height;
  }

  // Set our font.
  aRenderingContext.SetFont(fontMet);

  nscoord width;
  aRenderingContext.GetWidth(text, width);

  if (width > textRect.width) {
    // See if the width is even smaller than the ellipsis
    // If so, clear the text completely.
    nscoord ellipsisWidth;
    aRenderingContext.GetWidth(ELLIPSIS, ellipsisWidth);

    nscoord width = textRect.width;
    if (ellipsisWidth > width)
      text.SetLength(0);
    else if (ellipsisWidth == width)
      text.Assign(NS_LITERAL_STRING(ELLIPSIS));
    else {
      // We will be drawing an ellipsis, thank you very much.
      // Subtract out the required width of the ellipsis.
      // This is the total remaining width we have to play with.
      width -= ellipsisWidth;

      // Now we crop.
      switch (aColumn->GetCropStyle()) {
        default:
        case 0: {
          // Crop right.
          nscoord cwidth;
          nscoord twidth = 0;
          int length = text.Length();
          int i;
          for (i = 0; i < length; ++i) {
            PRUnichar ch = text[i];
            aRenderingContext.GetWidth(ch,cwidth);
            if (twidth + cwidth > width)
              break;
            twidth += cwidth;
          }
          text.Truncate(i);
          text += NS_LITERAL_STRING(ELLIPSIS);
        }
        break;

        case 2: {
          // Crop left.
          nscoord cwidth;
          nscoord twidth = 0;
          int length = text.Length();
          int i;
          for (i=length-1; i >= 0; --i) {
            PRUnichar ch = text[i];
            aRenderingContext.GetWidth(ch,cwidth);
            if (twidth + cwidth > width)
              break;
            twidth += cwidth;
          }

          nsAutoString copy;
          text.Right(copy, length-1-i);
          text.Assign(NS_LITERAL_STRING(ELLIPSIS));
          text += copy;
        }
        break;

        case 1:
        {
          // Crop center.
          nsAutoString leftStr, rightStr;
          nscoord cwidth, twidth = 0;
          int length = text.Length();
          int rightPos = length - 1;
          for (int leftPos = 0; leftPos < rightPos; ++leftPos) {
            PRUnichar ch = text[leftPos];
            aRenderingContext.GetWidth(ch, cwidth);
            twidth += cwidth;
            if (twidth > width)
              break;
            leftStr.Append(ch);

            ch = text[rightPos];
            aRenderingContext.GetWidth(ch, cwidth);
            twidth += cwidth;
            if (twidth > width)
              break;
            rightStr.Insert(ch, 0);
            --rightPos;
          }
          text = leftStr + NS_LITERAL_STRING(ELLIPSIS) + rightStr;
        }
        break;
      }
    }
  }
  else {
    switch (aColumn->GetTextAlignment()) {
      case NS_STYLE_TEXT_ALIGN_RIGHT: {
        textRect.x += textRect.width - width;
      }
      break;
      case NS_STYLE_TEXT_ALIGN_CENTER: {
        textRect.x += (textRect.width - width) / 2;
      }
      break;
    }
  }

  aRenderingContext.GetWidth(text, width);
  textRect.width = width;

  // Subtract out the remaining width.
  nsRect copyRect(textRect);
  copyRect.Inflate(textMargin);
  aCurrX += copyRect.width;

  textRect.Inflate(bp);
  PaintBackgroundLayer(textContext, aPresContext, aRenderingContext, textRect, aDirtyRect);

  // Time to paint our text.
  textRect.Deflate(bp);

  // Set our color.
  aRenderingContext.SetColor(textContext->GetStyleColor()->mColor);

  // Draw decorations.
  PRUint8 decorations = textContext->GetStyleTextReset()->mTextDecoration;

  nscoord offset;
  nscoord size;
  if (decorations & (NS_FONT_DECORATION_OVERLINE | NS_FONT_DECORATION_UNDERLINE)) {
    fontMet->GetUnderline(offset, size);
    if (decorations & NS_FONT_DECORATION_OVERLINE)
      aRenderingContext.FillRect(textRect.x, textRect.y, width, size);
    if (decorations & NS_FONT_DECORATION_UNDERLINE)
      aRenderingContext.FillRect(textRect.x, textRect.y + baseline - offset, width, size);
  }
  if (decorations & NS_FONT_DECORATION_LINE_THROUGH) {
    fontMet->GetStrikeout(offset, size);
    aRenderingContext.FillRect(textRect.x, textRect.y + baseline - offset, width, size);
  }
#ifdef MOZ_TIMELINE
  NS_TIMELINE_START_TIMER("Render Outline Text");
#endif
#ifdef IBMBIDI
  nsresult rv = NS_ERROR_FAILURE;
  nsBidiPresUtils* bidiUtils;
  aPresContext->GetBidiUtils(&bidiUtils);

  if (bidiUtils) {
    const nsStyleVisibility* vis = GetStyleVisibility();
    nsBidiDirection direction =
      (NS_STYLE_DIRECTION_RTL == vis->mDirection) ?
      NSBIDI_RTL : NSBIDI_LTR;
    PRUnichar* buffer = text.BeginWriting();
    rv = bidiUtils->RenderText(buffer, text.Length(), direction,
                               aPresContext, aRenderingContext,
                               textRect.x, textRect.y + baseline);
  }
  if (NS_FAILED(rv))
#endif // IBMBIDI
  aRenderingContext.DrawString(text, textRect.x, textRect.y + baseline);
#ifdef MOZ_TIMELINE
  NS_TIMELINE_STOP_TIMER("Render Outline Text");
  NS_TIMELINE_MARK_TIMER("Render Outline Text");
#endif

  return NS_OK;
}

nsresult
nsTreeBodyFrame::PaintCheckbox(PRInt32              aRowIndex,
                               nsTreeColumn*        aColumn,
                               const nsRect&        aCheckboxRect,
                               nsIPresContext*      aPresContext,
                               nsIRenderingContext& aRenderingContext,
                               const nsRect&        aDirtyRect)
{
  return NS_OK;
}

nsresult
nsTreeBodyFrame::PaintProgressMeter(PRInt32              aRowIndex,
                                    nsTreeColumn*        aColumn,
                                    const nsRect&        aProgressMeterRect,
                                    nsIPresContext*      aPresContext,
                                    nsIRenderingContext& aRenderingContext,
                                    const nsRect&        aDirtyRect)
{

  // Resolve style for the progress meter.  It contains all the info we need
  // to lay ourselves out and to paint.
  nsStyleContext* meterContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeprogressmeter);

  // Obtain the margins for the progress meter and then deflate our rect by that 
  // amount. The progress meter is assumed to be contained within the deflated
  // rect.
  nsRect meterRect(aProgressMeterRect);
  nsMargin meterMargin;
  meterContext->GetStyleMargin()->GetMargin(meterMargin);
  meterRect.Deflate(meterMargin);

  // Paint our borders and background for our progress meter rect.
  PaintBackgroundLayer(meterContext, aPresContext, aRenderingContext, meterRect, aDirtyRect);

  // Time to paint our progress. 
  PRInt32 state;
  mView->GetProgressMode(aRowIndex, aColumn->GetID().get(), &state);
  if (state == nsITreeView::progressNormal) {
    // Adjust the rect for its border and padding.
    AdjustForBorderPadding(meterContext, meterRect);

    // Set our color.
    aRenderingContext.SetColor(meterContext->GetStyleColor()->mColor);

    // Now obtain the value for our cell.
    nsAutoString value;
    mView->GetCellValue(aRowIndex, aColumn->GetID().get(), value);

    PRInt32 rv;
    PRInt32 intValue = value.ToInteger(&rv);
    if (intValue < 0)
      intValue = 0;
    else if (intValue > 100)
      intValue = 100;

    meterRect.width = NSToCoordRound((float)intValue / 100 * meterRect.width);
    PRBool useImageRegion = PR_TRUE;
    nsCOMPtr<imgIContainer> image;
    GetImage(aRowIndex, aColumn->GetID().get(), PR_TRUE, meterContext, useImageRegion, getter_AddRefs(image));
    if (image)
      aRenderingContext.DrawTile(image, 0, 0, &meterRect);
    else
      aRenderingContext.FillRect(meterRect);
  }
  else if (state == nsITreeView::progressUndetermined) {
    // Adjust the rect for its border and padding.
    AdjustForBorderPadding(meterContext, meterRect);

    PRBool useImageRegion = PR_TRUE;
    nsCOMPtr<imgIContainer> image;
    GetImage(aRowIndex, aColumn->GetID().get(), PR_TRUE, meterContext, useImageRegion, getter_AddRefs(image));
    if (image)
      aRenderingContext.DrawTile(image, 0, 0, &meterRect);
  }

  return NS_OK;
}


nsresult
nsTreeBodyFrame::PaintDropFeedback(const nsRect&        aDropFeedbackRect,
                                   nsIPresContext*      aPresContext,
                                   nsIRenderingContext& aRenderingContext,
                                   const nsRect&        aDirtyRect)
{
  // Paint the drop feedback in between rows.

  // Find the primary cell.
  nscoord currX = aDropFeedbackRect.x;
  nsTreeColumn* currCol;
  for (currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width;
       currCol = currCol->GetNext()) {
    if (currCol->IsPrimary())
      break;
    currX += currCol->GetWidth();
  }
  PrefillPropertyArray(mDropRow, currCol);

  // Resolve the style to use for the drop feedback.
  nsStyleContext* feedbackContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreedropfeedback);

  // Paint only if it is visible.
  if (feedbackContext->GetStyleVisibility()->IsVisibleOrCollapsed()) {
    PRInt32 level;
    mView->GetLevel(mDropRow, &level);

    // If our previous or next row has greater level use that for 
    // correct visual indentation.
    if (mDropOrient == nsITreeView::inDropBefore) {
      if (mDropRow > 0) {
        PRInt32 previousLevel;
        mView->GetLevel(mDropRow - 1, &previousLevel);
        if (previousLevel > level)
          level = previousLevel;
      }
    }
    else {
      if (mDropRow < mRowCount - 1) {
        PRInt32 nextLevel;
        mView->GetLevel(mDropRow + 1, &nextLevel);
        if (nextLevel > level)
          level = nextLevel;
      }
    }

    currX += mIndentation * level;

    nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);
    nsRect twistySize = GetImageSize(mDropRow, currCol->GetID().get(), PR_TRUE, twistyContext);
    nsMargin twistyMargin;
    twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
    twistySize.Inflate(twistyMargin);
    currX += twistySize.width;

    const nsStylePosition* stylePosition = feedbackContext->GetStylePosition();

    // Obtain the width for the drop feedback or use default value.
    nscoord width;
    if (stylePosition->mWidth.GetUnit() == eStyleUnit_Coord)
      width = stylePosition->mWidth.GetCoordValue();
    else {
      // Use default width 50px.
      float p2t;
      p2t = mPresContext->PixelsToTwips();
      width = NSIntPixelsToTwips(50, p2t);
    }

    // Obtain the height for the drop feedback or use default value.
    nscoord height;
    if (stylePosition->mHeight.GetUnit() == eStyleUnit_Coord)
      height = stylePosition->mHeight.GetCoordValue();
    else {
      // Use default height 2px.
      float p2t;
      p2t = mPresContext->PixelsToTwips();
      height = NSIntPixelsToTwips(2, p2t);
    }

    // Obtain the margins for the drop feedback and then deflate our rect
    // by that amount.
    nsRect feedbackRect(currX, aDropFeedbackRect.y, width, height);
    nsMargin margin;
    feedbackContext->GetStyleMargin()->GetMargin(margin);
    feedbackRect.Deflate(margin);

    // Finally paint the drop feedback.
    PaintBackgroundLayer(feedbackContext, aPresContext, aRenderingContext, feedbackRect, aDirtyRect);
  }

  return NS_OK;
}

nsresult
nsTreeBodyFrame::PaintBackgroundLayer(nsStyleContext*      aStyleContext,
                                      nsIPresContext*      aPresContext,
                                      nsIRenderingContext& aRenderingContext,
                                      const nsRect&        aRect,
                                      const nsRect&        aDirtyRect)
{
  const nsStyleBackground* myColor = aStyleContext->GetStyleBackground();
  const nsStyleBorder* myBorder = aStyleContext->GetStyleBorder();
  const nsStylePadding* myPadding = aStyleContext->GetStylePadding();
  const nsStyleOutline* myOutline = aStyleContext->GetStyleOutline();
  
  nsCSSRendering::PaintBackgroundWithSC(aPresContext, aRenderingContext,
                                        this, aDirtyRect, aRect,
                                        *myColor, *myBorder, *myPadding,
                                        PR_TRUE);

  nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                              aDirtyRect, aRect, *myBorder, mStyleContext, 0);

  nsCSSRendering::PaintOutline(aPresContext, aRenderingContext, this,
                               aDirtyRect, aRect, *myBorder, *myOutline,
                               aStyleContext, 0);

  return NS_OK;
}

// Scrolling
NS_IMETHODIMP nsTreeBodyFrame::EnsureRowIsVisible(PRInt32 aRow)
{
  if (!mView)
    return NS_OK;

  if (mTopRowIndex <= aRow && mTopRowIndex+mPageCount > aRow)
    return NS_OK;

  if (aRow < mTopRowIndex)
    ScrollToRow(aRow);
  else {
    // Bring it just on-screen.
    PRInt32 distance = aRow - (mTopRowIndex+mPageCount)+1;
    ScrollToRow(mTopRowIndex+distance);
  }

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::ScrollToRow(PRInt32 aRow)
{
  ScrollInternal(aRow);
  UpdateScrollbar();

#if defined(XP_MAC) || defined(XP_MACOSX)
  // mac can't process the event loop during a drag, so if we're dragging,
  // grab the scroll widget and make it paint synchronously. This is
  // sorta slow (having to paint the entire tree), but it works.
  if ( mDragSession ) {
    nsIWidget* scrollWidget = mScrollbar->GetWindow();
    if ( scrollWidget )
      scrollWidget->Invalidate(PR_TRUE);
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::ScrollByLines(PRInt32 aNumLines)
{
  if (!mView)
    return NS_OK;

  PRInt32 newIndex = mTopRowIndex + aNumLines;
  if (newIndex < 0)
    newIndex = 0;
  else {
    PRInt32 lastPageTopRow = mRowCount - mPageCount;
    if (newIndex > lastPageTopRow)
      newIndex = lastPageTopRow;
  }
  ScrollToRow(newIndex);
  
  return NS_OK;
}

NS_IMETHODIMP nsTreeBodyFrame::ScrollByPages(PRInt32 aNumPages)
{
  if (!mView)
    return NS_OK;

  PRInt32 newIndex = mTopRowIndex + aNumPages * mPageCount;
  if (newIndex < 0)
    newIndex = 0;
  else {
    PRInt32 lastPageTopRow = mRowCount - mPageCount;
    if (newIndex > lastPageTopRow)
      newIndex = lastPageTopRow;
  }
  ScrollToRow(newIndex);
    
  return NS_OK;
}

nsresult
nsTreeBodyFrame::ScrollInternal(PRInt32 aRow)
{
  if (!mView)
    return NS_OK;

  PRInt32 delta = aRow - mTopRowIndex;

  if (delta > 0) {
    if (mTopRowIndex == (mRowCount - mPageCount + 1))
      return NS_OK;
  }
  else {
    if (mTopRowIndex == 0)
      return NS_OK;
  }

  mTopRowIndex += delta;

  float t2p;
  t2p = mPresContext->TwipsToPixels();
  nscoord rowHeightAsPixels = NSToCoordRound((float)mRowHeight*t2p);

  // See if we have a background image.  If we do, then we cannot blit.
  PRBool hasBackground = GetStyleBackground()->mBackgroundImage != nsnull;

  PRInt32 absDelta = PR_ABS(delta);
  if (hasBackground || absDelta*mRowHeight >= mRect.height)
    Invalidate();
  else if (mTreeWidget)
    mTreeWidget->Scroll(0, -delta*rowHeightAsPixels, nsnull);

  return NS_OK;
}

NS_IMETHODIMP
nsTreeBodyFrame::ScrollbarButtonPressed(PRInt32 aOldIndex, PRInt32 aNewIndex)
{
  if (aNewIndex > aOldIndex)
    ScrollToRow(mTopRowIndex+1);
  else if (aNewIndex < aOldIndex)
    ScrollToRow(mTopRowIndex-1);
  return NS_OK;
}
  
NS_IMETHODIMP
nsTreeBodyFrame::PositionChanged(PRInt32 aOldIndex, PRInt32& aNewIndex)
{
  if (!EnsureScrollbar())
    return NS_ERROR_UNEXPECTED;

  float t2p;
  t2p = mPresContext->TwipsToPixels();
  nscoord rh = NSToCoordRound((float)mRowHeight*t2p);

  nscoord oldrow = aOldIndex/rh;
  nscoord newrow = aNewIndex/rh;

  if (oldrow != newrow)
    ScrollInternal(newrow);

  // Go exactly where we're supposed to
  // Update the scrollbar.
  nsAutoString curPos;
  curPos.AppendInt(aNewIndex);
  mScrollbar->GetContent()->SetAttr(kNameSpaceID_None,
                                    nsXULAtoms::curpos, curPos, PR_TRUE);

  return NS_OK;
}

// The style cache.
nsStyleContext*
nsTreeBodyFrame::GetPseudoStyleContext(nsIAtom* aPseudoElement)
{
  return mStyleCache.GetStyleContext(this, mPresContext, mContent,
                                     mStyleContext, aPseudoElement,
                                     mScratchArray);
}

// Our comparator for resolving our complex pseudos
NS_IMETHODIMP
nsTreeBodyFrame::PseudoMatches(nsIAtom* aTag, nsCSSSelector* aSelector, PRBool* aResult)
{
  if (aSelector->mTag == aTag) {
    // Iterate the pseudoclass list.  For each item in the list, see if
    // it is contained in our scratch array.  If we have a miss, then
    // we aren't a match.  If all items in the pseudoclass list are
    // present in the scratch array, then we have a match.
    nsAtomStringList* curr = aSelector->mPseudoClassList;
    while (curr) {
      PRInt32 index;
      mScratchArray->GetIndexOf(curr->mAtom, &index);
      if (index == -1) {
        *aResult = PR_FALSE;
        return NS_OK;
      }
      curr = curr->mNext;
    }
    *aResult = PR_TRUE;
  }
  else 
    *aResult = PR_FALSE;

  return NS_OK;
}

void
nsTreeBodyFrame::InvalidateColumnCache()
{
  mColumnsDirty = PR_TRUE;
}

void
nsTreeBodyFrame::EnsureColumns()
{
  if (!mColumns || mColumnsDirty) {
    delete mColumns;
    mColumns = nsnull;
    mColumnsDirty = PR_FALSE;

    nsCOMPtr<nsIContent> parent;
    GetBaseElement(getter_AddRefs(parent));

    if (!parent)
      return;

    nsIPresShell *shell = mPresContext->PresShell();

    // Note: this is dependent on the anonymous content for select
    // defined in select.xml
    if (parent->Tag() == nsHTMLAtoms::select &&
        parent->IsContentOfType(nsIContent::eHTML)) {
      // We can avoid crawling the content nodes in this case, since we know
      // that we have a single column, and we know where it's at.

      ChildIterator iter, last;
      ChildIterator::Init(parent, &iter, &last);
      nsCOMPtr<nsIContent> treeCols = *iter;

      nsIContent *column = treeCols->GetChildAt(0);

      nsIFrame* colFrame = nsnull;
      shell->GetPrimaryFrameFor(column, &colFrame);
      mColumns = new nsTreeColumn(column, colFrame);
      return;
    }

    nsCOMPtr<nsIContent> colsContent;
    nsTreeUtils::GetImmediateChild(parent, nsXULAtoms::treecols, getter_AddRefs(colsContent));
    if (!colsContent)
      return;

    nsIFrame* colsFrame = nsnull;
    shell->GetPrimaryFrameFor(colsContent, &colsFrame);
    if (!colsFrame)
      return;

    const nsStyleVisibility* vis = GetStyleVisibility();
    PRBool normalDirection = (vis->mDirection == NS_STYLE_DIRECTION_LTR);

    nsIBox* colsBox;
    CallQueryInterface(colsFrame, &colsBox);
    nsIBox* colBox = nsnull;
    colsBox->GetChildBox(&colBox);
    nsTreeColumn* currCol = nsnull;
    while (colBox) {
      nsIFrame* frame = nsnull;
      colBox->GetFrame(&frame);
      nsIContent* content = frame->GetContent();

      nsINodeInfo *ni = content->GetNodeInfo();
      if (ni && ni->Equals(nsXULAtoms::treecol, kNameSpaceID_XUL)) {
        // Create a new column structure.
        nsTreeColumn* col = new nsTreeColumn(content, frame);
        if (!col) {
          /* XXX What should happen if we can't make a tree column?
           * We could destroy the tree and mark it dirty, but that
           * risks an infinite loop. For now we'll just finish trying
           * to build the tree and hope nothing else misbehaves when
           * it isn't given enough memory.
           */
        } else {
          if (normalDirection) {
            if (currCol)
              currCol->SetNext(col);
            else
              mColumns = col;
             currCol = col;
          }
          else {
            col->SetNext(mColumns);
            mColumns = col;
          }
        }
      }

      colBox->GetNextBox(&colBox);
    }
  }
}

nsresult
nsTreeBodyFrame::GetBaseElement(nsIContent** aContent)
{
  nsINodeInfo *ni;
  nsIContent* parent = mContent;
  while (parent) {
    ni = parent->GetNodeInfo();

    if (ni && (ni->Equals(nsXULAtoms::tree, kNameSpaceID_XUL) ||
               (ni->Equals(nsHTMLAtoms::select) &&
                parent->IsContentOfType(nsIContent::eHTML)))) {
      break;
    }

    parent = parent->GetParent();
  }

  NS_IF_ADDREF(*aContent = parent);
  return NS_OK;
}

NS_IMETHODIMP
nsTreeBodyFrame::ClearStyleAndImageCaches()
{
  mStyleCache.Clear();
  if (mImageCache) {
    mImageCache->Enumerate(CancelImageRequest);
    delete mImageCache;
  }
  mImageCache = nsnull;
  mScrollbar = nsnull;
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark - 
#endif

// Tell the view where the drop happened
NS_IMETHODIMP
nsTreeBodyFrame::OnDragDrop (nsIDOMEvent* aEvent)
{
  // Remove the drop folder and all its parents from the array.
  PRInt32 parentIndex;
  nsresult rv = mView->GetParentIndex(mDropRow, &parentIndex);
  while (NS_SUCCEEDED(rv) && parentIndex >= 0) {
    mValueArray.RemoveValue(parentIndex);
    rv = mView->GetParentIndex(parentIndex, &parentIndex);
  }

  mView->Drop(mDropRow, mDropOrient);

  return rv;
} // OnDragDrop

// Clear out all our tracking vars.
NS_IMETHODIMP
nsTreeBodyFrame::OnDragExit(nsIDOMEvent* aEvent)
{
  if (mDropAllowed) {
    mDropAllowed = PR_FALSE;
    InvalidatePrimaryCell(mDropRow + (mDropOrient == nsITreeView::inDropAfter ? 1 : 0));
  }
  else
    mDropAllowed = PR_FALSE;
  mDropRow = -1;
  mDropOrient = -1;
  mDragSession = nsnull;
  mScrollLines = 0;

  if (mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }

  if (mValueArray.Count()) {
    // Close all spring loaded folders except the drop folder.
    CreateTimer(nsILookAndFeel::eMetric_TreeCloseDelay,
                CloseCallback, nsITimer::TYPE_ONE_SHOT,
                getter_AddRefs(mTimer));
  }

  return NS_OK;
} // OnDragExit

// The mouse is hovering over this tree. If we determine things are different from the
// last time, invalidate primary cell at the old position, query the view to see if the current location is 
// droppable, and then invalidate primary cell at the new location if it is. The mouse may or may
// not have changed position from the last time we were called, so optimize out a lot of
// the extra notifications by checking if anything changed first.
// For drop feedback we use drop, dropBefore and dropAfter property.
NS_IMETHODIMP
nsTreeBodyFrame::OnDragOver(nsIDOMEvent* aEvent)
{
  if (! mView)
    return NS_OK;

  // Save last values, we will need them.
  PRInt32 lastDropRow = mDropRow;
  PRInt16 lastDropOrient = mDropOrient;
  PRInt16 lastScrollLines = mScrollLines;

  // Compute the row mouse is over and the above/below/on state.
  // Below we'll use this to see if anything changed.
  // Also check if we want to auto-scroll.
  ComputeDropPosition(aEvent, &mDropRow, &mDropOrient, &mScrollLines);

  // While we're here, handle tracking of scrolling during a drag.
  if (mScrollLines) {
    if (mDropAllowed) {
      // Invalidate primary cell at old location.
      mDropAllowed = PR_FALSE;
      InvalidatePrimaryCell(lastDropRow + (lastDropOrient == nsITreeView::inDropAfter ? 1 : 0));
    }
#if !defined(XP_MAC) && !defined(XP_MACOSX)
    if (!lastScrollLines) {
      // Cancel any previosly initialized timer.
      if (mTimer) {
        mTimer->Cancel();
        mTimer = nsnull;
      }

      // Set a timer to trigger the tree scrolling.
      CreateTimer(nsILookAndFeel::eMetric_TreeLazyScrollDelay,
                  LazyScrollCallback, nsITimer::TYPE_ONE_SHOT,
                  getter_AddRefs(mTimer));
     }
#else
    ScrollByLines(mScrollLines);
#endif
    // Bail out to prevent spring loaded timer and feedback line settings.
    return NS_OK;
  }

  // If changed from last time, invalidate primary cell at the old location and if allowed, 
  // invalidate primary cell at the new location. If nothing changed, just bail.
  if (mDropRow != lastDropRow || mDropOrient != lastDropOrient) {
    // Invalidate row at the old location.
    if (mDropAllowed) {
      mDropAllowed = PR_FALSE;
      InvalidatePrimaryCell(lastDropRow + (lastDropOrient == nsITreeView::inDropAfter ? 1 : 0));
    }

    if (mTimer) {
      // Timer is active but for a different row than the current one, kill it.
      mTimer->Cancel();
      mTimer = nsnull;
    }

    if (mDropRow >= 0) {
      if (!mTimer && mDropOrient == nsITreeView::inDropOn) {
        // Either there wasn't a timer running or it was just killed above.
        // If over a folder, start up a timer to open the folder.
        PRBool isContainer = PR_FALSE;
        mView->IsContainer(mDropRow, &isContainer);
        if (isContainer) {
          PRBool isOpen = PR_FALSE;
          mView->IsContainerOpen(mDropRow, &isOpen);
          if (!isOpen) {
            // This node isn't expanded, set a timer to expand it.
            CreateTimer(nsILookAndFeel::eMetric_TreeOpenDelay,
                        OpenCallback, nsITimer::TYPE_ONE_SHOT,
                        getter_AddRefs(mTimer));
          }
        }
      }

      PRBool canDropAtNewLocation = PR_FALSE;
      if (mDropOrient == nsITreeView::inDropOn)
        mView->CanDropOn(mDropRow, &canDropAtNewLocation);
      else
        mView->CanDropBeforeAfter(mDropRow, mDropOrient == nsITreeView::inDropBefore, &canDropAtNewLocation);
      
      if (canDropAtNewLocation) {
        // Invalidate row at the new location.
        mDropAllowed = canDropAtNewLocation;
        InvalidatePrimaryCell(mDropRow + (mDropOrient == nsITreeView::inDropAfter ? 1 : 0));
      }
    }
  }
  
  // alert the drag session we accept the drop. We have to do this every time
  // since the |canDrop| attribute is reset before we're called.
  if (mDropAllowed && mDragSession)
    mDragSession->SetCanDrop(PR_TRUE);

  // Prevent default handler to fire.
  aEvent->PreventDefault();

  return NS_OK;
} // OnDragOver

PRBool
nsTreeBodyFrame::CanAutoScroll(PRInt32 aRowIndex)
{
  // Check first for partially visible last row.
  if (aRowIndex == mRowCount - 1) {
    nscoord y = mInnerBox.y + (aRowIndex - mTopRowIndex) * mRowHeight;
    if (y < mInnerBox.height && y + mRowHeight > mInnerBox.height)
      return PR_TRUE;
  }

  if (aRowIndex > 0 && aRowIndex < mRowCount - 1)
    return PR_TRUE;

  return PR_FALSE;
}

// Given a dom event, figure out which row in the tree the mouse is over,
// if we should drop before/after/on that row or we should auto-scroll.
// Doesn't query the content about if the drag is allowable, that's done elsewhere.
//
// For containers, we break up the vertical space of the row as follows: if in
// the topmost 25%, the drop is _before_ the row the mouse is over; if in the
// last 25%, _after_; in the middle 50%, we consider it a drop _on_ the container.
//
// For non-containers, if the mouse is in the top 50% of the row, the drop is
// _before_ and the bottom 50% _after_
void 
nsTreeBodyFrame::ComputeDropPosition(nsIDOMEvent* aEvent, PRInt32* aRow, PRInt16* aOrient,
                                     PRInt16* aScrollLines)
{
  *aRow = -1;
  *aOrient = -1;
  *aScrollLines = 0;

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent (do_QueryInterface(aEvent));
  if (mouseEvent) {
    PRInt32 x = 0;
    PRInt32 y = 0;
    mouseEvent->GetClientX(&x);
    mouseEvent->GetClientY(&y);
    
    PRInt32 xTwips;
    PRInt32 yTwips;
    AdjustEventCoordsToBoxCoordSpace (x, y, &xTwips, &yTwips);

    GetRowAt(x, y, aRow);
    if (*aRow >=0) {
      // Compute the top/bottom of the row in question.
      PRInt32 yOffset = yTwips - mRowHeight * (*aRow - mTopRowIndex);
   
      PRBool isContainer = PR_FALSE;
      mView->IsContainer (*aRow, &isContainer);
      if (isContainer) {
        // for a container, use a 25%/50%/25% breakdown
        if (yOffset < mRowHeight / 4)
          *aOrient = nsITreeView::inDropBefore;
        else if (yOffset > mRowHeight - (mRowHeight / 4))
          *aOrient = nsITreeView::inDropAfter;
        else
          *aOrient = nsITreeView::inDropOn;
      }
      else {
        // for a non-container use a 50%/50% breakdown
        if (yOffset < mRowHeight / 2)
          *aOrient = nsITreeView::inDropBefore;
        else
          *aOrient = nsITreeView::inDropAfter;
      }
    }

    if (CanAutoScroll(*aRow)) {
      // Get the max value from the look and feel service.
      PRInt32 scrollLinesMax = 0;
      mPresContext->LookAndFeel()->
        GetMetric(nsILookAndFeel::eMetric_TreeScrollLinesMax, scrollLinesMax);
      scrollLinesMax--;
      if (scrollLinesMax < 0)
        scrollLinesMax = 0;

      // Determine if we're w/in a margin of the top/bottom of the tree during a drag.
      // This will ultimately cause us to scroll, but that's done elsewhere.
      nscoord height = (3 * mRowHeight) / 4;
      if (yTwips < height) {
        // scroll up
        *aScrollLines = NSToIntRound(-scrollLinesMax * (1 - (float)yTwips / height) - 1);
      }
      else if (yTwips > mRect.height - height) {
        // scroll down
        *aScrollLines = NSToIntRound(scrollLinesMax * (1 - (float)(mRect.height - yTwips) / height) + 1);
      }
    }
  }
} // ComputeDropPosition

// Cache several things we'll need throughout the course of our work. These
// will all get released on a drag exit
NS_IMETHODIMP
nsTreeBodyFrame::OnDragEnter(nsIDOMEvent* aEvent)
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }

  // cache the drag session
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService = 
           do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(mDragSession));
  NS_ASSERTION(mDragSession, "can't get drag session");

  return NS_OK;
} // OnDragEnter

void
nsTreeBodyFrame::OpenCallback(nsITimer *aTimer, void *aClosure)
{
  nsTreeBodyFrame* self = NS_STATIC_CAST(nsTreeBodyFrame*, aClosure);
  if (self) {
    aTimer->Cancel();
    self->mTimer = nsnull;

    if (self->mDropRow >= 0) {
      self->mValueArray.AppendValue(self->mDropRow);
      self->mView->ToggleOpenState(self->mDropRow);
    }
  }
}

void
nsTreeBodyFrame::CloseCallback(nsITimer *aTimer, void *aClosure)
{
  nsTreeBodyFrame* self = NS_STATIC_CAST(nsTreeBodyFrame*, aClosure);
  if (self) {
    aTimer->Cancel();
    self->mTimer = nsnull;

    for (PRInt32 i = self->mValueArray.Count() - 1; i >= 0; i--) {
      if (self->mView)
        self->mView->ToggleOpenState(self->mValueArray[i]);
      self->mValueArray.RemoveValueAt(i);
    }
  }
}

void
nsTreeBodyFrame::LazyScrollCallback(nsITimer *aTimer, void *aClosure)
{
  nsTreeBodyFrame* self = NS_STATIC_CAST(nsTreeBodyFrame*, aClosure);
  if (self) {
    aTimer->Cancel();
    self->mTimer = nsnull;

    if (self->mView) {
      self->ScrollByLines(self->mScrollLines);
      // Set a new timer to scroll the tree repeatedly.
      self->CreateTimer(nsILookAndFeel::eMetric_TreeScrollDelay,
                        ScrollCallback, nsITimer::TYPE_REPEATING_SLACK,
                        getter_AddRefs(self->mTimer));
    }
  }
}

void
nsTreeBodyFrame::ScrollCallback(nsITimer *aTimer, void *aClosure)
{
  nsTreeBodyFrame* self = NS_STATIC_CAST(nsTreeBodyFrame*, aClosure);
  if (self) {
    // Don't scroll if we are already at the top or bottom of the view.
    if (self->mView && self->CanAutoScroll(self->mDropRow)) {
      self->ScrollByLines(self->mScrollLines);
    }
    else {
      aTimer->Cancel();
      self->mTimer = nsnull;
    }
  }
}

#ifdef XP_MAC
#pragma mark - 
#endif


// ==============================================================================
// The ImageListener implementation
// ==============================================================================

NS_IMPL_ISUPPORTS3(nsTreeImageListener, imgIDecoderObserver, imgIContainerObserver, nsITreeImageListener)

nsTreeImageListener::nsTreeImageListener(nsITreeBoxObject* aTree, const PRUnichar* aColID)
{
  mTree = aTree;
  mColID = aColID;
  mMin = -1; // min should start out "undefined"
  mMax = 0;
}

nsTreeImageListener::~nsTreeImageListener()
{
}

NS_IMETHODIMP nsTreeImageListener::OnStartDecode(imgIRequest *aRequest)
{
  return NS_OK;
}

NS_IMETHODIMP nsTreeImageListener::OnStartContainer(imgIRequest *aRequest,
                                                    imgIContainer *aImage)
{
  // Ensure the animation (if any) is started
  aImage->StartAnimation();
  return NS_OK;
}

NS_IMETHODIMP nsTreeImageListener::OnStartFrame(imgIRequest *aRequest,
                                                gfxIImageFrame *aFrame)
{
  return NS_OK;
}

NS_IMETHODIMP nsTreeImageListener::OnDataAvailable(imgIRequest *aRequest,
                                                   gfxIImageFrame *aFrame,
                                                   const nsRect *aRect)
{
  Invalidate();
  return NS_OK;
}

NS_IMETHODIMP nsTreeImageListener::OnStopFrame(imgIRequest *aRequest,
                                               gfxIImageFrame *aFrame)
{
  return NS_OK;
}

NS_IMETHODIMP nsTreeImageListener::OnStopContainer(imgIRequest *aRequest,
                                                   imgIContainer *aImage)
{
  return NS_OK;
}

NS_IMETHODIMP nsTreeImageListener::OnStopDecode(imgIRequest *aRequest,
                                                nsresult status,
                                                const PRUnichar *statusArg)
{
  return NS_OK;
}

NS_IMETHODIMP nsTreeImageListener::FrameChanged(imgIContainer *aContainer,
                                                gfxIImageFrame *newframe,
                                                nsRect * dirtyRect)
{
  Invalidate();
  return NS_OK;
}

NS_IMETHODIMP
nsTreeImageListener::AddRow(int aIndex)
{
  if (mMin == -1)
    mMin = mMax = aIndex;
  else if (aIndex < mMin)
    mMin = aIndex;
  else if (aIndex > mMax)
    mMax = aIndex;

  return NS_OK;
}

NS_IMETHODIMP 
nsTreeImageListener::Invalidate()
{
  // Loop from min to max, invalidating each cell that was listening for this image.
  for (PRInt32 i = mMin; i <= mMax; i++) {
    mTree->InvalidateCell(i, mColID.get());
  }

  return NS_OK;
}
