/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
// vim:cindent:ts=2:et:sw=2:
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
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
#include "nsFrame.h"
#include "nsFrameList.h"
#include "nsLineLayout.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsStyleContext.h"
#include "nsReflowPath.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIPresContext.h"
#include "nsCRT.h"
#include "nsGUIEvent.h"
#include "nsIDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsIPresShell.h"
#include "prlog.h"
#include "prprf.h"
#include <stdarg.h>
#include "nsFrameManager.h"
#include "nsCSSRendering.h"
#ifdef ACCESSIBILITY
#include "nsIAccessible.h"
#endif

#include "nsIDOMText.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLHRElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDeviceContext.h"
#include "nsIEventStateManager.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIFrameSelection.h"
#include "nsHTMLParts.h"
#include "nsLayoutAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSPseudoElements.h"
#include "nsHTMLAtoms.h"
#include "nsIHTMLContentSink.h" 

#include "nsFrameTraversal.h"
#include "nsStyleChangeList.h"
#include "nsIDOMRange.h"
#include "nsITableLayout.h"    //selection neccesity
#include "nsITableCellLayout.h"//  "
#include "nsITextControlFrame.h"
#include "nsINameSpaceManager.h"
#include "nsIPercentHeightObserver.h"

// For triple-click pref
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"
#include "nsISelectionImageService.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsILookAndFeel.h"
#include "nsLayoutCID.h"
#include "nsWidgetsCID.h"     // for NS_LOOKANDFEEL_CID
#include "nsUnicharUtils.h"
#include "nsLayoutErrors.h"

static NS_DEFINE_CID(kSelectionImageService, NS_SELECTIONIMAGESERVICE_CID);
static NS_DEFINE_CID(kLookAndFeelCID,  NS_LOOKANDFEEL_CID);


// Some Misc #defines
#define SELECTION_DEBUG        0
#define FORCE_SELECTION_UPDATE 1
#define CALC_DEBUG             0


#include "nsICaret.h"
#include "nsILineIterator.h"

//non Hack prototypes
#if 0
static void RefreshContentFrames(nsIPresContext* aPresContext, nsIContent * aStartContent, nsIContent * aEndContent);
#endif

#include "prenv.h"

// start nsIFrameDebug

#ifdef NS_DEBUG
static PRBool gShowFrameBorders = PR_FALSE;

void nsIFrameDebug::ShowFrameBorders(PRBool aEnable)
{
  gShowFrameBorders = aEnable;
}

PRBool nsIFrameDebug::GetShowFrameBorders()
{
  return gShowFrameBorders;
}

static PRBool gShowEventTargetFrameBorder = PR_FALSE;

void nsIFrameDebug::ShowEventTargetFrameBorder(PRBool aEnable)
{
  gShowEventTargetFrameBorder = aEnable;
}

PRBool nsIFrameDebug::GetShowEventTargetFrameBorder()
{
  return gShowEventTargetFrameBorder;
}

/**
 * Note: the log module is created during library initialization which
 * means that you cannot perform logging before then.
 */
static PRLogModuleInfo* gLogModule;

static PRLogModuleInfo* gFrameVerifyTreeLogModuleInfo;

static PRBool gFrameVerifyTreeEnable = PRBool(0x55);

PRBool
nsIFrameDebug::GetVerifyTreeEnable()
{
  if (gFrameVerifyTreeEnable == PRBool(0x55)) {
    if (nsnull == gFrameVerifyTreeLogModuleInfo) {
      gFrameVerifyTreeLogModuleInfo = PR_NewLogModule("frameverifytree");
      gFrameVerifyTreeEnable = 0 != gFrameVerifyTreeLogModuleInfo->level;
      printf("Note: frameverifytree is %sabled\n",
             gFrameVerifyTreeEnable ? "en" : "dis");
    }
  }
  return gFrameVerifyTreeEnable;
}

void
nsIFrameDebug::SetVerifyTreeEnable(PRBool aEnabled)
{
  gFrameVerifyTreeEnable = aEnabled;
}

static PRLogModuleInfo* gStyleVerifyTreeLogModuleInfo;

static PRBool gStyleVerifyTreeEnable = PRBool(0x55);

PRBool
nsIFrameDebug::GetVerifyStyleTreeEnable()
{
  if (gStyleVerifyTreeEnable == PRBool(0x55)) {
    if (nsnull == gStyleVerifyTreeLogModuleInfo) {
      gStyleVerifyTreeLogModuleInfo = PR_NewLogModule("styleverifytree");
      gStyleVerifyTreeEnable = 0 != gStyleVerifyTreeLogModuleInfo->level;
      printf("Note: styleverifytree is %sabled\n",
             gStyleVerifyTreeEnable ? "en" : "dis");
    }
  }
  return gStyleVerifyTreeEnable;
}

void
nsIFrameDebug::SetVerifyStyleTreeEnable(PRBool aEnabled)
{
  gStyleVerifyTreeEnable = aEnabled;
}

PRLogModuleInfo*
nsIFrameDebug::GetLogModuleInfo()
{
  if (nsnull == gLogModule) {
    gLogModule = PR_NewLogModule("frame");
  }
  return gLogModule;
}

void
nsIFrameDebug::RootFrameList(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent)
{
  if((nsnull == aPresContext) || (nsnull == out))
    return;

  nsIPresShell *shell = aPresContext->GetPresShell();
  if (nsnull != shell) {
    nsIFrame* frame;
    shell->GetRootFrame(&frame);
    if(nsnull != frame) {
      nsIFrameDebug* debugFrame;
      nsresult rv;
      rv = frame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&debugFrame);
      if(NS_SUCCEEDED(rv))
        debugFrame->List(aPresContext, out, aIndent);
    }
  }
}
#endif
// end nsIFrameDebug


// frame image selection drawing service implementation
class SelectionImageService : public nsISelectionImageService
{
public:
  SelectionImageService();
  virtual ~SelectionImageService();
  NS_DECL_ISUPPORTS
  NS_DECL_NSISELECTIONIMAGESERVICE
private:
  nsresult CreateImage(nscolor aImageColor, imgIContainer *aContainer);
  nsCOMPtr<imgIContainer> mContainer;
  nsCOMPtr<imgIContainer> mDisabledContainer;
};

NS_IMPL_ISUPPORTS1(SelectionImageService, nsISelectionImageService)

SelectionImageService::SelectionImageService()
{
}

SelectionImageService::~SelectionImageService()
{
}

NS_IMETHODIMP
SelectionImageService::GetImage(PRInt16 aSelectionValue, imgIContainer **aContainer)
{
  nsresult result;
  if (aSelectionValue != nsISelectionController::SELECTION_ON)
  {
    if (!mDisabledContainer)
    {
        mDisabledContainer = do_CreateInstance("@mozilla.org/image/container;1",&result);
        if (NS_FAILED(result))
          return result;
        if (mDisabledContainer)
        {
          nscolor disabledTextColor = NS_RGB(255, 255, 255);
          nsCOMPtr<nsILookAndFeel> look;
          look = do_GetService(kLookAndFeelCID,&result);
          if (NS_SUCCEEDED(result) && look)
            look->GetColor(nsILookAndFeel::eColor_TextSelectBackgroundDisabled, disabledTextColor);
          CreateImage(disabledTextColor, mDisabledContainer);
        }
    }
    *aContainer = mDisabledContainer; 
  }
  else
  {
    if (!mContainer)
    {
        mContainer = do_CreateInstance("@mozilla.org/image/container;1",&result);
        if (NS_FAILED(result))
          return result;
        if (mContainer)
        {
          nscolor selectionTextColor = NS_RGB(255, 255, 255);
          nsCOMPtr<nsILookAndFeel> look;
          look = do_GetService(kLookAndFeelCID,&result);
          if (NS_SUCCEEDED(result) && look)
            look->GetColor(nsILookAndFeel::eColor_TextSelectBackground, selectionTextColor);
          CreateImage(selectionTextColor, mContainer);
        }
    }
    *aContainer = mContainer; 
  }
  if (!*aContainer)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aContainer);
  return NS_OK;
}

NS_IMETHODIMP
SelectionImageService::Reset()
{
  mContainer = 0;
  mDisabledContainer = 0;
  return NS_OK;
}

#define SEL_IMAGE_WIDTH 32
#define SEL_IMAGE_HEIGHT 32
#define SEL_ALPHA_AMOUNT 128

nsresult
SelectionImageService::CreateImage(nscolor aImageColor, imgIContainer *aContainer)
{
  if (aContainer)
  {
    nsresult result = aContainer->Init(SEL_IMAGE_WIDTH,SEL_IMAGE_HEIGHT,nsnull);
    if (NS_SUCCEEDED(result))
    {
      nsCOMPtr<gfxIImageFrame> image = do_CreateInstance("@mozilla.org/gfx/image/frame;2",&result);
      if (NS_SUCCEEDED(result) && image)
      {
        image->Init(0, 0, SEL_IMAGE_WIDTH, SEL_IMAGE_HEIGHT, gfxIFormats::RGB_A8, 24);
        aContainer->AppendFrame(image);

        PRUint32 bpr, abpr;
        image->GetImageBytesPerRow(&bpr);
        image->GetAlphaBytesPerRow(&abpr);

        //its better to temporarily go after heap than put big data on stack
        unsigned char *row_data = (unsigned char *)malloc(bpr);
        if (!row_data)
          return NS_ERROR_OUT_OF_MEMORY;
        unsigned char *alpha = (unsigned char *)malloc(abpr);
        if (!alpha)
        {
          free (row_data);
          return NS_ERROR_OUT_OF_MEMORY;
        }
        unsigned char *data = row_data;

        PRInt16 i;
        for (i = 0; i < SEL_IMAGE_WIDTH; i++)
        {
#if defined(XP_WIN) || defined(XP_OS2)
          *data++ = NS_GET_B(aImageColor);
          *data++ = NS_GET_G(aImageColor);
          *data++ = NS_GET_R(aImageColor);
#else
#if defined(XP_MAC) || defined(XP_MACOSX)
          *data++ = 0;
#endif
          *data++ = NS_GET_R(aImageColor);
          *data++ = NS_GET_G(aImageColor);
          *data++ = NS_GET_B(aImageColor);
#endif
        }

        memset((void *)alpha, SEL_ALPHA_AMOUNT, abpr);

        for (i = 0; i < SEL_IMAGE_HEIGHT; i++)
        {
          image->SetAlphaData(alpha, abpr, i*abpr);
          image->SetImageData(row_data,  bpr, i*bpr);
        }
        free(row_data);
        free(alpha);
        return NS_OK;
      }
    } 
  }
  return NS_ERROR_FAILURE;
}


nsresult NS_NewSelectionImageService(nsISelectionImageService** aResult)
{
  *aResult = new SelectionImageService;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}


//end selection service

// a handy utility to set font
void SetFontFromStyle(nsIRenderingContext* aRC, nsStyleContext* aSC) 
{
  const nsStyleFont* font = aSC->GetStyleFont();
  const nsStyleVisibility* visibility = aSC->GetStyleVisibility();

  nsCOMPtr<nsIAtom> langGroup;
  if (visibility->mLanguage) {
    visibility->mLanguage->GetLanguageGroup(getter_AddRefs(langGroup));
  }

  aRC->SetFont(font->mFont, langGroup);
}

nsresult
NS_NewEmptyFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsFrame* it = new (aPresShell) nsFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

MOZ_DECL_CTOR_COUNTER(nsFrame)

// Overloaded new operator. Initializes the memory to 0 and relies on an arena
// (which comes from the presShell) to perform the allocation.
void* 
nsFrame::operator new(size_t sz, nsIPresShell* aPresShell) CPP_THROW_NEW
{
  // Check the recycle list first.
  void* result = nsnull;
  aPresShell->AllocateFrame(sz, &result);
  
  if (result) {
    memset(result, 0, sz);
  }

  return result;
}

// Overridden to prevent the global delete from being called, since the memory
// came out of an nsIArena instead of the global delete operator's heap.
void 
nsFrame::operator delete(void* aPtr, size_t sz)
{
  // Don't let the memory be freed, since it will be recycled
  // instead. Don't call the global operator delete.

  // Stash the size of the object in the first four bytes of the
  // freed up memory.  The Destroy method can then use this information
  // to recycle the object.
  size_t* szPtr = (size_t*)aPtr;
  *szPtr = sz;
}


nsFrame::nsFrame()
{
  MOZ_COUNT_CTOR(nsFrame);

  mState = NS_FRAME_FIRST_REFLOW | NS_FRAME_SYNC_FRAME_AND_VIEW |
    NS_FRAME_IS_DIRTY;
}

nsFrame::~nsFrame()
{
  MOZ_COUNT_DTOR(nsFrame);

  NS_IF_RELEASE(mContent);
  if (mStyleContext)
    mStyleContext->Release();
}

/////////////////////////////////////////////////////////////////////////////
// nsISupports

nsresult nsFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

#ifdef DEBUG
  if (aIID.Equals(NS_GET_IID(nsIFrameDebug))) {
    *aInstancePtr = NS_STATIC_CAST(void*,NS_STATIC_CAST(nsIFrameDebug*,this));
    return NS_OK;
  }
#endif

  if (aIID.Equals(NS_GET_IID(nsIFrame)) ||
      aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = NS_STATIC_CAST(void*,NS_STATIC_CAST(nsIFrame*,this));
    return NS_OK;
  }

  *aInstancePtr = nsnull;
  return NS_NOINTERFACE;
}

nsrefcnt nsFrame::AddRef(void)
{
  NS_WARNING("not supported for frames");
  return 1;
}

nsrefcnt nsFrame::Release(void)
{
  NS_WARNING("not supported for frames");
  return 1;
}

/////////////////////////////////////////////////////////////////////////////
// nsIFrame

