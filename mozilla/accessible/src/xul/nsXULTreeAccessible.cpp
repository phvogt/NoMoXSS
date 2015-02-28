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
 * Author: Kyle Yuan (kyle.yuan@sun.com)
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

#include "nsIBoxObject.h"
#include "nsIDOMXULElement.h"
#include "nsITreeSelection.h"
#include "nsXULTreeAccessible.h"
#include "nsArray.h"

#ifdef MOZ_ACCESSIBILITY_ATK
#include "nsIAccessibleTable.h"
#endif

// ---------- nsXULTreeAccessible ----------

nsXULTreeAccessible::nsXULTreeAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell):
nsXULSelectableAccessible(aDOMNode, aShell)
{
  GetTreeBoxObject(aDOMNode, getter_AddRefs(mTree));
  if (mTree)
    mTree->GetView(getter_AddRefs(mTreeView));
  NS_ASSERTION(mTree && mTreeView, "Can't get mTree or mTreeView!\n");
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeAccessible, nsXULSelectableAccessible)

// Get the nsITreeBoxObject interface from any levels DOMNode under the <tree>
void nsXULTreeAccessible::GetTreeBoxObject(nsIDOMNode *aDOMNode, nsITreeBoxObject **aBoxObject)
{
  nsAutoString name;
  nsCOMPtr<nsIDOMNode> parentNode, currentNode;

  // Find DOMNode's parents recursively until reach the <tree> tag
  currentNode = aDOMNode;
  while (currentNode) {
    currentNode->GetLocalName(name);
    if (name.Equals(NS_LITERAL_STRING("tree"))) {
      // We will get the nsITreeBoxObject from the tree node
      nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(currentNode));
      if (xulElement) {
        nsCOMPtr<nsIBoxObject> box;
        xulElement->GetBoxObject(getter_AddRefs(box));
        nsCOMPtr<nsITreeBoxObject> treeBox(do_QueryInterface(box));
        if (treeBox) {
          *aBoxObject = treeBox;
          NS_ADDREF(*aBoxObject);
          return;
        }
      }
    }
    currentNode->GetParentNode(getter_AddRefs(parentNode));
    currentNode = parentNode;
  }

  *aBoxObject = nsnull;
}

NS_IMETHODIMP nsXULTreeAccessible::GetState(PRUint32 *_retval)
{
  // Get focus status from base class                                           
  nsAccessible::GetState(_retval);                                           

  // see if we are multiple select if so set ourselves as such
  nsCOMPtr<nsIDOMElement> element (do_QueryInterface(mDOMNode));
  if (element) {
    // the default selection type is multiple
    nsAutoString selType;
    element->GetAttribute(NS_LITERAL_STRING("seltype"), selType);
    if (selType.IsEmpty() || !selType.Equals(NS_LITERAL_STRING("single")))
      *_retval |= STATE_MULTISELECTABLE;
  }

  *_retval |= STATE_READONLY | STATE_FOCUSABLE;

  return NS_OK;
}

