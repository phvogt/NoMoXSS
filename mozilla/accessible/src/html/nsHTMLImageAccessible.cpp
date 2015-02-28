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
 * Author: Aaron Leventhal (aaronl@netscape.com)
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

#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsHTMLImageAccessible.h"
#include "nsIAccessibilityService.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIImageLoadingContent.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"

// --- image -----

nsHTMLImageAccessible::nsHTMLImageAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsLinkableAccessible(aDOMNode, aShell)
{ 
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(aDOMNode));
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));
  if (!shell)
    return;

  shell->GetDocument(getter_AddRefs(doc));
  nsAutoString mapElementName;

  if (doc && element) {
    nsCOMPtr<nsIHTMLDocument> htmlDoc(do_QueryInterface(doc));
    element->GetAttribute(NS_LITERAL_STRING("usemap"),mapElementName);
    if (htmlDoc && !mapElementName.IsEmpty()) {
      if (mapElementName.CharAt(0) == '#')
        mapElementName.Cut(0,1);
      mMapElement = htmlDoc->GetImageMap(mapElementName);
    }
  }
}

NS_IMETHODIMP nsHTMLImageAccessible::GetState(PRUint32 *_retval)
{
  // The state is a bitfield, get our inherited state, then logically OR it with STATE_ANIMATED if this
  // is an animated image.

  nsLinkableAccessible::GetState(_retval);

  nsCOMPtr<nsIImageLoadingContent> content(do_QueryInterface(mDOMNode));
  nsCOMPtr<imgIRequest> imageRequest;

  if (content) 
    content->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                        getter_AddRefs(imageRequest));
  
  nsCOMPtr<imgIContainer> imgContainer;
  if (imageRequest) 
    imageRequest->GetImage(getter_AddRefs(imgContainer));

  if (imgContainer) {
    PRUint32 numFrames;
    imgContainer->GetNumFrames(&numFrames);
    if (numFrames > 1)
      *_retval |= STATE_ANIMATED;
  }

  return NS_OK;
}


/* wstring getName (); */
NS_IMETHODIMP nsHTMLImageAccessible::GetName(nsAString& _retval)
{
  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> imageContent(do_QueryInterface(mDOMNode));
  if (imageContent) {
    nsAutoString name;
    rv = AppendFlatStringFromContentNode(imageContent, &name);
    if (NS_SUCCEEDED(rv)) {
      // Temp var needed until CompressWhitespace built for nsAString
      name.CompressWhitespace();
      _retval.Assign(name);
    }
  }
  return rv;
}

/* wstring getRole (); */
NS_IMETHODIMP nsHTMLImageAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = ROLE_GRAPHIC;
  return NS_OK;
}


nsIAccessible *nsHTMLImageAccessible::CreateAreaAccessible(PRInt32 areaNum)
{
  if (!mMapElement) 
    return nsnull;

   if (areaNum == -1) {
    PRInt32 numAreaMaps;
    GetChildCount(&numAreaMaps);
    if (numAreaMaps<=0)
      return nsnull;
    areaNum = NS_STATIC_CAST(PRUint32,numAreaMaps-1);
  }

  nsCOMPtr<nsIDOMHTMLCollection> mapAreas;
  mMapElement->GetAreas(getter_AddRefs(mapAreas));
  if (!mapAreas) 
    return nsnull;

  nsCOMPtr<nsIDOMNode> domNode;
  mapAreas->Item(areaNum,getter_AddRefs(domNode));
  if (!domNode)
    return nsnull;

  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  if (!accService)
    return nsnull;
  if (accService) {
    nsIAccessible* acc = nsnull;
    accService->GetCachedAccessible(domNode, mWeakShell, &acc);
    if (!acc) {
      accService->CreateHTMLAreaAccessible(mWeakShell, domNode, this, &acc);
    }
    return acc;
  }
  return nsnull;
}


/* nsIAccessible getFirstChild (); */
NS_IMETHODIMP nsHTMLImageAccessible::GetFirstChild(nsIAccessible **_retval)
{
  *_retval = CreateAreaAccessible(0);
  return NS_OK;
}


/* nsIAccessible getLastChild (); */
NS_IMETHODIMP nsHTMLImageAccessible::GetLastChild(nsIAccessible **_retval)
{
  *_retval = CreateAreaAccessible(-1);
  return NS_OK;
}

#ifdef NEVER
/* long getAccChildCount (); */
NS_IMETHODIMP nsHTMLImageAccessible::GetChildCount(PRInt32 *_retval)
{
  *_retval = 0;
  if (mMapElement) {
    nsCOMPtr<nsIDOMHTMLCollection> mapAreas;
    mMapElement->GetAreas(getter_AddRefs(mapAreas));
    if (mapAreas) {
      PRUint32 length;
      mapAreas->GetLength(&length);
      *_retval = NS_STATIC_CAST(PRInt32, length);
    }
  }

  return NS_OK;
}
#endif