NS_IMETHODIMP
nsFrame::Init(nsIPresContext*  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsStyleContext*  aContext,
              nsIFrame*        aPrevInFlow)
{
  mContent = aContent;
  NS_IF_ADDREF(mContent);
  mParent = aParent;

  if (aPrevInFlow) {
    // Make sure the general flags bits are the same
    nsFrameState state = aPrevInFlow->GetStateBits();

    // Make bits that are currently on (see constructor) the same:
    mState &= state | ~(NS_FRAME_SYNC_FRAME_AND_VIEW);

    // Make bits that are currently off (see constructor) the same:
    mState |= state & (NS_FRAME_REPLACED_ELEMENT |
                       NS_FRAME_SELECTED_CONTENT |
                       NS_FRAME_INDEPENDENT_SELECTION |
                       NS_FRAME_IS_SPECIAL);
  }
  if (mParent) {
    nsFrameState state = mParent->GetStateBits();

    // Make bits that are currently off (see constructor) the same:
    mState |= state & (NS_FRAME_INDEPENDENT_SELECTION |
                       NS_FRAME_GENERATED_CONTENT);
  }
  SetStyleContext(aPresContext, aContext);
  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetInitialChildList(nsIPresContext* aPresContext,
                                           nsIAtom*        aListName,
                                           nsIFrame*       aChildList)
{
  // XXX This shouldn't be getting called at all, but currently is for backwards
  // compatility reasons...
#if 0
  NS_ERROR("not a container");
  return NS_ERROR_UNEXPECTED;
#else
  NS_ASSERTION(nsnull == aChildList, "not a container");
  return NS_OK;
#endif
}

NS_IMETHODIMP
nsFrame::AppendFrames(nsIPresContext* aPresContext,
                      nsIPresShell&   aPresShell,
                      nsIAtom*        aListName,
                      nsIFrame*       aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::InsertFrames(nsIPresContext* aPresContext,
                      nsIPresShell&   aPresShell,
                      nsIAtom*        aListName,
                      nsIFrame*       aPrevFrame,
                      nsIFrame*       aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::RemoveFrame(nsIPresContext* aPresContext,
                     nsIPresShell&   aPresShell,
                     nsIAtom*        aListName,
                     nsIFrame*       aOldFrame)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::ReplaceFrame(nsIPresContext* aPresContext,
                      nsIPresShell&   aPresShell,
                      nsIAtom*        aListName,
                      nsIFrame*       aOldFrame,
                      nsIFrame*       aNewFrame)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::Destroy(nsIPresContext* aPresContext)
{
  nsIPresShell *shell = aPresContext->GetPresShell();

  // Get the view pointer now before the frame properties disappear
  // when we call NotifyDestroyingFrame()
  nsIView* view = GetView();
  
  // XXX Rather than always doing this it would be better if it was part of
  // a frame observer mechanism and the pres shell could register as an
  // observer of the frame while the reflow command is pending...
  if (shell) {
    shell->NotifyDestroyingFrame(this);
  }

  if ((mState & NS_FRAME_EXTERNAL_REFERENCE) ||
      (mState & NS_FRAME_SELECTED_CONTENT)) {
    if (shell) {
      shell->ClearFrameRefs(this);
    }
  }

  //XXX Why is this done in nsFrame instead of some frame class
  // that actually loads images?
  aPresContext->StopImagesFor(this);

  if (view) {
    // Break association between view and frame
    view->SetClientData(nsnull);
    
    // Destroy the view
    view->Destroy();
  }

  // Deleting the frame doesn't really free the memory, since we're using an
  // arena for allocation, but we will get our destructors called.
  delete this;

  // Now that we're totally cleaned out, we need to add ourselves to the presshell's
  // recycler.
  size_t* sz = (size_t*)this;
  shell->FreeFrame(*sz, (void*)this);

  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetOffsets(PRInt32 &aStart, PRInt32 &aEnd) const
{
  aStart = 0;
  aEnd = 0;
  return NS_OK;
}

// Subclass hook for style post processing
NS_IMETHODIMP nsFrame::DidSetStyleContext(nsIPresContext* aPresContext)
{
  return NS_OK;
}

NS_IMETHODIMP  nsFrame::CalcBorderPadding(nsMargin& aBorderPadding) const {
  NS_ASSERTION(mStyleContext!=nsnull,"null style context");
  if (mStyleContext) {
    nsStyleBorderPadding bpad;
    mStyleContext->GetBorderPaddingFor(bpad);
    if (!bpad.GetBorderPadding(aBorderPadding)) {
      const nsStylePadding* paddingStyle = GetStylePadding();
      paddingStyle->CalcPaddingFor(this, aBorderPadding);
      const nsStyleBorder* borderStyle = GetStyleBorder();
      nsMargin  border;
      if (borderStyle->GetBorder(border)) {
        aBorderPadding += border;
      }
    }
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsStyleContext*
nsFrame::GetAdditionalStyleContext(PRInt32 aIndex) const
{
  NS_PRECONDITION(aIndex >= 0, "invalid index number");
  return nsnull;
}

void
nsFrame::SetAdditionalStyleContext(PRInt32 aIndex, 
                                   nsStyleContext* aStyleContext)
{
  NS_PRECONDITION(aIndex >= 0, "invalid index number");
}

// Child frame enumeration

nsIAtom*
nsFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  NS_PRECONDITION(aIndex >= 0, "invalid index number");
  return nsnull;
}

nsIFrame*
nsFrame::GetFirstChild(nsIAtom* aListName) const
{
  return nsnull;
}

PRInt16
nsFrame::DisplaySelection(nsIPresContext* aPresContext, PRBool isOkToTurnOn)
{
  PRInt16 selType = nsISelectionController::SELECTION_OFF;

  nsCOMPtr<nsISelectionController> selCon;
  nsresult result = GetSelectionController(aPresContext, getter_AddRefs(selCon));
  if (NS_SUCCEEDED(result) && selCon) {
    result = selCon->GetDisplaySelection(&selType);
    if (NS_SUCCEEDED(result) && (selType != nsISelectionController::SELECTION_OFF)) {
      // Check whether style allows selection.
      PRBool selectable;
      IsSelectable(&selectable, nsnull);
      if (!selectable) {
        selType = nsISelectionController::SELECTION_OFF;
        isOkToTurnOn = PR_FALSE;
      }
    }
    if (isOkToTurnOn && (selType == nsISelectionController::SELECTION_OFF)) {
      selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
      selType = nsISelectionController::SELECTION_ON;
    }
  }
  return selType;
}

void
nsFrame::SetOverflowClipRect(nsIRenderingContext& aRenderingContext)
{
  // 'overflow-clip' only applies to block-level elements and replaced
  // elements that have 'overflow' set to 'hidden', and it is relative
  // to the content area and applies to content only (not border or background)
  const nsStyleBorder* borderStyle = GetStyleBorder();
  const nsStylePadding* paddingStyle = GetStylePadding();
  
  // Start with the 'auto' values and then factor in user specified values
  nsRect  clipRect(0, 0, mRect.width, mRect.height);

  // XXX We don't support the 'overflow-clip' property yet, so just use the
  // content area (which is the default value) as the clip shape
  nsMargin  border, padding;

  borderStyle->GetBorder(border);
  clipRect.Deflate(border);
  // XXX We need to handle percentage padding
  if (paddingStyle->GetPadding(padding)) {
    clipRect.Deflate(padding);
  }

  // Set updated clip-rect into the rendering context
  PRBool clipState;
  aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect, clipState);
}

/********************************************************
* Refreshes this frame and all child frames that are frames for aContent
*********************************************************/
static void RefreshAllContentFrames(nsIFrame* aFrame, nsIContent* aContent)
{
  if (aFrame->GetContent() == aContent) {
    aFrame->Invalidate(aFrame->GetOutlineRect(), PR_FALSE);
  }

  aFrame = aFrame->GetFirstChild(nsnull);
  while (aFrame) {
    RefreshAllContentFrames(aFrame, aContent);
    aFrame = aFrame->GetNextSibling();
  }
}

/********************************************************
* Refreshes each content's frame
*********************************************************/

NS_IMETHODIMP
nsFrame::Paint(nsIPresContext*      aPresContext,
               nsIRenderingContext& aRenderingContext,
               const nsRect&        aDirtyRect,
               nsFramePaintLayer    aWhichLayer,
               PRUint32             aFlags)
{
  if (aWhichLayer != NS_FRAME_PAINT_LAYER_FOREGROUND)
    return NS_OK;
  
  nsresult result; 
  nsIPresShell *shell = aPresContext->PresShell();

  PRInt16 displaySelection = nsISelectionDisplay::DISPLAY_ALL;
  if (!(aFlags & nsISelectionDisplay::DISPLAY_IMAGES))
  {
    result = shell->GetSelectionFlags(&displaySelection);
    if (NS_FAILED(result))
      return result;
    if (!(displaySelection & nsISelectionDisplay::DISPLAY_FRAMES))
      return NS_OK;
  }

//check frame selection state
  PRBool isSelected;
  isSelected = (GetStateBits() & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
//if not selected then return 
  if (!isSelected)
    return NS_OK; //nothing to do

//get the selection controller
  nsCOMPtr<nsISelectionController> selCon;
  result = GetSelectionController(aPresContext, getter_AddRefs(selCon));
  PRInt16 selectionValue;
  selCon->GetDisplaySelection(&selectionValue);
  displaySelection = selectionValue > nsISelectionController::SELECTION_HIDDEN;
//check display selection state.
  if (!displaySelection)
    return NS_OK; //if frame does not allow selection. do nothing


  nsIContent *newContent = mContent->GetParent();

  //check to see if we are anonymous content
  PRInt32 offset = 0;
  if (newContent) {
    // XXXbz there has GOT to be a better way of determining this!
    offset = newContent->IndexOf(mContent);
  }

  SelectionDetails *details;
  if (NS_SUCCEEDED(result) && shell)
  {
    nsCOMPtr<nsIFrameSelection> frameSelection;
    if (NS_SUCCEEDED(result) && selCon)
    {
      frameSelection = do_QueryInterface(selCon); //this MAY implement
    }
    if (!frameSelection)
      result = shell->GetFrameSelection(getter_AddRefs(frameSelection));
    if (NS_SUCCEEDED(result) && frameSelection){
      result = frameSelection->LookUpSelection(newContent, offset, 
                            1, &details, PR_FALSE);//look up to see what selection(s) are on this frame
    }
  }
  
  if (details)
  {
    nsRect rect = GetRect();
    rect.width-=2;
    rect.height-=2;
    rect.x=1; //we are in the coordinate system of the frame now with regards to the rendering context.
    rect.y=1;

    nsCOMPtr<nsISelectionImageService> imageService;
    imageService = do_GetService(kSelectionImageService, &result);
    if (NS_SUCCEEDED(result) && imageService)
    {
      nsCOMPtr<imgIContainer> container;
      imageService->GetImage(selectionValue, getter_AddRefs(container));
      if (container)
      {
        nsRect rect(0, 0, mRect.width, mRect.height);
        rect.IntersectRect(rect,aDirtyRect);
        aRenderingContext.DrawTile(container,0,0,&rect);
      }
    }

    
    
    SelectionDetails *deletingDetails = details;
    while ((deletingDetails = details->mNext) != nsnull) {
      delete details;
      details = deletingDetails;
    }
    delete details;
  }
  return NS_OK;
}

void
nsFrame::PaintSelf(nsIPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   PRIntn               aSkipSides,
                   PRBool               aUsePrintBackgroundSettings)
{
  // The visibility check belongs here since child elements have the
  // opportunity to override the visibility property and display even if
  // their parent is hidden.

  PRBool isVisible;
  if (mRect.height == 0 || mRect.width == 0 ||
      NS_FAILED(IsVisibleForPainting(aPresContext, aRenderingContext,
                                     PR_TRUE, &isVisible)) ||
      !isVisible) {
    return;
  }

  // Paint our background and border
  const nsStyleBorder* border = GetStyleBorder();
  const nsStylePadding* padding = GetStylePadding();
  const nsStyleOutline* outline = GetStyleOutline();

  nsRect rect(0, 0, mRect.width, mRect.height);
  nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                  aDirtyRect, rect, *border, *padding,
                                  aUsePrintBackgroundSettings);
  nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                              aDirtyRect, rect, *border, mStyleContext,
                              aSkipSides);
  nsCSSRendering::PaintOutline(aPresContext, aRenderingContext, this,
                               aDirtyRect, rect, *border, *outline,
                               mStyleContext, 0);
}

/**
  *
 */
NS_IMETHODIMP  
nsFrame::GetContentForEvent(nsIPresContext* aPresContext,
                            nsEvent* aEvent,
                            nsIContent** aContent)
{
  *aContent = GetContent();
  NS_IF_ADDREF(*aContent);
  return NS_OK;
}

/**
  *
 */
NS_IMETHODIMP
nsFrame::HandleEvent(nsIPresContext* aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus*  aEventStatus)
{
  switch (aEvent->message)
  {
  case NS_MOUSE_MOVE:
    {
      HandleDrag(aPresContext, aEvent, aEventStatus);
    }break;
  case NS_MOUSE_LEFT_BUTTON_DOWN:
    {
      HandlePress(aPresContext, aEvent, aEventStatus);
    }break;
  case NS_MOUSE_LEFT_BUTTON_UP:
    {
      HandleRelease(aPresContext, aEvent, aEventStatus);
    } break;
  default:
    break;
  }//end switch
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetDataForTableSelection(nsIFrameSelection *aFrameSelection, 
                                  nsIPresShell *aPresShell, nsMouseEvent *aMouseEvent, 
                                  nsIContent **aParentContent, PRInt32 *aContentOffset, PRInt32 *aTarget)
{
  if (!aFrameSelection || !aPresShell || !aMouseEvent || !aParentContent || !aContentOffset || !aTarget)
    return NS_ERROR_NULL_POINTER;

  *aParentContent = nsnull;
  *aContentOffset = 0;
  *aTarget = 0;

  PRInt16 displaySelection;
  nsresult result = aPresShell->GetSelectionFlags(&displaySelection);
  if (NS_FAILED(result))
    return result;

  PRBool selectingTableCells = PR_FALSE;
  aFrameSelection->GetTableCellSelection(&selectingTableCells);

  // DISPLAY_ALL means we're in an editor.
  // If already in cell selection mode, 
  //  continue selecting with mouse drag or end on mouse up,
  //  or when using shift key to extend block of cells
  //  (Mouse down does normal selection unless Ctrl/Cmd is pressed)
  PRBool doTableSelection =
     displaySelection == nsISelectionDisplay::DISPLAY_ALL && selectingTableCells &&
     (aMouseEvent->message == NS_MOUSE_MOVE ||
      aMouseEvent->message == NS_MOUSE_LEFT_BUTTON_UP || 
      aMouseEvent->isShift);

  if (!doTableSelection)
  {  
    // In Browser, special 'table selection' key must be pressed for table selection
    // or when just Shift is pressed and we're already in table/cell selection mode
#if defined(XP_MAC) || defined(XP_MACOSX)
    doTableSelection = aMouseEvent->isMeta || (aMouseEvent->isShift && selectingTableCells);
#else
    doTableSelection = aMouseEvent->isControl || (aMouseEvent->isShift && selectingTableCells);
#endif
  }
  if (!doTableSelection) 
    return NS_OK;

  // Get the cell frame or table frame (or parent) of the current content node
  nsIFrame *frame = this;
  PRBool foundCell = PR_FALSE;
  PRBool foundTable = PR_FALSE;

  // Get the limiting node to stop parent frame search
  nsCOMPtr<nsIContent> limiter;
  result = aFrameSelection->GetLimiter(getter_AddRefs(limiter));

  //We don't initiate row/col selection from here now,
  //  but we may in future
  //PRBool selectColumn = PR_FALSE;
  //PRBool selectRow = PR_FALSE;

  while (frame && NS_SUCCEEDED(result))
  {
    // Check for a table cell by querying to a known CellFrame interface
    nsITableCellLayout *cellElement;
    result = (frame)->QueryInterface(NS_GET_IID(nsITableCellLayout), (void **)&cellElement);
    if (NS_SUCCEEDED(result) && cellElement)
    {
      foundCell = PR_TRUE;
      //TODO: If we want to use proximity to top or left border
      //      for row and column selection, this is the place to do it
      break;
    }
    else
    {
      // If not a cell, check for table
      // This will happen when starting frame is the table or child of a table,
      //  such as a row (we were inbetween cells or in table border)
      nsITableLayout *tableElement;
      result = (frame)->QueryInterface(NS_GET_IID(nsITableLayout), (void **)&tableElement);
      if (NS_SUCCEEDED(result) && tableElement)
      {
        foundTable = PR_TRUE;
        //TODO: How can we select row when along left table edge
        //  or select column when along top edge?
        break;
      } else {
        frame = frame->GetParent();
        result = NS_OK;
        // Stop if we have hit the selection's limiting content node
        if (frame && frame->GetContent() == limiter.get())
          break;
      }
    }
  }
  // We aren't in a cell or table
  if (!foundCell && !foundTable) return NS_OK;

  nsIContent* tableOrCellContent = frame->GetContent();
  if (!tableOrCellContent) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> parentContent = tableOrCellContent->GetParent();
  if (!parentContent) return NS_ERROR_FAILURE;

  PRInt32 offset = parentContent->IndexOf(tableOrCellContent);
  // Not likely?
  if (offset < 0) return NS_ERROR_FAILURE;

  // Everything is OK -- set the return values
  *aParentContent = parentContent;
  NS_ADDREF(*aParentContent);

  *aContentOffset = offset;

#if 0
  if (selectRow)
    *aTarget = nsISelectionPrivate::TABLESELECTION_ROW;
  else if (selectColumn)
    *aTarget = nsISelectionPrivate::TABLESELECTION_COLUMN;
  else 
#endif
  if (foundCell)
    *aTarget = nsISelectionPrivate::TABLESELECTION_CELL;
  else if (foundTable)
    *aTarget = nsISelectionPrivate::TABLESELECTION_TABLE;

  return NS_OK;
}

/*
NS_IMETHODIMP
nsFrame::FrameOrParentHasSpecialSelectionStyle(PRUint8 aSelectionStyle, nsIFrame* *foundFrame)
{
  nsIFrame* thisFrame = this;
  
  while (thisFrame)
  {
    if (thisFrame->GetStyleUserInterface()->mUserSelect == aSelectionStyle)
    {
      *foundFrame = thisFrame;
      return NS_OK;
    }
  
    thisFrame = thisFrame->GetParent();
  }
  
  *foundFrame = nsnull;
  return NS_OK;
}
*/

NS_IMETHODIMP
nsFrame::IsSelectable(PRBool* aSelectable, PRUint8* aSelectStyle) const
{
  if (!aSelectable) //its ok if aSelectStyle is null
    return NS_ERROR_NULL_POINTER;

  // Like 'visibility', we must check all the parents: if a parent
  // is not selectable, none of its children is selectable.
  //
  // The -moz-all value acts similarly: if a frame has 'user-select:-moz-all',
  // all its children are selectable, even those with 'user-select:none'.
  //
  // As a result, if 'none' and '-moz-all' are not present in the frame hierarchy,
  // aSelectStyle returns the first style that is not AUTO. If these values
  // are present in the frame hierarchy, aSelectStyle returns the style of the
  // topmost parent that has either 'none' or '-moz-all'.
  //
  // For instance, if the frame hierarchy is:
  //    AUTO     -> _MOZ_ALL -> NONE -> TEXT,     the returned value is _MOZ_ALL
  //    TEXT     -> NONE     -> AUTO -> _MOZ_ALL, the returned value is NONE
  //    _MOZ_ALL -> TEXT     -> AUTO -> AUTO,     the returned value is _MOZ_ALL
  //    AUTO     -> CELL     -> TEXT -> AUTO,     the returned value is TEXT
  //
  PRUint8 selectStyle  = NS_STYLE_USER_SELECT_AUTO;
  nsIFrame* frame      = (nsIFrame*)this;

  while (frame) {
    const nsStyleUIReset* userinterface = frame->GetStyleUIReset();
    switch (userinterface->mUserSelect) {
      case NS_STYLE_USER_SELECT_ALL:
      case NS_STYLE_USER_SELECT_NONE:
      case NS_STYLE_USER_SELECT_MOZ_ALL:
        // override the previous values
        selectStyle = userinterface->mUserSelect;
        break;
      default:
        // otherwise return the first value which is not 'auto'
        if (selectStyle == NS_STYLE_USER_SELECT_AUTO) {
          selectStyle = userinterface->mUserSelect;
        }
        break;
    }
    frame = frame->GetParent();
  }

  // convert internal values to standard values
  if (selectStyle == NS_STYLE_USER_SELECT_AUTO)
    selectStyle = NS_STYLE_USER_SELECT_TEXT;
  else
  if (selectStyle == NS_STYLE_USER_SELECT_MOZ_ALL)
    selectStyle = NS_STYLE_USER_SELECT_ALL;

  // return stuff
  if (aSelectable)
    *aSelectable = (selectStyle != NS_STYLE_USER_SELECT_NONE);
  if (aSelectStyle)
    *aSelectStyle = selectStyle;
  if (mState & NS_FRAME_GENERATED_CONTENT)
    *aSelectable = PR_FALSE;
  return NS_OK;
}

PRBool
ContentContainsPoint(nsIPresContext *aPresContext,
                     nsIContent *aContent,
                     nsPoint *aPoint,
                     nsIView *aRelativeView)
{
  nsIPresShell *presShell = aPresContext->GetPresShell();

  if (!presShell) return PR_FALSE;

  nsIFrame *frame = nsnull;

  nsresult rv = presShell->GetPrimaryFrameFor(aContent, &frame);

  if (NS_FAILED(rv) || !frame) return PR_FALSE;

  nsIView *frameView = nsnull;
  nsPoint offsetPoint;

  // Get the view that contains the content's frame.

  rv = frame->GetOffsetFromView(aPresContext, offsetPoint, &frameView);

  if (NS_FAILED(rv) || !frameView) return PR_FALSE;

  // aPoint is relative to aRelativeView's upper left corner! Make sure
  // that our point is in the same view space our content frame's
  // rects are in.

  nsPoint point(*aPoint);

  if (frameView != aRelativeView) {
    // Views are different, just assume frameView is an ancestor
    // of aRelativeView and walk up the view hierarchy calculating what
    // the actual point is, relative to frameView.

    while (aRelativeView && aRelativeView != frameView) {
      point += aRelativeView->GetPosition();
      aRelativeView = aRelativeView->GetParent();
    }

    // At this point the point should be in the correct
    // view coordinate space. If not, just bail!

    if (aRelativeView != frameView) return PR_FALSE;
  }

  // Now check to see if the point is within the bounds of the
  // content's primary frame, or any of it's continuation frames.

  while (frame) {
    // Get the frame's rect and make it relative to the
    // upper left corner of it's parent view.

    nsRect frameRect = frame->GetRect();
    frameRect.x = offsetPoint.x;
    frameRect.y = offsetPoint.y;

    if (frameRect.x <= point.x &&
        frameRect.XMost() >= point.x &&
        frameRect.y <= point.y &&
        frameRect.YMost() >= point.y) {
      // point is within this frame's rect!
      return PR_TRUE;
    }

    rv = frame->GetNextInFlow(&frame);

    if (NS_FAILED(rv)) return PR_FALSE;
  }

  return PR_FALSE;
}

/**
  * Handles the Mouse Press Event for the frame
 */
NS_IMETHODIMP
nsFrame::HandlePress(nsIPresContext* aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus*  aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  //We often get out of sync state issues with mousedown events that
  //get interrupted by alerts/dialogs.
  //Check with the ESM to see if we should process this one
  PRBool eventOK;
  aPresContext->EventStateManager()->EventStatusOK(aEvent, &eventOK);
  if (!eventOK) 
    return NS_OK;

  nsresult rv;
  nsIPresShell *shell = aPresContext->GetPresShell();
  if (!shell)
    return NS_ERROR_FAILURE;

  // if we are in Navigator and the click is in a link, we don't want to start
  // selection because we don't want to interfere with a potential drag of said
  // link and steal all its glory.
  PRInt16 isEditor = 0;
  shell->GetSelectionFlags ( &isEditor );
  //weaaak. only the editor can display frame selction not just text and images
  isEditor = isEditor == nsISelectionDisplay::DISPLAY_ALL;

  nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
  if (!isEditor && !keyEvent->isAlt) {
    static NS_NAMED_LITERAL_STRING(simple, "simple");
    
    for (nsIContent* content = mContent; content;
         content = content->GetParent()) {
       // are we a link with an href? If so, bail out now!
       nsAutoString href;
       // a?
       nsCOMPtr<nsIDOMHTMLAnchorElement> a(do_QueryInterface(content));
       if (a) {
         a->GetHref(href);
       } else {
         // area?
        nsCOMPtr<nsIDOMHTMLAreaElement> area(do_QueryInterface(content));
         if (area) {
           area->GetHref(href);
         } else {
           // img?
           nsCOMPtr<nsIDOMHTMLImageElement> img(do_QueryInterface(content));
           if (img) {
             img->GetSrc(href);
           } else {
            // input (image type) ?
            nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(content));
            if (inputElement) {
              nsAutoString type;
              rv = inputElement->GetType(type);
              if (NS_SUCCEEDED(rv) &&
                  type.Equals(NS_LITERAL_STRING("image"), nsCaseInsensitiveStringComparator()))
                inputElement->GetSrc(href);
            } else {
              // XLink ?
              nsAutoString value;
              content->GetAttr(kNameSpaceID_XLink, nsHTMLAtoms::type, value);
              if (value.Equals(simple))
                content->GetAttr(kNameSpaceID_XLink, nsHTMLAtoms::href, href);
            }
           }
         }
       }
       // Fix for bug #53326: Make sure we bail only
       // in the presence of an href with a value!
       if ( !href.IsEmpty() ) {
         // coordinate stuff is the fix for bug #55921
         nsIView *dummyView = 0;
         nsRect frameRect = mRect;
         nsPoint offsetPoint;

         GetOffsetFromView(aPresContext, offsetPoint, &dummyView);

         frameRect.x = offsetPoint.x;
         frameRect.y = offsetPoint.y;

         if (frameRect.x <= aEvent->point.x && (frameRect.x + frameRect.width >= aEvent->point.x) &&
             frameRect.y <= aEvent->point.y && (frameRect.y + frameRect.height >= aEvent->point.y))
           return NS_OK;
       }
    } // if browser, not editor
  }

  // check whether style allows selection
  // if not, don't tell selection the mouse event even occurred.  
  PRBool  selectable;
  PRUint8 selectStyle;
  rv = IsSelectable(&selectable, &selectStyle);
  if (NS_FAILED(rv)) return rv;
  
  // check for select: none
  if (!selectable)
    return NS_OK;

  // When implementing NS_STYLE_USER_SELECT_ELEMENT, NS_STYLE_USER_SELECT_ELEMENTS and
  // NS_STYLE_USER_SELECT_TOGGLE, need to change this logic
  PRBool useFrameSelection = (selectStyle == NS_STYLE_USER_SELECT_TEXT);

  if (!IsMouseCaptured(aPresContext))
    CaptureMouse(aPresContext, PR_TRUE);

  PRInt16 displayresult = nsISelectionController::SELECTION_OFF;
  nsCOMPtr<nsISelectionController> selCon;
  rv = GetSelectionController(aPresContext, getter_AddRefs(selCon));
  //get the selection controller
  if (NS_SUCCEEDED(rv) && selCon) 
  {
    selCon->GetDisplaySelection(&displayresult);
    if (displayresult == nsISelectionController::SELECTION_OFF)
      return NS_OK;//nothing to do we cannot affect selection from here
  }

  //get the frame selection from sel controller

  // nsFrameState  state = GetStateBits();
  // if (state & NS_FRAME_INDEPENDENT_SELECTION) 
  nsCOMPtr<nsIFrameSelection> frameselection;

  if (useFrameSelection)
    frameselection = do_QueryInterface(selCon); //this MAY implement

  if (!frameselection)//if we must get it from the pres shell's
    rv = shell->GetFrameSelection(getter_AddRefs(frameselection));

  if (!frameselection)
    return NS_ERROR_FAILURE;

  nsMouseEvent *me = (nsMouseEvent *)aEvent;

#if defined(XP_MAC) || defined(XP_MACOSX)
  if (me->isControl)
    return NS_OK;//short ciruit. hard coded for mac due to time restraints.
#endif
    
  if (me->clickCount >1 )
  {
    rv = frameselection->SetMouseDownState( PR_TRUE );
  
    frameselection->SetMouseDoubleDown(PR_TRUE);
    return HandleMultiplePress(aPresContext, aEvent, aEventStatus);
  }

  nsCOMPtr<nsIContent> content;
  PRInt32 startOffset = 0, endOffset = 0;
  PRBool  beginFrameContent = PR_FALSE;

  rv = GetContentAndOffsetsFromPoint(aPresContext, aEvent->point, getter_AddRefs(content), startOffset, endOffset, beginFrameContent);
  // do we have CSS that changes selection behaviour?
  PRBool changeSelection = PR_FALSE;
  {
    nsCOMPtr<nsIContent>  selectContent;
    PRInt32   newStart, newEnd;
    if (NS_SUCCEEDED(frameselection->AdjustOffsetsFromStyle(this, &changeSelection, getter_AddRefs(selectContent), &newStart, &newEnd))
      && changeSelection)
    {
      content = selectContent;
      startOffset = newStart;
      endOffset = newEnd;
    }
  }

  if (NS_FAILED(rv))
    return rv;

  // Let Ctrl/Cmd+mouse down do table selection instead of drag initiation
  nsCOMPtr<nsIContent>parentContent;
  PRInt32  contentOffset;
  PRInt32 target;
  rv = GetDataForTableSelection(frameselection, shell, me, getter_AddRefs(parentContent), &contentOffset, &target);
  if (NS_SUCCEEDED(rv) && parentContent)
  {
    rv = frameselection->SetMouseDownState( PR_TRUE );
    if (NS_FAILED(rv)) return rv;
  
    return frameselection->HandleTableSelection(parentContent, contentOffset, target, me);
  }

  PRBool supportsDelay = PR_FALSE;

  frameselection->GetDelayCaretOverExistingSelection(&supportsDelay);
  frameselection->SetDelayedCaretData(0);

  if (supportsDelay)
  {
    // Check if any part of this frame is selected, and if the
    // user clicked inside the selected region. If so, we delay
    // starting a new selection since the user may be trying to
    // drag the selected region to some other app.

    SelectionDetails *details = 0;
    PRBool isSelected = ((GetStateBits() & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT);

    if (isSelected)
    {
      rv = frameselection->LookUpSelection(content, 0, endOffset, &details, PR_FALSE);

      if (NS_FAILED(rv))
        return rv;

      //
      // If there are any details, check to see if the user clicked
      // within any selected region of the frame.
      //

      if (details)
      {
        SelectionDetails *curDetail = details;

        while (curDetail)
        {
          //
          // If the user clicked inside a selection, then just
          // return without doing anything. We will handle placing
          // the caret later on when the mouse is released.
          //
          if (curDetail->mStart <= startOffset && endOffset <= curDetail->mEnd)
          {
            delete details;
            rv = frameselection->SetMouseDownState( PR_FALSE );

            if (NS_FAILED(rv))
              return rv;

            return frameselection->SetDelayedCaretData(me);
          }

          curDetail = curDetail->mNext;
        }

        delete details;
      }
    }
  }

  rv = frameselection->SetMouseDownState( PR_TRUE );

  if (NS_FAILED(rv))
    return rv;

  rv = frameselection->HandleClick(content, startOffset , endOffset, me->isShift, PR_FALSE, beginFrameContent);

  if (NS_FAILED(rv))
    return rv;

  if (startOffset != endOffset)
    frameselection->MaintainSelection();

  if (isEditor && !me->isShift && (endOffset - startOffset) == 1)
  {
    // A single node is selected and we aren't extending an existing
    // selection, which means the user clicked directly on an object.
    // Check if the user clicked in a -moz-user-select:all subtree,
    // image, or hr. If so, we want to give the drag and drop
    // code a chance to execute so we need to turn off selection extension
    // when processing mouse move/drag events that follow this mouse
    // down event.

    PRBool disableDragSelect = PR_FALSE;

    if (changeSelection)
    {
      // The click hilited a -moz-user-select:all subtree.
      //
      // XXX: We really should be able to just do a:
      //
      //        disableDragSelect = PR_TRUE;
      //
      //      but we are working around the fact that in some cases,
      //      selection selects a -moz-user-select:all subtree even
      //      when the click was outside of the subtree. An example of
      //      this case would be when the subtree is at the end of a
      //      line and the user clicks to the right of it. In this case
      //      I would expect the caret to be placed next to the root of
      //      the subtree, but right now the whole subtree gets selected.
      //      This means that we have to do geometric frame containment
      //      checks on the point to see if the user truly clicked
      //      inside the subtree.
      
      nsIView *view = nsnull;
      nsPoint dummyPoint;

      // aEvent->point is relative to the upper left corner of the
      // frame's parent view. Unfortunately, the only way to get
      // the parent view is to call GetOffsetFromView().

      GetOffsetFromView(aPresContext, dummyPoint, &view);

      // Now check to see if the point is truly within the bounds
      // of any of the frames that make up the -moz-user-select:all subtree:

      if (view)
        disableDragSelect = ContentContainsPoint(aPresContext, content,
                                                 &aEvent->point, view);
    }
    else
    {
      // Check if click was in an image.

      nsIContent* frameContent = GetContent();
      nsCOMPtr<nsIDOMHTMLImageElement> img(do_QueryInterface(frameContent));

      disableDragSelect = img != nsnull;

      if (!img)
      {
        // Check if click was in an hr.

        nsCOMPtr<nsIDOMHTMLHRElement> hr(do_QueryInterface(frameContent));
        disableDragSelect = hr != nsnull;
      }
    }

    if (disableDragSelect)
    {
      // Click was in one of our draggable objects, so disable
      // selection extension during mouse moves.

      rv = frameselection->SetMouseDownState( PR_FALSE );
    }
  }

  return rv;
}
 
/**
  * Multiple Mouse Press -- line or paragraph selection -- for the frame.
  * Wouldn't it be nice if this didn't have to be hardwired into Frame code?
 */
NS_IMETHODIMP
nsFrame::HandleMultiplePress(nsIPresContext* aPresContext, 
                             nsGUIEvent*     aEvent,
                             nsEventStatus*  aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  nsresult rv;
  if (DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF) {
    return NS_OK;
  }

  // Find out whether we're doing line or paragraph selection.
  // Currently, triple-click selects line unless the user sets
  // browser.triple_click_selects_paragraph; quadruple-click
  // selects paragraph, if any platform actually implements it.
  PRBool selectPara = PR_FALSE;
  nsMouseEvent *me = (nsMouseEvent *)aEvent;
  if (!me) return NS_OK;

  if (me->clickCount == 4)
    selectPara = PR_TRUE;
  else if (me->clickCount == 3)
  {
    nsCOMPtr<nsIPrefBranch> prefBranch( do_GetService(NS_PREFSERVICE_CONTRACTID) );
    if (prefBranch)
      prefBranch->GetBoolPref("browser.triple_click_selects_paragraph", &selectPara);
  }
  else
    return NS_OK;
#ifdef DEBUG_akkana
  if (selectPara) printf("Selecting Paragraph\n");
  else printf("Selecting Line\n");
#endif

  // Line or paragraph selection:
  PRInt32 startPos = 0;
  PRInt32 contentOffsetEnd = 0;
  nsCOMPtr<nsIContent> newContent;
  PRBool beginContent = PR_FALSE;
  rv = GetContentAndOffsetsFromPoint(aPresContext,
                                     aEvent->point,
                                     getter_AddRefs(newContent),
                                     startPos,
                                     contentOffsetEnd,
                                     beginContent);
  if (NS_FAILED(rv)) return rv;
  
  
  return PeekBackwardAndForward(selectPara ? eSelectParagraph
                                           : eSelectBeginLine,
                                selectPara ? eSelectParagraph
                                           : eSelectEndLine,
                                startPos, aPresContext, PR_TRUE);
}

NS_IMETHODIMP
nsFrame::PeekBackwardAndForward(nsSelectionAmount aAmountBack,
                                nsSelectionAmount aAmountForward,
                                PRInt32 aStartPos,
                                nsIPresContext* aPresContext,
                                PRBool aJumpLines)
{
  nsCOMPtr<nsISelectionController> selcon;
  nsresult rv = GetSelectionController(aPresContext, getter_AddRefs(selcon));
  if (NS_FAILED(rv)) return rv;

  nsIPresShell *shell = aPresContext->GetPresShell();
  if (!shell || !selcon)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIFocusTracker> tracker;
  tracker = do_QueryInterface(shell, &rv);
  if (NS_FAILED(rv) || !tracker)
    return rv;

  // Use peek offset one way then the other:
  nsCOMPtr<nsIContent> startContent;
  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIContent> endContent;
  nsCOMPtr<nsIDOMNode> endNode;
  nsPeekOffsetStruct startpos;
  startpos.SetData(tracker, 
                   0, 
                   aAmountBack,
                   eDirPrevious,
                   aStartPos,
                   PR_FALSE,
                   PR_TRUE,
                   aJumpLines,
                   PR_TRUE,  //limit on scrolled views
                   PR_FALSE);
  rv = PeekOffset(aPresContext, &startpos);
  if (NS_FAILED(rv))
    return rv;
  nsPeekOffsetStruct endpos;
  endpos.SetData(tracker, 
                 0, 
                 aAmountForward,
                 eDirNext,
                 aStartPos,
                 PR_FALSE,
                 PR_FALSE,
                 aJumpLines,
                 PR_TRUE,  //limit on scrolled views
                 PR_FALSE);
  rv = PeekOffset(aPresContext, &endpos);
  if (NS_FAILED(rv))
    return rv;

  endNode = do_QueryInterface(endpos.mResultContent, &rv);
  if (NS_FAILED(rv))
    return rv;
  startNode = do_QueryInterface(startpos.mResultContent, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISelection> selection;
  if (NS_SUCCEEDED(selcon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                        getter_AddRefs(selection)))){
    rv = selection->Collapse(startNode,startpos.mContentOffset);
    if (NS_FAILED(rv))
      return rv;
    rv = selection->Extend(endNode,endpos.mContentOffset);
    if (NS_FAILED(rv))
      return rv;
  }
  //no release 

  // maintain selection
  nsCOMPtr<nsIFrameSelection> frameselection = do_QueryInterface(selcon); //this MAY implement
  if (!frameselection)
  {
    rv = aPresContext->PresShell()->GetFrameSelection(getter_AddRefs(frameselection));
    if (NS_FAILED(rv) || !frameselection)
      return NS_OK;   // return NS_OK; we don't care if this fails
  }

  return frameselection->MaintainSelection();
}

NS_IMETHODIMP nsFrame::HandleDrag(nsIPresContext* aPresContext, 
                                  nsGUIEvent*     aEvent,
                                  nsEventStatus*  aEventStatus)
{
  PRBool  selectable;
  IsSelectable(&selectable, nsnull);
  if (!selectable)
    return NS_OK;
  if (DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF) {
    return NS_OK;
  }
  nsIPresShell *presShell = aPresContext->PresShell();

  nsCOMPtr<nsIFrameSelection> frameselection;
  nsCOMPtr<nsISelectionController> selCon;
  
  nsresult result = GetSelectionController(aPresContext,
                                           getter_AddRefs(selCon));
  if (NS_SUCCEEDED(result) && selCon)
  {
    frameselection = do_QueryInterface(selCon); //this MAY implement
  }
  if (!frameselection)
    result = presShell->GetFrameSelection(getter_AddRefs(frameselection));

  if (NS_SUCCEEDED(result) && frameselection)
  {
    PRBool mouseDown = PR_FALSE;
    if (NS_SUCCEEDED(frameselection->GetMouseDownState(&mouseDown)) && !mouseDown)
      return NS_OK;            

    // check whether style allows selection
    // if not, don't tell selection the mouse event even occurred.  
    PRBool  selectable;
    PRUint8 selectStyle;
    result = IsSelectable(&selectable, &selectStyle);
    if (NS_FAILED(result)) 
      return result;

    // check for select: none
    if (selectable)
    {
      frameselection->StopAutoScrollTimer();
      // Check if we are dragging in a table cell
      nsCOMPtr<nsIContent> parentContent;
      PRInt32 contentOffset;
      PRInt32 target;
      nsMouseEvent *me = (nsMouseEvent *)aEvent;
      result = GetDataForTableSelection(frameselection, presShell, me, getter_AddRefs(parentContent), &contentOffset, &target);      

      if (NS_SUCCEEDED(result) && parentContent)
        frameselection->HandleTableSelection(parentContent, contentOffset, target, me);
      else
        frameselection->HandleDrag(aPresContext, this, aEvent->point);
      
      frameselection->StartAutoScrollTimer(aPresContext, this, aEvent->point, 30);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsFrame::HandleRelease(nsIPresContext* aPresContext, 
                                     nsGUIEvent*     aEvent,
                                     nsEventStatus*  aEventStatus)
{
  if (IsMouseCaptured(aPresContext))
    CaptureMouse(aPresContext, PR_FALSE);

  if (DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF)
    return NS_OK;

  nsIPresShell *presShell = aPresContext->GetPresShell();

  if (!presShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIFrameSelection> frameselection;
  nsCOMPtr<nsISelectionController> selCon;
  
  nsresult result = GetSelectionController(aPresContext,
                                           getter_AddRefs(selCon));
    
  if (NS_SUCCEEDED(result) && selCon)
    frameselection = do_QueryInterface(selCon); //this MAY implement
  if (!frameselection)
    result = presShell->GetFrameSelection(getter_AddRefs(frameselection));

  if (NS_FAILED(result))
    return result;

  if (!frameselection)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault != *aEventStatus) {
    PRBool supportsDelay = PR_FALSE;

    frameselection->GetDelayCaretOverExistingSelection(&supportsDelay);

    if (supportsDelay)
    {
      // Check if the frameselection recorded the mouse going down.
      // If not, the user must have clicked in a part of the selection.
      // Place the caret before continuing!

      PRBool mouseDown = PR_FALSE;

      result = frameselection->GetMouseDownState(&mouseDown);

      if (NS_FAILED(result))
        return result;

      nsMouseEvent *me = 0;

      result = frameselection->GetDelayedCaretData(&me);

      if (NS_SUCCEEDED(result) && !mouseDown && me && me->clickCount < 2)
      {
        // We are doing this to simulate what we would have done on HandlePress
        result = frameselection->SetMouseDownState( PR_TRUE );

        nsCOMPtr<nsIContent> content;
        PRInt32 startOffset = 0, endOffset = 0;
        PRBool  beginFrameContent = PR_FALSE;

        result = GetContentAndOffsetsFromPoint(aPresContext, me->point, getter_AddRefs(content), startOffset, endOffset, beginFrameContent);
        if (NS_FAILED(result)) return result;

      // do we have CSS that changes selection behaviour?
      {
        PRBool    changeSelection;
        nsCOMPtr<nsIContent>  selectContent;
        PRInt32   newStart, newEnd;
        if (NS_SUCCEEDED(frameselection->AdjustOffsetsFromStyle(this, &changeSelection, getter_AddRefs(selectContent), &newStart, &newEnd))
          && changeSelection)
        {
          content = selectContent;
          startOffset = newStart;
          endOffset = newEnd;
        }
      }

        result = frameselection->HandleClick(content, startOffset , endOffset, me->isShift, PR_FALSE, beginFrameContent);
        if (NS_FAILED(result)) return result;
      }
      else
      {
        me = (nsMouseEvent *)aEvent;
        nsCOMPtr<nsIContent>parentContent;
        PRInt32  contentOffset;
        PRInt32 target;
        result = GetDataForTableSelection(frameselection, presShell, me, getter_AddRefs(parentContent), &contentOffset, &target);

        if (NS_SUCCEEDED(result) && parentContent)
        {
          frameselection->SetMouseDownState( PR_FALSE );
          result = frameselection->HandleTableSelection(parentContent, contentOffset, target, me);
          if (NS_FAILED(result)) return result;
        }
      }
      result = frameselection->SetDelayedCaretData(0);
    }
  }

  // Now handle the normal HandleRelase business.

  if (NS_SUCCEEDED(result) && frameselection) {
    frameselection->SetMouseDownState( PR_FALSE );
    frameselection->StopAutoScrollTimer();
  }

  return NS_OK;
}


nsresult nsFrame::GetContentAndOffsetsFromPoint(nsIPresContext* aCX,
                                                const nsPoint&  aPoint,
                                                nsIContent **   aNewContent,
                                                PRInt32&        aContentOffset,
                                                PRInt32&        aContentOffsetEnd,
                                                PRBool&         aBeginFrameContent)
{
  if (!aNewContent)
    return NS_ERROR_NULL_POINTER;

  // Traverse through children and look for the best one to give this
  // to if it fails the getposition call, make it yourself also only
  // look at primary list
  nsIFrame *closestFrame = nsnull;
  nsIView *view = GetClosestView();
  nsIFrame *kid = GetFirstChild(nsnull);

  if (kid) {
#define HUGE_DISTANCE 999999 //some HUGE number that will always fail first comparison

    PRInt32 closestXDistance = HUGE_DISTANCE;
    PRInt32 closestYDistance = HUGE_DISTANCE;

    while (nsnull != kid) {

      // Skip over generated content kid frames, or frames
      // that don't have a proper parent-child relationship!

      PRBool skipThisKid = (kid->GetStateBits() & NS_FRAME_GENERATED_CONTENT) != 0;
#if 0
      if (!skipThisKid) {
        // The frame's content is not generated. Now check
        // if it is anonymous content!

        nsIContent* kidContent = kid->GetContent();
        if (kidContent) {
          nsCOMPtr<nsIContent> content = kidContent->GetParent();

          if (content) {
            PRInt32 kidCount = content->ChildCount();
            PRInt32 kidIndex = content->IndexOf(kidContent);

            // IndexOf() should return -1 for the index if it doesn't
            // find kidContent in it's child list.

            if (kidIndex < 0 || kidIndex >= kidCount) {
              // Must be anonymous content! So skip it!
              skipThisKid = PR_TRUE;
            }
          }
        }
      }
#endif //XXX we USED to skip anonymous content i dont think we should anymore leaving this here as a flah

      if (skipThisKid) {
        kid = kid->GetNextSibling();
        continue;
      }

      // Kid frame has content that has a proper parent-child
      // relationship. Now see if the aPoint inside it's bounding
      // rect or close by.

      nsPoint offsetPoint(0,0);
      nsIView * kidView = nsnull;
      kid->GetOffsetFromView(aCX, offsetPoint, &kidView);

      nsRect rect = kid->GetRect();
      rect.x = offsetPoint.x;
      rect.y = offsetPoint.y;

      nscoord fromTop = aPoint.y - rect.y;
      nscoord fromBottom = aPoint.y - rect.y - rect.height;

      PRInt32 yDistance;
      if (fromTop > 0 && fromBottom < 0)
        yDistance = 0;
      else
        yDistance = PR_MIN(abs(fromTop), abs(fromBottom));

      if (yDistance <= closestYDistance && rect.width > 0 && rect.height > 0)
      {
        if (yDistance < closestYDistance)
          closestXDistance = HUGE_DISTANCE;

        nscoord fromLeft = aPoint.x - rect.x;
        nscoord fromRight = aPoint.x - rect.x - rect.width;

        PRInt32 xDistance;
        if (fromLeft > 0 && fromRight < 0)
          xDistance = 0;
        else
          xDistance = PR_MIN(abs(fromLeft), abs(fromRight));

        if (xDistance == 0 && yDistance == 0)
        {
          closestFrame = kid;
          break;
        }

        if (xDistance < closestXDistance || (xDistance == closestXDistance && rect.x <= aPoint.x))
        {
          closestXDistance = xDistance;
          closestYDistance = yDistance;
          closestFrame     = kid;
        }
        // else if (xDistance > closestXDistance)
        //   break;//done
      }
    
      kid = kid->GetNextSibling();
    }
    if (closestFrame) {

      // If we cross a view boundary, we need to adjust
      // the coordinates because GetPosition() expects
      // them to be relative to the closest view.

      nsPoint newPoint     = aPoint;
      nsIView *closestView = closestFrame->GetClosestView();

      if (closestView && view != closestView)
        newPoint -= closestView->GetPosition();

      // printf("      0x%.8x   0x%.8x  %4d  %4d\n",
      //        closestFrame, closestView, closestXDistance, closestYDistance);

      return closestFrame->GetContentAndOffsetsFromPoint(aCX, newPoint, aNewContent,
                                                         aContentOffset, aContentOffsetEnd,aBeginFrameContent);
    }
  }

  if (!mContent)
    return NS_ERROR_NULL_POINTER;

  nsPoint offsetPoint;
  GetOffsetFromView(aCX, offsetPoint, &view);
  nsRect thisRect = GetRect();
  thisRect.x = offsetPoint.x;
  thisRect.y = offsetPoint.y;

  NS_IF_ADDREF(*aNewContent = mContent->GetParent());
  if (*aNewContent){
    
    PRInt32 contentOffset(aContentOffset); //temp to hold old value in case of failure
    
    contentOffset = (*aNewContent)->IndexOf(mContent);
    if (contentOffset < 0) 
    {
      return NS_ERROR_FAILURE;
    }
    aContentOffset = contentOffset; //its clear save the result

    aBeginFrameContent = PR_TRUE;
    if (thisRect.Contains(aPoint))
      aContentOffsetEnd = aContentOffset +1;
    else 
    {
      //if we are a collapsed frame then dont check to see if we need to skip past this content
      //see bug http://bugzilla.mozilla.org/show_bug.cgi?id=103888
      if (thisRect.width && thisRect.height && ((thisRect.x + thisRect.width) < aPoint.x  || thisRect.y > aPoint.y))
      {
        aBeginFrameContent = PR_FALSE;
        aContentOffset++;
      }
      aContentOffsetEnd = aContentOffset;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetCursor(nsIPresContext* aPresContext,
                   nsPoint& aPoint,
                   PRInt32& aCursor)
{
  aCursor = GetStyleUserInterface()->mCursor;
  if (NS_STYLE_CURSOR_AUTO == aCursor) {
    aCursor = NS_STYLE_CURSOR_DEFAULT;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                          const nsPoint& aPoint,
                          nsFramePaintLayer aWhichLayer,
                          nsIFrame** aFrame)
{
  if ((aWhichLayer == NS_FRAME_PAINT_LAYER_FOREGROUND) &&
      (mRect.Contains(aPoint))) {
    if (GetStyleVisibility()->IsVisible()) {
      *aFrame = this;
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

// Resize and incremental reflow

// nsIHTMLReflow member functions

NS_IMETHODIMP
nsFrame::WillReflow(nsIPresContext* aPresContext)
{
#ifdef DEBUG_dbaron_off
  // bug 81268
  NS_ASSERTION(!(mState & NS_FRAME_IN_REFLOW),
               "nsFrame::WillReflow: frame is already in reflow");
#endif

  NS_FRAME_TRACE_MSG(NS_FRAME_TRACE_CALLS,
                     ("WillReflow: oldState=%x", mState));
  mState |= NS_FRAME_IN_REFLOW;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::DidReflow(nsIPresContext*           aPresContext,
                   const nsHTMLReflowState*  aReflowState,
                   nsDidReflowStatus         aStatus)
{
  NS_FRAME_TRACE_MSG(NS_FRAME_TRACE_CALLS,
                     ("nsFrame::DidReflow: aStatus=%d", aStatus));
  if (NS_FRAME_REFLOW_FINISHED == aStatus) {
    mState &= ~(NS_FRAME_IN_REFLOW | NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
                NS_FRAME_HAS_DIRTY_CHILDREN);
  }

  // Notify the percent height observer if this is an initial or resize reflow (XXX
  // it should probably be any type of reflow, but this would need further testing)
  // and there is a percent height but no computed height. The observer may be able to
  // initiate another reflow with a computed height. This happens in the case where a table
  // cell has no computed height but can fabricate one when the cell height is known.
  if (aReflowState && (aReflowState->mPercentHeightObserver)           && // an observer
      ((eReflowReason_Initial == aReflowState->reason) ||                 // initial or resize reflow
       (eReflowReason_Resize  == aReflowState->reason))                &&
      ((NS_UNCONSTRAINEDSIZE == aReflowState->mComputedHeight) ||         // no computed height 
       (0                    == aReflowState->mComputedHeight))        && 
      aReflowState->mStylePosition                                     && // percent height
      (eStyleUnit_Percent == aReflowState->mStylePosition->mHeight.GetUnit())) {

    nsIFrame* prevInFlow;
    GetPrevInFlow(&prevInFlow);
    if (!prevInFlow) { // 1st in flow
      aReflowState->mPercentHeightObserver->NotifyPercentHeight(*aReflowState);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFrame::CanContinueTextRun(PRBool& aContinueTextRun) const
{
  // By default, a frame will *not* allow a text run to be continued
  // through it.
  aContinueTextRun = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::Reflow(nsIPresContext*          aPresContext,
                nsHTMLReflowMetrics&     aDesiredSize,
                const nsHTMLReflowState& aReflowState,
                nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsFrame", aReflowState.reason);
  aDesiredSize.width = 0;
  aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  aDesiredSize.descent = 0;
  if (aDesiredSize.mComputeMEW) {
    aDesiredSize.mMaxElementWidth = 0;
  }
  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::AdjustFrameSize(nscoord aExtraSpace, nscoord& aUsedSpace)
{
  aUsedSpace = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::TrimTrailingWhiteSpace(nsIPresContext* aPresContext,
                                nsIRenderingContext& aRC,
                                nscoord& aDeltaWidth)
{
  aDeltaWidth = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::CharacterDataChanged(nsIPresContext* aPresContext,
                              nsIContent*     aChild,
                              PRBool          aAppend)
{
  NS_NOTREACHED("should only be called for text frames");
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::AttributeChanged(nsIPresContext* aPresContext,
                          nsIContent*     aChild,
                          PRInt32         aNameSpaceID,
                          nsIAtom*        aAttribute,
                          PRInt32         aModType)
{
  return NS_OK;
}

// Flow member functions

NS_IMETHODIMP nsFrame::IsSplittable(nsSplittableType& aIsSplittable) const
{
  aIsSplittable = NS_FRAME_NOT_SPLITTABLE;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::GetPrevInFlow(nsIFrame** aPrevInFlow) const
{
  *aPrevInFlow = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetPrevInFlow(nsIFrame* aPrevInFlow)
{
  // Ignore harmless requests to set it to NULL
  if (aPrevInFlow) {
    NS_ERROR("not splittable");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  return NS_OK;
}

NS_IMETHODIMP nsFrame::GetNextInFlow(nsIFrame** aNextInFlow) const
{
  *aNextInFlow = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetNextInFlow(nsIFrame*)
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

// Associated view object
nsIView*
nsIFrame::GetView() const
{
  // Check the frame state bit and see if the frame has a view
  if (!(GetStateBits() & NS_FRAME_HAS_VIEW))
    return nsnull;

  // Check for a property on the frame
  nsresult rv;
  void* value = GetPresContext()->FrameManager()->
    GetFrameProperty(this, nsLayoutAtoms::viewProperty, 0, &rv);

  NS_ENSURE_SUCCESS(rv, nsnull);
  NS_ASSERTION(value, "frame state bit was set but frame has no view");
  return NS_STATIC_CAST(nsIView*, value);
}

/* virtual */ nsIView*
nsIFrame::GetViewExternal() const
{
  return GetView();
}

nsresult
nsIFrame::SetView(nsIView* aView)
{
  if (aView) {
    aView->SetClientData(this);

    // Set a property on the frame
    nsresult rv = GetPresContext()->FrameManager()->
      SetFrameProperty(this, nsLayoutAtoms::viewProperty, aView, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the frame state bit that says the frame has a view
    AddStateBits(NS_FRAME_HAS_VIEW);

    // Let all of the ancestors know they have a descendant with a view.
    for (nsIFrame* f = GetParent();
         f && !(f->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW);
         f = f->GetParent())
      f->AddStateBits(NS_FRAME_HAS_CHILD_WITH_VIEW);
  }

  return NS_OK;
}

nsIFrame* nsIFrame::GetAncestorWithViewExternal() const
{
  return GetAncestorWithView();
}

// Find the first geometric parent that has a view
nsIFrame* nsIFrame::GetAncestorWithView() const
{
  for (nsIFrame* f = mParent; nsnull != f; f = f->GetParent()) {
    if (f->HasView()) {
      return f;
    }
  }
  return nsnull;
}

// Returns the offset from this frame to the closest geometric parent that
// has a view. Also returns the containing view or null in case of error
NS_IMETHODIMP nsFrame::GetOffsetFromView(nsIPresContext* aPresContext,
                                         nsPoint&        aOffset,
                                         nsIView**       aView) const
{
  NS_PRECONDITION(nsnull != aView, "null OUT parameter pointer");
  nsIFrame* frame = (nsIFrame*)this;

  *aView = nsnull;
  aOffset.MoveTo(0, 0);
  do {
    aOffset += frame->GetPosition();
    frame = frame->GetParent();
  } while (frame && !frame->HasView());
  if (frame)
    *aView = frame->GetView();
  return NS_OK;
}

// The (x,y) value of the frame's upper left corner is always
// relative to its parentFrame's upper left corner, unless
// its parentFrame has a view associated with it, in which case, it
// will be relative to the upper left corner of the view returned
// by a call to parentFrame->GetView().
//
// This means that while drilling down the frame hierarchy, from
// parent to child frame, we sometimes need to take into account
// crossing these view boundaries, because the coordinate system
// changes from parent frame coordinate system, to the associated
// view's coordinate system.
//
// GetOriginToViewOffset() is a utility method that returns the
// offset necessary to map a point, relative to the frame's upper
// left corner, into the coordinate system of the view associated
// with the frame.
//
// If there is no view associated with the frame, or the view is
// not a descendant of the frame's parent view (ex: scrolling popup menu),
// the offset returned will be (0,0).

NS_IMETHODIMP nsFrame::GetOriginToViewOffset(nsIPresContext* aPresContext,
                                             nsPoint&        aOffset,
                                             nsIView**       aView) const
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  nsresult rv = NS_OK;

  aOffset.MoveTo(0,0);

  if (aView)
    *aView = nsnull;

  if (HasView()) {
    nsIView *view = GetView();
    nsIView *parentView = nsnull;
    nsPoint offsetToParentView;
    rv = GetOffsetFromView(aPresContext, offsetToParentView, &parentView);

    if (NS_SUCCEEDED(rv)) {
      nsPoint viewOffsetFromParent(0,0);
      nsIView *pview = view;

      nsIViewManager* vVM = view->GetViewManager();

      while (pview && pview != parentView) {
        viewOffsetFromParent += pview->GetPosition();

        nsIView *tmpView = pview->GetParent();
        if (tmpView && vVM != tmpView->GetViewManager()) {
          // Don't cross ViewManager boundaries!
          break;
        }
        pview = tmpView;
      }

#ifdef DEBUG_KIN
      if (pview != parentView) {
        // XXX: At this point, pview is probably null since it traversed
        //      all the way up view's parent hierarchy and did not run across
        //      parentView. In the future, instead of just returning an offset
        //      of (0,0) for this case, we may want offsetToParentView to
        //      include the offset from the parentView to the top of the
        //      view hierarchy which would make both offsetToParentView and
        //      viewOffsetFromParent, offsets to the global coordinate space.
        //      We'd have to investigate any perf impact this would have before
        //      checking in such a change, so for now we just return (0,0).
        //      -- kin    
        NS_WARNING("view is not a descendant of parentView!");
      }
#endif // DEBUG

      if (pview == parentView)
        aOffset = offsetToParentView - viewOffsetFromParent;

      if (aView)
        *aView = view;
    }
  }

  return rv;
}

/* virtual */ PRBool
nsIFrame::AreAncestorViewsVisible() const
{
  for (nsIView* view = GetClosestView(); view; view = view->GetParent()) {
    if (view->GetVisibility() == nsViewVisibility_kHide) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

nsIWidget* nsIFrame::GetWindow() const
{
  const nsIFrame* frame;
  for (frame = this; frame; frame = frame->GetAncestorWithView()) {
    if (frame->HasView()) {
      nsIWidget* window = frame->GetView()->GetWidget();
      if (window) {
        return window;
      }
    }
  }

  // Ask the view manager for the widget
  NS_NOTREACHED("this shouldn't happen, should it?");
  nsIWidget* window;
  GetPresContext()->GetViewManager()->GetWidget(&window);
  // drop refcount that the view manager added, since we are not supposed
  // to be adding a refcount
  if (window) {
    window->Release();
  }

  NS_POSTCONDITION(window, "no window in frame tree");
  return window;
}

nsIAtom*
nsFrame::GetType() const
{
  return nsnull;
}

void
nsIFrame::Invalidate(const nsRect& aDamageRect,
                     PRBool        aImmediate) const
{
  if (aDamageRect.IsEmpty()) {
    return;
  }

  // Don't allow invalidates to do anything when
  // painting is suppressed.
  nsIPresShell *shell = GetPresContext()->GetPresShell();
  if (shell) {
    PRBool suppressed = PR_FALSE;
    shell->IsPaintingSuppressed(&suppressed);
    if (suppressed)
      return;
  }

  nsRect damageRect(aDamageRect);

  PRUint32 flags = aImmediate ? NS_VMREFRESH_IMMEDIATE : NS_VMREFRESH_NO_SYNC;
  if (HasView()) {
    nsIView* view = GetView();
    view->GetViewManager()->UpdateView(view, damageRect, flags);
  } else {
    nsRect    rect(damageRect);
    nsPoint   offset;
  
    nsIView *view;
    GetOffsetFromView(GetPresContext(), offset, &view);
    NS_ASSERTION(view, "no view");
    rect += offset;
    view->GetViewManager()->UpdateView(view, rect, flags);
  }
}

nsRect
nsIFrame::GetOutlineRect(PRBool* aAnyOutline) const
{
  const nsStyleOutline* outline = GetStyleOutline();
  PRUint8 outlineStyle = outline->GetOutlineStyle();
  nsRect r(0, 0, mRect.width, mRect.height);
  PRBool anyOutline = PR_FALSE;
  if (outlineStyle != NS_STYLE_BORDER_STYLE_NONE) {
    nscoord width;
#ifdef DEBUG
    PRBool result = 
#endif
      outline->GetOutlineWidth(width);
    NS_ASSERTION(result, "GetOutlineWidth had no cached outline width");
    if (width > 0) {
      r.Inflate(width, width);
      anyOutline = PR_TRUE;
    }
  }
  if (aAnyOutline) {
    *aAnyOutline = anyOutline;
  }
  return r;
}

void
nsFrame::CheckInvalidateSizeChange(nsIPresContext* aPresContext,
                                   nsHTMLReflowMetrics& aDesiredSize,
                                   const nsHTMLReflowState& aReflowState)
{
  if (aDesiredSize.width == mRect.width
      && aDesiredSize.height == mRect.height)
    return;

  // Below, we invalidate the old frame area (or, in the case of
  // outline, combined area) if the outline, border or background
  // settings indicate that something other than the difference
  // between the old and new areas needs to be painted. We are
  // assuming that the difference between the old and new areas will
  // be invalidated by some other means. That also means invalidating
  // the old frame area is the same as invalidating the new frame area
  // (since in either case the UNION of old and new areas will be
  // invalidated)

  // Invalidate the entire old frame+outline if the frame has an outline

  // This assumes 'outline' is painted outside the element, as CSS2 requires.
  // Currently we actually paint 'outline' inside the element so this code
  // isn't strictly necessary. But we're trying to get ready to switch to
  // CSS2 compliance.
  PRBool anyOutline;
  nsRect r = GetOutlineRect(&anyOutline);
  if (anyOutline) {
    Invalidate(r);
    return;
  }

  // Invalidate the old frame borders if the frame has borders. Those borders
  // may be moving.
  const nsStyleBorder* border = GetStyleBorder();
  if (border->IsBorderSideVisible(NS_SIDE_LEFT)
      || border->IsBorderSideVisible(NS_SIDE_RIGHT)
      || border->IsBorderSideVisible(NS_SIDE_TOP)
      || border->IsBorderSideVisible(NS_SIDE_BOTTOM)) {
    Invalidate(nsRect(0, 0, mRect.width, mRect.height));
    return;
  }

  // Invalidate the old frame background if the frame has a background
  // whose position depends on the size of the frame
  const nsStyleBackground* background = GetStyleBackground();
  if (background->mBackgroundFlags &
      (NS_STYLE_BG_X_POSITION_PERCENT | NS_STYLE_BG_Y_POSITION_PERCENT)) {
    Invalidate(nsRect(0, 0, mRect.width, mRect.height));
    return;
  }
}

// Define the MAX_FRAME_DEPTH to be the ContentSink's MAX_REFLOW_DEPTH plus
// 4 for the frames above the document's frames: 
//  the Viewport, GFXScroll, ScrollPort, and Canvas
#define MAX_FRAME_DEPTH (MAX_REFLOW_DEPTH+4)

PRBool
nsFrame::IsFrameTreeTooDeep(const nsHTMLReflowState& aReflowState,
                            nsHTMLReflowMetrics& aMetrics)
{
  if (aReflowState.mReflowDepth >  MAX_FRAME_DEPTH) {
    mState |= NS_FRAME_IS_UNFLOWABLE;
    mState &= ~NS_FRAME_OUTSIDE_CHILDREN;
    aMetrics.width = 0;
    aMetrics.height = 0;
    aMetrics.ascent = 0;
    aMetrics.descent = 0;
    aMetrics.mCarriedOutBottomMargin.Zero();
    aMetrics.mOverflowArea.x = 0;
    aMetrics.mOverflowArea.y = 0;
    aMetrics.mOverflowArea.width = 0;
    aMetrics.mOverflowArea.height = 0;
    if (aMetrics.mComputeMEW) {
      aMetrics.mMaxElementWidth = 0;
    }
    return PR_TRUE;
  }
  mState &= ~NS_FRAME_IS_UNFLOWABLE;
  return PR_FALSE;
}

// Style sizing methods
/* virtual */ PRBool nsFrame::IsContainingBlock() const
{
  const nsStyleDisplay* display = GetStyleDisplay();

  // Absolute positioning causes |display->mDisplay| to be set to block,
  // if needed.
  return display->mDisplay == NS_STYLE_DISPLAY_BLOCK || 
         display->mDisplay == NS_STYLE_DISPLAY_LIST_ITEM ||
         display->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL;
}

#ifdef NS_DEBUG

PRInt32 nsFrame::ContentIndexInContainer(const nsIFrame* aFrame)
{
  PRInt32 result = -1;

  nsIContent* content = aFrame->GetContent();
  if (content) {
    nsIContent* parentContent = content->GetParent();
    if (parentContent) {
      result = parentContent->IndexOf(content);
    }
  }

  return result;
}

#ifdef DEBUG_waterson

/**
 * List a single frame to stdout. Meant to be called from gdb.
 */
void
DebugListFrame(nsIPresContext* aPresContext, nsIFrame* aFrame)
{
  ((nsFrame*) aFrame)->List(aPresContext, stdout, 0);
  printf("\n");
}

/**
 * List a frame tree to stdout. Meant to be called from gdb.
 */
void
DebugListFrameTree(nsIPresContext* aPresContext, nsIFrame* aFrame)
{
  nsIFrameDebug* fdbg;
  aFrame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**) &fdbg);
  if (fdbg)
    fdbg->List(aPresContext, stdout, 0);
}

#endif


// Debugging
NS_IMETHODIMP
nsFrame::List(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", NS_STATIC_CAST(void*, mParent));
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", NS_STATIC_CAST(void*, GetView()));
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  fprintf(out, " [content=%p]", NS_STATIC_CAST(void*, mContent));
  fputs("\n", out);
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Frame"), aResult);
}

NS_IMETHODIMP_(nsFrameState)
nsFrame::GetDebugStateBits() const
{
  // We'll ignore these flags for the purposes of comparing frame state:
  //
  //   NS_FRAME_EXTERNAL_REFERENCE
  //     because this is set by the event state manager or the
  //     caret code when a frame is focused. Depending on whether
  //     or not the regression tests are run as the focused window
  //     will make this value vary randomly.
#define IRRELEVANT_FRAME_STATE_FLAGS NS_FRAME_EXTERNAL_REFERENCE

#define FRAME_STATE_MASK (~(IRRELEVANT_FRAME_STATE_FLAGS))

  return GetStateBits() & FRAME_STATE_MASK;
}

nsresult
nsFrame::MakeFrameName(const nsAString& aType, nsAString& aResult) const
{
  aResult = aType;
  if (mContent) {
    nsIAtom *tag = mContent->Tag();
    if (tag != nsLayoutAtoms::textTagName) {
      nsAutoString buf;
      tag->ToString(buf);
      aResult.Append(NS_LITERAL_STRING("(") + buf + NS_LITERAL_STRING(")"));
    }
  }
  char buf[40];
  PR_snprintf(buf, sizeof(buf), "(%d)", ContentIndexInContainer(this));
  AppendASCIItoUTF16(buf, aResult);
  return NS_OK;
}
#endif

void
nsFrame::XMLQuote(nsString& aString)
{
  PRInt32 i, len = aString.Length();
  for (i = 0; i < len; i++) {
    PRUnichar ch = aString.CharAt(i);
    if (ch == '<') {
      nsAutoString tmp(NS_LITERAL_STRING("&lt;"));
      aString.Cut(i, 1);
      aString.Insert(tmp, i);
      len += 3;
      i += 3;
    }
    else if (ch == '>') {
      nsAutoString tmp(NS_LITERAL_STRING("&gt;"));
      aString.Cut(i, 1);
      aString.Insert(tmp, i);
      len += 3;
      i += 3;
    }
    else if (ch == '\"') {
      nsAutoString tmp(NS_LITERAL_STRING("&quot;"));
      aString.Cut(i, 1);
      aString.Insert(tmp, i);
      len += 5;
      i += 5;
    }
  }
}

PRBool
nsFrame::ParentDisablesSelection() const
{
/*
  // should never be called now
  nsIFrame* parent = GetParent();
  if (parent) {
	  PRBool selectable;
	  parent->IsSelectable(selectable);
    return (selectable ? PR_FALSE : PR_TRUE);
  }
  return PR_FALSE;
*/
/*
  PRBool selected;
  if (NS_FAILED(GetSelected(&selected)))
    return PR_FALSE;
  if (selected)
    return PR_FALSE; //if this frame is selected and no one has overridden the selection from "higher up"
                     //then no one below us will be disabled by this frame.
  nsIFrame* target = GetParent();
  if (target)
    return ((nsFrame *)target)->ParentDisablesSelection();
  return PR_FALSE; //default this does not happen
  */
  
  return PR_FALSE;
}

nsresult 
nsFrame::GetSelectionForVisCheck(nsIPresContext * aPresContext, nsISelection** aSelection)
{
  *aSelection = nsnull;
  nsresult rv = NS_OK;

  // start by checking to see if we are paginated which probably means
  // we are in print preview or printing
  if (aPresContext->IsPaginated()) {
    // now see if we are rendering selection only
    if (aPresContext->IsRenderingOnlySelection()) {
      // Check the quick way first (typically only leaf nodes)
      PRBool isSelected = (mState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
      // if we aren't selected in the mState,
      // we could be a container so check to see if we are in the selection range
      // this is a expensive
      if (!isSelected) {
        nsIPresShell *shell = aPresContext->GetPresShell();
        if (shell) {
          nsCOMPtr<nsISelectionController> selcon(do_QueryInterface(shell));
          if (selcon) {
            rv = selcon->GetSelection(nsISelectionController::SELECTION_NORMAL, aSelection);
          }
        }
      }
    }
  } 

  return rv;
}


NS_IMETHODIMP
nsFrame::IsVisibleForPainting(nsIPresContext *     aPresContext, 
                              nsIRenderingContext& aRenderingContext,
                              PRBool               aCheckVis,
                              PRBool*              aIsVisible)
{
  // first check to see if we are visible
  if (aCheckVis) {
    if (!GetStyleVisibility()->IsVisible()) {
      *aIsVisible = PR_FALSE;
      return NS_OK;
    }
  }

  // Start by assuming we are visible and need to be painted
  *aIsVisible = PR_TRUE;

  // NOTE: GetSelectionforVisCheck checks the pagination to make sure we are printing
  // In otherwords, the selection will ALWAYS be null if we are not printing, meaning
  // the visibility will be TRUE in that case
  nsCOMPtr<nsISelection> selection;
  nsresult rv = GetSelectionForVisCheck(aPresContext, getter_AddRefs(selection));
  if (NS_SUCCEEDED(rv) && selection) {
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
    selection->ContainsNode(node, PR_TRUE, aIsVisible);
  }

  return rv;
}

/* virtual */ PRBool
nsFrame::IsEmpty()
{
  return PR_FALSE;
}

NS_IMETHODIMP
nsFrame::GetSelectionController(nsIPresContext *aPresContext, nsISelectionController **aSelCon)
{
  if (!aPresContext || !aSelCon)
    return NS_ERROR_INVALID_ARG;
  if (GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION) 
  {
    nsIFrame *tmp = this;
    while (tmp)
    {
      nsITextControlFrame *tcf;
      if (NS_SUCCEEDED(tmp->QueryInterface(NS_GET_IID(nsITextControlFrame),(void**)&tcf)))
      {
        return tcf->GetSelectionContr(aSelCon);
      }
      tmp = tmp->GetParent();
    }
  }
  nsIPresShell *shell = aPresContext->GetPresShell();
  if (shell)
  {
    nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(shell);
    NS_IF_ADDREF(*aSelCon = selCon);
    return NS_OK;
  }
  return NS_OK;
}



#ifdef NS_DEBUG
NS_IMETHODIMP
nsFrame::DumpRegressionData(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent, PRBool aIncludeStyleData)
{
  IndentBy(out, aIndent);
  fprintf(out, "<frame va=\"%ld\" type=\"", PRUptrdiff(this));
  nsAutoString name;
  GetFrameName(name);
  XMLQuote(name);
  fputs(NS_LossyConvertUCS2toASCII(name).get(), out);
  fprintf(out, "\" state=\"%d\" parent=\"%ld\">\n",
          GetDebugStateBits(), PRUptrdiff(mParent));

  aIndent++;
  DumpBaseRegressionData(aPresContext, out, aIndent, aIncludeStyleData);
  aIndent--;

  IndentBy(out, aIndent);
  fprintf(out, "</frame>\n");

  return NS_OK;
}

void
nsFrame::DumpBaseRegressionData(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent, PRBool aIncludeStyleData)
{
  if (nsnull != mNextSibling) {
    IndentBy(out, aIndent);
    fprintf(out, "<next-sibling va=\"%ld\"/>\n", PRUptrdiff(mNextSibling));
  }

  if (HasView()) {
    IndentBy(out, aIndent);
    fprintf(out, "<view va=\"%ld\">\n", PRUptrdiff(GetView()));
    aIndent++;
    // XXX add in code to dump out view state too...
    aIndent--;
    IndentBy(out, aIndent);
    fprintf(out, "</view>\n");
  }

  if(aIncludeStyleData) {
    if(mStyleContext) {
      IndentBy(out, aIndent);
      fprintf(out, "<stylecontext va=\"%ld\">\n", PRUptrdiff(mStyleContext));
      aIndent++;
      // Dump style context regression data
      mStyleContext->DumpRegressionData(aPresContext, out, aIndent);
      aIndent--;
      IndentBy(out, aIndent);
      fprintf(out, "</stylecontext>\n");
    }
  }

  IndentBy(out, aIndent);
  fprintf(out, "<bbox x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\"/>\n",
          mRect.x, mRect.y, mRect.width, mRect.height);

  // Now dump all of the children on all of the child lists
  nsIFrame* kid;
  nsIAtom* list = nsnull;
  PRInt32 listIndex = 0;
  do {
    kid = GetFirstChild(list);
    if (kid) {
      IndentBy(out, aIndent);
      if (nsnull != list) {
        nsAutoString listName;
        list->ToString(listName);
        fprintf(out, "<child-list name=\"");
        XMLQuote(listName);
        fputs(NS_LossyConvertUCS2toASCII(listName).get(), out);
        fprintf(out, "\">\n");
      }
      else {
        fprintf(out, "<child-list>\n");
      }
      aIndent++;
      while (kid) {
        nsIFrameDebug*  frameDebug;

        if (NS_SUCCEEDED(kid->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&frameDebug))) {
          frameDebug->DumpRegressionData(aPresContext, out, aIndent, aIncludeStyleData);
        }
        kid = kid->GetNextSibling();
      }
      aIndent--;
      IndentBy(out, aIndent);
      fprintf(out, "</child-list>\n");
    }
    list = GetAdditionalChildListName(listIndex++);
  } while (nsnull != list);
}

NS_IMETHODIMP
nsFrame::VerifyTree() const
{
  NS_ASSERTION(0 == (mState & NS_FRAME_IN_REFLOW), "frame is in reflow");
  return NS_OK;
}
#endif

/*this method may.. invalidate if the state was changed or if aForceRedraw is PR_TRUE
  it will not update immediately.*/
NS_IMETHODIMP
nsFrame::SetSelected(nsIPresContext* aPresContext, nsIDOMRange *aRange, PRBool aSelected, nsSpread aSpread)
{
/*
  if (aSelected && ParentDisablesSelection())
    return NS_OK;
*/

  // check whether style allows selection
  PRBool  selectable;
  IsSelectable(&selectable, nsnull);
  if (!selectable)
    return NS_OK;

/*
  if (eSpreadDown == aSpread){
    nsIFrame* kid = GetFirstChild(nsnull);
    while (nsnull != kid) {
      kid->SetSelected(nsnull,aSelected,aSpread);
      kid = kid->GetNextSibling();
    }
  }
*/
  if ( aSelected ){
    AddStateBits(NS_FRAME_SELECTED_CONTENT);
  }
  else
    RemoveStateBits(NS_FRAME_SELECTED_CONTENT);

  // repaint this frame's outline area.
  // In CSS3 selection can change the outline style! and border and content too
  Invalidate(GetOutlineRect(), PR_FALSE);

  if (GetStateBits() & NS_FRAME_OUTSIDE_CHILDREN)
  {
    RefreshAllContentFrames(this, mContent);
  }

#ifdef IBMBIDI
  PRInt32 start, end;
  nsIFrame* frame = GetNextSibling();
  if (frame) {
    GetFirstLeaf(aPresContext, &frame);
    GetOffsets(start, end);
    if (start && end) {
      frame->SetSelected(aPresContext, aRange, aSelected, aSpread);
    }
  }
#endif // IBMBIDI

  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetSelected(PRBool *aSelected) const
{
  if (!aSelected )
    return NS_ERROR_NULL_POINTER;
  *aSelected = (PRBool)(mState & NS_FRAME_SELECTED_CONTENT);
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetPointFromOffset(nsIPresContext* inPresContext, nsIRenderingContext* inRendContext, PRInt32 inOffset, nsPoint* outPoint)
{
  NS_PRECONDITION(outPoint != nsnull, "Null parameter");
  nsPoint bottomLeft(0, 0);
  if (mContent)
  {
    nsIContent* newContent = mContent->GetParent();
    if (newContent){
      PRInt32 newOffset = newContent->IndexOf(mContent);

      if (inOffset > newOffset)
        bottomLeft.x = GetRect().width;
    }
  }
  *outPoint = bottomLeft;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetChildFrameContainingOffset(PRInt32 inContentOffset, PRBool inHint, PRInt32* outFrameContentOffset, nsIFrame **outChildFrame)
{
  NS_PRECONDITION(outChildFrame && outFrameContentOffset, "Null parameter");
  *outFrameContentOffset = (PRInt32)inHint;
  //the best frame to reflect any given offset would be a visible frame if possible
  //i.e. we are looking for a valid frame to place the blinking caret 
  nsRect rect = GetRect();
  if (!rect.width || !rect.height)
  {
    nsIFrame *nextFlow = nsnull;
    //if we have a 0 width or height then lets look for another frame that possibly has
    //the same content.  If we have no frames in flow then just let us return 'this' frame
    if (NS_SUCCEEDED(GetNextInFlow(&nextFlow)) && nextFlow)
      return nextFlow->GetChildFrameContainingOffset(inContentOffset, inHint, outFrameContentOffset, outChildFrame);
  }
  *outChildFrame = this;
  return NS_OK;
}

//
// What I've pieced together about this routine:
// Starting with a block frame (from which a line frame can be gotten)
// and a line number, drill down and get the first/last selectable
// frame on that line, depending on aPos->mDirection.
// aOutSideLimit != 0 means ignore aLineStart, instead work from
// the end (if > 0) or beginning (if < 0).
//
nsresult
nsFrame::GetNextPrevLineFromeBlockFrame(nsIPresContext* aPresContext,
                                        nsPeekOffsetStruct *aPos,
                                        nsIFrame *aBlockFrame, 
                                        PRInt32 aLineStart, 
                                        PRInt8 aOutSideLimit
                                        )
{
  //magic numbers aLineStart will be -1 for end of block 0 will be start of block
  if (!aBlockFrame || !aPos)
    return NS_ERROR_NULL_POINTER;

  aPos->mResultFrame = nsnull;
  aPos->mResultContent = nsnull;
  aPos->mPreferLeft = (aPos->mDirection == eDirNext);

   nsresult result;
  nsCOMPtr<nsILineIteratorNavigator> it; 
  result = aBlockFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),getter_AddRefs(it));
  if (NS_FAILED(result) || !it)
    return result;
  PRInt32 searchingLine = aLineStart;
  PRInt32 countLines;
  result = it->GetNumLines(&countLines);
  if (aOutSideLimit > 0) //start at end
    searchingLine = countLines;
  else if (aOutSideLimit <0)//start at begining
    searchingLine = -1;//"next" will be 0  
  else 
    if ((aPos->mDirection == eDirPrevious && searchingLine == 0) || 
       (aPos->mDirection == eDirNext && searchingLine >= (countLines -1) )){
      //we need to jump to new block frame.
           return NS_ERROR_FAILURE;
    }
  PRInt32 lineFrameCount;
  nsIFrame *resultFrame = nsnull;
  nsIFrame *farStoppingFrame = nsnull; //we keep searching until we find a "this" frame then we go to next line
  nsIFrame *nearStoppingFrame = nsnull; //if we are backing up from edge, stop here
  nsIFrame *firstFrame;
  nsIFrame *lastFrame;
  nsRect  rect;
  PRBool isBeforeFirstFrame, isAfterLastFrame;
  PRBool found = PR_FALSE;
  while (!found)
  {
    if (aPos->mDirection == eDirPrevious)
      searchingLine --;
    else
      searchingLine ++;
    if ((aPos->mDirection == eDirPrevious && searchingLine < 0) || 
       (aPos->mDirection == eDirNext && searchingLine >= countLines ))
    {
      //we need to jump to new block frame.
      return NS_ERROR_FAILURE;
    }
    PRUint32 lineFlags;
    result = it->GetLine(searchingLine, &firstFrame, &lineFrameCount,
                         rect, &lineFlags);
    if (!lineFrameCount) 
      continue;
    if (NS_SUCCEEDED(result)){
      lastFrame = firstFrame;
      for (;lineFrameCount > 1;lineFrameCount --){
        //result = lastFrame->GetNextSibling(&lastFrame, searchingLine);
        result = it->GetNextSiblingOnLine(lastFrame, searchingLine);
        if (NS_FAILED(result) || !lastFrame){
          NS_ASSERTION(0,"should not be reached nsFrame\n");
          return NS_ERROR_FAILURE;
        }
      }
      GetLastLeaf(aPresContext, &lastFrame);

      if (aPos->mDirection == eDirNext){
        nearStoppingFrame = firstFrame;
        farStoppingFrame = lastFrame;
      }
      else{
        nearStoppingFrame = lastFrame;
        farStoppingFrame = firstFrame;
      }
      nsPoint offset;
      nsIView * view; //used for call of get offset from view
      aBlockFrame->GetOffsetFromView(aPresContext, offset,&view);
      nscoord newDesiredX  = aPos->mDesiredX - offset.x;//get desired x into blockframe coordinates!
#ifdef IBMBIDI
      PRBool bidiEnabled;
      aPresContext->GetBidiEnabled(&bidiEnabled);
      result = it->FindFrameAt(searchingLine, newDesiredX, bidiEnabled, &resultFrame, &isBeforeFirstFrame, &isAfterLastFrame);
#else
      result = it->FindFrameAt(searchingLine, newDesiredX, &resultFrame, &isBeforeFirstFrame, &isAfterLastFrame);
#endif // IBMBIDI
      if(NS_FAILED(result))
        continue;
    }

    if (NS_SUCCEEDED(result) && resultFrame)
    {
      nsCOMPtr<nsILineIteratorNavigator> newIt; 
      //check to see if this is ANOTHER blockframe inside the other one if so then call into its lines
      result = resultFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),getter_AddRefs(newIt));
      if (NS_SUCCEEDED(result) && newIt)
      {
        aPos->mResultFrame = resultFrame;
        return NS_OK;
      }
      //resultFrame is not a block frame

      nsCOMPtr<nsIBidirectionalEnumerator> frameTraversal;
      result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal), LEAF,
                                    aPresContext, resultFrame, aPos->mScrollViewStop);
      if (NS_FAILED(result))
        return result;
      nsISupports *isupports = nsnull;
      nsIFrame *storeOldResultFrame = resultFrame;
      while ( !found ){
        nsCOMPtr<nsIPresContext> context;
        result = aPos->mTracker->GetPresContext(getter_AddRefs(context));
        if (NS_FAILED(result))
          return result;
        nsPoint point;
        point.x = aPos->mDesiredX;

        nsRect tempRect = resultFrame->GetRect();
        nsPoint offset;
        nsIView * view; //used for call of get offset from view
        result = resultFrame->GetOffsetFromView(aPresContext, offset, &view);
        if (NS_FAILED(result))
          return result;
        point.y = tempRect.height + offset.y;

        //special check. if we allow non-text selection then we can allow a hit location to fall before a table. 
        //otherwise there is no way to get and click signal to fall before a table (it being a line iterator itself)
        PRInt16 isEditor = 0;
        nsIPresShell *shell = aPresContext->GetPresShell();
        if (!shell)
          return NS_ERROR_FAILURE;
        shell->GetSelectionFlags ( &isEditor );
        isEditor = isEditor == nsISelectionDisplay::DISPLAY_ALL;
        if ( isEditor ) 
        {
          if (resultFrame->GetType() == nsLayoutAtoms::tableOuterFrame)
          {
            if (((point.x - offset.x + tempRect.x)<0) ||  ((point.x - offset.x+ tempRect.x)>tempRect.width))//off left/right side
            {
              nsIContent* content = resultFrame->GetContent();
              if (content)
              {
                nsIContent* parent = content->GetParent();
                if (parent)
                {
                  aPos->mResultContent = parent;
                  aPos->mContentOffset = parent->IndexOf(content);
                  aPos->mPreferLeft = PR_FALSE;
                  if ((point.x - offset.x+ tempRect.x)>tempRect.width)
                  {
                    aPos->mContentOffset++;//go to end of this frame
                    aPos->mPreferLeft = PR_TRUE;
                  }
                  aPos->mContentOffsetEnd = aPos->mContentOffset;
                  //result frame is the result frames parent.
                  aPos->mResultFrame = resultFrame->GetParent();
                  return NS_POSITION_BEFORE_TABLE;
                }
              }
            }
          }
        }

        if (!resultFrame->HasView())
        {
          rect = resultFrame->GetRect();
          if (!rect.width || !rect.height)
            result = NS_ERROR_FAILURE;
          else
            result = resultFrame->GetContentAndOffsetsFromPoint(context,point,
                                          getter_AddRefs(aPos->mResultContent),
                                          aPos->mContentOffset,
                                          aPos->mContentOffsetEnd,
                                          aPos->mPreferLeft);
        }
        if (NS_SUCCEEDED(result))
        {
          found = PR_TRUE;
        }
        else {
          if (aPos->mDirection == eDirPrevious && (resultFrame == farStoppingFrame))
            break;
          if (aPos->mDirection == eDirNext && (resultFrame == nearStoppingFrame))
            break;
          //always try previous on THAT line if that fails go the other way
          result = frameTraversal->Prev();
          if (NS_FAILED(result))
            break;
          result = frameTraversal->CurrentItem(&isupports);
          if (NS_FAILED(result) || !isupports)
            return result;
          //we must CAST here to an nsIFrame. nsIFrame doesnt really follow the rules
          resultFrame = (nsIFrame *)isupports;
        }
      }

      if (!found){
        resultFrame = storeOldResultFrame;
        result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal), LEAF,
                                      aPresContext, resultFrame, aPos->mScrollViewStop);
      }
      while ( !found ){
        nsCOMPtr<nsIPresContext> context;
        result = aPos->mTracker->GetPresContext(getter_AddRefs(context));

        nsPoint point;
        point.x = aPos->mDesiredX;
        point.y = 0;

        result = resultFrame->GetContentAndOffsetsFromPoint(context,point,
                                          getter_AddRefs(aPos->mResultContent), aPos->mContentOffset,
                                          aPos->mContentOffsetEnd, aPos->mPreferLeft);
			  PRBool selectable;
        resultFrame->IsSelectable(&selectable, nsnull);
			  if (!selectable)
          return NS_ERROR_FAILURE;//cant go to unselectable frame
        
        if (NS_SUCCEEDED(result))
        {
          found = PR_TRUE;
          if (resultFrame == farStoppingFrame)
            aPos->mPreferLeft = PR_FALSE;
          else
            aPos->mPreferLeft = PR_TRUE;
        }
        else {
          if (aPos->mDirection == eDirPrevious && (resultFrame == nearStoppingFrame))
            break;
          if (aPos->mDirection == eDirNext && (resultFrame == farStoppingFrame))
            break;
          //previous didnt work now we try "next"
          result = frameTraversal->Next();
          if (NS_FAILED(result))
            break;
          result = frameTraversal->CurrentItem(&isupports);
          if (NS_FAILED(result) || !isupports)
            return result;
          //we must CAST here to an nsIFrame. nsIFrame doesnt really follow the rules
          resultFrame = (nsIFrame *)isupports;
        }
      }
      aPos->mResultFrame = resultFrame;
    }
    else {
        //we need to jump to new block frame.
      aPos->mAmount = eSelectLine;
      aPos->mStartOffset = 0;
      aPos->mEatingWS = PR_FALSE;
      aPos->mPreferLeft = !(aPos->mDirection == eDirNext);
      if (aPos->mDirection == eDirPrevious)
        aPos->mStartOffset = -1;//start from end
     return aBlockFrame->PeekOffset(aPresContext, aPos);
    }
  }
  return NS_OK;
}

// Get a frame which can contain a line iterator
// (which generally means it's a block frame).
static nsILineIterator*
GetBlockFrameAndLineIter(nsIFrame* aFrame, nsIFrame** aBlockFrame)
{
  nsILineIterator* it;
  nsIFrame *blockFrame = aFrame;
  nsIFrame *thisBlock = aFrame;

  blockFrame = blockFrame->GetParent();
  if (!blockFrame) //if at line 0 then nothing to do
    return 0;
  nsresult result = blockFrame->QueryInterface(NS_GET_IID(nsILineIterator), (void**)&it);
  if (NS_SUCCEEDED(result) && it)
  {
    if (aBlockFrame)
      *aBlockFrame = blockFrame;
    return it;
  }

  while (blockFrame)
  {
    thisBlock = blockFrame;
    blockFrame = blockFrame->GetParent();
    if (blockFrame) {
      result = blockFrame->QueryInterface(NS_GET_IID(nsILineIterator),
                                          (void**)&it);
      if (NS_SUCCEEDED(result) && it)
      {
        if (aBlockFrame)
          *aBlockFrame = blockFrame;
        return it;
      }
    }
  }
  return 0;
}

nsresult
nsFrame::PeekOffsetParagraph(nsIPresContext* aPresContext,
                             nsPeekOffsetStruct *aPos)
{
#ifdef DEBUG_paragraph
  printf("Selecting paragraph\n");
#endif
  nsIFrame* blockFrame;
  nsCOMPtr<nsILineIterator> iter (getter_AddRefs(GetBlockFrameAndLineIter(this, &blockFrame)));
  if (!blockFrame || !iter)
    return NS_ERROR_UNEXPECTED;

  PRInt32 thisLine;
  nsresult result = iter->FindLineContaining(this, &thisLine);
#ifdef DEBUG_paragraph
  printf("Looping %s from line %d\n",
         aPos->mDirection == eDirPrevious ? "back" : "forward",
         thisLine);
#endif
  if (NS_FAILED(result) || thisLine < 0)
    return result ? result : NS_ERROR_UNEXPECTED;

  // Now, theoretically, we should be able to loop over lines
  // looking for lines that end in breaks.
  PRInt32 di = (aPos->mDirection == eDirPrevious ? -1 : 1);
  for (PRInt32 i = thisLine; ; i += di)
  {
    nsIFrame* firstFrameOnLine;
    PRInt32 numFramesOnLine;
    nsRect lineBounds;
    PRUint32 lineFlags;
    if (i >= 0)
    {
      result = iter->GetLine(i, &firstFrameOnLine, &numFramesOnLine,
                             lineBounds, &lineFlags);
      if (NS_FAILED(result) || !firstFrameOnLine || !numFramesOnLine)
      {
#ifdef DEBUG_paragraph
        printf("End loop at line %d\n", i);
#endif
        break;
      }
    }
    if (lineFlags & NS_LINE_FLAG_ENDS_IN_BREAK || i < 0)
    {
      // Fill in aPos with the info on the new position
#ifdef DEBUG_paragraph
      printf("Found a paragraph break at line %d\n", i);
#endif

      // Save the old direction, but now go one line back the other way
      nsDirection oldDirection = aPos->mDirection;
      if (oldDirection == eDirPrevious)
        aPos->mDirection = eDirNext;
      else
        aPos->mDirection = eDirPrevious;
#ifdef SIMPLE /* nope */
      result = GetNextPrevLineFromeBlockFrame(aPresContext,
                                              aPos,
                                              blockFrame,
                                              i,
                                              0);
      if (NS_FAILED(result))
        printf("GetNextPrevLineFromeBlockFrame failed\n");

#else /* SIMPLE -- alas, nope */
      int edgeCase = 0;//no edge case. this should look at thisLine
      PRBool doneLooping = PR_FALSE;//tells us when no more block frames hit.
      //this part will find a frame or a block frame. if its a block frame
      //it will "drill down" to find a viable frame or it will return an error.
      do {
        result = GetNextPrevLineFromeBlockFrame(aPresContext,
                                                aPos, 
                                                blockFrame, 
                                                thisLine, 
                                                edgeCase //start from thisLine
          );
        if (aPos->mResultFrame == this)//we came back to same spot! keep going
        {
          aPos->mResultFrame = nsnull;
          if (aPos->mDirection == eDirPrevious)
            thisLine--;
          else
            thisLine++;
        }
        else
          doneLooping = PR_TRUE; //do not continue with while loop
        if (NS_SUCCEEDED(result) && aPos->mResultFrame)
        {
          result = aPos->mResultFrame->QueryInterface(NS_GET_IID(nsILineIterator),
                                                      getter_AddRefs(iter));
          if (NS_SUCCEEDED(result) && iter)//we've struck another block element!
          {
            doneLooping = PR_FALSE;
            if (aPos->mDirection == eDirPrevious)
              edgeCase = 1;//far edge, search from end backwards
            else
              edgeCase = -1;//near edge search from beginning onwards
            thisLine=0;//this line means nothing now.
            //everything else means something so keep looking "inside" the block
            blockFrame = aPos->mResultFrame;

          }
          else
            result = NS_OK;//THIS is to mean that everything is ok to the containing while loop
        }
      } while (!doneLooping);

#endif /* SIMPLE -- alas, nope */

      // Restore old direction before returning:
      aPos->mDirection = oldDirection;
      return result;
    }
  }

  return NS_OK;
}

// Line and paragraph selection (and probably several other cases)
// can get a containing frame from a line iterator, but then need
// to "drill down" to get the content and offset corresponding to
// the last child subframe.  Hence:
// Alas, this doesn't entirely work; it's blocked by some style changes.
static nsresult
DrillDownToEndOfLine(nsIFrame* aFrame, PRInt32 aLineNo, PRInt32 aLineFrameCount,
                     nsRect& aUsedRect,
                     nsIPresContext* aPresContext, nsPeekOffsetStruct* aPos)
{
  if (!aFrame)
    return NS_ERROR_UNEXPECTED;
  nsresult rv = NS_ERROR_FAILURE;
  PRBool found = PR_FALSE;
  nsCOMPtr<nsIAtom> frameType;
  while (!found)  // this loop searches for a valid point to leave the peek offset struct.
  {
    nsIFrame *nextFrame = aFrame;
    nsIFrame *currentFrame = aFrame;
    PRInt32 i;
    for (i=1; i<aLineFrameCount && nextFrame; i++) //already have 1st frame
    {
		  currentFrame = nextFrame;
      // If we do GetNextSibling, we don't go far enough
      // (is aLineFrameCount too small?)
      // If we do GetNextInFlow, we hit a null.
      nextFrame = currentFrame->GetNextSibling();
    }
    if (!nextFrame) //premature leaving of loop.
    {
      nextFrame = currentFrame; //back it up. lets show a warning
      NS_WARNING("lineFrame Count lied to us from nsILineIterator!\n");
    }
    if (!nextFrame->GetRect().width) //this can happen with BR frames and or empty placeholder frames.
    {
      //if we do hit an empty frame then back up the current frame to the frame before it if there is one.
      nextFrame = currentFrame; 
    }
      
    nsPoint offsetPoint; //used for offset of result frame
    nsIView * view; //used for call of get offset from view
    nextFrame->GetOffsetFromView(aPresContext, offsetPoint, &view);

    offsetPoint.x += 2* aUsedRect.width; //2* just to be sure we are off the edge
    // This doesn't seem very efficient since GetPosition
    // has to do a binary search.

    nsCOMPtr<nsIPresContext> context;
    rv = aPos->mTracker->GetPresContext(getter_AddRefs(context));
    if (NS_FAILED(rv)) return rv;
    PRInt32 endoffset;
    rv = nextFrame->GetContentAndOffsetsFromPoint(context,
                                                  offsetPoint,
                                                  getter_AddRefs(aPos->mResultContent),
                                                  aPos->mContentOffset,
                                                  endoffset,
                                                  aPos->mPreferLeft);
    if (NS_SUCCEEDED(rv))
      return PR_TRUE;

#ifdef DEBUG_paragraph
    NS_ASSERTION(PR_FALSE, "Looping around in PeekOffset\n");
#endif
    aLineFrameCount--;
    if (aLineFrameCount == 0)
      break;//just fail out
  }
  return rv;
}


NS_IMETHODIMP
nsFrame::PeekOffset(nsIPresContext* aPresContext, nsPeekOffsetStruct *aPos)
{
  if (!aPos || !aPos->mTracker )
    return NS_ERROR_NULL_POINTER;
  nsresult result = NS_ERROR_FAILURE; 
  PRInt32 endoffset;
  nsPoint point;
  point.x = aPos->mDesiredX;
  point.y = 0;
  switch (aPos->mAmount){
  case eSelectCharacter : case eSelectWord:
    {
      if (mContent)
      {
        nsIContent* newContent = mContent->GetParent();
        if (newContent){
          aPos->mResultContent = newContent;

          PRInt32 newOffset = newContent->IndexOf(mContent);

          if (aPos->mStartOffset < 0)//start at "end"
            aPos->mStartOffset = newOffset + 1;

          if ((aPos->mDirection == eDirNext && newOffset < aPos->mStartOffset) || //need to go to next one
              (aPos->mDirection == eDirPrevious && newOffset >= aPos->mStartOffset))
          {
            result = GetFrameFromDirection(aPresContext, aPos);
            if (NS_FAILED(result))
              return result;
					  PRBool selectable = PR_FALSE;
					  if (aPos->mResultFrame)
  					  aPos->mResultFrame->IsSelectable(&selectable, nsnull);
            if (NS_FAILED(result) || !aPos->mResultFrame || !selectable)
            {
              return result?result:NS_ERROR_FAILURE;
            }
            return aPos->mResultFrame->PeekOffset(aPresContext, aPos);
          }
          else
          {
            if (aPos->mDirection == eDirNext)
              aPos->mContentOffset = newOffset +1;
            else
              aPos->mContentOffset = newOffset;//to beginning of frame
          }
        }
      }
      break;
    }//drop into no amount
    case eSelectNoAmount:
    {
      nsCOMPtr<nsIPresContext> context;
      result = aPos->mTracker->GetPresContext(getter_AddRefs(context));
      if (NS_FAILED(result) || !context)
        return result;
      result = GetContentAndOffsetsFromPoint(context,point,
                             getter_AddRefs(aPos->mResultContent),
                             aPos->mContentOffset,
                             endoffset,
                             aPos->mPreferLeft);
    }break;
    case eSelectLine :
    {
      nsCOMPtr<nsILineIteratorNavigator> iter; 
      nsIFrame *blockFrame = this;
      nsIFrame *thisBlock = this;
      PRInt32   thisLine;

      while (NS_FAILED(result)){
        thisBlock = blockFrame;
        blockFrame = blockFrame->GetParent();
        if (!blockFrame) //if at line 0 then nothing to do
          return NS_OK;
        result = blockFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),getter_AddRefs(iter));
        while (NS_FAILED(result) && blockFrame)
        {
          thisBlock = blockFrame;
          blockFrame = blockFrame->GetParent();
          result = NS_OK;
          if (blockFrame) {
            result = blockFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),getter_AddRefs(iter));
          }
        }
        //this block is now one child down from blockframe
        if (NS_FAILED(result) || !iter || !blockFrame || !thisBlock)
        {
          return ((result) ? result : NS_ERROR_FAILURE);
        }

        if (thisBlock->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
        {
          //if we are searching for a frame that is not in flow we will not find it. 
          //we must instead look for its placeholder
          thisBlock =
            aPresContext->FrameManager()->GetPlaceholderFrameFor(thisBlock);

          if (!thisBlock)
            return NS_ERROR_FAILURE;
        }

        result = iter->FindLineContaining(thisBlock, &thisLine);

        if (NS_FAILED(result))
           return result;

        if (thisLine < 0) 
          return  NS_ERROR_FAILURE;

        int edgeCase = 0;//no edge case. this should look at thisLine
        PRBool doneLooping = PR_FALSE;//tells us when no more block frames hit.
        //this part will find a frame or a block frame. if its a block frame
        //it will "drill down" to find a viable frame or it will return an error.
        nsIFrame *lastFrame = this;
        do {
          result = GetNextPrevLineFromeBlockFrame(aPresContext,
                                                  aPos, 
                                                  blockFrame, 
                                                  thisLine, 
                                                  edgeCase //start from thisLine
            );
          if (NS_SUCCEEDED(result) && (!aPos->mResultFrame || aPos->mResultFrame == lastFrame))//we came back to same spot! keep going
          {
            aPos->mResultFrame = nsnull;
            if (aPos->mDirection == eDirPrevious)
              thisLine--;
            else
              thisLine++;
          }
          else //if failure or success with different frame.
            doneLooping = PR_TRUE; //do not continue with while loop

          lastFrame = aPos->mResultFrame; //set last frame 

          if (NS_SUCCEEDED(result) && aPos->mResultFrame 
            && blockFrame != aPos->mResultFrame)// make sure block element is not the same as the one we had before
          {
/* SPECIAL CHECK FOR TABLE NAVIGATION
  tables need to navigate also and the frame that supports it is nsTableRowGroupFrame which is INSIDE
  nsTableOuterFrame.  if we have stumbled onto an nsTableOuter we need to drill into nsTableRowGroup
  if we hit a header or footer thats ok just go into them,
*/
            PRBool searchTableBool = PR_FALSE;
            if (aPos->mResultFrame->GetType() == nsLayoutAtoms::tableOuterFrame)
            {
              nsIFrame *frame = aPos->mResultFrame->GetFirstChild(nsnull);
              result = NS_OK;
              //got the table frame now
              while(frame) //ok time to drill down to find iterator
              {
                frame = frame->GetFirstChild(nsnull);
                if (frame && NS_SUCCEEDED(result))
                {
                  result = frame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),
                                                            getter_AddRefs(iter));
                  if (NS_SUCCEEDED(result))
                  {
                    aPos->mResultFrame = frame;
                    searchTableBool = PR_TRUE;
                    break; //while aPos->mResultFrame
                  }
                }
                else
                  break;
              }
            }
            if (!searchTableBool)
              result = aPos->mResultFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),
                                                        getter_AddRefs(iter));
            if (NS_SUCCEEDED(result) && iter)//we've struck another block element!
            {
              doneLooping = PR_FALSE;
              if (aPos->mDirection == eDirPrevious)
                edgeCase = 1;//far edge, search from end backwards
              else
                edgeCase = -1;//near edge search from beginning onwards
              thisLine=0;//this line means nothing now.
              //everything else means something so keep looking "inside" the block
              blockFrame = aPos->mResultFrame;

            }
            else
            {
              result = NS_OK;//THIS is to mean that everything is ok to the containing while loop
              break;
            }
          }
        } while (!doneLooping);
      }
      break;
    }

    case eSelectParagraph:
      return PeekOffsetParagraph(aPresContext, aPos);

    case eSelectBeginLine:
    case eSelectEndLine:
    {
      nsCOMPtr<nsILineIteratorNavigator> it; 
      nsIFrame* thisBlock = this;
      nsIFrame* blockFrame = GetParent();
      if (!blockFrame) //if at line 0 then nothing to do
        return NS_OK;
      result = blockFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),getter_AddRefs(it));
      while (NS_FAILED(result) && blockFrame)
      {
        thisBlock = blockFrame;
        blockFrame = blockFrame->GetParent();
        result = NS_OK;
        if (blockFrame) {
          result = blockFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),getter_AddRefs(it));
        }
      }
      if (NS_FAILED(result) || !it || !blockFrame || !thisBlock)
        return result;
      //this block is now one child down from blockframe

      PRInt32   thisLine;
      result = it->FindLineContaining(thisBlock, &thisLine);
      if (NS_FAILED(result) || thisLine < 0 )
        return result;

      PRInt32 lineFrameCount;
      nsIFrame *firstFrame;
      nsRect  usedRect; 
      PRUint32 lineFlags;
      result = it->GetLine(thisLine, &firstFrame, &lineFrameCount,usedRect,
                           &lineFlags);

      if (eSelectBeginLine == aPos->mAmount)
      {
        nsCOMPtr<nsIPresContext> context;
        result = aPos->mTracker->GetPresContext(getter_AddRefs(context));
        if (NS_FAILED(result) || !context)
          return result;

        while (firstFrame)
        {
          nsPoint offsetPoint; //used for offset of result frame
          nsIView * view; //used for call of get offset from view
          firstFrame->GetOffsetFromView(aPresContext, offsetPoint, &view);

          offsetPoint.x = 0;//all the way to the left
          result = firstFrame->GetContentAndOffsetsFromPoint(context,
                                                             offsetPoint,
                                           getter_AddRefs(aPos->mResultContent),
                                           aPos->mContentOffset,
                                           endoffset,
                                           aPos->mPreferLeft);
          if (NS_SUCCEEDED(result))
            break;
          result = it->GetNextSiblingOnLine(firstFrame,thisLine);
          if (NS_FAILED(result))
            break;
        }
      }
      else  // eSelectEndLine
      {
        // We have the last frame, but we need to drill down
        // to get the last offset in the last content represented
        // by that frame.
        return DrillDownToEndOfLine(firstFrame, thisLine, lineFrameCount,
                                    usedRect, aPresContext, aPos);
      }
      return result;
    }
    break;

    default: 
    {
      if (NS_SUCCEEDED(result))
        result = aPos->mResultFrame->PeekOffset(aPresContext, aPos);
    }
  }                          
  return result;
}


NS_IMETHODIMP
nsFrame::CheckVisibility(nsIPresContext* , PRInt32 , PRInt32 , PRBool , PRBool *, PRBool *)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


PRInt32
nsFrame::GetLineNumber(nsIFrame *aFrame)
{
  nsIFrame *blockFrame = aFrame;
  nsIFrame *thisBlock;
  PRInt32   thisLine;
  nsCOMPtr<nsILineIteratorNavigator> it; 
  nsresult result = NS_ERROR_FAILURE;
  while (NS_FAILED(result) && blockFrame)
  {
    thisBlock = blockFrame;
    blockFrame = blockFrame->GetParent();
    result = NS_OK;
    if (blockFrame) {
      result = blockFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),getter_AddRefs(it));
    }
  }
  if (!blockFrame || !it)
    return NS_ERROR_FAILURE;
  result = it->FindLineContaining(thisBlock, &thisLine);
  if (NS_FAILED(result))
    return -1;
  return thisLine;
}


//this will use the nsFrameTraversal as the default peek method.
//this should change to use geometry and also look to ALL the child lists
//we need to set up line information to make sure we dont jump across line boundaries
NS_IMETHODIMP
nsFrame::GetFrameFromDirection(nsIPresContext* aPresContext, nsPeekOffsetStruct *aPos)
{
  nsIFrame *blockFrame = this;
  nsIFrame *thisBlock;
  PRInt32   thisLine;
  nsCOMPtr<nsILineIteratorNavigator> it; 
  nsresult result = NS_ERROR_FAILURE;
  while (NS_FAILED(result) && blockFrame)
  {
    thisBlock = blockFrame;
    blockFrame = blockFrame->GetParent();
    result = NS_OK;
    if (blockFrame) {
      result = blockFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),getter_AddRefs(it));
    }
  }
  if (!blockFrame || !it)
    return NS_ERROR_FAILURE;
  result = it->FindLineContaining(thisBlock, &thisLine);
  if (NS_FAILED(result))
    return result;

  nsIFrame *traversedFrame = this;

  nsIFrame *firstFrame;
  nsIFrame *lastFrame;
  nsRect  nonUsedRect;
  PRInt32 lineFrameCount;
  PRUint32 lineFlags;

#ifdef IBMBIDI
  /* Check whether the visual and logical order of the frames are different */
  PRBool lineIsReordered = PR_FALSE;
  nsIFrame *firstVisual;
  nsIFrame *lastVisual;
  PRBool lineIsRTL;
  PRBool lineJump = PR_FALSE;

  it->GetDirection(&lineIsRTL);
  result = it->CheckLineOrder(thisLine, &lineIsReordered, &firstVisual, &lastVisual);
  if (NS_FAILED(result))
    return result;

  if (lineIsReordered) {
    firstFrame = firstVisual;
    lastFrame = lastVisual;
  }
  else
#endif
  {
    //check to see if "this" is a blockframe if so we need to advance the linenum immediately
    //only if its dir is "next"
    if (aPos->mDirection == eDirNext)
    {
      nsCOMPtr<nsILineIteratorNavigator> tempLi; 
      if (NS_SUCCEEDED(QueryInterface(NS_GET_IID(nsILineIteratorNavigator),getter_AddRefs(tempLi))))
      {
        thisLine++;
        result = it->GetLine(thisLine, &firstFrame, &lineFrameCount,nonUsedRect,
                           &lineFlags);
        if (NS_SUCCEEDED(result) && firstFrame)
          traversedFrame = firstFrame;
        else
        {
          return NS_ERROR_FAILURE;
        }
      }
    }

    result = it->GetLine(thisLine, &firstFrame, &lineFrameCount,nonUsedRect,
                         &lineFlags);
    if (NS_FAILED(result))
      return result;

    lastFrame = firstFrame;
    for (;lineFrameCount > 1;lineFrameCount --){
      result = it->GetNextSiblingOnLine(lastFrame, thisLine);
      if (NS_FAILED(result) || !lastFrame){
        NS_ASSERTION(0,"should not be reached nsFrame\n");
        return NS_ERROR_FAILURE;
      }
    }
  }

  GetFirstLeaf(aPresContext, &firstFrame);
  GetLastLeaf(aPresContext, &lastFrame);
  //END LINE DATA CODE
  if 
#ifndef IBMBIDI  // jumping lines in RTL paragraphs
      ((aPos->mDirection == eDirNext && lastFrame == this)
       ||(aPos->mDirection == eDirPrevious && firstFrame == this))
#else
      (((lineIsRTL) &&
        ((aPos->mDirection == eDirNext && firstFrame == this)
         ||(aPos->mDirection == eDirPrevious && lastFrame == this)))
       ||
       ((!lineIsRTL) &&
        ((aPos->mDirection == eDirNext && lastFrame == this)
         ||(aPos->mDirection == eDirPrevious && firstFrame == this))))
#endif
  {
    if (aPos->mJumpLines != PR_TRUE)
      return NS_ERROR_FAILURE;//we are done. cannot jump lines
#ifdef IBMBIDI
    lineJump = PR_TRUE;
#endif
    if (aPos->mAmount != eSelectWord)
    {
      aPos->mPreferLeft = (PRBool)!(aPos->mPreferLeft);//drift to other side
      aPos->mAmount = eSelectNoAmount;
    }
    else{
      if (aPos->mEatingWS)//done finding what we wanted
        return NS_ERROR_FAILURE;
      if (aPos->mDirection == eDirNext)
      {
        aPos->mPreferLeft = (PRBool)!(aPos->mPreferLeft);//drift to other side
        aPos->mAmount = eSelectNoAmount;
      }
    }

  }
  if (aPos->mAmount == eSelectDir)
    aPos->mAmount = eSelectNoAmount;//just get to next frame.
  nsCOMPtr<nsIBidirectionalEnumerator> frameTraversal;
#ifdef IBMBIDI
  result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),
                                (lineIsReordered) ? VISUAL : LEAF,
                                aPresContext, 
                                (lineJump && lineIsRTL)
                                ? (aPos->mDirection == eDirNext) ? lastFrame : firstFrame
                                : traversedFrame, aPos->mScrollViewStop);
#else
  //if we are a container frame we MUST init with last leaf for eDirNext
  //
  result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal), LEAF, aPresContext, traversedFrame, aPos->mScrollViewStop);
#endif
  if (NS_FAILED(result))
    return result;
  nsISupports *isupports = nsnull;
#ifdef IBMBIDI
  nsIFrame *newFrame;
  PRBool isBidiGhostFrame = PR_FALSE;
  PRBool selectable =  PR_TRUE; //usually fine

  do {
    if (lineIsRTL && lineJump) 
      if (aPos->mDirection == eDirPrevious)
        result = frameTraversal->Next();
      else 
        result = frameTraversal->Prev();
    else
#endif
      if (aPos->mDirection == eDirNext)
        result = frameTraversal->Next();
      else 
        result = frameTraversal->Prev();
    
  if (NS_FAILED(result))
    return result;
  result = frameTraversal->CurrentItem(&isupports);
  if (NS_FAILED(result))
    return result;
  if (!isupports)
    return NS_ERROR_NULL_POINTER;
  //we must CAST here to an nsIFrame. nsIFrame doesnt really follow the rules
  //for speed reasons
#ifndef IBMBIDI
  nsIFrame *newFrame = (nsIFrame *)isupports;
#else
  
  newFrame = (nsIFrame *)isupports;
  nsIContent *content = newFrame->GetContent();
  if (!lineJump && (content == mContent))
  {
    //we will continue if this is NOT a text node. 
    //in the case of a text node since that does not mean we are stuck. it could mean a change in style for
    //the text node.  in the case of a hruleframe with generated before and after content, we do not
    //want the splittable generated frame to get us stuck on an HR
    if (nsLayoutAtoms::textFrame != newFrame->GetType())
      continue;  //we should NOT be getting stuck on the same piece of content on the same line. skip to next line.
  }
  isBidiGhostFrame = (newFrame->GetRect().IsEmpty() &&
                      (newFrame->GetStateBits() & NS_FRAME_IS_BIDI));
  if (isBidiGhostFrame)
  {
    // If the rectangle is empty and the NS_FRAME_IS_BIDI flag is set, this is most likely 
    // a non-renderable frame created at the end of the line by Bidi reordering.
    lineJump = PR_TRUE;
    aPos->mAmount = eSelectNoAmount;
  }
  PRBool newLineIsRTL = PR_FALSE;
  if (lineJump) {
    blockFrame = newFrame;
    nsresult result = NS_ERROR_FAILURE;
    while (NS_FAILED(result) && blockFrame)
    {
      thisBlock = blockFrame;
      blockFrame = blockFrame->GetParent();
      result = NS_OK;
      if (blockFrame) {
        result = blockFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator), getter_AddRefs(it));
      }
    }
    if (!blockFrame || !it)
      return NS_ERROR_FAILURE;
    result = it->FindLineContaining(thisBlock, &thisLine);
    if (NS_FAILED(result))
      return result;
    it->GetDirection(&newLineIsRTL);

    result = it->CheckLineOrder(thisLine, &lineIsReordered, &firstVisual, &lastVisual);
    if (NS_FAILED(result))
      return result;

    if (lineIsReordered)
    {
      firstFrame = firstVisual;
      lastFrame = lastVisual;
    }
    else
    {
      result = it->GetLine(thisLine, &firstFrame, &lineFrameCount, nonUsedRect,
                           &lineFlags);
      if (NS_FAILED(result))
        return result;
      if (!firstFrame)
        return NS_ERROR_FAILURE;

      lastFrame = firstFrame;
      for (;lineFrameCount > 1;lineFrameCount --){
        result = it->GetNextSiblingOnLine(lastFrame, thisLine);
        if (NS_FAILED(result) || !lastFrame){
          NS_ASSERTION(0,"should not be reached nsFrame\n");
          return NS_ERROR_FAILURE;
        }
      }
    }

    GetFirstLeaf(aPresContext, &firstFrame);
    GetLastLeaf(aPresContext, &lastFrame);

    if (newLineIsRTL) {
      if (aPos->mDirection == eDirPrevious)
        newFrame = firstFrame;
      else
        newFrame = lastFrame;
    }
    else {
      if (aPos->mDirection == eDirNext)
        newFrame = firstFrame;
      else
        newFrame = lastFrame;
      }
    }
    newFrame->IsSelectable(&selectable, nsnull);
    if (!selectable)
      lineJump = PR_FALSE;
  } while (isBidiGhostFrame || !selectable);
#endif // IBMBIDI
  if (aPos->mDirection == eDirNext)
    aPos->mStartOffset = 0;
  else
    aPos->mStartOffset = -1;
#ifdef IBMBIDI
  PRUint8 oldLevel, newLevel, baseLevel;
  GetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel, (void**)&oldLevel, sizeof(oldLevel));
  newFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel, (void**)&newLevel, sizeof(newLevel));
  newFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::baseLevel, (void**)&baseLevel, sizeof(baseLevel));
  if (newLevel & 1) // The new frame is RTL, go to the other end
    aPos->mStartOffset = -1 - aPos->mStartOffset;

  if ((aPos->mAmount == eSelectNoAmount) && ((newLevel & 1) != (oldLevel & 1)))  {
    aPos->mPreferLeft = !(aPos->mPreferLeft);
  }
