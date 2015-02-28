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
#ifndef nsCSSFrameConstructor_h___
#define nsCSSFrameConstructor_h___

#include "nsCOMPtr.h"
#include "nsILayoutHistoryState.h"
#include "nsIXBLService.h"

class nsIDocument;
struct nsFrameItems;
struct nsAbsoluteItems;
struct nsTableCreator;
class nsStyleContext;
struct nsTableList;
struct nsStyleContent;
struct nsStyleDisplay;
class nsIPresShell;
class nsVoidArray;
class nsFrameManager;
class nsFrameConstructorState;
class nsIDOMHTMLSelectElement;
class nsIPresContext;
class nsStyleChangeList;
class nsIFrame;

struct nsFindFrameHint
{
  nsIFrame *mPrimaryFrameForPrevSibling;  // weak ref to the primary frame for the content for which we need a frame
  nsFindFrameHint() : mPrimaryFrameForPrevSibling(nsnull) { }
};

class nsCSSFrameConstructor
{
public:
  nsCSSFrameConstructor(nsIDocument *aDocument);
  ~nsCSSFrameConstructor(void) {}

  // Maintain global objects - gXBLService
  static nsIXBLService * GetXBLService();
  static void ReleaseGlobals() { NS_IF_RELEASE(gXBLService); }

  // get the alternate text for a content node
  static void GetAlternateTextFor(nsIContent* aContent,
                                  nsIAtom*    aTag,  // content object's tag
                                  nsString&   aAltText);
private: 
  // These are not supported and are not implemented! 
  nsCSSFrameConstructor(const nsCSSFrameConstructor& aCopy); 
  nsCSSFrameConstructor& operator=(const nsCSSFrameConstructor& aCopy); 

public:
  nsresult ConstructRootFrame(nsIPresShell*   aPresShell, 
                              nsIPresContext* aPresContext,
                              nsIContent*     aDocElement,
                              nsIFrame*&      aNewFrame);

  nsresult ReconstructDocElementHierarchy(nsIPresContext* aPresContext);

  nsresult ContentAppended(nsIPresContext* aPresContext,
                           nsIContent*     aContainer,
                           PRInt32         aNewIndexInContainer);

  nsresult ContentInserted(nsIPresContext*        aPresContext,
                           nsIContent*            aContainer,
                           nsIFrame*              aContainerFrame,
                           nsIContent*            aChild,
                           PRInt32                aIndexInContainer,
                           nsILayoutHistoryState* aFrameState,
                           PRBool                 aInContentReplaced);

  nsresult ContentReplaced(nsIPresContext* aPresContext,
                           nsIContent*     aContainer,
                           nsIContent*     aOldChild,
                           nsIContent*     aNewChild,
                           PRInt32         aIndexInContainer);

  nsresult ContentRemoved(nsIPresContext* aPresContext,
                          nsIContent*     aContainer,
                          nsIContent*     aChild,
                          PRInt32         aIndexInContainer,
                          PRBool          aInContentReplaced);

  nsresult CharacterDataChanged(nsIPresContext* aPresContext,
                                nsIContent*     aContent,
                                PRBool          aAppend);

  nsresult ContentStatesChanged(nsIPresContext* aPresContext, 
                                nsIContent*     aContent1,
                                nsIContent*     aContent2,
                                PRInt32         aStateMask);

  nsresult AttributeChanged(nsIPresContext* aPresContext,
                            nsIContent*     aContent,
                            PRInt32         aNameSpaceID,
                            nsIAtom*        aAttribute,
                            PRInt32         aModType);

  nsresult ProcessRestyledFrames(nsStyleChangeList& aRestyleArray, 
                                 nsIPresContext*    aPresContext);

  // Notification that we were unable to render a replaced element.
  nsresult CantRenderReplacedElement(nsIPresShell*    aPresShell, 
                                     nsIPresContext*  aPresContext,
                                     nsIFrame*        aFrame);

  // Request to create a continuing frame
  nsresult CreateContinuingFrame(nsIPresContext* aPresContext,
                                 nsIFrame*       aFrame,
                                 nsIFrame*       aParentFrame,
                                 nsIFrame**      aContinuingFrame);

  // Request to find the primary frame associated with a given content object.
  // This is typically called by the pres shell when there is no mapping in
  // the pres shell hash table
  nsresult FindPrimaryFrameFor(nsIPresContext*  aPresContext,
                               nsFrameManager*  aFrameManager,
                               nsIContent*      aContent,
                               nsIFrame**       aFrame,
                               nsFindFrameHint* aHint);

  // Get the XBL insertion point for a child
  nsresult GetInsertionPoint(nsIPresShell* aPresShell,
                             nsIFrame*     aParentFrame,
                             nsIContent*   aChildContent,
                             nsIFrame**    aInsertionPoint,
                             PRBool*       aMultiple = nsnull);

  nsresult CreateListBoxContent(nsIPresContext* aPresContext,
                                nsIFrame*       aParentFrame,
                                nsIFrame*       aPrevFrame,
                                nsIContent*     aChild,
                                nsIFrame**      aResult,
                                PRBool          aIsAppend,
                                PRBool          aIsScrollbar,
                                nsILayoutHistoryState* aFrameState);

  nsresult RemoveMappingsForFrameSubtree(nsIPresContext*        aParentFrame,
                                         nsIFrame*              aRemovedFrame,
                                         nsILayoutHistoryState* aFrameState);

protected:

