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
#include "nsIDOMHTMLTableElement.h"
#include "nsIDOMHTMLTableCaptionElem.h"
#include "nsIDOMHTMLTableSectionElem.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventReceiver.h"
#include "nsDOMError.h"
#include "GenericElementCollection.h"
#include "nsIHTMLContent.h"
#include "nsMappedAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsHTMLParts.h"
#include "nsRuleNode.h"
#include "nsStyleContext.h"
#include "nsIDocument.h"

/* for collections */
#include "nsIDOMElement.h"
#include "nsGenericHTMLElement.h"
/* end for collections */

class GenericElementCollection;
class TableRowsCollection;

class nsHTMLTableElement :  public nsGenericHTMLElement,
                            public nsIDOMHTMLTableElement
{
public:
  nsHTMLTableElement();
  virtual ~nsHTMLTableElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  // nsIDOMHTMLTableElement
  NS_DECL_NSIDOMHTMLTABLEELEMENT

  virtual PRBool ParseAttribute(nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAString& aResult) const;
  NS_IMETHOD GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

protected:
  already_AddRefed<nsIDOMHTMLTableSectionElement> GetSection(nsIAtom *aTag);

  GenericElementCollection *mTBodies;
  TableRowsCollection *mRows;
};


/* ------------------------------ TableRowsCollection -------------------------------- */
/**
 * This class provides a late-bound collection of rows in a table.
 * mParent is NOT ref-counted to avoid circular references
 */
class TableRowsCollection : public nsGenericDOMHTMLCollection 
{
public:
  TableRowsCollection(nsHTMLTableElement *aParent);
  virtual ~TableRowsCollection();

  NS_IMETHOD    GetLength(PRUint32* aLength);
  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMNode** aReturn);
  NS_IMETHOD    NamedItem(const nsAString& aName,
                          nsIDOMNode** aReturn);

  NS_IMETHOD    ParentDestroyed();

protected:
  nsHTMLTableElement * mParent;
};


TableRowsCollection::TableRowsCollection(nsHTMLTableElement *aParent)
  : nsGenericDOMHTMLCollection()
{
  mParent = aParent;
}

TableRowsCollection::~TableRowsCollection()
{
  // we do NOT have a ref-counted reference to mParent, so do NOT
  // release it!  this is to avoid circular references.  The
  // instantiator who provided mParent is responsible for managing our
  // reference for us.
}

// we re-count every call.  A better implementation would be to set
// ourselves up as an observer of contentAppended, contentInserted,
// and contentDeleted
NS_IMETHODIMP 
TableRowsCollection::GetLength(PRUint32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  *aLength=0;
  nsresult rv = NS_OK;

  if (mParent) {
    // count the rows in the thead, tfoot, and all tbodies
    nsCOMPtr<nsIDOMHTMLTableSectionElement> rowGroup;

    mParent->GetTHead(getter_AddRefs(rowGroup));

    if (rowGroup) {
      nsCOMPtr<nsIContent> content(do_QueryInterface(rowGroup));

      GenericElementCollection head(content, nsHTMLAtoms::tr);

      PRUint32 rows;
      head.GetLength(&rows);

      *aLength = rows;
    }

    mParent->GetTFoot(getter_AddRefs(rowGroup));

    if (rowGroup) {
      nsCOMPtr<nsIContent> content(do_QueryInterface(rowGroup));

      GenericElementCollection foot(content, nsHTMLAtoms::tr);

      PRUint32 rows;
      foot.GetLength(&rows);
      *aLength += rows;
    }

    nsCOMPtr<nsIDOMHTMLCollection> tbodies;

    mParent->GetTBodies(getter_AddRefs(tbodies));

    if (tbodies) {
      nsCOMPtr<nsIDOMNode> node;

      rowGroup = nsnull;

      PRUint32 theIndex=0;

      tbodies->Item(theIndex, getter_AddRefs(node));

      while (node) {
        nsCOMPtr<nsIContent> content(do_QueryInterface(node));

        GenericElementCollection body(content, nsHTMLAtoms::tr);

        PRUint32 rows;
        body.GetLength(&rows);

        *aLength += rows;

        theIndex++;

        tbodies->Item(theIndex, getter_AddRefs(node));
      }
    }
  }

  return rv;
}

// increments aReturn refcnt by 1
NS_IMETHODIMP 
TableRowsCollection::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  *aReturn = nsnull;
  nsresult rv = NS_OK;
  PRUint32 count = 0;
 
  if (mParent) {
    nsCOMPtr<nsIDOMHTMLTableSectionElement> rowGroup;

    // check the thead
    mParent->GetTHead(getter_AddRefs(rowGroup));

    if (rowGroup) {
      nsCOMPtr<nsIContent> content(do_QueryInterface(rowGroup));

      GenericElementCollection head(content, nsHTMLAtoms::tr);

      PRUint32 rowsInHead;
      head.GetLength(&rowsInHead);

      count = rowsInHead;

      if (count > aIndex) {
        head.Item(aIndex, aReturn);
        return NS_OK; 
      }
    }

    // check the tbodies
    nsCOMPtr<nsIDOMHTMLCollection> tbodies;

    mParent->GetTBodies(getter_AddRefs(tbodies));

    if (tbodies) {
      rowGroup = nsnull;
      nsCOMPtr<nsIDOMNode> node;
      PRUint32 theIndex=0;

      tbodies->Item(theIndex, getter_AddRefs(node));

      while (node) {
        nsCOMPtr<nsIContent> content(do_QueryInterface(node));

        GenericElementCollection body(content, nsHTMLAtoms::tr);

        PRUint32 rows;
        body.GetLength(&rows);

        if ((count+rows) > aIndex) {
          body.Item(aIndex-count, aReturn);

		      return NS_OK;
        }

        count += rows;
        theIndex++;
        tbodies->Item(theIndex, getter_AddRefs(node));
      }
    }

    // check the tfoot
    mParent->GetTFoot(getter_AddRefs(rowGroup));

    if (rowGroup) {
      nsCOMPtr<nsIContent> content(do_QueryInterface(rowGroup));

      GenericElementCollection foot(content, nsHTMLAtoms::tr);
      foot.Item(aIndex-count, aReturn);
    }
  }

  return rv;
}