#endif
  aPos->mResultFrame = newFrame;
  return NS_OK;
}

nsIView* nsIFrame::GetClosestView() const
{
  for (const nsIFrame *f = this; f; f = f->GetParent())
    if (f->HasView())
      return f->GetView();

  return nsnull;
}


NS_IMETHODIMP
nsFrame::ReflowDirtyChild(nsIPresShell* aPresShell, nsIFrame* aChild)
{
  NS_ASSERTION(0, "nsFrame::ReflowDirtyChild() should never be called.");  
  return NS_ERROR_NOT_IMPLEMENTED;    
}


#ifdef ACCESSIBILITY
NS_IMETHODIMP
nsFrame::GetAccessible(nsIAccessible** aAccessible)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
#endif

// Destructor function for the overflow area property
static void
DestroyRectFunc(nsIPresContext* aPresContext,
                nsIFrame*       aFrame,
                nsIAtom*        aPropertyName,
                void*           aPropertyValue)
{
  delete (nsRect*)aPropertyValue;
}

nsRect*
nsFrame::GetOverflowAreaProperty(nsIPresContext* aPresContext,
                                 PRBool          aCreateIfNecessary) 
{
  if (!((GetStateBits() & NS_FRAME_OUTSIDE_CHILDREN) || aCreateIfNecessary)) {
    return nsnull;
  }

  nsFrameManager *frameManager = aPresContext->FrameManager();

  void *value =
    frameManager->GetFrameProperty(this, nsLayoutAtoms::overflowAreaProperty,
                                   0);

  if (value) {
    return (nsRect*)value;  // the property already exists
  } else if (aCreateIfNecessary) {
    // The property isn't set yet, so allocate a new rect, set the property,
    // and return the newly allocated rect
    nsRect*  overflow = new nsRect(0, 0, 0, 0);

    frameManager->SetFrameProperty(this, nsLayoutAtoms::overflowAreaProperty,
                                   overflow, DestroyRectFunc);
    return overflow;
  }

  return nsnull;
}