  nsresult ConstructPageFrame(nsIPresShell*   aPresShell, 
                              nsIPresContext* aPresContext,
                              nsIFrame*       aParentFrame,
                              nsIFrame*       aPrevPageFrame,
                              nsIFrame*&      aPageFrame,
                              nsIFrame*&      aPageContentFrame);

  void DoContentStateChanged(nsIPresContext* aPresContext,
                             nsIContent*     aContent,
                             PRInt32         aStateMask);

  void RestyleElement(nsIPresContext* aPresContext,
                      nsIContent*     aContent,
                      nsIFrame*       aPrimaryFrame);

  void RestyleLaterSiblings(nsIPresContext* aPresContext,
                            nsIContent*     aContent);

  nsresult InitAndRestoreFrame (nsIPresContext*          aPresContext,
                                nsFrameConstructorState& aState,
                                nsIContent*              aContent,
                                nsIFrame*                aParentFrame,
                                nsStyleContext*          aStyleContext,
                                nsIFrame*                aPrevInFlow,
                                nsIFrame*                aNewFrame);

  already_AddRefed<nsStyleContext>
  ResolveStyleContext(nsIPresContext*   aPresContext,
                      nsIFrame*         aParentFrame,
                      nsIContent*       aContent);

  nsresult ConstructFrame(nsIPresShell*            aPresShell,
                          nsIPresContext*          aPresContext,
                          nsFrameConstructorState& aState,
                          nsIContent*              aContent,
                          nsIFrame*                aParentFrame,
                          nsFrameItems&            aFrameItems);

  nsresult ConstructDocElementFrame(nsIPresShell*            aPresShell, 
                                    nsIPresContext*          aPresContext,
                                    nsFrameConstructorState& aState,
                                    nsIContent*              aDocElement,
                                    nsIFrame*                aParentFrame,
                                    nsIFrame*&               aNewFrame);

  nsresult ConstructDocElementTableFrame(nsIPresShell*          aPresShell, 
                                         nsIPresContext*        aPresContext,
                                         nsIContent*            aDocElement,
                                         nsIFrame*              aParentFrame,
                                         nsIFrame*&             aNewTableFrame,
                                         nsILayoutHistoryState* aFrameState);

  nsresult CreateGeneratedFrameFor(nsIPresContext*       aPresContext,
                                   nsIDocument*          aDocument,
                                   nsIFrame*             aParentFrame,
                                   nsIContent*           aContent,
                                   nsStyleContext*       aStyleContext,
                                   const nsStyleContent* aStyleContent,
                                   PRUint32              aContentIndex,
                                   nsIFrame**            aFrame);

  PRBool CreateGeneratedContentFrame(nsIPresShell*            aPresShell, 
                                     nsIPresContext*          aPresContext,
                                     nsFrameConstructorState& aState,
                                     nsIFrame*                aFrame,
                                     nsIContent*              aContent,
                                     nsStyleContext*          aStyleContext,
                                     nsIAtom*                 aPseudoElement,
                                     nsIFrame**               aResult);

  nsresult AppendFrames(nsIPresContext*  aPresContext,
                        nsIPresShell*    aPresShell,
                        nsFrameManager*  aFrameManager,
                        nsIContent*      aContainer,
                        nsIFrame*        aParentFrame,
                        nsIFrame*        aFrameList);

  // BEGIN TABLE SECTION
  nsresult ConstructTableFrame(nsIPresShell*            aPresShell, 
                               nsIPresContext*          aPresContext,
                               nsFrameConstructorState& aState,
                               nsIContent*              aContent,
                               nsIFrame*                aGeometricParent,
                               nsIFrame*                aContentParent,
                               nsStyleContext*          aStyleContext,
                               nsTableCreator&          aTableCreator,
                               PRBool                   aIsPseudo,
                               nsFrameItems&            aChildItems,
                               nsIFrame*&               aNewOuterFrame,
                               nsIFrame*&               aNewInnerFrame,
                               PRBool&                  aIsPseudoParent);

  nsresult ConstructTableCaptionFrame(nsIPresShell*            aPresShell, 
                                      nsIPresContext*          aPresContext,
                                      nsFrameConstructorState& aState,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParent,
                                      nsStyleContext*          aStyleContext,
                                      nsTableCreator&          aTableCreator,
                                      nsFrameItems&            aChildItems,
                                      nsIFrame*&               aNewFrame,
                                      PRBool&                  aIsPseudoParent);

  nsresult ConstructTableRowGroupFrame(nsIPresShell*            aPresShell, 
                                       nsIPresContext*          aPresContext,
                                       nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsIFrame*                aParent,
                                       nsStyleContext*          aStyleContext,
                                       nsTableCreator&          aTableCreator,
                                       PRBool                   aIsPseudo,
                                       nsFrameItems&            aChildItems,
                                       nsIFrame*&               aNewFrame,
                                       PRBool&                  aIsPseudoParent);

  nsresult ConstructTableColGroupFrame(nsIPresShell*            aPresShell, 
                                       nsIPresContext*          aPresContext,
                                       nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsIFrame*                aParent,
                                       nsStyleContext*          aStyleContext,
                                       nsTableCreator&          aTableCreator,
                                       PRBool                   aIsPseudo,
                                       nsFrameItems&            aChildItems,
                                       nsIFrame*&               aNewFrame,
                                       PRBool&                  aIsPseudoParent);