// The value is the first selected child
NS_IMETHODIMP nsXULTreeAccessible::GetValue(nsAString& _retval)
{
  _retval.Truncate(0);

  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  nsCOMPtr<nsITreeSelection> selection;
  mTree->GetSelection(getter_AddRefs(selection));
  if (! selection)
    return NS_ERROR_FAILURE;

  PRInt32 currentIndex;
  nsCOMPtr<nsIDOMElement> selectItem;
  selection->GetCurrentIndex(&currentIndex);
  if (currentIndex >= 0) {
    nsAutoString colID;
    PRInt32 keyColumn;
    mTree->GetKeyColumnIndex(&keyColumn);
    mTree->GetColumnID(keyColumn, colID);
    return mTreeView->GetCellText(currentIndex, colID.get(), _retval);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = ROLE_OUTLINE;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::GetFirstChild(nsIAccessible **aFirstChild)
{
  nsAccessible::GetFirstChild(aFirstChild);

  // in normal case, tree's first child should be treecols, if it is not here, 
  //   use the first row as tree's first child
  if (*aFirstChild == nsnull) {
    NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

    PRInt32 rowCount;
    mTreeView->GetRowCount(&rowCount);
    if (rowCount > 0) {
      *aFirstChild = new nsXULTreeitemAccessible(this, mDOMNode, mWeakShell, 0);
      if (! *aFirstChild)
        return NS_ERROR_OUT_OF_MEMORY;
      NS_ADDREF(*aFirstChild);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::GetLastChild(nsIAccessible **aLastChild)
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  PRInt32 rowCount;
  mTreeView->GetRowCount(&rowCount);
  if (rowCount > 0) {
    *aLastChild = new nsXULTreeitemAccessible(this, mDOMNode, mWeakShell, rowCount - 1);
    if (! *aLastChild)
      return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*aLastChild);
  }
  else // if there is not any rows, use treecols as tree's last child
    nsAccessible::GetLastChild(aLastChild);

  return NS_OK;
}

// tree's children count is row count + treecols count
NS_IMETHODIMP nsXULTreeAccessible::GetChildCount(PRInt32 *aAccChildCount)
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  nsAccessible::GetChildCount(aAccChildCount);

  PRInt32 rowCount;
  mTreeView->GetRowCount(&rowCount);
  *aAccChildCount += rowCount;

  return NS_OK;
}

// Ask treeselection to get all selected children
NS_IMETHODIMP nsXULTreeAccessible::GetSelectedChildren(nsIArray **_retval)
{
  *_retval = nsnull;

  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  nsCOMPtr<nsITreeSelection> selection;
  mTree->GetSelection(getter_AddRefs(selection));
  if (!selection)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIMutableArray> selectedAccessibles;
  NS_NewArray(getter_AddRefs(selectedAccessibles));
  if (!selectedAccessibles)
    return NS_ERROR_OUT_OF_MEMORY;

  PRInt32 rowIndex, rowCount;
  PRBool isSelected;
  mTreeView->GetRowCount(&rowCount);
  for (rowIndex = 0; rowIndex < rowCount; rowIndex++) {
    selection->IsSelected(rowIndex, &isSelected);
    if (isSelected) {
      nsCOMPtr<nsIAccessible> tempAccess;
      tempAccess = new nsXULTreeitemAccessible(this, mDOMNode, mWeakShell, rowIndex);
      if (!tempAccess)
        return NS_ERROR_OUT_OF_MEMORY;
      selectedAccessibles->AppendElement(tempAccess, PR_FALSE);
    }
  }

  PRUint32 length;
  selectedAccessibles->GetLength(&length);
  if (length != 0) {
    *_retval = selectedAccessibles;
    NS_IF_ADDREF(*_retval);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  *aSelectionCount = 0;

  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  nsCOMPtr<nsITreeSelection> selection;
  mTree->GetSelection(getter_AddRefs(selection));
  if (selection)
    selection->GetCount(aSelectionCount);

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState)
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  nsCOMPtr<nsITreeSelection> selection;
  mTree->GetSelection(getter_AddRefs(selection));
  if (selection) {
    selection->IsSelected(aIndex, aSelState);
    if ((!(*aSelState) && eSelection_Add == aMethod) || 
        ((*aSelState) && eSelection_Remove == aMethod))
      return selection->ToggleSelect(aIndex);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::AddChildToSelection(PRInt32 aIndex)
{
  PRBool isSelected;
  return ChangeSelection(aIndex, eSelection_Add, &isSelected);
}

NS_IMETHODIMP nsXULTreeAccessible::RemoveChildFromSelection(PRInt32 aIndex)
{
  PRBool isSelected;
  return ChangeSelection(aIndex, eSelection_Remove, &isSelected);
}

NS_IMETHODIMP nsXULTreeAccessible::IsChildSelected(PRInt32 aIndex, PRBool *_retval)
{
  return ChangeSelection(aIndex, eSelection_GetState, _retval);
}

NS_IMETHODIMP nsXULTreeAccessible::ClearSelection()
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  nsCOMPtr<nsITreeSelection> selection;
  mTree->GetSelection(getter_AddRefs(selection));
  if (selection)
    selection->ClearSelection();

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::RefSelection(PRInt32 aIndex, nsIAccessible **_retval)
{
  *_retval = nsnull;

  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  nsCOMPtr<nsITreeSelection> selection;
  mTree->GetSelection(getter_AddRefs(selection));
  if (!selection)
    return NS_ERROR_FAILURE;

  PRInt32 rowIndex, rowCount;
  PRInt32 selCount = 0;
  PRBool isSelected;
  mTreeView->GetRowCount(&rowCount);
  for (rowIndex = 0; rowIndex < rowCount; rowIndex++) {
    selection->IsSelected(rowIndex, &isSelected);
    if (isSelected) {
      if (selCount == aIndex) {
        nsCOMPtr<nsIAccessible> tempAccess;
        tempAccess = new nsXULTreeitemAccessible(this, mDOMNode, mWeakShell, rowIndex);
        if (!tempAccess)
          return NS_ERROR_OUT_OF_MEMORY;
        *_retval = tempAccess;
        NS_ADDREF(*_retval);
      }
      selCount++;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::SelectAllSelection(PRBool *_retval)
{
  *_retval = PR_FALSE;

  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  // see if we are multiple select if so set ourselves as such
  nsCOMPtr<nsIDOMElement> element (do_QueryInterface(mDOMNode));
  if (element) {
    nsAutoString selType;
    element->GetAttribute(NS_LITERAL_STRING("seltype"), selType);
    if (selType.IsEmpty() || !selType.Equals(NS_LITERAL_STRING("single"))) {
      *_retval = PR_TRUE;
      nsCOMPtr<nsITreeSelection> selection;
      mTree->GetSelection(getter_AddRefs(selection));
      if (selection)
        selection->SelectAll();
    }
  }

  return NS_OK;
}

// ---------- nsXULTreeitemAccessible ---------- 

nsXULTreeitemAccessible::nsXULTreeitemAccessible(nsIAccessible *aParent, nsIDOMNode *aDOMNode, nsIWeakReference *aShell, PRInt32 aRow, PRInt32 aColumn):
nsLeafAccessible(aDOMNode, aShell)
{
  Init(); // Add ourselves to cache using GetUniqueID() override
  mParent = aParent;  // xxx todo: do we need this? We already have mParent on nsAccessible

  nsXULTreeAccessible::GetTreeBoxObject(aDOMNode, getter_AddRefs(mTree));
  if (mTree)
    mTree->GetView(getter_AddRefs(mTreeView));
  NS_ASSERTION(mTree && mTreeView, "Can't get mTree or mTreeView!\n");

  // Since the real tree item does not correspond to any DOMNode, use the row index to distinguish each item
  mRow = aRow;
  mColumnIndex = aColumn;
  if (mTree) {
    if (mColumnIndex < 0) {
      PRInt32 keyColumn;
      mTree->GetKeyColumnIndex(&keyColumn);
      mTree->GetColumnID(keyColumn, mColumn);
    } else {
      mTree->GetColumnID(aColumn, mColumn);
    }
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeitemAccessible, nsLeafAccessible)

NS_IMETHODIMP nsXULTreeitemAccessible::GetName(nsAString& _retval)
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  return mTreeView->GetCellText(mRow, mColumn.get(), _retval);
}

NS_IMETHODIMP nsXULTreeitemAccessible::GetValue(nsAString& _retval)
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  PRInt32 level;
  mTreeView->GetLevel(mRow, &level);

  nsString str;
  str.AppendInt(level);
  _retval = str;

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeitemAccessible::GetUniqueID(void **aUniqueID)
{
  // Since mDOMNode is same for all tree item, use |this| pointer as the unique Id
  *aUniqueID = NS_STATIC_CAST(void*, this);
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeitemAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = ROLE_OUTLINEITEM;
  return NS_OK;
}

// Possible states: focused, focusable, selected, expanded/collapsed
NS_IMETHODIMP nsXULTreeitemAccessible::GetState(PRUint32 *_retval)
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  *_retval = STATE_FOCUSABLE | STATE_SELECTABLE;

  // get expanded/collapsed state
  PRBool isContainer, isContainerOpen, isContainerEmpty;
  mTreeView->IsContainer(mRow, &isContainer);
  if (isContainer) {
    mTreeView->IsContainerEmpty(mRow, &isContainerEmpty);
    if (!isContainerEmpty) {
      mTreeView->IsContainerOpen(mRow, &isContainerOpen);
      *_retval |= isContainerOpen? STATE_EXPANDED: STATE_COLLAPSED;
    }
  }

  // get selected state
  nsCOMPtr<nsITreeSelection> selection;
  mTree->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool isSelected, currentIndex;
    selection->IsSelected(mRow, &isSelected);
    if (isSelected)
      *_retval |= STATE_SELECTED;
    selection->GetCurrentIndex(&currentIndex);
    if (currentIndex == mRow)
      *_retval |= STATE_FOCUSED;
  }

  PRInt32 firstVisibleRow, lastVisibleRow;
  mTree->GetFirstVisibleRow(&firstVisibleRow);
  mTree->GetLastVisibleRow(&lastVisibleRow);
  if (mRow < firstVisibleRow || mRow > lastVisibleRow)
    *_retval |= STATE_INVISIBLE;

  return NS_OK;
}

// Only one actions available
NS_IMETHODIMP nsXULTreeitemAccessible::GetNumActions(PRUint8 *_retval)
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  *_retval = eNo_Action;

  PRBool isContainer;
  mTreeView->IsContainer(mRow, &isContainer);
  if (isContainer)
    *_retval = eSingle_Action;

  return NS_OK;
}

// Return the name of our only action
NS_IMETHODIMP nsXULTreeitemAccessible::GetActionName(PRUint8 index, nsAString& _retval)
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  if (index == eAction_Click) {
    PRBool isContainer, isContainerOpen;
    mTreeView->IsContainer(mRow, &isContainer);
    if (isContainer) {
      mTreeView->IsContainerOpen(mRow, &isContainerOpen);
      if (isContainerOpen)
        nsAccessible::GetTranslatedString(NS_LITERAL_STRING("collapse"), _retval);
      else
        nsAccessible::GetTranslatedString(NS_LITERAL_STRING("expand"), _retval);
    }

    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsXULTreeitemAccessible::GetParent(nsIAccessible **aParent)
{
  *aParent = nsnull;

  if (mParent) {
    *aParent = mParent;
    NS_ADDREF(*aParent);
  }

  return NS_OK;
}

// Return the next row of tree if mColumnIndex < 0 (if any),
// otherwise return the next cell.
NS_IMETHODIMP nsXULTreeitemAccessible::GetNextSibling(nsIAccessible **aNextSibling)
{
  *aNextSibling = nsnull;

  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  PRInt32 rowCount;
  mTreeView->GetRowCount(&rowCount);

  if (mColumnIndex < 0) {
    if (mRow < rowCount - 1) {
      *aNextSibling = new nsXULTreeitemAccessible(mParent, mDOMNode, mWeakShell, mRow + 1);
      if (! *aNextSibling)
        return NS_ERROR_OUT_OF_MEMORY;
      NS_ADDREF(*aNextSibling);
    }

    return NS_OK;
  }

  nsresult rv = NS_OK;
#ifdef MOZ_ACCESSIBILITY_ATK
  nsCOMPtr<nsIAccessibleTable> table(do_QueryInterface(mParent, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 columnCount, row = mRow, column = mColumnIndex;
  rv = table->GetColumns(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mColumnIndex < columnCount - 1) {
    column++;
  } else if (mRow < rowCount - 1) {
    column = 0;
    row++;
  }

  *aNextSibling = new nsXULTreeitemAccessible(mParent, mDOMNode, mWeakShell, row, column);
  NS_ENSURE_TRUE(*aNextSibling, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aNextSibling);
#endif //MOZ_ACCESSIBILITY_ATK
  
  return rv;
}

// Return the previou row of tree if mColumnIndex < 0 (if any),
// otherwise return the previou cell.
NS_IMETHODIMP nsXULTreeitemAccessible::GetPreviousSibling(nsIAccessible **aPreviousSibling)
{
  *aPreviousSibling = nsnull;

  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  if (mRow > 0 && mColumnIndex < 0) {
    *aPreviousSibling = new nsXULTreeitemAccessible(mParent, mDOMNode, mWeakShell, mRow - 1);
    if (! *aPreviousSibling)
      return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*aPreviousSibling);

    return NS_OK;
  }

  nsresult rv = NS_OK;
#ifdef MOZ_ACCESSIBILITY_ATK
  nsCOMPtr<nsIAccessibleTable> table(do_QueryInterface(mParent, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 columnCount, row = mRow, column = mColumnIndex;
  rv = table->GetColumns(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mColumnIndex > 0) {
    column--;
  } else if (mRow > 0) {
    column = columnCount - 1;
    row--;
  }

  *aPreviousSibling = new nsXULTreeitemAccessible(mParent, mDOMNode, mWeakShell, row, column);
  NS_ENSURE_TRUE(*aPreviousSibling, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aPreviousSibling);
#endif //MOZ_ACCESSIBILITY_ATK

  return rv;
}

NS_IMETHODIMP nsXULTreeitemAccessible::DoAction(PRUint8 index)
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  if (index == eAction_Click)
    return mTreeView->ToggleOpenState(mRow);

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsXULTreeitemAccessible::GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  *x = *y = *width = *height = 0;

  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  const PRUnichar empty[] = {'\0'};

  // This Bounds are based on Tree's coord
  mTree->GetCoordsForCellItem(mRow, mColumn.get(), empty, x, y, width, height);

  // Get treechildren's BoxObject to adjust the Bounds' upper left corner
  nsCOMPtr<nsIBoxObject> boxObject(do_QueryInterface(mTree));
  if (boxObject) {
    nsCOMPtr<nsIDOMElement> boxElement;
    boxObject->GetElement(getter_AddRefs(boxElement));
    nsCOMPtr<nsIDOMNode> boxNode(do_QueryInterface(boxElement));
    if (boxNode) {
      nsCOMPtr<nsIDOMNodeList> childNodes;
      boxNode->GetChildNodes(getter_AddRefs(childNodes));
      if (childNodes) {
        nsAutoString name;
        nsCOMPtr<nsIDOMNode> childNode;
        PRUint32 childCount, childIndex;

        childNodes->GetLength(&childCount);
        for (childIndex = 0; childIndex < childCount; childIndex++) {
          childNodes->Item(childIndex, getter_AddRefs(childNode));
          childNode->GetLocalName(name);
          if (name.Equals(NS_LITERAL_STRING("treechildren"))) {
            nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(childNode));
            if (xulElement) {
              nsCOMPtr<nsIBoxObject> box;
              xulElement->GetBoxObject(getter_AddRefs(box));
              if (box) {
                PRInt32 myX, myY;
                box->GetScreenX(&myX);
                box->GetScreenY(&myY);
                *x += myX;
                *y += myY;
              }
            }
            break;
          }
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeitemAccessible::RemoveSelection()
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  nsCOMPtr<nsITreeSelection> selection;
  mTree->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool isSelected;
    selection->IsSelected(mRow, &isSelected);
    if (isSelected)
      selection->ToggleSelect(mRow);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeitemAccessible::TakeSelection()
{
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  nsCOMPtr<nsITreeSelection> selection;
  mTree->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool isSelected;
    selection->IsSelected(mRow, &isSelected);
    if (! isSelected)
      selection->ToggleSelect(mRow);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeitemAccessible::TakeFocus()
{ 
  NS_ENSURE_TRUE(mTree && mTreeView, NS_ERROR_FAILURE);

  nsCOMPtr<nsITreeSelection> selection;
  mTree->GetSelection(getter_AddRefs(selection));
  if (selection)
    selection->SetCurrentIndex(mRow);

  // focus event will be fired here
  return nsAccessible::TakeFocus();
}

// ---------- nsXULTreeColumnsAccessible ----------

nsXULTreeColumnsAccessible::nsXULTreeColumnsAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell):
nsAccessibleWrap(aDOMNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeColumnsAccessible, nsAccessible)

NS_IMETHODIMP nsXULTreeColumnsAccessible::GetState(PRUint32 *_retval)
{
  *_retval = STATE_READONLY;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = ROLE_LIST;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = eSingle_Action;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessible::GetActionName(PRUint8 index, nsAString& _retval)
{
  if (index == eAction_Click) {
    nsAccessible::GetTranslatedString(NS_LITERAL_STRING("click"), _retval);
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsXULTreeColumnsAccessible::GetNextSibling(nsIAccessible **aNextSibling) 
{
  nsresult ret = nsAccessible::GetNextSibling(aNextSibling);

  if (*aNextSibling == nsnull) { // if there is not other sibling, use the first row as its sibling
    nsCOMPtr<nsITreeBoxObject> tree;
    nsCOMPtr<nsITreeView> treeView;

    nsXULTreeAccessible::GetTreeBoxObject(mDOMNode, getter_AddRefs(tree));
    if (tree) {
      tree->GetView(getter_AddRefs(treeView));
      if (treeView) {
        PRInt32 rowCount;
        treeView->GetRowCount(&rowCount);
        if (rowCount > 0) {
          *aNextSibling = new nsXULTreeitemAccessible(mParent, mDOMNode, mWeakShell, 0);
          if (! *aNextSibling)
            return NS_ERROR_OUT_OF_MEMORY;
          NS_ADDREF(*aNextSibling);
          ret = NS_OK;
        }
      }
    }
  }

  return ret;  
}

NS_IMETHODIMP nsXULTreeColumnsAccessible::GetPreviousSibling(nsIAccessible **aPreviousSibling) 
{  
  return nsAccessible::GetPreviousSibling(aPreviousSibling);
}

NS_IMETHODIMP nsXULTreeColumnsAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Click)
    return NS_OK;

  return NS_ERROR_INVALID_ARG;
}

// ---------- nsXULTreeColumnitemAccessible ----------

nsXULTreeColumnitemAccessible::nsXULTreeColumnitemAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell):
nsLeafAccessible(aDOMNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeColumnitemAccessible, nsLeafAccessible)

NS_IMETHODIMP nsXULTreeColumnitemAccessible::GetState(PRUint32 *_retval)
{
  *_retval = STATE_READONLY;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnitemAccessible::GetName(nsAString& _retval)
{
  return GetXULName(_retval);
}

NS_IMETHODIMP nsXULTreeColumnitemAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = ROLE_COLUMNHEADER;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnitemAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = eSingle_Action;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnitemAccessible::GetActionName(PRUint8 index, nsAString& _retval)
{
  if (index == eAction_Click) {
    nsAccessible::GetTranslatedString(NS_LITERAL_STRING("click"), _retval);
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsXULTreeColumnitemAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Click) {
    nsCOMPtr<nsIDOMXULElement> colElement(do_QueryInterface(mDOMNode));
    if (colElement)
      colElement->Click();

    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}