void 
nsFrame::StoreOverflow(nsIPresContext*      aPresContext,
                       nsHTMLReflowMetrics& aMetrics)
{ 
  if ((aMetrics.mOverflowArea.x < 0) ||
      (aMetrics.mOverflowArea.y < 0) ||
      (aMetrics.mOverflowArea.XMost() > aMetrics.width) ||
      (aMetrics.mOverflowArea.YMost() > aMetrics.height)) {
    mState |= NS_FRAME_OUTSIDE_CHILDREN;
    nsRect* overflowArea = GetOverflowAreaProperty(aPresContext, PR_TRUE); 
    NS_ASSERTION(overflowArea, "should have created rect");
    if (overflowArea) {
       *overflowArea = aMetrics.mOverflowArea;
    }
  } 
  else {
    if (mState & NS_FRAME_OUTSIDE_CHILDREN) {
      // remove the previously stored overflow area 
      aPresContext->FrameManager()->
        RemoveFrameProperty(this, nsLayoutAtoms::overflowAreaProperty);
    }
    mState &= ~NS_FRAME_OUTSIDE_CHILDREN;
  }   
}

void
nsFrame::ConsiderChildOverflow(nsIPresContext* aPresContext,
                               nsRect&         aOverflowArea,
                               nsIFrame*       aChildFrame)
{
  if (GetStyleDisplay()->mOverflow != NS_STYLE_OVERFLOW_HIDDEN) {
    nsRect* overflowArea = aChildFrame->GetOverflowAreaProperty(aPresContext);
    if (overflowArea) {
      nsRect childOverflow(*overflowArea);
      childOverflow.MoveBy(aChildFrame->GetPosition());
      aOverflowArea.UnionRect(aOverflowArea, childOverflow);
    }
    else {
      aOverflowArea.UnionRect(aOverflowArea, aChildFrame->GetRect());
    }
  }
}