  nsresult ConstructTableRowFrame(nsIPresShell*            aPresShell, 
                                  nsIPresContext*          aPresContext,
                                  nsFrameConstructorState& aState,
                                  nsIContent*              aContent,
                                  nsIFrame*                aParent,
                                  nsStyleContext*          aStyleContext,
                                  nsTableCreator&          aTableCreator,
                                  PRBool                   aIsPseudo,
                                  nsFrameItems&            aChildItems,
                                  nsIFrame*&               aNewFrame,
                                  PRBool&                  aIsPseudoParent);

  nsresult ConstructTableColFrame(nsIPresShell*            aPresShell, 
                                  nsIPresContext*          aPresContext,
                                  nsFrameConstructorState& aState,
                                  nsIContent*              aContent,
                                  nsIFrame*                aParent,
                                  nsStyleContext*          aStyleContext,
                                  nsTableCreator&          aTableCreator,
                                  PRBool                   aIsPseudo,
                                  nsFrameItems&            aChildItems,
                                  nsIFrame*&               aNewFrame,
                                  PRBool&                  aIsPseudoParent);

  nsresult ConstructTableCellFrame(nsIPresShell*            aPresShell, 
                                   nsIPresContext*          aPresContext,
                                   nsFrameConstructorState& aState,
                                   nsIContent*              aContent,
                                   nsIFrame*                aParentFrame,
                                   nsStyleContext*          aStyleContext,
                                   nsTableCreator&          aTableCreator,
                                   PRBool                   aIsPseudo,
                                   nsFrameItems&            aChildItems,
                                   nsIFrame*&               aNewCellOuterFrame,
                                   nsIFrame*&               aNewCellInnerFrame,
                                   PRBool&                  aIsPseudoParent);

  PRBool MustGeneratePseudoParent(nsIPresContext*  aPresContext,
                                  nsIFrame*        aParentFrame,
                                  nsIAtom*         aTag,
                                  nsIContent*      aContent,
                                  nsStyleContext*  aContext);

  nsresult ConstructTableForeignFrame(nsIPresShell*            aPresShell, 
                                      nsIPresContext*          aPresContext,
                                      nsFrameConstructorState& aState,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParentFrameIn,
                                      nsStyleContext*          aStyleContext,
                                      nsTableCreator&          aTableCreator,
                                      nsFrameItems&            aChildItems,
                                      nsIFrame*&               aNewFrame,
                                      PRBool&                  aIsPseudoParent);

  nsresult CreatePseudoTableFrame(nsIPresShell*            aPresShell,
                                  nsIPresContext*          aPresContext,
                                  nsTableCreator&          aTableCreator,
                                  nsFrameConstructorState& aState, 
                                  nsIFrame*                aParentFrameIn = nsnull);

  nsresult CreatePseudoRowGroupFrame(nsIPresShell*            aPresShell,
                                     nsIPresContext*          aPresContext,
                                     nsTableCreator&          aTableCreator,
                                     nsFrameConstructorState& aState, 
                                     nsIFrame*                aParentFrameIn = nsnull);

  nsresult CreatePseudoColGroupFrame(nsIPresShell*            aPresShell,
                                     nsIPresContext*          aPresContext,
                                     nsTableCreator&          aTableCreator,
                                     nsFrameConstructorState& aState, 
                                     nsIFrame*                aParentFrameIn = nsnull);

  nsresult CreatePseudoRowFrame(nsIPresShell*            aPresShell,
                                nsIPresContext*          aPresContext,
                                nsTableCreator&          aTableCreator,
                                nsFrameConstructorState& aState, 
                                nsIFrame*                aParentFrameIn = nsnull);

  nsresult CreatePseudoCellFrame(nsIPresShell*            aPresShell,
                                 nsIPresContext*          aPresContext,
                                 nsTableCreator&          aTableCreator,
                                 nsFrameConstructorState& aState, 
                                 nsIFrame*                aParentFrameIn = nsnull);

  nsresult GetPseudoTableFrame(nsIPresShell*            aPresShell, 
                               nsIPresContext*          aPresContext, 
                               nsTableCreator&          aTableCreator,
                               nsFrameConstructorState& aState, 
                               nsIFrame&                aParentFrameIn);

  nsresult GetPseudoColGroupFrame(nsIPresShell*            aPresShell, 
                                  nsIPresContext*          aPresContext, 
                                  nsTableCreator&          aTableCreator,
                                  nsFrameConstructorState& aState, 
                                  nsIFrame&                aParentFrameIn);

  nsresult GetPseudoRowGroupFrame(nsIPresShell*            aPresShell, 
                                  nsIPresContext*          aPresContext, 
                                  nsTableCreator&          aTableCreator,
                                  nsFrameConstructorState& aState, 
                                  nsIFrame&                aParentFrameIn);

  nsresult GetPseudoRowFrame(nsIPresShell*            aPresShell, 
                             nsIPresContext*          aPresContext, 
                             nsTableCreator&          aTableCreator,
                             nsFrameConstructorState& aState, 
                             nsIFrame&                aParentFrameIn);

  nsresult GetPseudoCellFrame(nsIPresShell*            aPresShell, 
                              nsIPresContext*          aPresContext, 
                              nsTableCreator&          aTableCreator,
                              nsFrameConstructorState& aState, 
                              nsIFrame&                aParentFrameIn);

