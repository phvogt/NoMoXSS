/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Original Author: Eric Vaughan (evaughan@netscape.com)
 *                  Kyle Yuan (kyle.yuan@sun.com)
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

#include "nsXULSelectAccessible.h"
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIServiceManager.h"
#include "nsArray.h"

/**
  * Selects, Listboxes and Comboboxes, are made up of a number of different
  *  widgets, some of which are shared between the two. This file contains 
  *  all of the widgets for both of the Selects, for XUL only.
  *
  *  Listbox:
  *     - nsXULListboxAccessible
  *        - nsXULSelectListAccessible
  *           - nsXULSelectOptionAccessible
  *
  *  Comboboxes:
  *     - nsXULComboboxAccessible
  *        - nsHTMLTextFieldAccessible (editable) or nsTextAccessible (readonly)
  *        - nsXULComboboxButtonAccessible
  *         - nsXULSelectListAccessible
  *            - nsXULSelectOptionAccessible
  */

/** ------------------------------------------------------ */
/**  Impl. of nsXULSelectableAccessible                    */
/** ------------------------------------------------------ */

// Helper methos
nsXULSelectableAccessible::nsXULSelectableAccessible(nsIDOMNode* aDOMNode, 
                                                     nsIWeakReference* aShell):
nsAccessibleWrap(aDOMNode, aShell)
{
}

NS_IMETHODIMP nsXULSelectableAccessible::GetName(nsAString& _retval)
{
  return GetXULName(_retval);
}

NS_IMPL_ISUPPORTS_INHERITED1(nsXULSelectableAccessible, nsAccessible, nsIAccessibleSelectable)

NS_IMETHODIMP nsXULSelectableAccessible::ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState)
{
  *aSelState = PR_FALSE;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect(do_QueryInterface(mDOMNode));
  if (xulMultiSelect) {
    nsCOMPtr<nsIDOMNodeList> nodeList;
    xulMultiSelect->GetChildNodes(getter_AddRefs(nodeList));
    if (nodeList) {
      nsCOMPtr<nsIDOMNode> node;
      nodeList->Item(aIndex, getter_AddRefs(node));
      nsCOMPtr<nsIDOMXULSelectControlItemElement> item(do_QueryInterface(node));
      item->GetSelected(aSelState);
      if (eSelection_Add == aMethod && !(*aSelState))
        xulMultiSelect->AddItemToSelection(item);
      else if (eSelection_Remove == aMethod && (*aSelState))
        xulMultiSelect->RemoveItemFromSelection(item);
    }
    return NS_OK;
  }

  nsCOMPtr<nsIDOMXULSelectControlElement> xulSelect(do_QueryInterface(mDOMNode));
  if (xulSelect) {
    nsresult rv = NS_OK;
    PRInt32 selIndex;
    xulSelect->GetSelectedIndex(&selIndex);
    if (selIndex == aIndex)
      *aSelState = PR_TRUE;
    if (eSelection_Add == aMethod && !(*aSelState))
      rv = xulSelect->SetSelectedIndex(aIndex);
    else if (eSelection_Remove == aMethod && (*aSelState)) {
      rv = xulSelect->SetSelectedIndex(-1);
    }
    return rv;
  }

  return NS_ERROR_FAILURE;
}

// Interface methods
NS_IMETHODIMP nsXULSelectableAccessible::GetSelectedChildren(nsIArray **_retval)
{
  *_retval = nsnull;

  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  if (!accService)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIMutableArray> selectedAccessibles;
  NS_NewArray(getter_AddRefs(selectedAccessibles));
  if (!selectedAccessibles)
    return NS_ERROR_OUT_OF_MEMORY;

  // For XUL multi-select control
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect(do_QueryInterface(mDOMNode));
  if (xulMultiSelect) {
    PRInt32 length = 0;
    xulMultiSelect->GetSelectedCount(&length);
    for (PRInt32 index = 0; index < length; index++) {
      nsCOMPtr<nsIAccessible> tempAccessible;
      nsCOMPtr<nsIDOMXULSelectControlItemElement> tempNode;
      xulMultiSelect->GetSelectedItem(index, getter_AddRefs(tempNode));
      nsCOMPtr<nsIDOMNode> tempDOMNode (do_QueryInterface(tempNode));
      accService->GetAccessibleInWeakShell(tempDOMNode, mWeakShell, getter_AddRefs(tempAccessible));
      if (tempAccessible)
        selectedAccessibles->AppendElement(tempAccessible, PR_FALSE);
    }
  }

  PRUint32 uLength = 0;
  selectedAccessibles->GetLength(&uLength); 
  if (uLength != 0) { // length of nsIArray containing selected options
    *_retval = selectedAccessibles;
    NS_ADDREF(*_retval);
  }

  return NS_OK;
}