NS_IMETHODIMP 
nsFrame::GetParentStyleContextFrame(nsIPresContext* aPresContext,
                                    nsIFrame**      aProviderFrame,
                                    PRBool*         aIsChild)
{
  return DoGetParentStyleContextFrame(aPresContext, aProviderFrame, aIsChild);
}


/**
 * This function takes a "special" frame and _if_ that frame is the
 * anonymous block crated by an ib split it returns the split inline
 * as aSpecialSibling.  This is needed because the split inline's
 * style context is the parent of the anonymous block's srtyle context.
 *
 * If aFrame is not the anonymous block, aSpecialSibling is not
 * touched.
 */
static nsresult
GetIBSpecialSibling(nsIPresContext* aPresContext,
                    nsIFrame* aFrame,
                    nsIFrame** aSpecialSibling)
{
  NS_PRECONDITION(aFrame, "Must have a non-null frame!");
  NS_ASSERTION(aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL,
               "GetIBSpecialSibling should not be called on a non-special frame");
  
  // Find the first-in-flow of the frame.  (Ugh.  This ends up
  // being O(N^2) when it is called O(N) times.)
  aFrame = aFrame->GetFirstInFlow();

  /*
   * Now look up the nsLayoutAtoms::IBSplitSpecialPrevSibling
   * property, which is only set on the anonymous block frames we're
   * interested in.
   */
  nsresult rv;
  nsIFrame *specialSibling = NS_STATIC_CAST(nsIFrame*,
    aPresContext->FrameManager()->GetFrameProperty(aFrame,
                            nsLayoutAtoms::IBSplitSpecialPrevSibling, 0, &rv));

  if (NS_OK == rv) {
    NS_ASSERTION(specialSibling, "null special sibling");
    *aSpecialSibling = specialSibling;
  }

  return NS_OK;
}