  nsresult GetParentFrame(nsIPresShell*            aPresShell,
                          nsIPresContext*          aPresContext,
                          nsTableCreator&          aTableCreator,
                          nsIFrame&                aParentFrameIn, 
                          nsIAtom*                 aChildFrameType, 
                          nsFrameConstructorState& aState, 
                          nsIFrame*&               aParentFrame,
                          PRBool&                  aIsPseudoParent);

  nsresult TableProcessChildren(nsIPresShell*            aPresShell, 
                                nsIPresContext*          aPresContext,
                                nsFrameConstructorState& aState,
                                nsIContent*              aContent,
                                nsIFrame*                aParentFrame,
                                nsTableCreator&          aTableCreator,
                                nsFrameItems&            aChildItems,
                                nsIFrame*&               aCaption);

  nsresult TableProcessChild(nsIPresShell*            aPresShell, 
                             nsIPresContext*          aPresContext,
                             nsFrameConstructorState& aState,
                             nsIContent*              aChildContent,
                             nsIContent*              aParentContent,
                             nsIFrame*                aParentFrame,
                             nsIAtom*                 aParentFrameType,
                             nsStyleContext*          aParentStyleContext,
                             nsTableCreator&          aTableCreator,
                             nsFrameItems&            aChildItems,
                             nsIFrame*&               aCaption);

  const nsStyleDisplay* GetDisplay(nsIFrame* aFrame);

  // END TABLE SECTION

  nsresult CreatePlaceholderFrameFor(nsIPresShell*    aPresShell, 
                                     nsIPresContext*  aPresContext,
                                     nsFrameManager*  aFrameManager,
                                     nsIContent*      aContent,
                                     nsIFrame*        aFrame,
                                     nsStyleContext*  aStyleContext,
                                     nsIFrame*        aParentFrame,
                                     nsIFrame**       aPlaceholderFrame);

  nsresult ConstructAlternateFrame(nsIPresShell*    aPresShell, 
                                   nsIPresContext*  aPresContext,
                                   nsIContent*      aContent,
                                   nsStyleContext*  aStyleContext,
                                   nsIFrame*        aGeometricParent,
                                   nsIFrame*        aContentParent,
                                   nsIFrame*&       aFrame);

  nsresult ConstructRadioControlFrame(nsIPresShell*      aPresShell, 
                                      nsIPresContext*    aPresContext,
                                      nsIFrame*&         aNewFrame,
                                      nsIContent*        aContent,
                                      nsStyleContext*    aStyleContext);

  nsresult ConstructCheckboxControlFrame(nsIPresShell*    aPresShell, 
                                         nsIPresContext*  aPresContext,
                                         nsIFrame*&       aNewFrame,
                                         nsIContent*      aContent,
                                         nsStyleContext*  aStyleContext);

  nsresult ConstructSelectFrame(nsIPresShell*            aPresShell, 
                                nsIPresContext*          aPresContext,
                                nsFrameConstructorState& aState,
                                nsIContent*              aContent,
                                nsIFrame*                aParentFrame,
                                nsIAtom*                 aTag,
                                nsStyleContext*          aStyleContext,
                                nsIFrame*&               aNewFrame,
                                PRBool&                  aProcessChildren,
                                PRBool                   aIsAbsolutelyPositioned,
                                PRBool&                  aFrameHasBeenInitialized,
                                PRBool                   aIsFixedPositioned,
                                nsFrameItems&            aFrameItems);

  nsresult ConstructFieldSetFrame(nsIPresShell*            aPresShell, 
                                  nsIPresContext*          aPresContext,
                                  nsFrameConstructorState& aState,
                                  nsIContent*              aContent,
                                  nsIFrame*                aParentFrame,
                                  nsIAtom*                 aTag,
                                  nsStyleContext*          aStyleContext,
                                  nsIFrame*&               aNewFrame,
                                  PRBool&                  aProcessChildren,
                                  PRBool                   aIsAbsolutelyPositioned,
                                  PRBool&                  aFrameHasBeenInitialized,
                                  PRBool                   aIsFixedPositioned);

  nsresult ConstructTextFrame(nsIPresShell*            aPresShell, 
                              nsIPresContext*          aPresContext,
                              nsFrameConstructorState& aState,
                              nsIContent*              aContent,
                              nsIFrame*                aParentFrame,
                              nsStyleContext*          aStyleContext,
                              nsFrameItems&            aFrameItems);

  nsresult ConstructPageBreakFrame(nsIPresShell*            aPresShell, 
                                   nsIPresContext*          aPresContext,
                                   nsFrameConstructorState& aState,
                                   nsIContent*              aContent,
                                   nsIFrame*                aParentFrame,
                                   nsStyleContext*          aStyleContext,
                                   nsFrameItems&            aFrameItems);

  // Construct a page break frame if page-break-before:always is set in aStyleContext
  // and add it to aFrameItems. Return true if page-break-after:always is set on aStyleContext.
  // Don't do this for row groups, rows or cell, because tables handle those internally.
  PRBool PageBreakBefore(nsIPresShell*            aPresShell,
                         nsIPresContext*          aPresContext,
                         nsFrameConstructorState& aState,
                         nsIContent*              aContent,
                         nsIFrame*                aParentFrame,
                         nsStyleContext*          aStyleContext,
                         nsFrameItems&            aFrameItems);