NS_IMETHODIMP 
TableRowsCollection::NamedItem(const nsAString& aName,
                               nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  // FIXME: Implement this!

  *aReturn = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
TableRowsCollection::ParentDestroyed()
{
  // see comment in destructor, do NOT release mParent!
  mParent = nsnull;

  return NS_OK;
}

/* -------------------------- nsHTMLTableElement --------------------------- */
// the class declaration is at the top of this file

nsresult
NS_NewHTMLTableElement(nsIHTMLContent** aInstancePtrResult,
                       nsINodeInfo *aNodeInfo, PRBool aFromParser)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLTableElement* it = new nsHTMLTableElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = it->Init(aNodeInfo);

  if (NS_FAILED(rv)) {
    delete it;

    return rv;
  }

  *aInstancePtrResult = NS_STATIC_CAST(nsIHTMLContent *, it);
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}


nsHTMLTableElement::nsHTMLTableElement()
{
  mTBodies=nsnull;
  mRows=nsnull;
}

nsHTMLTableElement::~nsHTMLTableElement()
{
  if (mTBodies) {
    mTBodies->ParentDestroyed();
    NS_RELEASE(mTBodies);
  }

  if (mRows) {
    mRows->ParentDestroyed();
    NS_RELEASE(mRows);
  }
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTableElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableElement, nsGenericElement) 


// QueryInterface implementation for nsHTMLTableElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLTableElement, nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLTableElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLTableElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLTableElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLTableElement* it = new nsHTMLTableElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIDOMNode> kungFuDeathGrip(it);

  nsresult rv = it->Init(mNodeInfo);

  if (NS_FAILED(rv))
    return rv;

  CopyInnerTo(it, aDeep);

  *aReturn = NS_STATIC_CAST(nsIDOMNode *, it);

  NS_ADDREF(*aReturn);

  return NS_OK;
}


// the DOM spec says border, cellpadding, cellSpacing are all "wstring"
// in fact, they are integers or they are meaningless.  so we store them
// here as ints.

NS_IMPL_STRING_ATTR(nsHTMLTableElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, BgColor, bgcolor)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, Border, border)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, CellPadding, cellpadding)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, CellSpacing, cellspacing)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, Frame, frame)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, Rules, rules)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, Summary, summary)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, Width, width)