/*
 * Get the last-in-flow's next sibling, or, if there is none, get the
 * parent's next in flow's first child.
 */
static nsIFrame*
GetNextSiblingAcrossLines(nsIPresContext *aPresContext, nsIFrame *aFrame)
{
  aFrame = aFrame->GetLastInFlow();

  nsIFrame *result = aFrame->GetNextSibling();
  if (result)
    return result;

  nsIFrame *parent;
  aFrame->GetParent()->GetNextInFlow(&parent);
  if (!parent)
    return nsnull;
  return parent->GetFirstChild(nsnull);
}

/**
 * Get the parent, corrected for the mangled frame tree resulting from
 * having a block within an inline.  The result only differs from the
 * result of |GetParent| when |GetParent| returns an anonymous block
 * that was created for an element that was 'display: inline' because
 * that element contained a block.
 *
 * Also skip anonymous scrolled-content parents; inherit directly from the
 * outer scroll frame.
 */
static nsresult
GetCorrectedParent(nsIPresContext* aPresContext, nsIFrame* aFrame,
                   nsIFrame** aSpecialParent)
{
  nsIFrame *parent = aFrame->GetParent();
  *aSpecialParent = parent;
  if (parent) {
    nsIAtom* parentPseudo = parent->GetStyleContext()->GetPseudoType();

    // if this frame itself is not scrolled-content, then skip any scrolled-content
    // parents since they're basically anonymous as far as the style system goes
    if (parentPseudo == nsCSSAnonBoxes::scrolledContent) {
      nsIAtom* pseudo = aFrame->GetStyleContext()->GetPseudoType();
      if (pseudo != nsCSSAnonBoxes::scrolledContent) {
        do {
          parent = parent->GetParent();
          parentPseudo = parent->GetStyleContext()->GetPseudoType();
        } while (parentPseudo == nsCSSAnonBoxes::scrolledContent);
      }
    }

    if (parent->GetStateBits() & NS_FRAME_IS_SPECIAL) {
      GetIBSpecialSibling(aPresContext, parent, aSpecialParent);
    } else {
      *aSpecialParent = parent;
    }
  }

  return NS_OK;
}