  nsresult ConstructHTMLFrame(nsIPresShell*            aPresShell, 
                              nsIPresContext*          aPresContext,
                              nsFrameConstructorState& aState,
                              nsIContent*              aContent,
                              nsIFrame*                aParentFrame,
                              nsIAtom*                 aTag,
                              PRInt32                  aNameSpaceID,
                              nsStyleContext*          aStyleContext,
                              nsFrameItems&            aFrameItems);

  nsresult ConstructFrameInternal( nsIPresShell*            aPresShell, 
                                   nsIPresContext*          aPresContext,
                                   nsFrameConstructorState& aState,
                                   nsIContent*              aContent,
                                   nsIFrame*                aParentFrame,
                                   nsIAtom*                 aTag,
                                   PRInt32                  aNameSpaceID,
                                   nsStyleContext*          aStyleContext,
                                   nsFrameItems&            aFrameItems,
                                   PRBool                   aXBLBaseTag);

  nsresult CreateAnonymousFrames(nsIPresShell*            aPresShell, 
                                 nsIPresContext*          aPresContext,
                                 nsIAtom*                 aTag,
                                 nsFrameConstructorState& aState,
                                 nsIContent*              aParent,
                                 nsIFrame*                aNewFrame,
                                 PRBool                   aAppendToExisting,
                                 nsFrameItems&            aChildItems,
                                 PRBool                   aIsRoot = PR_FALSE);

  nsresult CreateAnonymousFrames(nsIPresShell*            aPresShell, 
                                 nsIPresContext*          aPresContext,
                                 nsFrameConstructorState& aState,
                                 nsIContent*              aParent,
                                 nsIDocument*             aDocument,
                                 nsIFrame*                aNewFrame,
                                 PRBool                   aAppendToExisting,
                                 nsFrameItems&            aChildItems);

//MathML Mod - RBS
#ifdef MOZ_MATHML
  nsresult ConstructMathMLFrame(nsIPresShell*            aPresShell,
                                nsIPresContext*          aPresContext,
                                nsFrameConstructorState& aState,
                                nsIContent*              aContent,
                                nsIFrame*                aParentFrame,
                                nsIAtom*                 aTag,
                                PRInt32                  aNameSpaceID,
                                nsStyleContext*          aStyleContext,
                                nsFrameItems&            aFrameItems);
#endif

  nsresult ConstructXULFrame(nsIPresShell*            aPresShell, 
                             nsIPresContext*          aPresContext,
                             nsFrameConstructorState& aState,
                             nsIContent*              aContent,
                             nsIFrame*                aParentFrame,
                             nsIAtom*                 aTag,
                             PRInt32                  aNameSpaceID,
                             nsStyleContext*          aStyleContext,
                             nsFrameItems&            aFrameItems,
                             PRBool                   aXBLBaseTag,
                             PRBool&                  aHaltProcessing);


// SVG - rods
#ifdef MOZ_SVG
  nsresult ConstructSVGFrame(nsIPresShell*               aPresShell,
                                nsIPresContext*          aPresContext,
                                nsFrameConstructorState& aState,
                                nsIContent*              aContent,
                                nsIFrame*                aParentFrame,
                                nsIAtom*                 aTag,
                                PRInt32                  aNameSpaceID,
                                nsStyleContext*          aStyleContext,
                                nsFrameItems&            aFrameItems);
#endif

  nsresult ConstructFrameByDisplayType(nsIPresShell*            aPresShell, 
                                       nsIPresContext*          aPresContext,
                                       nsFrameConstructorState& aState,
                                       const nsStyleDisplay*    aDisplay,
                                       nsIContent*              aContent,
                                       PRInt32                  aNameSpaceID,
                                       nsIAtom*                 aTag,
                                       nsIFrame*                aParentFrame,
                                       nsStyleContext*          aStyleContext,
                                       nsFrameItems&            aFrameItems);

  nsresult ProcessChildren(nsIPresShell*            aPresShell, 
                           nsIPresContext*          aPresContext,
                           nsFrameConstructorState& aState,
                           nsIContent*              aContent,
                           nsIFrame*                aFrame,
                           PRBool                   aCanHaveGeneratedContent,
                           nsFrameItems&            aFrameItems,
                           PRBool                   aParentIsBlock,
                           nsTableCreator*          aTableCreator = nsnull);

  nsresult CreateInputFrame(nsIPresShell*    aPresShell,
                            nsIPresContext*  aPresContext,
                            nsIContent*      aContent,
                            nsIFrame*&       aFrame,
                            nsStyleContext*  aStyleContext);

  nsresult AddDummyFrameToSelect(nsIPresContext*          aPresContext,
                                 nsIPresShell*            aPresShell,
                                 nsFrameConstructorState& aState,
                                 nsIFrame*                aListFrame,
                                 nsIFrame*                aParentFrame,
                                 nsFrameItems*            aChildItems,
                                 nsIContent*              aContainer,
                                 nsIDOMHTMLSelectElement* aSelectElement);

  nsresult RemoveDummyFrameFromSelect(nsIPresContext*           aPresContext,
                                      nsIPresShell*             aPresShell,
                                      nsIContent*               aContainer,
                                      nsIContent*               aChild,
                                      nsIDOMHTMLSelectElement*  aSelectElement);