NS_IMETHODIMP
nsHTMLTableElement::GetCaption(nsIDOMHTMLTableCaptionElement** aValue)
{
  *aValue = nsnull;
  nsCOMPtr<nsIDOMNode> child;
  GetFirstChild(getter_AddRefs(child));

  while (child) {
    nsCOMPtr<nsIDOMHTMLTableCaptionElement> caption(do_QueryInterface(child));

    if (caption) {
      *aValue = caption;
      NS_ADDREF(*aValue);

      break;
    }

    nsIDOMNode *temp = child.get();
    temp->GetNextSibling(getter_AddRefs(child));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::SetCaption(nsIDOMHTMLTableCaptionElement* aValue)
{
  nsresult rv = DeleteCaption();

  if (NS_SUCCEEDED(rv)) {
    if (aValue) {
      nsCOMPtr<nsIDOMNode> resultingChild;
      AppendChild(aValue, getter_AddRefs(resultingChild));
    }
  }

  return rv;
}

already_AddRefed<nsIDOMHTMLTableSectionElement>
nsHTMLTableElement::GetSection(nsIAtom *aTag)
{
  PRUint32 childCount = GetChildCount();

  nsCOMPtr<nsIDOMHTMLTableSectionElement> section;

  for (PRUint32 i = 0; i < childCount; ++i) {
    nsIContent *child = GetChildAt(i);

    section = do_QueryInterface(child);

    if (section && child->GetNodeInfo()->Equals(aTag)) {
      nsIDOMHTMLTableSectionElement *result = section;
      NS_ADDREF(result);

      return result;
    }
  }

  return nsnull;
}

NS_IMETHODIMP
nsHTMLTableElement::GetTHead(nsIDOMHTMLTableSectionElement** aValue)
{
  *aValue = GetSection(nsHTMLAtoms::thead).get();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::SetTHead(nsIDOMHTMLTableSectionElement* aValue)
{
  nsresult rv = DeleteTHead();
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (aValue) {
    nsCOMPtr<nsIDOMNode> child;
    rv = GetFirstChild(getter_AddRefs(child));
    if (NS_FAILED(rv)) {
      return rv;
    }
     
    nsCOMPtr<nsIDOMNode> resultChild;
    rv = InsertBefore(aValue, child, getter_AddRefs(resultChild));
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLTableElement::GetTFoot(nsIDOMHTMLTableSectionElement** aValue)
{
  *aValue = GetSection(nsHTMLAtoms::tfoot).get();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::SetTFoot(nsIDOMHTMLTableSectionElement* aValue)
{
  nsresult rv = DeleteTFoot();
  if (NS_SUCCEEDED(rv)) {
    if (aValue) {
      nsCOMPtr<nsIDOMNode> resultingChild;
      AppendChild(aValue, getter_AddRefs(resultingChild));
    }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLTableElement::GetRows(nsIDOMHTMLCollection** aValue)
{
  if (!mRows) {
    // XXX why was this here NS_ADDREF(nsHTMLAtoms::tr);
    mRows = new TableRowsCollection(this);
    NS_ENSURE_TRUE(mRows, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(mRows); // this table's reference, released in the destructor
  }

  *aValue = mRows;
  NS_ADDREF(*aValue);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::GetTBodies(nsIDOMHTMLCollection** aValue)
{
  if (!mTBodies) {
    mTBodies = new GenericElementCollection((nsIContent*)this,
                                            nsHTMLAtoms::tbody);

    NS_ENSURE_TRUE(mTBodies, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(mTBodies); // this table's reference, released in the destructor
  }

  return CallQueryInterface(mTBodies, aValue);
}

NS_IMETHODIMP
nsHTMLTableElement::CreateTHead(nsIDOMHTMLElement** aValue)
{
  *aValue = nsnull;
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMHTMLTableSectionElement> head;

  GetTHead(getter_AddRefs(head));

  if (head) { // return the existing thead
    CallQueryInterface(head, aValue);

    NS_ASSERTION(*aValue, "head must be a DOMHTMLElement");
  }
  else
  { // create a new head rowgroup
    nsCOMPtr<nsIHTMLContent> newHead;
    nsCOMPtr<nsINodeInfo> nodeInfo;

    mNodeInfo->NameChanged(nsHTMLAtoms::thead, getter_AddRefs(nodeInfo));

    rv = NS_NewHTMLTableSectionElement(getter_AddRefs(newHead),nodeInfo);

    if (NS_SUCCEEDED(rv) && newHead) {
      nsCOMPtr<nsIDOMNode> child;

      rv = GetFirstChild(getter_AddRefs(child));

      if (NS_FAILED(rv)) {
        return rv;
      }

      CallQueryInterface(newHead, aValue);

      nsCOMPtr<nsIDOMNode> resultChild;
      rv = InsertBefore(*aValue, child, getter_AddRefs(resultChild));
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::DeleteTHead()
{
  nsCOMPtr<nsIDOMHTMLTableSectionElement> childToDelete;
  nsresult rv = GetTHead(getter_AddRefs(childToDelete));

  if ((NS_SUCCEEDED(rv)) && childToDelete) {
    nsCOMPtr<nsIDOMNode> resultingChild;
    // mInner does the notification
    RemoveChild(childToDelete, getter_AddRefs(resultingChild));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::CreateTFoot(nsIDOMHTMLElement** aValue)
{
  *aValue = nsnull;
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMHTMLTableSectionElement> foot;

  GetTFoot(getter_AddRefs(foot));

  if (foot) { // return the existing tfoot
    CallQueryInterface(foot, aValue);

    NS_ASSERTION(*aValue, "foot must be a DOMHTMLElement");
  }
  else
  { // create a new foot rowgroup
    nsCOMPtr<nsIHTMLContent> newFoot;

    nsCOMPtr<nsINodeInfo> nodeInfo;
    mNodeInfo->NameChanged(nsHTMLAtoms::tfoot, getter_AddRefs(nodeInfo));

    rv = NS_NewHTMLTableSectionElement(getter_AddRefs(newFoot),nodeInfo);

    if (NS_SUCCEEDED(rv) && newFoot) {
      rv = AppendChildTo(newFoot, PR_TRUE, PR_FALSE);
      CallQueryInterface(newFoot, aValue);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::DeleteTFoot()
{
  nsCOMPtr<nsIDOMHTMLTableSectionElement> childToDelete;
  nsresult rv = GetTFoot(getter_AddRefs(childToDelete));

  if ((NS_SUCCEEDED(rv)) && childToDelete) {
    nsCOMPtr<nsIDOMNode> resultingChild;
    // mInner does the notification
    RemoveChild(childToDelete, getter_AddRefs(resultingChild));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::CreateCaption(nsIDOMHTMLElement** aValue)
{
  *aValue = nsnull;
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMHTMLTableCaptionElement> caption;

  GetCaption(getter_AddRefs(caption));

  if (caption) { // return the existing thead
    CallQueryInterface(caption, aValue);

    NS_ASSERTION(*aValue, "caption must be a DOMHTMLElement");
  }
  else
  { // create a new head rowgroup
    nsCOMPtr<nsIHTMLContent> newCaption;

    nsCOMPtr<nsINodeInfo> nodeInfo;
    mNodeInfo->NameChanged(nsHTMLAtoms::caption, getter_AddRefs(nodeInfo));

    rv = NS_NewHTMLTableCaptionElement(getter_AddRefs(newCaption),nodeInfo);

    if (NS_SUCCEEDED(rv) && newCaption) {
      rv = AppendChildTo(newCaption, PR_TRUE, PR_FALSE);
      CallQueryInterface(newCaption, aValue);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::DeleteCaption()
{
  nsCOMPtr<nsIDOMHTMLTableCaptionElement> childToDelete;
  nsresult rv = GetCaption(getter_AddRefs(childToDelete));

  if ((NS_SUCCEEDED(rv)) && childToDelete) {
    nsCOMPtr<nsIDOMNode> resultingChild;
    RemoveChild(childToDelete, getter_AddRefs(resultingChild));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::InsertRow(PRInt32 aIndex, nsIDOMHTMLElement** aValue)
{
  /* get the ref row at aIndex
     if there is one, 
       get it's parent
       insert the new row just before the ref row
     else
       get the first row group
       insert the new row as its first child
  */
  *aValue = nsnull;

  if (aIndex < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsresult rv;

  nsCOMPtr<nsIDOMHTMLCollection> rows;
  GetRows(getter_AddRefs(rows));

  PRUint32 rowCount;
  rows->GetLength(&rowCount);

  if ((PRUint32)aIndex > rowCount && aIndex != -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  // use local variable refIndex so we can remember original aIndex
  PRUint32 refIndex = (PRUint32)aIndex;

  if (rowCount > 0) {
    if (refIndex == rowCount || aIndex == -1) {
      // we set refIndex to the last row so we can get the last row's
      // parent we then do an AppendChild below if (rowCount<aIndex)

      refIndex = rowCount - 1;
    }

    nsCOMPtr<nsIDOMNode> refRow;
    rows->Item(refIndex, getter_AddRefs(refRow));

    nsCOMPtr<nsIDOMNode> parent;

    refRow->GetParentNode(getter_AddRefs(parent));
    // create the row

    nsCOMPtr<nsIHTMLContent> newRow;

    nsCOMPtr<nsINodeInfo> nodeInfo;
    mNodeInfo->NameChanged(nsHTMLAtoms::tr, getter_AddRefs(nodeInfo));

    rv = NS_NewHTMLTableRowElement(getter_AddRefs(newRow), nodeInfo);

    if (NS_SUCCEEDED(rv) && newRow) {
      nsCOMPtr<nsIDOMNode> newRowNode(do_QueryInterface(newRow));
      nsCOMPtr<nsIDOMNode> retChild;

      // If index is -1 or equal to the number of rows, the new row
      // is appended.
      if (aIndex == -1 || PRUint32(aIndex) == rowCount) {
        rv = parent->AppendChild(newRowNode, getter_AddRefs(retChild));
      }
      else
      {
        // insert the new row before the reference row we found above
        rv = parent->InsertBefore(newRowNode, refRow,
                                  getter_AddRefs(retChild));
      }

      if (retChild) {
        CallQueryInterface(retChild, aValue);
      }
    }
  }
  else
  { // the row count was 0, so 
    // find the first row group and insert there as first child
    nsCOMPtr<nsIDOMNode> rowGroup;

    GenericElementCollection head(this, nsHTMLAtoms::thead);

    PRUint32 length = 0;

    head.GetLength(&length);

    if (0 != length) {
      head.Item(0, getter_AddRefs(rowGroup));
    }
    else
    {
      GenericElementCollection body(this, nsHTMLAtoms::tbody);
      length = 0;

      body.GetLength(&length);

      if (length > 0) {
        body.Item(0, getter_AddRefs(rowGroup));
      }
      else
      {
        GenericElementCollection foot(this, nsHTMLAtoms::tfoot);

        length = 0;

        foot.GetLength(&length);

        if (length > 0) {
          foot.Item(0, getter_AddRefs(rowGroup));
        }
      }
    }

    if (!rowGroup) { // need to create a TBODY
      nsCOMPtr<nsIHTMLContent> newRowGroup;

      nsCOMPtr<nsINodeInfo> nodeInfo;
      mNodeInfo->NameChanged(nsHTMLAtoms::tbody, getter_AddRefs(nodeInfo));

      rv = NS_NewHTMLTableSectionElement(getter_AddRefs(newRowGroup),
                                         nodeInfo);

      if (NS_SUCCEEDED(rv) && newRowGroup) {
        rv = AppendChildTo(newRowGroup, PR_TRUE, PR_FALSE);

        rowGroup = do_QueryInterface(newRowGroup);
      }
    }

    if (rowGroup) {
      nsCOMPtr<nsIHTMLContent> newRow;

      nsCOMPtr<nsINodeInfo> nodeInfo;
      mNodeInfo->NameChanged(nsHTMLAtoms::tr, getter_AddRefs(nodeInfo));

      rv = NS_NewHTMLTableRowElement(getter_AddRefs(newRow), nodeInfo);
      nsCOMPtr<nsIContent> rowGroupContent(do_QueryInterface(rowGroup));

      GenericElementCollection rowGroupRows(rowGroupContent, nsHTMLAtoms::tr);

      nsCOMPtr<nsIDOMNode> firstRow;

      // it's ok if this returns nsnull
      rowGroupRows.Item(0, getter_AddRefs(firstRow));

      if (newRow) {
        nsCOMPtr<nsIDOMNode> retNode, newRowNode(do_QueryInterface(newRow));

        rowGroup->InsertBefore(newRowNode, firstRow, getter_AddRefs(retNode));

        if (retNode) {
          CallQueryInterface(retNode, aValue);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::DeleteRow(PRInt32 aValue)
{
  if (aValue < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHTMLCollection> rows;
  GetRows(getter_AddRefs(rows));

  nsresult rv;
  PRUint32 refIndex;
  if (aValue == -1) {
    rv = rows->GetLength(&refIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    if (refIndex == 0) {
      return NS_OK;
    }

    --refIndex;
  }
  else {
    refIndex = (PRUint32)aValue;
  }

  nsCOMPtr<nsIDOMNode> row;
  rv = rows->Item(refIndex, getter_AddRefs(row));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!row) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMNode> parent;
  row->GetParentNode(getter_AddRefs(parent));
  NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMNode> deleted_row;
  return parent->RemoveChild(row, getter_AddRefs(deleted_row));
}

static const nsHTMLValue::EnumTable kFrameTable[] = {
  { "void",   NS_STYLE_TABLE_FRAME_NONE },
  { "above",  NS_STYLE_TABLE_FRAME_ABOVE },
  { "below",  NS_STYLE_TABLE_FRAME_BELOW },
  { "hsides", NS_STYLE_TABLE_FRAME_HSIDES },
  { "lhs",    NS_STYLE_TABLE_FRAME_LEFT },
  { "rhs",    NS_STYLE_TABLE_FRAME_RIGHT },
  { "vsides", NS_STYLE_TABLE_FRAME_VSIDES },
  { "box",    NS_STYLE_TABLE_FRAME_BOX },
  { "border", NS_STYLE_TABLE_FRAME_BORDER },
  { 0 }
};

static const nsHTMLValue::EnumTable kRulesTable[] = {
  { "none",   NS_STYLE_TABLE_RULES_NONE },
  { "groups", NS_STYLE_TABLE_RULES_GROUPS },
  { "rows",   NS_STYLE_TABLE_RULES_ROWS },
  { "cols",   NS_STYLE_TABLE_RULES_COLS },
  { "all",    NS_STYLE_TABLE_RULES_ALL },
  { 0 }
};

static const nsHTMLValue::EnumTable kLayoutTable[] = {
  { "auto",   NS_STYLE_TABLE_LAYOUT_AUTO },
  { "fixed",  NS_STYLE_TABLE_LAYOUT_FIXED },
  { 0 }
};


PRBool
nsHTMLTableElement::ParseAttribute(nsIAtom* aAttribute,
                                   const nsAString& aValue,
                                   nsAttrValue& aResult)
{
  /* ignore summary, just a string */
  if (aAttribute == nsHTMLAtoms::cellspacing ||
      aAttribute == nsHTMLAtoms::cellpadding) {
    return aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE);
  }
  if (aAttribute == nsHTMLAtoms::cols) {
    return aResult.ParseIntWithBounds(aValue, 0);
  }
  if (aAttribute == nsHTMLAtoms::border) {
    if (!aResult.ParseIntWithBounds(aValue, 0)) {
      // XXX this should really be NavQuirks only to allow non numeric value
      aResult.SetTo(1, nsAttrValue::eInteger);
    }

    return PR_TRUE;
  }
  if (aAttribute == nsHTMLAtoms::height) {
    return aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE);
  }
  if (aAttribute == nsHTMLAtoms::width) {
    if (aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE)) {
      // treat 0 width as auto
      nsAttrValue::ValueType type = aResult.Type();
      if ((type == nsAttrValue::eInteger && aResult.GetIntegerValue() == 0) ||
          (type == nsAttrValue::ePercent && aResult.GetPercentValue() == 0.0f)) {
        return PR_FALSE;
      }
    }
    return PR_TRUE;
  }
  if (aAttribute == nsHTMLAtoms::align) {
    return ParseTableHAlignValue(aValue, aResult);
  }
  if (aAttribute == nsHTMLAtoms::bgcolor ||
           aAttribute == nsHTMLAtoms::bordercolor) {
    return aResult.ParseColor(aValue, nsGenericHTMLElement::GetOwnerDocument());
  }
  if (aAttribute == nsHTMLAtoms::frame) {
    return aResult.ParseEnumValue(aValue, kFrameTable);
  }
  if (aAttribute == nsHTMLAtoms::layout) {
    return aResult.ParseEnumValue(aValue, kLayoutTable);
  }
  if (aAttribute == nsHTMLAtoms::rules) {
    return aResult.ParseEnumValue(aValue, kRulesTable);
  }
  if (aAttribute == nsHTMLAtoms::hspace ||
           aAttribute == nsHTMLAtoms::vspace) {
    return aResult.ParseIntWithBounds(aValue, 0);
  }

  return nsGenericHTMLElement::ParseAttribute(aAttribute, aValue, aResult);
}

NS_IMETHODIMP
nsHTMLTableElement::AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAString& aResult) const
{
  /* ignore summary, just a string */
  /* ignore attributes that are of standard types border, cellpadding,
     cellspacing, cols, height, width, background, bgcolor */
  if (aAttribute == nsHTMLAtoms::align) {
    if (TableHAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::frame) {
    if (aValue.EnumValueToString(kFrameTable, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::layout) {
    if (aValue.EnumValueToString(kLayoutTable, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::rules) {
    if (aValue.EnumValueToString(kRulesTable, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return nsGenericHTMLElement::AttributeToString(aAttribute, aValue, aResult);
}

static void 
MapTableFrameInto(const nsMappedAttributes* aAttributes,
                  nsRuleData* aData, PRUint8 aBorderStyle)
{
  if (!aData->mMarginData)
    return;

  // set up defaults
  if (aData->mMarginData->mBorderStyle.mLeft.GetUnit() == eCSSUnit_Null)
    aData->mMarginData->mBorderStyle.mLeft.SetIntValue(aBorderStyle, eCSSUnit_Enumerated);
  if (aData->mMarginData->mBorderStyle.mRight.GetUnit() == eCSSUnit_Null)
    aData->mMarginData->mBorderStyle.mRight.SetIntValue(aBorderStyle, eCSSUnit_Enumerated);
  if (aData->mMarginData->mBorderStyle.mTop.GetUnit() == eCSSUnit_Null)
    aData->mMarginData->mBorderStyle.mTop.SetIntValue(aBorderStyle, eCSSUnit_Enumerated);
  if (aData->mMarginData->mBorderStyle.mBottom.GetUnit() == eCSSUnit_Null)
    aData->mMarginData->mBorderStyle.mBottom.SetIntValue(aBorderStyle, eCSSUnit_Enumerated);

  nsHTMLValue frameValue;
  // 0 out the sides that we want to hide based on the frame attribute
  aAttributes->GetAttribute(nsHTMLAtoms::frame, frameValue);

  if (frameValue.GetUnit() == eHTMLUnit_Enumerated) {
    // adjust the border style based on the value of frame
    switch (frameValue.GetIntValue())
    {
    case NS_STYLE_TABLE_FRAME_NONE:
      aData->mMarginData->mBorderStyle.mLeft.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mRight.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mTop.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mBottom.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      break;
    case NS_STYLE_TABLE_FRAME_ABOVE:
      aData->mMarginData->mBorderStyle.mLeft.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mRight.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mBottom.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      break;
    case NS_STYLE_TABLE_FRAME_BELOW: 
      aData->mMarginData->mBorderStyle.mLeft.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mRight.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mTop.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      break;
    case NS_STYLE_TABLE_FRAME_HSIDES:
      aData->mMarginData->mBorderStyle.mLeft.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mRight.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      break;
    case NS_STYLE_TABLE_FRAME_LEFT:
      aData->mMarginData->mBorderStyle.mRight.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mTop.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mBottom.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      break;
    case NS_STYLE_TABLE_FRAME_RIGHT:
      aData->mMarginData->mBorderStyle.mLeft.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mTop.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mBottom.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      break;
    case NS_STYLE_TABLE_FRAME_VSIDES:
      aData->mMarginData->mBorderStyle.mTop.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      aData->mMarginData->mBorderStyle.mBottom.SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
      break;
    // BOX and BORDER are ignored, the caller has already set all the border sides
    // any illegal value is also ignored
    }
  }
}

static void 
MapTableBorderInto(const nsMappedAttributes* aAttributes,
                   nsRuleData* aData, PRUint8 aBorderStyle)
{
  nsHTMLValue borderValue;
  if (aAttributes->GetAttribute(nsHTMLAtoms::border, borderValue) ==
      NS_CONTENT_ATTR_NOT_THERE) {
    // the absence of "border" with the presence of "frame" implies
    // border = 1 pixel
    if (!aAttributes->GetAttr(nsHTMLAtoms::frame)) {
      return;
    }
    borderValue.SetIntValue(1, eHTMLUnit_Integer);
  }

  if (borderValue.GetUnit() != eHTMLUnit_Integer) {
    // empty values of border get rules=all and frame=border
    if (aData->mTableData) {
      aData->mTableData->mRules.SetIntValue(NS_STYLE_TABLE_RULES_ALL, eCSSUnit_Enumerated);
      aData->mTableData->mFrame.SetIntValue(NS_STYLE_TABLE_FRAME_BORDER, eCSSUnit_Enumerated);
    }
    borderValue.SetIntValue(1, eHTMLUnit_Integer);
  }
  else {
    PRInt32 borderThickness = borderValue.GetIntValue();

    if (0 != borderThickness) {
      // border != 0 implies rules=all and frame=border
      if (aData->mTableData) {
        aData->mTableData->mRules.SetIntValue(NS_STYLE_TABLE_RULES_ALL, eCSSUnit_Enumerated);
        aData->mTableData->mFrame.SetIntValue(NS_STYLE_TABLE_FRAME_BORDER, eCSSUnit_Enumerated);
      }
    }
    else {
      // border = 0 implies rules=none and frame=void
      if (aData->mTableData) {
        aData->mTableData->mRules.SetIntValue(NS_STYLE_TABLE_RULES_NONE, eCSSUnit_Enumerated);
        aData->mTableData->mFrame.SetIntValue(NS_STYLE_TABLE_FRAME_NONE, eCSSUnit_Enumerated);
      }
    }
  }

  PRInt32 borderThickness = borderValue.GetIntValue();

  if (aData->mMarginData) {
    // by default, set all border sides to the specified width
    if (aData->mMarginData->mBorderWidth.mLeft.GetUnit() == eCSSUnit_Null)
      aData->mMarginData->mBorderWidth.mLeft.SetFloatValue((float)borderThickness, eCSSUnit_Pixel);
    if (aData->mMarginData->mBorderWidth.mRight.GetUnit() == eCSSUnit_Null)
      aData->mMarginData->mBorderWidth.mRight.SetFloatValue((float)borderThickness, eCSSUnit_Pixel);
    if (aData->mMarginData->mBorderWidth.mTop.GetUnit() == eCSSUnit_Null)
      aData->mMarginData->mBorderWidth.mTop .SetFloatValue((float)borderThickness, eCSSUnit_Pixel);
    if (aData->mMarginData->mBorderWidth.mBottom.GetUnit() == eCSSUnit_Null)
      aData->mMarginData->mBorderWidth.mBottom.SetFloatValue((float)borderThickness, eCSSUnit_Pixel);

    // now account for the frame attribute
    MapTableFrameInto(aAttributes, aData, aBorderStyle);
  }
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  // XXX Bug 211636:  This function is used by a single style rule
  // that's used to match two different type of elements -- tables, and
  // table cells.  (nsHTMLTableCellElement overrides
  // WalkContentStyleRules so that this happens.)  This violates the
  // nsIStyleRule contract, since it's the same style rule object doing
  // the mapping in two different ways.  It's also incorrect since it's
  // testing the display type of the style context rather than checking
  // which *element* it's matching (style rules should not stop matching
  // when the display type is changed).

  nsCompatibility mode = aData->mPresContext->CompatibilityMode();

  if (aData->mSID == eStyleStruct_TableBorder) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL) {
      // cellspacing 
      nsHTMLValue value;
      aAttributes->GetAttribute(nsHTMLAtoms::cellspacing, value);
      if (value.GetUnit() == eHTMLUnit_Integer) {
        if (aData->mTableData->mBorderSpacingX.GetUnit() == eCSSUnit_Null)
          aData->mTableData->mBorderSpacingX.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel);
        if (aData->mTableData->mBorderSpacingY.GetUnit() == eCSSUnit_Null)
          aData->mTableData->mBorderSpacingY.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel);
      }
      else if ((value.GetUnit() == eHTMLUnit_Percent) && (eCompatibility_NavQuirks == mode)) {
        // in quirks mode, treat a % cellspacing value a pixel value.
        if (aData->mTableData->mBorderSpacingX.GetUnit() == eCSSUnit_Null)
          aData->mTableData->mBorderSpacingX.SetFloatValue(100.0f * value.GetPercentValue(), eCSSUnit_Pixel);
        if (aData->mTableData->mBorderSpacingY.GetUnit() == eCSSUnit_Null)
          aData->mTableData->mBorderSpacingY.SetFloatValue(100.0f * value.GetPercentValue(), eCSSUnit_Pixel);
      }
    }
  } 
  else if (aData->mSID == eStyleStruct_Table) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL) {
      MapTableBorderInto(aAttributes, aData, 0);

      nsHTMLValue value;
      // layout
      if (aData->mTableData->mLayout.GetUnit() == eCSSUnit_Null) {
        aAttributes->GetAttribute(nsHTMLAtoms::layout, value);
        if (value.GetUnit() == eHTMLUnit_Enumerated)
          aData->mTableData->mLayout.SetIntValue(value.GetIntValue(), eCSSUnit_Enumerated);
      }
      
      // cols
      if (aAttributes->GetAttribute(nsHTMLAtoms::cols, value) !=
          NS_CONTENT_ATTR_NOT_THERE) {
        if (value.GetUnit() == eHTMLUnit_Integer) 
          aData->mTableData->mCols.SetIntValue(value.GetIntValue(), eCSSUnit_Integer);
        else // COLS had no value, so it refers to all columns
          aData->mTableData->mCols.SetIntValue(NS_STYLE_TABLE_COLS_ALL, eCSSUnit_Enumerated);
      }

      // rules
      aAttributes->GetAttribute(nsHTMLAtoms::rules, value);
      if (value.GetUnit() == eHTMLUnit_Enumerated)
        aData->mTableData->mRules.SetIntValue(value.GetIntValue(), eCSSUnit_Enumerated);
    }
  }
  else if (aData->mSID == eStyleStruct_Margin) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
  
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL) {
      // align; Check for enumerated type (it may be another type if
      // illegal)
      nsHTMLValue value;
      aAttributes->GetAttribute(nsHTMLAtoms::align, value);

      if (value.GetUnit() == eHTMLUnit_Enumerated) {
        if ((NS_STYLE_TEXT_ALIGN_CENTER == value.GetIntValue()) ||
            (NS_STYLE_TEXT_ALIGN_MOZ_CENTER == value.GetIntValue())) {
          nsCSSRect& margin = aData->mMarginData->mMargin;
          if (margin.mLeft.GetUnit() == eCSSUnit_Null)
            margin.mLeft.SetAutoValue();
          if (margin.mRight.GetUnit() == eCSSUnit_Null)
            margin.mRight.SetAutoValue();
        }
      }

      // hspace is mapped into left and right margin, 
      // vspace is mapped into top and bottom margins
      // - *** Quirks Mode only ***
      if (eCompatibility_NavQuirks == mode) {
        aAttributes->GetAttribute(nsHTMLAtoms::hspace, value);

        if (value.GetUnit() == eHTMLUnit_Integer) {
          nsCSSRect& margin = aData->mMarginData->mMargin;
          if (margin.mLeft.GetUnit() == eCSSUnit_Null)
            margin.mLeft.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel); 
          if (margin.mRight.GetUnit() == eCSSUnit_Null)
            margin.mRight.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel);
        }

        aAttributes->GetAttribute(nsHTMLAtoms::vspace, value);

        if (value.GetUnit() == eHTMLUnit_Integer) {
          nsCSSRect& margin = aData->mMarginData->mMargin;
          if (margin.mTop.GetUnit() == eCSSUnit_Null)
            margin.mTop.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel); 
          if (margin.mBottom.GetUnit() == eCSSUnit_Null)
            margin.mBottom.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel); 
        }
      }
    }
  }
  else if (aData->mSID == eStyleStruct_Padding) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
  
    if (readDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL) {
      nsHTMLValue value;
      aAttributes->GetAttribute(nsHTMLAtoms::cellpadding, value);
      if (value.GetUnit() == eHTMLUnit_Integer || value.GetUnit() == eHTMLUnit_Percent) {
        // We have cellpadding.  This will override our padding values if we don't
        // have any set.
        nsCSSValue padVal;
        if (value.GetUnit() == eHTMLUnit_Integer)
          padVal.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel);
        else {
          // when we support % cellpadding in standard mode, uncomment the following
          float pctVal = value.GetPercentValue();
          //if (eCompatibility_NavQuirks == mode) {
            // in quirks mode treat a pct cellpadding value as a pixel value
            padVal.SetFloatValue(100.0f * pctVal, eCSSUnit_Pixel);
          //}
          //else {
          //  padVal.SetPercentValue(pctVal);
          //}
        }
        if (aData->mMarginData->mPadding.mLeft.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mPadding.mLeft = padVal;
        if (aData->mMarginData->mPadding.mRight.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mPadding.mRight = padVal;
        if (aData->mMarginData->mPadding.mTop.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mPadding.mTop = padVal;
        if (aData->mMarginData->mPadding.mBottom.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mPadding.mBottom = padVal;
      }
    }
  }
  else if (aData->mSID == eStyleStruct_Position) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
  
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL) {
      nsHTMLValue value;
      // width: value
      if (aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
        aAttributes->GetAttribute(nsHTMLAtoms::width, value);
        if (value.GetUnit() == eHTMLUnit_Integer) 
          aData->mPositionData->mWidth.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel);
        else if (value.GetUnit() == eHTMLUnit_Percent)
          aData->mPositionData->mWidth.SetPercentValue(value.GetPercentValue());
      }

      // height: value
      if (aData->mPositionData->mHeight.GetUnit() == eCSSUnit_Null) {
        aAttributes->GetAttribute(nsHTMLAtoms::height, value);
        if (value.GetUnit() == eHTMLUnit_Integer) 
          aData->mPositionData->mHeight.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel);
        else if (value.GetUnit() == eHTMLUnit_Percent)
          aData->mPositionData->mHeight.SetPercentValue(value.GetPercentValue()); 
      }
    }
  }
  else if (aData->mSID == eStyleStruct_Visibility) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
  
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL)
      nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
  }
  else if (aData->mSID == eStyleStruct_Border) {
    if (!aData->mStyleContext) return;
    const nsStyleTableBorder* tableStyle = aData->mStyleContext->GetStyleTableBorder();
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
    if (readDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL) {
      if (NS_STYLE_BORDER_SEPARATE == tableStyle->mBorderCollapse) {
        // Set the cell's border from the table in the separate border model. If there is a border
        // on the table, then the mapping to rules=all will take care of borders in the collapsing model.
        nsHTMLValue value;
        if (aAttributes->GetAttribute(nsHTMLAtoms::border, value) !=
            NS_CONTENT_ATTR_NOT_THERE &&
            ((value.GetUnit() == eHTMLUnit_Integer &&
              value.GetIntValue() > 0) ||
             value.IsEmptyString())) {
          if (aData->mMarginData->mBorderWidth.mLeft.GetUnit() == eCSSUnit_Null)
            aData->mMarginData->mBorderWidth.mLeft.SetFloatValue(1.0f, eCSSUnit_Pixel);
          if (aData->mMarginData->mBorderWidth.mRight.GetUnit() == eCSSUnit_Null)
            aData->mMarginData->mBorderWidth.mRight.SetFloatValue(1.0f, eCSSUnit_Pixel);
          if (aData->mMarginData->mBorderWidth.mTop.GetUnit() == eCSSUnit_Null)
            aData->mMarginData->mBorderWidth.mTop.SetFloatValue(1.0f, eCSSUnit_Pixel);
          if (aData->mMarginData->mBorderWidth.mBottom.GetUnit() == eCSSUnit_Null)
            aData->mMarginData->mBorderWidth.mBottom.SetFloatValue(1.0f, eCSSUnit_Pixel);

          PRUint8 borderStyle = (eCompatibility_NavQuirks == mode) 
                                ? NS_STYLE_BORDER_STYLE_BG_INSET : NS_STYLE_BORDER_STYLE_INSET;
          // BG_INSET results in a border color based on background colors
          // used for NavQuirks only...

          if (aData->mMarginData->mBorderStyle.mLeft.GetUnit() == eCSSUnit_Null)
            aData->mMarginData->mBorderStyle.mLeft.SetIntValue(borderStyle, eCSSUnit_Enumerated);
          if (aData->mMarginData->mBorderStyle.mRight.GetUnit() == eCSSUnit_Null)
            aData->mMarginData->mBorderStyle.mRight.SetIntValue(borderStyle, eCSSUnit_Enumerated);
          if (aData->mMarginData->mBorderStyle.mTop.GetUnit() == eCSSUnit_Null)
            aData->mMarginData->mBorderStyle.mTop.SetIntValue(borderStyle, eCSSUnit_Enumerated);
          if (aData->mMarginData->mBorderStyle.mBottom.GetUnit() == eCSSUnit_Null)
            aData->mMarginData->mBorderStyle.mBottom.SetIntValue(borderStyle, eCSSUnit_Enumerated);
        }
      }
    }
    else {
      // default border style is the Nav4.6 extension which uses the
      // background color as the basis of the outset border. If the
      // table has a transparent background then it finds the closest
      // ancestor that has a non-transparent
      // background. NS_STYLE_BORDER_OUTSET uses the border color of
      // the table and if that is not set, then it uses the color.

      PRUint8 borderStyle = (eCompatibility_NavQuirks == mode) 
                            ? NS_STYLE_BORDER_STYLE_BG_OUTSET :
                              NS_STYLE_BORDER_STYLE_OUTSET;
      // bordercolor
      nsHTMLValue value;
      nscolor color;
      if (aAttributes->GetAttribute(nsHTMLAtoms::bordercolor, value) !=
          NS_CONTENT_ATTR_NOT_THERE &&
          value.GetColorValue(color)) {
        if (aData->mMarginData->mBorderColor.mLeft.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderColor.mLeft.SetColorValue(color);
        if (aData->mMarginData->mBorderColor.mRight.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderColor.mRight.SetColorValue(color);
        if (aData->mMarginData->mBorderColor.mTop.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderColor.mTop.SetColorValue(color);
        if (aData->mMarginData->mBorderColor.mBottom.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderColor.mBottom.SetColorValue(color);

        borderStyle = NS_STYLE_BORDER_STYLE_OUTSET; // use css outset
      }
      else if (NS_STYLE_BORDER_COLLAPSE == tableStyle->mBorderCollapse) {
        // make the color grey
        nscolor color = NS_RGB(80, 80, 80);
        if (aData->mMarginData->mBorderColor.mLeft.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderColor.mLeft.SetColorValue(color);
        if (aData->mMarginData->mBorderColor.mRight.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderColor.mRight.SetColorValue(color);
        if (aData->mMarginData->mBorderColor.mTop.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderColor.mTop.SetColorValue(color);
        if (aData->mMarginData->mBorderColor.mBottom.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderColor.mBottom.SetColorValue(color);
      }

      // border and frame
      MapTableBorderInto(aAttributes, aData, borderStyle);
    }
  }
  else if (aData->mSID == eStyleStruct_Background) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
  
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL)
      nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aData);
  }
}

NS_IMETHODIMP_(PRBool)
nsHTMLTableElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsHTMLAtoms::layout },
    { &nsHTMLAtoms::cellpadding },
    { &nsHTMLAtoms::cellspacing },
    { &nsHTMLAtoms::cols },
    { &nsHTMLAtoms::border },
    { &nsHTMLAtoms::frame },
    { &nsHTMLAtoms::width },
    { &nsHTMLAtoms::height },
    { &nsHTMLAtoms::hspace },
    { &nsHTMLAtoms::vspace },
    
    { &nsHTMLAtoms::bordercolor },
    
    { &nsHTMLAtoms::align },
    { &nsHTMLAtoms::rules },
    { nsnull }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
    sBackgroundAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

NS_IMETHODIMP
nsHTMLTableElement::GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const
{
  aMapRuleFunc = &MapAttributesIntoRule;
  return NS_OK;
}