nsresult
nsFrame::DoGetParentStyleContextFrame(nsIPresContext* aPresContext,
                                      nsIFrame**      aProviderFrame,
                                      PRBool*         aIsChild)
{
  *aIsChild = PR_FALSE;
  *aProviderFrame = nsnull;
  if (mContent && !mContent->GetParent()) {
    // we're a frame for the root.  We have no style context parent.
    return NS_OK;
  }
  
  if (!(mState & NS_FRAME_OUT_OF_FLOW)) {
    /*
     * If this frame is the anonymous block created when an inline
     * with a block inside it got split, then the parent style context
     * is on the first of the three special frames.  We can get to it
     * using GetIBSpecialSibling
     */
    if (mState & NS_FRAME_IS_SPECIAL) {
      GetIBSpecialSibling(aPresContext, this, aProviderFrame);
      if (*aProviderFrame)
        return NS_OK;
    }

    // If this frame is one of the blocks that split an inline, we must
    // return the "special" inline parent, i.e., the parent that this
    // frame would have if we didn't mangle the frame structure.
    return GetCorrectedParent(aPresContext, this, aProviderFrame);
  }

  // For out-of-flow frames, we must resolve underneath the
  // placeholder's parent.
  nsIFrame *placeholder =
    aPresContext->FrameManager()->GetPlaceholderFrameFor(this);
  if (!placeholder) {
    NS_NOTREACHED("no placeholder frame for out-of-flow frame");
    GetCorrectedParent(aPresContext, this, aProviderFrame);
    return NS_ERROR_FAILURE;
  }
  return NS_STATIC_CAST(nsFrame*, placeholder)->
    GetParentStyleContextFrame(aPresContext, aProviderFrame, aIsChild);
}

//-----------------------------------------------------------------------------------




void
nsFrame::GetLastLeaf(nsIPresContext* aPresContext, nsIFrame **aFrame)
{
  if (!aFrame || !*aFrame)
    return;
  nsIFrame *child = *aFrame;
  //if we are a block frame then go for the last line of 'this'
  while (1){
    child = child->GetFirstChild(nsnull);
    if (!child)
      return;//nothing to do
    while (child->GetNextSibling())
      child = child->GetNextSibling();
    *aFrame = child;
  }
  *aFrame = child;
}

void
nsFrame::GetFirstLeaf(nsIPresContext* aPresContext, nsIFrame **aFrame)
{
  if (!aFrame || !*aFrame)
    return;
  nsIFrame *child = *aFrame;
  while (1){
    child = child->GetFirstChild(nsnull);
    if (!child)
      return;//nothing to do
    *aFrame = child;
  }
}

nsresult nsFrame::CreateAndPostReflowCommand(nsIPresShell* aPresShell,
                                             nsIFrame*     aTargetFrame,
                                             nsReflowType  aReflowType,
                                             nsIFrame*     aChildFrame,
                                             nsIAtom*      aAttribute,
                                             nsIAtom*      aListName)
{
  nsresult rv;

  if (!aPresShell || !aTargetFrame) {
    rv = NS_ERROR_NULL_POINTER;
  }
  else {
    nsHTMLReflowCommand* reflowCmd;
    rv = NS_NewHTMLReflowCommand(&reflowCmd, aTargetFrame,
                                 aReflowType, aChildFrame, 
                                 aAttribute);
    if (NS_SUCCEEDED(rv)) {
      if (nsnull != aListName) {
        reflowCmd->SetChildListName(aListName);
      }
      aPresShell->AppendReflowCommand(reflowCmd);    
    }
  } 

  return rv;
}


NS_IMETHODIMP
nsFrame::CaptureMouse(nsIPresContext* aPresContext, PRBool aGrabMouseEvents)
{
    // get its view
  nsIView* view = GetClosestView();

  PRBool result;

  if (view) {
    nsIViewManager* viewMan = view->GetViewManager();
    if (viewMan) {
      if (aGrabMouseEvents) {
        viewMan->GrabMouseEvents(view, result);
      } else {
        viewMan->GrabMouseEvents(nsnull, result);
      }
    }
  }

  return NS_OK;
}