  PRBool IsScrollable(nsIPresContext* aPresContext, const nsStyleDisplay* aDisplay);

  nsIFrame* GetFrameFor(nsIPresShell*   aPresShell,
                        nsIPresContext* aPresContext,
                        nsIContent*     aContent);

  nsIFrame* GetAbsoluteContainingBlock(nsIPresContext* aPresContext,
                                       nsIFrame*       aFrame);

  nsIFrame* GetFloatContainingBlock(nsIPresContext* aPresContext,
                                    nsIFrame*       aFrame);

  nsIContent* PropagateScrollToViewport(nsIPresContext* aPresContext);

  // Build a scroll frame: 
  //  Calls BeginBuildingScrollFrame, InitAndRestoreFrame, and then FinishBuildingScrollFrame
  //
  //  NOTE: this method does NOT set the primary frame for the content element
  //
  nsresult
  BuildScrollFrame(nsIPresShell*            aPresShell, 
                   nsIPresContext*          aPresContext,
                   nsFrameConstructorState& aState,
                   nsIContent*              aContent,
                   nsStyleContext*          aContentStyle,
                   nsIFrame*                aScrolledFrame,
                   nsIFrame*                aParentFrame,
                   nsIFrame*                aContentParentFrame,
                   nsIFrame*&               aNewFrame,
                   nsStyleContext*&         aScrolledChildStyle,
                   nsIFrame*                aScrollPort = nsnull);

  // Builds the initial ScrollFrame
  already_AddRefed<nsStyleContext>
  BeginBuildingScrollFrame(nsIPresShell*            aPresShell, 
                           nsIPresContext*          aPresContext,
                           nsFrameConstructorState& aState,
                           nsIContent*              aContent,
                           nsStyleContext*          aContentStyle,
                           nsIFrame*                aParentFrame,
                           nsIFrame*                aContentParentFrame,
                           nsIAtom*                 aScrolledPseudo,
                           nsIDocument*             aDocument,
                           PRBool                   aIsRoot,
                           nsIFrame*&               aNewFrame, 
                           nsIFrame*&               aScrollableFrame,
                           nsIFrame*                aScrollPort = nsnull);

  // Completes the building of the scrollframe:
  //  Creates and necessary views for the scrollframe and sets up the initial child list
  //
  nsresult 
  FinishBuildingScrollFrame(nsIPresContext*          aPresContext,
                            nsFrameConstructorState& aState,
                            nsIContent*              aContent,
                            nsIFrame*                aScrollFrame,
                            nsIFrame*                aScrolledFrame,
                            nsStyleContext*          scrolledPseudoStyle);

  // Creates a new GfxScrollFrame, Initializes it, and creates a scroll port for it
  //
  nsresult
  InitGfxScrollFrame(nsIPresShell*            aPresShell, 
                     nsIPresContext*          aPresContext,
                     nsFrameConstructorState& aState,
                     nsIContent*              aContent,
                     nsIDocument*             aDocument,
                     nsIFrame*                aParentFrame,
                     nsIFrame*                aContentParentFrame,
                     nsStyleContext*          aStyleContext,
                     PRBool                   aIsRoot,
                     nsIFrame*&               aNewFrame,
                     nsFrameItems&            aAnonymousFrames,
                     nsIFrame*                aScrollPort = nsnull);


  nsresult
  InitializeSelectFrame(nsIPresShell*            aPresShell, 
                        nsIPresContext*          aPresContext,
                        nsFrameConstructorState& aState,
                        nsIFrame*                scrollFrame,
                        nsIFrame*                scrolledFrame,
                        nsIContent*              aContent,
                        nsIFrame*                aParentFrame,
                        nsStyleContext*          aStyleContext,
                        PRBool                   aIsAbsolutelyPositioned,
                        PRBool                   aIsFixedPositioned,
                        PRBool                   aCreateBlock);

  nsresult MaybeRecreateFramesForContent(nsIPresContext*  aPresContext,
                                         nsIContent*      aContent);

  nsresult RecreateFramesForContent(nsIPresContext*  aPresContext,
                                    nsIContent*      aContent);

  nsresult RecreateFramesOnAttributeChange(nsIPresContext* aPresContext,
                                           nsIContent*     aContent,
                                           nsIAtom*        aAttribute);

  nsresult CreateContinuingOuterTableFrame(nsIPresShell*    aPresShell, 
                                           nsIPresContext*  aPresContext,
                                           nsIFrame*        aFrame,
                                           nsIFrame*        aParentFrame,
                                           nsIContent*      aContent,
                                           nsStyleContext*  aStyleContext,
                                           nsIFrame**       aContinuingFrame);

  nsresult CreateContinuingTableFrame(nsIPresShell*    aPresShell, 
                                      nsIPresContext*  aPresContext,
                                      nsIFrame*        aFrame,
                                      nsIFrame*        aParentFrame,
                                      nsIContent*      aContent,
                                      nsStyleContext*  aStyleContext,
                                      nsIFrame**       aContinuingFrame);

  //----------------------------------------

  // Methods support creating block frames and their children

  static already_AddRefed<nsStyleContext>
  GetFirstLetterStyle(nsIPresContext*  aPresContext,
                      nsIContent*      aContent,
                      nsStyleContext*  aStyleContext);

