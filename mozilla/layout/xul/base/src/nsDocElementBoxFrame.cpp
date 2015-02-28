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
#include "nsHTMLParts.h"
#include "nsContainerFrame.h"
#include "nsCSSRendering.h"
#include "nsIDocument.h"
#include "nsIPresContext.h"
#include "nsViewsCID.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"
#include "nsPageFrame.h"
#include "nsIRenderingContext.h"
#include "nsGUIEvent.h"
#include "nsIDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsIViewManager.h"
#include "nsHTMLAtoms.h"
#include "nsXULAtoms.h"
#include "nsIEventStateManager.h"
#include "nsIDeviceContext.h"
#include "nsIScrollableView.h"
#include "nsLayoutAtoms.h"
#include "nsIPresShell.h"
#include "nsBoxFrame.h"
#include "nsStackLayout.h"
#include "nsIRootBox.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIElementFactory.h"
#include "nsINodeInfo.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"

// Interface IDs

//#define DEBUG_REFLOW

class nsDocElementBoxFrame : public nsBoxFrame, public nsIAnonymousContentCreator {
public:

  friend nsresult NS_NewBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  nsDocElementBoxFrame(nsIPresShell* aShell);

  NS_DECL_ISUPPORTS_INHERITED

  // nsIAnonymousContentCreator
  NS_IMETHOD CreateAnonymousContent(nsIPresContext* aPresContext,
                                    nsISupportsArray& aAnonymousItems);
  NS_IMETHOD CreateFrameFor(nsIPresContext*   aPresContext,
                            nsIContent *      aContent,
                            nsIFrame**        aFrame) { if (aFrame) *aFrame = nsnull; return NS_ERROR_FAILURE; }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
};

//----------------------------------------------------------------------

nsresult
NS_NewDocElementBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsDocElementBoxFrame* it = new (aPresShell) nsDocElementBoxFrame (aPresShell);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *aNewFrame = it;

  return NS_OK;
}

nsDocElementBoxFrame::nsDocElementBoxFrame(nsIPresShell* aShell):nsBoxFrame(aShell, PR_TRUE)
{
}

NS_IMETHODIMP
nsDocElementBoxFrame::CreateAnonymousContent(nsIPresContext* aPresContext,
                                       nsISupportsArray& aAnonymousItems)
{
  nsresult rv;
  nsCOMPtr<nsIElementFactory> elementFactory = 
           do_GetService(NS_ELEMENT_FACTORY_CONTRACTID_PREFIX "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", &rv);
  if (!elementFactory)
    return NS_ERROR_FAILURE;

  nsIDocument* doc = mContent->GetDocument();
  if (!doc)
    // The page is currently being torn down.  Why bother.
    return NS_ERROR_FAILURE;
  nsINodeInfoManager *nodeInfoManager = doc->GetNodeInfoManager();
  NS_ENSURE_TRUE(nodeInfoManager, NS_ERROR_FAILURE);

  // create the top-secret popupgroup node. shhhhh!
  nsCOMPtr<nsINodeInfo> nodeInfo;
  rv = nodeInfoManager->GetNodeInfo(nsXULAtoms::popupgroup,
                                    nsnull, kNameSpaceID_XUL,
                                    getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> content;
  elementFactory->CreateInstanceByTag(nodeInfo, getter_AddRefs(content));
  aAnonymousItems.AppendElement(content);

  // create the top-secret default tooltip node. shhhhh!
  rv = nodeInfoManager->GetNodeInfo(nsXULAtoms::tooltip, nsnull,
                                    kNameSpaceID_XUL, getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  elementFactory->CreateInstanceByTag(nodeInfo, getter_AddRefs(content));
  content->SetAttr(nsnull, nsXULAtoms::defaultz, NS_LITERAL_STRING("true"), PR_FALSE);
  aAnonymousItems.AppendElement(content);
  
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsDocElementBoxFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsDocElementBoxFrame::Release(void)
{
  return NS_OK;
}

NS_INTERFACE_MAP_BEGIN(nsDocElementBoxFrame)
  NS_INTERFACE_MAP_ENTRY(nsIAnonymousContentCreator)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsDocElementBoxFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("DocElementBox"), aResult);
}
#endif