PRBool
nsFrame::IsMouseCaptured(nsIPresContext* aPresContext)
{
    // get its view
  nsIView* view = GetClosestView();
  
  if (view) {
    nsIViewManager* viewMan = view->GetViewManager();

    if (viewMan) {
        nsIView* grabbingView;
        viewMan->GetMouseEventGrabber(grabbingView);
        if (grabbingView == view)
          return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsresult 
nsFrame::SetProperty(nsIPresContext*         aPresContext,
                     nsIAtom*                aPropName,
                     void*                   aPropValue,
                     NSFramePropertyDtorFunc aPropDtorFunc)
{
  return aPresContext->FrameManager()->SetFrameProperty(this, aPropName,
                                                        aPropValue,
                                                        aPropDtorFunc);
}

void* 
nsFrame::GetProperty(nsIPresContext* aPresContext,
                     nsIAtom*        aPropName,
                     PRBool          aRemoveProp) const
{
  PRUint32 options = 0;

  if (aRemoveProp) {
    options |= NS_IFRAME_MGR_REMOVE_PROP;
  }

  
  return aPresContext->FrameManager()->GetFrameProperty(this, aPropName,
                                                        options);
}

/* virtual */ const nsStyleStruct*
nsFrame::GetStyleDataExternal(nsStyleStructID aSID) const
{
  NS_ASSERTION(mStyleContext, "unexpected null pointer");
  return mStyleContext->GetStyleData(aSID);
}

#ifdef IBMBIDI
/**
 *  retrieve Bidi property of this frame
 *  @lina 5/1/2000
 */

NS_IMETHODIMP nsFrame::GetBidiProperty(nsIPresContext* aPresContext,
                                  nsIAtom*        aPropertyName,
                                  void**          aPropertyValue,
                                  size_t          aSize) const
{
  if (!aPropertyValue || !aPropertyName) {
    return NS_ERROR_NULL_POINTER;
  }
  if ( (aSize < 1) || (aSize > sizeof(void*) ) ) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  memset(aPropertyValue, 0, aSize);
  void* val = aPresContext->FrameManager()->GetFrameProperty(this,
                                                             aPropertyName, 0);

  if (val) {
    // to fix bidi on big endian. We need to copy the right bytes from the void*, not the first aSize bytes.
#if IS_BIG_ENDIAN
    memcpy(aPropertyValue, ((char*)&val)+sizeof(void*) - aSize, aSize);
#else
    memcpy(aPropertyValue, &val, aSize);
#endif
  }

  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetBidiProperty(nsIPresContext* aPresContext,
                                  nsIAtom*        aPropertyName,
                                  void*           aPropertyValue) 
{
  return aPresContext->FrameManager()->SetFrameProperty(this, aPropertyName,
                                                        aPropertyValue,
                                                        nsnull);
}
#endif // IBMBIDI

#ifdef NS_DEBUG
static void
GetTagName(nsFrame* aFrame, nsIContent* aContent, PRIntn aResultSize,
           char* aResult)
{
  const char *nameStr = "";
  if (aContent) {
    aContent->Tag()->GetUTF8String(&nameStr);
  }
  PR_snprintf(aResult, aResultSize, "%s@%p", nameStr, aFrame);
}

void
nsFrame::Trace(const char* aMethod, PRBool aEnter)
{
  if (NS_FRAME_LOG_TEST(gLogModule, NS_FRAME_TRACE_CALLS)) {
    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s %s", tagbuf, aEnter ? "enter" : "exit", aMethod);
  }
}

void
nsFrame::Trace(const char* aMethod, PRBool aEnter, nsReflowStatus aStatus)
{
  if (NS_FRAME_LOG_TEST(gLogModule, NS_FRAME_TRACE_CALLS)) {
    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s %s, status=%scomplete%s",
                tagbuf, aEnter ? "enter" : "exit", aMethod,
                NS_FRAME_IS_NOT_COMPLETE(aStatus) ? "not" : "",
                (NS_FRAME_REFLOW_NEXTINFLOW & aStatus) ? "+reflow" : "");
  }
}

void
nsFrame::TraceMsg(const char* aFormatString, ...)
{
  if (NS_FRAME_LOG_TEST(gLogModule, NS_FRAME_TRACE_CALLS)) {
    // Format arguments into a buffer
    char argbuf[200];
    va_list ap;
    va_start(ap, aFormatString);
    PR_vsnprintf(argbuf, sizeof(argbuf), aFormatString, ap);
    va_end(ap);

    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s", tagbuf, argbuf);
  }
}

void
nsFrame::VerifyDirtyBitSet(nsIFrame* aFrameList)
{
  for (nsIFrame*f = aFrameList; f; f = f->GetNextSibling()) {
    NS_ASSERTION(f->GetStateBits() & NS_FRAME_IS_DIRTY, "dirty bit not set");
  }
}


// Start Display Reflow
#ifdef DEBUG

MOZ_DECL_CTOR_COUNTER(DR_cookie)

DR_cookie::DR_cookie(nsIPresContext*          aPresContext,
                     nsIFrame*                aFrame, 
                     const nsHTMLReflowState& aReflowState,
                     nsHTMLReflowMetrics&     aMetrics,
                     nsReflowStatus&          aStatus)
  :mPresContext(aPresContext), mFrame(aFrame), mReflowState(aReflowState), mMetrics(aMetrics), mStatus(aStatus)
{
  MOZ_COUNT_CTOR(DR_cookie);
  mValue = nsFrame::DisplayReflowEnter(aPresContext, mFrame, mReflowState);
}

DR_cookie::~DR_cookie()
{
  MOZ_COUNT_DTOR(DR_cookie);
  nsFrame::DisplayReflowExit(mPresContext, mFrame, mMetrics, mStatus, mValue);
}

struct DR_FrameTypeInfo;
struct DR_FrameTreeNode;
struct DR_Rule;

struct DR_State
{
  DR_State();
  ~DR_State();
  void Init();
  void AddFrameTypeInfo(nsIAtom* aFrameType,
                        const char* aFrameNameAbbrev,
                        const char* aFrameName);
  DR_FrameTypeInfo* GetFrameTypeInfo(nsIAtom* aFrameType);
  DR_FrameTypeInfo* GetFrameTypeInfo(char* aFrameName);
  void InitFrameTypeTable();
  DR_FrameTreeNode* CreateTreeNode(nsIFrame*                aFrame,
                                   const nsHTMLReflowState& aReflowState);
  void FindMatchingRule(DR_FrameTreeNode& aNode);
  PRBool RuleMatches(DR_Rule&          aRule,
                     DR_FrameTreeNode& aNode);
  PRBool GetToken(FILE* aFile,
                  char* aBuf);
  DR_Rule* ParseRule(FILE* aFile);
  void ParseRulesFile();
  void AddRule(nsVoidArray& aRules,
               DR_Rule&     aRule);
  PRBool IsWhiteSpace(int c);
  PRBool GetNumber(char*    aBuf, 
                 PRInt32&  aNumber);
  void PrettyUC(nscoord aSize,
                char*   aBuf);
  void DisplayFrameTypeInfo(nsIFrame* aFrame,
                            PRInt32   aIndent);
  void DeleteTreeNode(DR_FrameTreeNode& aNode);

  PRBool      mInited;
  PRBool      mActive;
  PRInt32     mCount;
  nsVoidArray mWildRules;
  PRInt32     mAssert;
  PRInt32     mIndentStart;
  PRBool      mIndentUndisplayedFrames;
  nsVoidArray mFrameTypeTable;
  PRBool      mDisplayPixelErrors;

  // reflow specific state
  nsVoidArray mFrameTreeLeaves;
};

static DR_State *DR_state; // the one and only DR_State

struct DR_RulePart 
{
  DR_RulePart(nsIAtom* aFrameType) : mFrameType(aFrameType), mNext(0) {}
  void Destroy();

  nsIAtom*     mFrameType;
  DR_RulePart* mNext;
};

void DR_RulePart::Destroy()
{
  if (mNext) {
    mNext->Destroy();
  }
  delete this;
}

MOZ_DECL_CTOR_COUNTER(DR_Rule)

struct DR_Rule 
{
  DR_Rule() : mLength(0), mTarget(nsnull), mDisplay(PR_FALSE) {
    MOZ_COUNT_CTOR(DR_Rule);
  }
  ~DR_Rule() {
    if (mTarget) mTarget->Destroy();
    MOZ_COUNT_DTOR(DR_Rule);
  }
  void AddPart(nsIAtom* aFrameType);

  PRUint32      mLength;
  DR_RulePart*  mTarget;
  PRBool        mDisplay;
};

void DR_Rule::AddPart(nsIAtom* aFrameType)
{
  DR_RulePart* newPart = new DR_RulePart(aFrameType);
  newPart->mNext = mTarget;
  mTarget = newPart;
  mLength++;
}

MOZ_DECL_CTOR_COUNTER(DR_FrameTypeInfo)

struct DR_FrameTypeInfo
{
  DR_FrameTypeInfo(nsIAtom* aFrmeType, const char* aFrameNameAbbrev, const char* aFrameName);
  ~DR_FrameTypeInfo() { 
      MOZ_COUNT_DTOR(DR_FrameTypeInfo);
      PRInt32 numElements;
      numElements = mRules.Count();
      for (PRInt32 i = numElements - 1; i >= 0; i--) {
        delete (DR_Rule *)mRules.ElementAt(i);
      }
   }

  nsIAtom*    mType;
  char        mNameAbbrev[16];
  char        mName[32];
  nsVoidArray mRules;
};

DR_FrameTypeInfo::DR_FrameTypeInfo(nsIAtom* aFrameType, 
                                   const char* aFrameNameAbbrev, 
                                   const char* aFrameName)
{
  mType = aFrameType;
  strcpy(mNameAbbrev, aFrameNameAbbrev);
  strcpy(mName, aFrameName);
  MOZ_COUNT_CTOR(DR_FrameTypeInfo);
}

MOZ_DECL_CTOR_COUNTER(DR_FrameTreeNode)

struct DR_FrameTreeNode
{
  DR_FrameTreeNode(nsIFrame* aFrame, DR_FrameTreeNode* aParent) : mFrame(aFrame), mParent(aParent), mDisplay(0), mIndent(0)
  {
    MOZ_COUNT_CTOR(DR_FrameTreeNode);
  }

  ~DR_FrameTreeNode()
  {
    MOZ_COUNT_DTOR(DR_FrameTreeNode);
  }

  nsIFrame*         mFrame;
  DR_FrameTreeNode* mParent;
  PRBool            mDisplay;
  PRUint32          mIndent;
};

// DR_State implementation

MOZ_DECL_CTOR_COUNTER(DR_State)

DR_State::DR_State() 
: mInited(PR_FALSE), mActive(PR_FALSE), mCount(0), mAssert(-1), mIndentStart(0), 
  mIndentUndisplayedFrames(PR_FALSE), mDisplayPixelErrors(PR_FALSE)
{
  MOZ_COUNT_CTOR(DR_State);
}

void DR_State::Init() 
{
  char* env = PR_GetEnv("GECKO_DISPLAY_REFLOW_ASSERT");
  PRInt32 num;
  if (env) {
    if (GetNumber(env, num)) 
      mAssert = num;
    else 
      printf("GECKO_DISPLAY_REFLOW_ASSERT - invalid value = %s", env);
  }

  env = PR_GetEnv("GECKO_DISPLAY_REFLOW_INDENT_START");
  if (env) {
    if (GetNumber(env, num)) 
      mIndentStart = num;
    else 
      printf("GECKO_DISPLAY_REFLOW_INDENT_START - invalid value = %s", env);
  }

  env = PR_GetEnv("GECKO_DISPLAY_REFLOW_INDENT_UNDISPLAYED_FRAMES");
  if (env) {
    if (GetNumber(env, num)) 
      mIndentUndisplayedFrames = num;
    else 
      printf("GECKO_DISPLAY_REFLOW_INDENT_UNDISPLAYED_FRAMES - invalid value = %s", env);
  }

  env = PR_GetEnv("GECKO_DISPLAY_REFLOW_FLAG_PIXEL_ERRORS");
  if (env) {
    if (GetNumber(env, num)) 
      mDisplayPixelErrors = num;
    else 
      printf("GECKO_DISPLAY_REFLOW_FLAG_PIXEL_ERRORS - invalid value = %s", env);
  }

  InitFrameTypeTable();
  ParseRulesFile();
  mInited = PR_TRUE;
}

DR_State::~DR_State()
{
  MOZ_COUNT_DTOR(DR_State);
  PRInt32 numElements, i;
  numElements = mWildRules.Count();
  for (i = numElements - 1; i >= 0; i--) {
    delete (DR_Rule *)mWildRules.ElementAt(i);
  }
  numElements = mFrameTreeLeaves.Count();
  for (i = numElements - 1; i >= 0; i--) {
    delete (DR_FrameTreeNode *)mFrameTreeLeaves.ElementAt(i);
  }
  numElements = mFrameTypeTable.Count();
  for (i = numElements - 1; i >= 0; i--) {
    delete (DR_FrameTypeInfo *)mFrameTypeTable.ElementAt(i);
  }
}

PRBool DR_State::GetNumber(char*     aBuf, 
                           PRInt32&  aNumber)
{
  if (sscanf(aBuf, "%d", &aNumber) > 0) 
    return PR_TRUE;
  else 
    return PR_FALSE;
}

PRBool DR_State::IsWhiteSpace(int c) {
  return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

PRBool DR_State::GetToken(FILE* aFile,
                          char* aBuf)
{
  PRBool haveToken = PR_FALSE;
  aBuf[0] = 0;
  // get the 1st non whitespace char
  int c = -1;
  for (c = getc(aFile); (c > 0) && IsWhiteSpace(c); c = getc(aFile)) {
  }

  if (c > 0) {
    haveToken = PR_TRUE;
    aBuf[0] = c;
    // get everything up to the next whitespace char
    PRInt32 cX;
    for (cX = 1, c = getc(aFile); ; cX++, c = getc(aFile)) {
      if (c < 0) { // EOF
        ungetc(' ', aFile); 
        break;
      }
      else {
        if (IsWhiteSpace(c)) {
          break;
        }
        else {
          aBuf[cX] = c;
        }
      }
    }
    aBuf[cX] = 0;
  }
  return haveToken;
}

DR_Rule* DR_State::ParseRule(FILE* aFile)
{
  char buf[128];
  PRInt32 doDisplay;
  DR_Rule* rule = nsnull;
  while (GetToken(aFile, buf)) {
    if (GetNumber(buf, doDisplay)) {
      if (rule) { 
        rule->mDisplay = (PRBool)doDisplay;
        break;
      }
      else {
        printf("unexpected token - %s \n", buf);
      }
    }
    else {
      if (!rule) {
        rule = new DR_Rule;
      }
      if (strcmp(buf, "*") == 0) {
        rule->AddPart(nsnull);
      }
      else {
        DR_FrameTypeInfo* info = GetFrameTypeInfo(buf);
        if (info) {
          rule->AddPart(info->mType);
        }
        else {
          printf("invalid frame type - %s \n", buf);
        }
      }
    }
  }
  return rule;
}

void DR_State::AddRule(nsVoidArray& aRules,
                       DR_Rule&     aRule)
{
  PRInt32 numRules = aRules.Count();
  for (PRInt32 ruleX = 0; ruleX < numRules; ruleX++) {
    DR_Rule* rule = (DR_Rule*)aRules.ElementAt(ruleX);
    NS_ASSERTION(rule, "program error");
    if (aRule.mLength > rule->mLength) {
      aRules.InsertElementAt(&aRule, ruleX);
      return;
    }
  }
  aRules.AppendElement(&aRule);
}

void DR_State::ParseRulesFile()
{
  char* path = PR_GetEnv("GECKO_DISPLAY_REFLOW_RULES_FILE");
  if (path) {
    FILE* inFile = fopen(path, "r");
    if (inFile) {
      for (DR_Rule* rule = ParseRule(inFile); rule; rule = ParseRule(inFile)) {
        if (rule->mTarget) {
          nsIAtom* fType = rule->mTarget->mFrameType;
          if (fType) {
            DR_FrameTypeInfo* info = GetFrameTypeInfo(fType);
            if (info) {
              AddRule(info->mRules, *rule);
            }
          }
          else {
            AddRule(mWildRules, *rule);
          }
          mActive = PR_TRUE;
        }
      }
    }
  }
}


void DR_State::AddFrameTypeInfo(nsIAtom* aFrameType,
                                const char* aFrameNameAbbrev,
                                const char* aFrameName)
{
  mFrameTypeTable.AppendElement(new DR_FrameTypeInfo(aFrameType, aFrameNameAbbrev, aFrameName));
}

DR_FrameTypeInfo* DR_State::GetFrameTypeInfo(nsIAtom* aFrameType)
{
  PRInt32 numEntries = mFrameTypeTable.Count();
  NS_ASSERTION(numEntries != 0, "empty FrameTypeTable");
  for (PRInt32 i = 0; i < numEntries; i++) {
    DR_FrameTypeInfo* info = (DR_FrameTypeInfo*)mFrameTypeTable.ElementAt(i);
    if (info && (info->mType == aFrameType)) {
      return info;
    }
  }
  return (DR_FrameTypeInfo*)mFrameTypeTable.ElementAt(numEntries - 1); // return unknown frame type
}

DR_FrameTypeInfo* DR_State::GetFrameTypeInfo(char* aFrameName)
{
  PRInt32 numEntries = mFrameTypeTable.Count();
  NS_ASSERTION(numEntries != 0, "empty FrameTypeTable");
  for (PRInt32 i = 0; i < numEntries; i++) {
    DR_FrameTypeInfo* info = (DR_FrameTypeInfo*)mFrameTypeTable.ElementAt(i);
    if (info && ((strcmp(aFrameName, info->mName) == 0) || (strcmp(aFrameName, info->mNameAbbrev) == 0))) {
      return info;
    }
  }
  return (DR_FrameTypeInfo*)mFrameTypeTable.ElementAt(numEntries - 1); // return unknown frame type
}

void DR_State::InitFrameTypeTable()
{  
  AddFrameTypeInfo(nsLayoutAtoms::areaFrame,             "area",      "area");
  AddFrameTypeInfo(nsLayoutAtoms::blockFrame,            "block",     "block");
  AddFrameTypeInfo(nsLayoutAtoms::brFrame,               "br",        "br");
  AddFrameTypeInfo(nsLayoutAtoms::bulletFrame,           "bullet",    "bullet");
  AddFrameTypeInfo(nsLayoutAtoms::gfxButtonControlFrame, "button",    "gfxButtonControl");
  AddFrameTypeInfo(nsLayoutAtoms::subDocumentFrame,      "subdoc",    "subDocument");
  AddFrameTypeInfo(nsLayoutAtoms::imageFrame,            "img",       "image");
  AddFrameTypeInfo(nsLayoutAtoms::inlineFrame,           "inline",    "inline");
  AddFrameTypeInfo(nsLayoutAtoms::letterFrame,           "letter",    "letter");
  AddFrameTypeInfo(nsLayoutAtoms::lineFrame,             "line",      "line");
  AddFrameTypeInfo(nsLayoutAtoms::listControlFrame,      "select",    "select");
  AddFrameTypeInfo(nsLayoutAtoms::objectFrame,           "obj",       "object");
  AddFrameTypeInfo(nsLayoutAtoms::pageFrame,             "page",      "page");
  AddFrameTypeInfo(nsLayoutAtoms::placeholderFrame,      "place",     "placeholder");
  AddFrameTypeInfo(nsLayoutAtoms::positionedInlineFrame, "posInline", "positionedInline");
  AddFrameTypeInfo(nsLayoutAtoms::canvasFrame,           "canvas",    "canvas");
  AddFrameTypeInfo(nsLayoutAtoms::rootFrame,             "root",      "root");
  AddFrameTypeInfo(nsLayoutAtoms::scrollFrame,           "scroll",    "scroll");
  AddFrameTypeInfo(nsLayoutAtoms::tableCaptionFrame,     "caption",   "tableCaption");
  AddFrameTypeInfo(nsLayoutAtoms::tableCellFrame,        "cell",      "tableCell");
  AddFrameTypeInfo(nsLayoutAtoms::bcTableCellFrame,      "bcCell",    "bcTableCell");
  AddFrameTypeInfo(nsLayoutAtoms::tableColFrame,         "col",       "tableCol");
  AddFrameTypeInfo(nsLayoutAtoms::tableColGroupFrame,    "colG",      "tableColGroup");
  AddFrameTypeInfo(nsLayoutAtoms::tableFrame,            "tbl",       "table");
  AddFrameTypeInfo(nsLayoutAtoms::tableOuterFrame,       "tblO",      "tableOuter");
  AddFrameTypeInfo(nsLayoutAtoms::tableRowGroupFrame,    "rowG",      "tableRowGroup");
  AddFrameTypeInfo(nsLayoutAtoms::tableRowFrame,         "row",       "tableRow");
  AddFrameTypeInfo(nsLayoutAtoms::textInputFrame,        "textCtl",   "textInput");
  AddFrameTypeInfo(nsLayoutAtoms::textFrame,             "text",      "text");
  AddFrameTypeInfo(nsLayoutAtoms::viewportFrame,         "VP",        "viewport");
  AddFrameTypeInfo(nsnull,                               "unknown",   "unknown");
}


void DR_State::DisplayFrameTypeInfo(nsIFrame* aFrame,
                                    PRInt32   aIndent)
{ 
  DR_FrameTypeInfo* frameTypeInfo = GetFrameTypeInfo(aFrame->GetType());
  if (frameTypeInfo) {
    for (PRInt32 i = 0; i < aIndent; i++) {
      printf(" ");
    }
    if(!strcmp(frameTypeInfo->mNameAbbrev, "unknown")) {
      nsAutoString  name;
      nsIFrameDebug*  frameDebug;
      if (NS_SUCCEEDED(aFrame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&frameDebug))) {
       frameDebug->GetFrameName(name);
       printf("%s %p ", NS_LossyConvertUCS2toASCII(name).get(), (void*)aFrame);
      }
      else {
        printf("%s %p ", frameTypeInfo->mNameAbbrev, (void*)aFrame);
      }
    }
    else {
      printf("%s %p ", frameTypeInfo->mNameAbbrev, (void*)aFrame);
    }
  }
}

PRBool DR_State::RuleMatches(DR_Rule&          aRule,
                             DR_FrameTreeNode& aNode)
{
  NS_ASSERTION(aRule.mTarget, "program error");

  DR_RulePart* rulePart;
  DR_FrameTreeNode* parentNode;
  for (rulePart = aRule.mTarget->mNext, parentNode = aNode.mParent;
       rulePart && parentNode;
       rulePart = rulePart->mNext, parentNode = parentNode->mParent) {
    if (rulePart->mFrameType) {
      if (parentNode->mFrame) {
        if (rulePart->mFrameType != parentNode->mFrame->GetType()) {
          return PR_FALSE;
        }
      }
      else NS_ASSERTION(PR_FALSE, "program error");
    }
    // else wild card match
  }
  return PR_TRUE;
}

void DR_State::FindMatchingRule(DR_FrameTreeNode& aNode)
{
  if (!aNode.mFrame) {
    NS_ASSERTION(PR_FALSE, "invalid DR_FrameTreeNode \n");
    return;
  }

  PRBool matchingRule = PR_FALSE;

  DR_FrameTypeInfo* info = GetFrameTypeInfo(aNode.mFrame->GetType());
  NS_ASSERTION(info, "program error");
  PRInt32 numRules = info->mRules.Count();
  for (PRInt32 ruleX = 0; ruleX < numRules; ruleX++) {
    DR_Rule* rule = (DR_Rule*)info->mRules.ElementAt(ruleX);
    if (rule && RuleMatches(*rule, aNode)) {
      aNode.mDisplay = rule->mDisplay;
      matchingRule = PR_TRUE;
      break;
    }
  }
  if (!matchingRule) {
    PRInt32 numWildRules = mWildRules.Count();
    for (PRInt32 ruleX = 0; ruleX < numWildRules; ruleX++) {
      DR_Rule* rule = (DR_Rule*)mWildRules.ElementAt(ruleX);
      if (rule && RuleMatches(*rule, aNode)) {
        aNode.mDisplay = rule->mDisplay;
        break;
      }
    }
  }

  if (aNode.mParent) {
    aNode.mIndent = aNode.mParent->mIndent;
    if (aNode.mDisplay || mIndentUndisplayedFrames) {
      aNode.mIndent++;
    }
  }
}
    
DR_FrameTreeNode* DR_State::CreateTreeNode(nsIFrame*                aFrame,
                                           const nsHTMLReflowState& aReflowState)
{
  // find the frame of the parent reflow state (usually just the parent of aFrame)
  const nsHTMLReflowState* parentRS = aReflowState.parentReflowState;
  nsIFrame* parentFrame = (parentRS) ? parentRS->frame : nsnull;

  // find the parent tree node leaf
  DR_FrameTreeNode* parentNode = nsnull;
  
  DR_FrameTreeNode* lastLeaf = nsnull;
  if(mFrameTreeLeaves.Count())
    lastLeaf = (DR_FrameTreeNode*)mFrameTreeLeaves.ElementAt(mFrameTreeLeaves.Count() - 1);
  if (lastLeaf) {
    for (parentNode = lastLeaf; parentNode && (parentNode->mFrame != parentFrame); parentNode = parentNode->mParent) {
    }
  }
  DR_FrameTreeNode* newNode = new DR_FrameTreeNode(aFrame, parentNode);
  FindMatchingRule(*newNode);
  if (lastLeaf && (lastLeaf == parentNode)) {
    mFrameTreeLeaves.RemoveElementAt(mFrameTreeLeaves.Count() - 1);
  }
  mFrameTreeLeaves.AppendElement(newNode);
  mCount++;

  return newNode;
}

void DR_State::PrettyUC(nscoord aSize,
                        char*   aBuf)
{
  if (NS_UNCONSTRAINEDSIZE == aSize) {
    strcpy(aBuf, "UC");
  }
  else {
    if ((nscoord)0xdeadbeefU == aSize)
    {
      strcpy(aBuf, "deadbeef");
    }
    else {
      sprintf(aBuf, "%d", aSize);
    }
  }
}

void DR_State::DeleteTreeNode(DR_FrameTreeNode& aNode)
{
  mFrameTreeLeaves.RemoveElement(&aNode);
  PRInt32 numLeaves = mFrameTreeLeaves.Count();
  if ((0 == numLeaves) || (aNode.mParent != (DR_FrameTreeNode*)mFrameTreeLeaves.ElementAt(numLeaves - 1))) {
    mFrameTreeLeaves.AppendElement(aNode.mParent);
  }
  // delete the tree node 
  delete &aNode;
}

static void
CheckPixelError(nscoord aSize,
                float   aPixelToTwips)
{
  if (NS_UNCONSTRAINEDSIZE != aSize) {
    if ((aSize % NSToCoordRound(aPixelToTwips)) > 0) {
      printf("VALUE %d is not a whole pixel \n", aSize);
    }
  }
}

static void DisplayReflowEnterPrint(nsIPresContext*          aPresContext,
                                    nsIFrame*                aFrame,
                                    const nsHTMLReflowState& aReflowState,
                                    DR_FrameTreeNode&        aTreeNode,
                                    PRBool                   aChanged)
{
  if (aTreeNode.mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, aTreeNode.mIndent);

    char width[16];
    char height[16];

    DR_state->PrettyUC(aReflowState.availableWidth, width);
    DR_state->PrettyUC(aReflowState.availableHeight, height);
    if (aReflowState.path && aReflowState.path->mReflowCommand) {
      nsReflowType type;
      aReflowState.path->mReflowCommand->GetType(type);
      const char *incr_reason;
      switch(type) {
        case eReflowType_ContentChanged:
          incr_reason = "incr. (Content)";
          break;
        case eReflowType_StyleChanged:
          incr_reason = "incr. (Style)";
          break;
        case eReflowType_ReflowDirty:
          incr_reason = "incr. (Dirty)";
          break;
        default:
          incr_reason = "incr. (Unknown)";
      }
      printf("r=%d %s a=%s,%s ", aReflowState.reason, incr_reason, width, height);
    }
    else {
      printf("r=%d a=%s,%s ", aReflowState.reason, width, height);
    }

    DR_state->PrettyUC(aReflowState.mComputedWidth, width);
    DR_state->PrettyUC(aReflowState.mComputedHeight, height);
    printf("c=%s,%s ", width, height);

    nsIFrame* inFlow;
    aFrame->GetPrevInFlow(&inFlow);
    if (inFlow) {
      printf("pif=%p ", (void*)inFlow);
    }
    aFrame->GetNextInFlow(&inFlow);
    if (inFlow) {
      printf("nif=%p ", (void*)inFlow);
    }
    if (aChanged) 
      printf("CHANGED \n");
    else 
      printf("cnt=%d \n", DR_state->mCount);
    if (DR_state->mDisplayPixelErrors) {
      float p2t;
      aPresContext->GetScaledPixelsToTwips(&p2t);
      CheckPixelError(aReflowState.availableWidth, p2t);
      CheckPixelError(aReflowState.availableHeight, p2t);
      CheckPixelError(aReflowState.mComputedWidth, p2t);
      CheckPixelError(aReflowState.mComputedHeight, p2t);
    }
  }
}

void* nsFrame::DisplayReflowEnter(nsIPresContext*          aPresContext,
                                  nsIFrame*                aFrame,
                                  const nsHTMLReflowState& aReflowState)
{
  if (!DR_state->mInited) DR_state->Init();
  if (!DR_state->mActive) return nsnull;

  NS_ASSERTION(aFrame, "invalid call");

  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, aReflowState);
  if (treeNode) {
    DisplayReflowEnterPrint(aPresContext, aFrame, aReflowState, *treeNode, PR_FALSE);
  }
  return treeNode;
}

void nsFrame::DisplayReflowExit(nsIPresContext*      aPresContext,
                                nsIFrame*            aFrame,
                                nsHTMLReflowMetrics& aMetrics,
                                nsReflowStatus       aStatus,
                                void*                aFrameTreeNode)
{
  if (!DR_state->mActive) return;

  NS_ASSERTION(aFrame, "DisplayReflowExit - invalid call");
  if (!aFrameTreeNode) return;

  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)aFrameTreeNode;
  if (treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);

    char width[16];
    char height[16];
    char x[16];
    char y[16];
    DR_state->PrettyUC(aMetrics.width, width);
    DR_state->PrettyUC(aMetrics.height, height);
    printf("d=%s,%s ", width, height);

    if (aMetrics.mComputeMEW) {
      DR_state->PrettyUC(aMetrics.mMaxElementWidth, width);
      printf("me=%s ", width);
    }
    if (aMetrics.mFlags & NS_REFLOW_CALC_MAX_WIDTH) {
      DR_state->PrettyUC(aMetrics.mMaximumWidth, width);
      printf("m=%s ", width);
    }
    if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
      printf("status=0x%x", aStatus);
    }
    if (aFrame->GetStateBits() & NS_FRAME_OUTSIDE_CHILDREN) {
       DR_state->PrettyUC(aMetrics.mOverflowArea.x, x);
       DR_state->PrettyUC(aMetrics.mOverflowArea.y, y);
       DR_state->PrettyUC(aMetrics.mOverflowArea.width, width);
       DR_state->PrettyUC(aMetrics.mOverflowArea.height, height);
       printf("o=(%s,%s) %s x %s", x, y, width, height);
       nsRect* storedOverflow = aFrame->GetOverflowAreaProperty(aPresContext);
       if (storedOverflow) {
         if (aMetrics.mOverflowArea != *storedOverflow) {
           DR_state->PrettyUC(storedOverflow->x, x);
           DR_state->PrettyUC(storedOverflow->y, y);
           DR_state->PrettyUC(storedOverflow->width, width);
           DR_state->PrettyUC(storedOverflow->height, height);
           printf("sto=(%s,%s) %s x %s", x, y, width, height);
         }
       }
    }
    printf("\n");
    if (DR_state->mDisplayPixelErrors) {
      float p2t;
      aPresContext->GetScaledPixelsToTwips(&p2t);
      CheckPixelError(aMetrics.width, p2t);
      CheckPixelError(aMetrics.height, p2t);
      if (aMetrics.mComputeMEW) 
        CheckPixelError(aMetrics.mMaxElementWidth, p2t);
      if (aMetrics.mFlags & NS_REFLOW_CALC_MAX_WIDTH) 
        CheckPixelError(aMetrics.mMaximumWidth, p2t);
    }
  }
  DR_state->DeleteTreeNode(*treeNode);
}

/* static */ void
nsFrame::DisplayReflowStartup()
{
  DR_state = new DR_State();
}

/* static */ void
nsFrame::DisplayReflowShutdown()
{
  delete DR_state;
  DR_state = nsnull;
}

void DR_cookie::Change() const
{
  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)mValue;
  if (treeNode && treeNode->mDisplay) {
    DisplayReflowEnterPrint(mPresContext, mFrame, mReflowState, *treeNode, PR_TRUE);
  }
}

#endif
// End Display Reflow

#endif