  static already_AddRefed<nsStyleContext>
  GetFirstLineStyle(nsIPresContext*  aPresContext,
                    nsIContent*      aContent,
                    nsStyleContext*  aStyleContext);

  static PRBool HaveFirstLetterStyle(nsIPresContext*  aPresContext,
                                     nsIContent*      aContent,
                                     nsStyleContext*  aStyleContext);

  static PRBool HaveFirstLineStyle(nsIPresContext*  aPresContext,
                                   nsIContent*      aContent,
                                   nsStyleContext*  aStyleContext);

  static void HaveSpecialBlockStyle(nsIPresContext*  aPresContext,
                                    nsIContent*      aContent,
                                    nsStyleContext*  aStyleContext,
                                    PRBool*          aHaveFirstLetterStyle,
                                    PRBool*          aHaveFirstLineStyle);

  PRBool ShouldCreateFirstLetterFrame(nsIPresContext* aPresContext,
                                      nsIContent*     aContent,
                                      nsIFrame*       aFrame);

  // |aContentParentFrame| should be null if it's really the same as
  // |aParentFrame|.
  nsresult ConstructBlock(nsIPresShell*            aPresShell, 
                          nsIPresContext*          aPresContext,
                          nsFrameConstructorState& aState,
                          const nsStyleDisplay*    aDisplay,
                          nsIContent*              aContent,
                          nsIFrame*                aParentFrame,
                          nsIFrame*                aContentParentFrame,
                          nsStyleContext*          aStyleContext,
                          nsIFrame*                aNewFrame,
                          PRBool                   aRelPos);

  nsresult ConstructInline(nsIPresShell*            aPresShell, 
                           nsIPresContext*          aPresContext,
                           nsFrameConstructorState& aState,
                           const nsStyleDisplay*    aDisplay,
                           nsIContent*              aContent,
                           nsIFrame*                aParentFrame,
                           nsStyleContext*          aStyleContext,
                           PRBool                   aIsPositioned,
                           nsIFrame*                aNewFrame,
                           nsIFrame**               aNewBlockFrame,
                           nsIFrame**               aNextInlineFrame);

  nsresult ProcessInlineChildren(nsIPresShell*            aPresShell, 
                                 nsIPresContext*          aPresContext,
                                 nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 nsIFrame*                aFrame,
                                 PRBool                   aCanHaveGeneratedContent,
                                 nsFrameItems&            aFrameItems,
                                 PRBool*                  aKidsAllInline);

  PRBool AreAllKidsInline(nsIFrame* aFrameList);

  PRBool WipeContainingBlock(nsIPresContext*          aPresContext,
                             nsFrameConstructorState& aState,
                             nsIContent*              blockContent,
                             nsIFrame*                aFrame,
                             nsIFrame*                aFrameList);

  PRBool NeedSpecialFrameReframe(nsIPresShell*    aPresShell,
                                 nsIPresContext*  aPresContext,
                                 nsIContent*      aParent1,
                                 nsIContent*      aParent2,
                                 nsIFrame*&       aParentFrame,
                                 nsIContent*      aChild,
                                 PRInt32          aIndexInContainer,
                                 nsIFrame*&       aPrevSibling,
                                 nsIFrame*        aNextSibling);

  nsresult SplitToContainingBlock(nsIPresContext*          aPresContext,
                                  nsFrameConstructorState& aState,
                                  nsIFrame*                aFrame,
                                  nsIFrame*                aLeftInlineChildFrame,
                                  nsIFrame*                aBlockChildFrame,
                                  nsIFrame*                aRightInlineChildFrame,
                                  PRBool                   aTransfer);

  nsresult ReframeContainingBlock(nsIPresContext* aPresContext, nsIFrame* aFrame);

  nsresult StyleChangeReflow(nsIPresContext* aPresContext, nsIFrame* aFrame, nsIAtom* aAttribute);

  /** Helper function that searches the immediate child frames 
    * (and their children if the frames are "special")
    * for a frame that maps the specified content object
    *
    * @param aPresContext   the presentation context
    * @param aParentFrame   the primary frame for aParentContent
    * @param aContent       the content node for which we seek a frame
    * @param aParentContent the parent for aContent 
    * @param aHint          an optional hint used to make the search for aFrame faster
    */
  nsIFrame* FindFrameWithContent(nsIPresContext*  aPresContext,
                                 nsFrameManager*  aFrameManager,
                                 nsIFrame*        aParentFrame,
                                 nsIContent*      aParentContent,
                                 nsIContent*      aContent,
                                 nsFindFrameHint* aHint);

  //----------------------------------------

  // Methods support :first-letter style

  nsIContent* FindBlockFor(nsIPresContext*          aPresContext,
                           nsFrameConstructorState& aState,
                           nsIContent*              aContainer);

  void CreateFloatingLetterFrame(nsIPresShell*            aPresShell, 
                                 nsIPresContext*          aPresContext,
                                 nsFrameConstructorState& aState,
                                 nsIContent*              aTextContent,
                                 nsIFrame*                aTextFrame,
                                 nsIContent*              aBlockContent,
                                 nsIFrame*                aParentFrame,
                                 nsStyleContext*          aStyleContext,
                                 nsFrameItems&            aResult);

  nsresult CreateLetterFrame(nsIPresShell*            aPresShell, 
                             nsIPresContext*          aPresContext,
                             nsFrameConstructorState& aState,
                             nsIContent*              aTextContent,
                             nsIFrame*                aParentFrame,
                             nsFrameItems&            aResult);