// return the nth selected child's nsIAccessible object
NS_IMETHODIMP nsXULSelectableAccessible::RefSelection(PRInt32 aIndex, nsIAccessible **_retval)
{
  *_retval = nsnull;

  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  if (!accService)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> tempDOMNode;
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect(do_QueryInterface(mDOMNode));
  if (xulMultiSelect)
    xulMultiSelect->GetSelectedItem(aIndex, getter_AddRefs(tempDOMNode));

  nsCOMPtr<nsIDOMXULSelectControlElement> xulSelect(do_QueryInterface(mDOMNode));
  if (xulSelect && aIndex == 0)
    xulSelect->GetSelectedItem(getter_AddRefs(tempDOMNode));

  if (tempDOMNode) {
    nsCOMPtr<nsIAccessible> tempAccess;
    accService->GetAccessibleInWeakShell(tempDOMNode, mWeakShell, getter_AddRefs(tempAccess));
    *_retval = tempAccess;
    NS_ADDREF(*_retval);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULSelectableAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  *aSelectionCount = 0;

  // For XUL multi-select control
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect(do_QueryInterface(mDOMNode));
  if (xulMultiSelect)
    return xulMultiSelect->GetSelectedCount(aSelectionCount);

  // For XUL single-select control/menulist
  nsCOMPtr<nsIDOMXULSelectControlElement> xulSelect(do_QueryInterface(mDOMNode));
  if (xulSelect) {
    PRInt32 index;
    xulSelect->GetSelectedIndex(&index);
    if (index >= 0)
      *aSelectionCount = 1;
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULSelectableAccessible::AddChildToSelection(PRInt32 aIndex)
{
  PRBool isSelected;
  return ChangeSelection(aIndex, eSelection_Add, &isSelected);
}

NS_IMETHODIMP nsXULSelectableAccessible::RemoveChildFromSelection(PRInt32 aIndex)
{
  PRBool isSelected;
  return ChangeSelection(aIndex, eSelection_Remove, &isSelected);
}

NS_IMETHODIMP nsXULSelectableAccessible::IsChildSelected(PRInt32 aIndex, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return ChangeSelection(aIndex, eSelection_GetState, _retval);
}

NS_IMETHODIMP nsXULSelectableAccessible::ClearSelection()
{
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect(do_QueryInterface(mDOMNode));
  if (xulMultiSelect)
    return xulMultiSelect->ClearSelection();

  nsCOMPtr<nsIDOMXULSelectControlElement> xulSelect(do_QueryInterface(mDOMNode));
  if (xulSelect)
    return xulSelect->SetSelectedIndex(-1);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULSelectableAccessible::SelectAllSelection(PRBool *_retval)
{
  *_retval = PR_TRUE;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect(do_QueryInterface(mDOMNode));
  if (xulMultiSelect)
    return xulMultiSelect->SelectAll();

  // otherwise, don't support this method
  *_retval = PR_FALSE;
  return NS_OK;
}

/** ------------------------------------------------------ */
/**  First, the common widgets                             */
/** ------------------------------------------------------ */

/** ----- nsXULSelectListAccessible ----- */

/** Default Constructor */
nsXULSelectListAccessible::nsXULSelectListAccessible(nsIDOMNode* aDOMNode, 
                                                     nsIWeakReference* aShell)
:nsAccessibleWrap(aDOMNode, aShell)
{
}

NS_IMETHODIMP nsXULSelectListAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = ROLE_LIST;
  return NS_OK;
}

/**
  * As a nsXULSelectListAccessible we can have the following states:
  *     STATE_MULTISELECTABLE
  *     STATE_EXTSELECTABLE
  */
NS_IMETHODIMP nsXULSelectListAccessible::GetState(PRUint32 *_retval)
{ 
  *_retval = 0;
  nsAutoString selectionTypeString;
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  NS_ASSERTION(element, "No nsIDOMElement for caption node!");
  element->GetAttribute(NS_LITERAL_STRING("seltype"), selectionTypeString) ;
  if (selectionTypeString.EqualsIgnoreCase("multiple"))
    *_retval |= STATE_MULTISELECTABLE | STATE_EXTSELECTABLE;

  return NS_OK;
}

/** ----- nsXULSelectOptionAccessible ----- */

/** Default Constructor */
nsXULSelectOptionAccessible::nsXULSelectOptionAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsXULMenuitemAccessible(aDOMNode, aShell)
{
}

NS_IMETHODIMP nsXULSelectOptionAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = ROLE_LISTITEM;
  return NS_OK;
}

/**
  * As a nsXULSelectOptionAccessible we can have the following states:
  *     STATE_SELECTABLE
  *     STATE_SELECTED
  *     STATE_FOCUSED
  *     STATE_FOCUSABLE
  */
NS_IMETHODIMP nsXULSelectOptionAccessible::GetState(PRUint32 *_retval)
{
  nsXULMenuitemAccessible::GetState(_retval);

  nsCOMPtr<nsIDOMXULSelectControlItemElement> item(do_QueryInterface(mDOMNode));
  PRBool isSelected = PR_FALSE;
  item->GetSelected(&isSelected);
  if (isSelected)
    *_retval |= STATE_SELECTED;

  return NS_OK;
}

/** ------------------------------------------------------ */
/**  Secondly, the Listbox widget                          */
/** ------------------------------------------------------ */

/** ----- nsXULListboxAccessible ----- */

/** Constructor */
nsXULListboxAccessible::nsXULListboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsXULSelectableAccessible(aDOMNode, aShell)
{
}

/**
  * Let Accessible count them up
  */
NS_IMETHODIMP nsXULListboxAccessible::GetChildCount(PRInt32 *_retval)
{
  return nsAccessible::GetChildCount(_retval);
}

/**
  * As a nsXULListboxAccessible we can have the following states:
  *     STATE_FOCUSED
  *     STATE_READONLY
  *     STATE_FOCUSABLE
  */
NS_IMETHODIMP nsXULListboxAccessible::GetState(PRUint32 *_retval)
{
  // Get focus status from base class
  nsAccessible::GetState(_retval);

  *_retval |= STATE_READONLY | STATE_FOCUSABLE;

// see if we are multiple select if so set ourselves as such
  nsCOMPtr<nsIDOMElement> element (do_QueryInterface(mDOMNode));
  if (element) {
    nsAutoString selType;
    element->GetAttribute(NS_LITERAL_STRING("seltype"), selType);
    if (!selType.IsEmpty() && selType.Equals(NS_LITERAL_STRING("multiple")))
        *_retval |= STATE_MULTISELECTABLE;
  }

  *_retval |= STATE_FOCUSABLE ;

  return NS_OK;
}

/**
  * Our value is the value of our ( first ) selected child. nsIDOMXULSelectElement
  *     returns this by default with GetValue().
  */
NS_IMETHODIMP nsXULListboxAccessible::GetValue(nsAString& _retval)
{
  _retval.Truncate();
  nsCOMPtr<nsIDOMXULSelectControlElement> select(do_QueryInterface(mDOMNode));
  if (select) {
    nsCOMPtr<nsIDOMXULSelectControlItemElement> selectedItem;
    select->GetSelectedItem(getter_AddRefs(selectedItem));
    if (selectedItem)
      return selectedItem->GetLabel(_retval);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULListboxAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = ROLE_LIST;
  return NS_OK;
}

/** ----- nsXULListitemAccessible ----- */

/** Constructor */
nsXULListitemAccessible::nsXULListitemAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsXULMenuitemAccessible(aDOMNode, aShell)
{
}

/** Inherit the ISupports impl from nsAccessible, we handle nsIAccessibleSelectable */
NS_IMPL_ISUPPORTS_INHERITED0(nsXULListitemAccessible, nsAccessible)

/**
  * If there is a Listcell as a child ( not anonymous ) use it, otherwise
  *   default to getting the name from GetXULName
  */
NS_IMETHODIMP nsXULListitemAccessible::GetName(nsAString& _retval)
{
  nsCOMPtr<nsIDOMNode> child;
  if (NS_SUCCEEDED(mDOMNode->GetFirstChild(getter_AddRefs(child)))) {
    nsCOMPtr<nsIDOMElement> childElement (do_QueryInterface(child));
    if (childElement) {
      nsAutoString tagName;
      childElement->GetLocalName(tagName);
      if (tagName.Equals(NS_LITERAL_STRING("listcell"))) {
        childElement->GetAttribute(NS_LITERAL_STRING("label"), _retval);
        return NS_OK;
      }
    }
  }
  return GetXULName(_retval);
}

/**
  *
  */
NS_IMETHODIMP nsXULListitemAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = ROLE_LISTITEM;
  return NS_OK;
}

/**
  *
  */
NS_IMETHODIMP nsXULListitemAccessible::GetState(PRUint32 *_retval)
{
//  nsAccessible::GetState(_retval); // get focused state

  nsCOMPtr<nsIDOMXULSelectControlItemElement> listItem (do_QueryInterface(mDOMNode));
  if (listItem) {
    PRBool isSelected;
    listItem->GetSelected(&isSelected);
    if (isSelected)
      *_retval |= STATE_SELECTED;

    nsCOMPtr<nsIDOMNode> domParent;
    mDOMNode->GetParentNode(getter_AddRefs(domParent));
    nsCOMPtr<nsIDOMXULMultiSelectControlElement> parent(do_QueryInterface(domParent));
    if (parent) {
      nsCOMPtr<nsIDOMXULSelectControlItemElement> current;
      parent->GetCurrentItem(getter_AddRefs(current));
      if (listItem == current)
        *_retval |= STATE_FOCUSED;
    }

  *_retval |= STATE_FOCUSABLE | STATE_SELECTABLE;

  }


  return NS_OK;
}

/** ------------------------------------------------------ */
/**  Finally, the Combobox widgets                         */
/** ------------------------------------------------------ */

/** ----- nsXULComboboxAccessible ----- */

/** Constructor */
nsXULComboboxAccessible::nsXULComboboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsXULSelectableAccessible(aDOMNode, aShell)
{
}

/** We are a combobox */
NS_IMETHODIMP nsXULComboboxAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = ROLE_COMBOBOX;
  return NS_OK;
}

/**
  * As a nsComboboxAccessible we can have the following states:
  *     STATE_FOCUSED
  *     STATE_READONLY
  *     STATE_FOCUSABLE
  *     STATE_HASPOPUP
  *     STATE_EXPANDED
  *     STATE_COLLAPSED
  */
NS_IMETHODIMP nsXULComboboxAccessible::GetState(PRUint32 *_retval)
{
  // Get focus status from base class
  nsAccessible::GetState(_retval);

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mDOMNode));
  if (menuList) {
    PRBool isOpen;
    menuList->GetOpen(&isOpen);
    if (isOpen)
      *_retval |= STATE_EXPANDED;
    else
      *_retval |= STATE_COLLAPSED;
  }

  *_retval |= STATE_HASPOPUP | STATE_READONLY | STATE_FOCUSABLE;

  return NS_OK;
}

/**
  * Our value is the name of our ( first ) selected child. nsIDOMXULSelectElement
  *     returns this by default with GetValue().
  */
NS_IMETHODIMP nsXULComboboxAccessible::GetValue(nsAString& _retval)
{
  // The first accessible child is the text accessible that contains the name of the selected element.
  // This is our value
  nsCOMPtr<nsIAccessible> firstChild;
  GetFirstChild(getter_AddRefs(firstChild));
  return firstChild->GetName(_retval);
}
