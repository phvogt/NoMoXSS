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

#ifndef nsContainerBox_h___
#define nsContainerBox_h___

#include "nsBox.h"
#include "nsCOMPtr.h" 
#include "nsIBoxLayout.h"

class nsFrameList;

class nsContainerBox : public nsBox {

public:

  NS_IMETHOD GetChildBox(nsIBox** aBox);

  nsContainerBox(nsIPresShell* aShell);
  virtual ~nsContainerBox();

  void Destroy(nsBoxLayoutState& aState);

  NS_IMETHOD SetLayoutManager(nsIBoxLayout* aLayout);
  NS_IMETHOD GetLayoutManager(nsIBoxLayout** aLayout);
  NS_IMETHOD GetPrefSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMaxSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetAscent(nsBoxLayoutState& aBoxLayoutState, nscoord& aAscent);
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  NS_IMETHOD RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIBox* aChild);
  NS_IMETHOD GetIndexOf(nsIBox* aChild, PRInt32* aIndex);

  virtual nsIBox* GetBoxAt(PRInt32 aIndex);
  virtual nsIBox* GetBox(nsIFrame* aFrame);
  virtual PRInt32 GetChildCount();
  virtual void ClearChildren(nsBoxLayoutState& aState);
  virtual PRInt32 CreateBoxList(nsBoxLayoutState& aState, nsIFrame* aList, nsIBox*& first, nsIBox*& last);
  virtual void RemoveAfter(nsBoxLayoutState& aState, nsIBox* aPrev);
  virtual void Remove(nsBoxLayoutState& aState, nsIFrame* aChild);
  virtual void Prepend(nsBoxLayoutState& aState, nsIFrame* aList);
  virtual void Append(nsBoxLayoutState& aState, nsIFrame* aList);
  virtual void Insert(nsBoxLayoutState& aState, nsIFrame* aPrevFrame, nsIFrame* aList);
  virtual void InsertAfter(nsBoxLayoutState& aState, nsIBox* aPrev, nsIFrame* aList);
  virtual void InitChildren(nsBoxLayoutState& aState, nsIFrame* aList);
  virtual nsIBox* GetPrevious(nsIFrame* aChild);
  virtual void SanityCheck(nsFrameList& aFrameList);
  virtual void SetDebugOnChildList(nsBoxLayoutState& aState, nsIBox* aChild, PRBool aDebug);
  virtual void CheckBoxOrder(nsBoxLayoutState& aState);
  
  static nsresult LayoutChildAt(nsBoxLayoutState& aState, nsIBox* aBox, const nsRect& aRect);


protected:

  //virtual nsresult LayoutChildAt(nsBoxLayoutState& aState, nsIBox* aBox, const nsRect& aRect, PRUint32 aFlags);

#ifdef DEBUG_LAYOUT
  virtual void GetBoxName(nsAutoString& aName);
#endif

  nsIBox* mFirstChild;
  nsIBox* mLastChild;
  PRInt32 mChildCount;
  PRBool mOrderBoxes;
  nsCOMPtr<nsIBoxLayout> mLayoutManager;
};

#endif