  nsresult WrapFramesInFirstLetterFrame(nsIPresShell*            aPresShell, 
                                        nsIPresContext*          aPresContext,
                                        nsFrameConstructorState& aState,
                                        nsIContent*              aBlockContent,
                                        nsIFrame*                aBlockFrame,
                                        nsFrameItems&            aBlockFrames);

  nsresult WrapFramesInFirstLetterFrame(nsIPresShell*            aPresShell, 
                                        nsIPresContext*          aPresContext,
                                        nsFrameConstructorState& aState,
                                        nsIFrame*                aParentFrame,
                                        nsIFrame*                aParentFrameList,
                                        nsIFrame**               aModifiedParent,
                                        nsIFrame**               aTextFrame,
                                        nsIFrame**               aPrevFrame,
                                        nsFrameItems&            aLetterFrame,
                                        PRBool*                  aStopLooking);

  nsresult RecoverLetterFrames(nsIPresShell*            aPresShell, 
                               nsIPresContext*          aPresContext,
                               nsFrameConstructorState& aState,
                               nsIFrame*                aBlockFrame);

  // 
  nsresult RemoveLetterFrames(nsIPresContext*  aPresContext,
                              nsIPresShell*    aPresShell,
                              nsFrameManager*  aFrameManager,
                              nsIFrame*        aBlockFrame);

  // Recursive helper for RemoveLetterFrames
  nsresult RemoveFirstLetterFrames(nsIPresContext*  aPresContext,
                                   nsIPresShell*    aPresShell,
                                   nsFrameManager*  aFrameManager,
                                   nsIFrame*        aFrame,
                                   PRBool*          aStopLooking);

  // Special remove method for those pesky floating first-letter frames
  nsresult RemoveFloatingFirstLetterFrames(nsIPresContext*  aPresContext,
                                           nsIPresShell*    aPresShell,
                                           nsFrameManager*  aFrameManager,
                                           nsIFrame*        aBlockFrame,
                                           PRBool*          aStopLooking);

  // Capture state for the frame tree rooted at the frame associated with the
  // content object, aContent
  nsresult CaptureStateForFramesOf(nsIPresContext* aPresContext,
                                   nsIContent* aContent,
                                   nsILayoutHistoryState* aHistoryState);

  // Capture state for the frame tree rooted at aFrame.
  nsresult CaptureStateFor(nsIPresContext*        aPresContext,
                           nsIFrame*              aFrame,
                           nsILayoutHistoryState* aHistoryState);

  //----------------------------------------

  // Methods support :first-line style

  nsresult WrapFramesInFirstLineFrame(nsIPresShell*            aPresShell, 
                                      nsIPresContext*          aPresContext,
                                      nsFrameConstructorState& aState,
                                      nsIContent*              aContent,
                                      nsIFrame*                aFrame,
                                      nsFrameItems&            aFrameItems);

  nsresult AppendFirstLineFrames(nsIPresShell*            aPresShell, 
                                 nsIPresContext*          aPresContext,
                                 nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 nsIFrame*                aBlockFrame,
                                 nsFrameItems&            aFrameItems);

  nsresult InsertFirstLineFrames(nsIPresContext*          aPresContext,
                                 nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 nsIFrame*                aBlockFrame,
                                 nsIFrame**               aParentFrame,
                                 nsIFrame*                aPrevSibling,
                                 nsFrameItems&            aFrameItems);

  nsresult RemoveFixedItems(nsIPresContext*  aPresContext,
                            nsIPresShell*    aPresShell,
                            nsFrameManager*  aFrameManager);

  // Find the ``rightmost'' frame for the content immediately preceding
  // aIndexInContainer, following continuations if necessary. If aChild is
  // not null, make sure it passes the call to IsValidSibling
  nsIFrame* FindPreviousSibling(nsIPresShell*     aPresShell,
                                nsIContent*       aContainer,
                                nsIFrame*         aContainerFrame,
                                PRInt32           aIndexInContainer,
                                const nsIContent* aChild = nsnull);

  // Find the frame for the content node immediately following aIndexInContainer.
  // If aChild is not null, make sure it passes the call to IsValidSibling
  nsIFrame* FindNextSibling(nsIPresShell*     aPresShell,
                            nsIContent*       aContainer,
                            nsIFrame*         aContainerFrame,
                            PRInt32           aIndexInContainer,
                            const nsIContent* aChild = nsnull);

  // see if aContent and aSibling are legimiate siblings due to restrictions
  // imposed by table columns
  PRBool IsValidSibling(nsIPresShell&          aPresShell,
                        nsIFrame*              aParentFrame,
                        const nsIFrame&        aSibling,
                        PRUint8                aSiblingDisplay,
                        nsIContent&            aContent,
                        PRUint8&               aDisplay);
protected:
  nsIDocument*        mDocument;

  nsIFrame*           mInitialContainingBlock;
  nsIFrame*           mFixedContainingBlock;
  nsIFrame*           mDocElementContainingBlock;
  nsIFrame*           mGfxScrollFrame;

  nsCOMPtr<nsILayoutHistoryState> mTempFrameTreeState;

  static nsIXBLService * gXBLService;
};

#endif /* nsCSSFrameConstructor_h___ */
