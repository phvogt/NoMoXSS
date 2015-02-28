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
#include "nsIDOMHTMLTableRowElement.h"
#include "nsIDOMHTMLTableElement.h"
#include "nsIDOMHTMLTableSectionElem.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsDOMError.h"
#include "nsIHTMLContent.h"
#include "nsMappedAttributes.h"
#include "nsGenericHTMLElement.h"
#include "GenericElementCollection.h"
#include "nsHTMLAtoms.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsHTMLParts.h"
#include "nsRuleNode.h"

// temporary
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"

// nsTableCellCollection is needed because GenericElementCollection 
// only supports element of a single tag. This collection supports
// elements <td> or <th> elements.

class nsTableCellCollection : public GenericElementCollection
{
public:
  nsTableCellCollection(nsIContent* aParent, 
                        nsIAtom*    aTag);
  ~nsTableCellCollection();

  NS_IMETHOD GetLength(PRUint32* aLength);
  NS_IMETHOD Item(PRUint32 aIndex, nsIDOMNode** aReturn);
};


nsTableCellCollection::nsTableCellCollection(nsIContent* aParent, 
                                             nsIAtom*    aTag)
  : GenericElementCollection(aParent, aTag)
{
}

nsTableCellCollection::~nsTableCellCollection()
{
}

static PRBool
IsCell(nsIContent *aContent)
{
  nsINodeInfo *ni = aContent->GetNodeInfo();

  return (ni && (ni->Equals(nsHTMLAtoms::td) || ni->Equals(nsHTMLAtoms::th)) &&
          aContent->IsContentOfType(nsIContent::eHTML));
}

NS_IMETHODIMP 
nsTableCellCollection::GetLength(PRUint32* aLength)
{
  if (!aLength) {
    return NS_ERROR_NULL_POINTER;
  }

  *aLength = 0;

  nsresult result = NS_OK;

  if (mParent) {
    nsIContent *child;
    PRUint32 childIndex = 0;

    while ((child = mParent->GetChildAt(childIndex++))) {
      if (IsCell(child)) {
        (*aLength)++;
      }
    }
  }

  return result;
}

NS_IMETHODIMP 
nsTableCellCollection::Item(PRUint32     aIndex, 
                            nsIDOMNode** aReturn)
{
  *aReturn = nsnull;
  PRUint32 theIndex = 0;
  nsresult rv = NS_OK;

  if (mParent) {
    nsIContent *child;
    PRUint32 childIndex = 0;

    while ((child = mParent->GetChildAt(childIndex++))) {
      if (IsCell(child)) {
        if (aIndex == theIndex) {
          CallQueryInterface(child, aReturn);
          NS_ASSERTION(aReturn, "content element must be an nsIDOMNode");

          break;
        }

        theIndex++;
      }
    }
  }

  return rv;
}

//----------------------------------------------------------------------

class nsHTMLTableRowElement : public nsGenericHTMLElement,
                              public nsIDOMHTMLTableRowElement
{
public:
  nsHTMLTableRowElement();
  virtual ~nsHTMLTableRowElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  // nsIDOMHTMLTableRowElement
  NS_DECL_NSIDOMHTMLTABLEROWELEMENT

  virtual PRBool ParseAttribute(nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAString& aResult) const;
  NS_IMETHOD GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

protected:
  nsresult GetSection(nsIDOMHTMLTableSectionElement** aSection);
  nsresult GetTable(nsIDOMHTMLTableElement** aTable);
  nsTableCellCollection* mCells;
};

#ifdef XXX_debugging
static
void DebugList(nsIDOMHTMLTableElement* aTable) {
  nsCOMPtr<nsIHTMLContent> content = do_QueryInterface(aTable);
  if (content) {
    nsCOMPtr<nsIDocument> doc;
    result = content->GetDocument(getter_AddRefs(doc));
    if (doc) {
      nsCOMPtr<nsIContent> root;
      doc->GetRootContent(getter_AddRefs(root));
      if (root) {
        root->List();
      }
      nsIPresShell *shell = doc->GetShellAt(0);
      if (shell) {
        nsIFrame* rootFrame;
        shell->GetRootFrame(rootFrame);
        if (rootFrame) {
          rootFrame->List(stdout, 0);
        }
      }
    }
  }
}
#endif 

nsresult
NS_NewHTMLTableRowElement(nsIHTMLContent** aInstancePtrResult,
                          nsINodeInfo *aNodeInfo, PRBool aFromParser)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLTableRowElement* it = new nsHTMLTableRowElement();

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


nsHTMLTableRowElement::nsHTMLTableRowElement()
{
  mCells = nsnull;
}

nsHTMLTableRowElement::~nsHTMLTableRowElement()
{
  if (nsnull != mCells) {
    mCells->ParentDestroyed();
    NS_RELEASE(mCells);
  }
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTableRowElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableRowElement, nsGenericElement) 


// QueryInterface implementation for nsHTMLTableRowElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLTableRowElement,
                                    nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLTableRowElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLTableRowElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLTableRowElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLTableRowElement* it = new nsHTMLTableRowElement();

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

// protected method
nsresult
nsHTMLTableRowElement::GetSection(nsIDOMHTMLTableSectionElement** aSection)
{
  NS_ENSURE_ARG_POINTER(aSection);
  *aSection = nsnull;

  nsCOMPtr<nsIDOMNode> sectionNode;
  nsresult rv = GetParentNode(getter_AddRefs(sectionNode));
  if (NS_SUCCEEDED(rv) && sectionNode) {
    rv = CallQueryInterface(sectionNode, aSection);
  }

  return rv;
}

// protected method
nsresult
nsHTMLTableRowElement::GetTable(nsIDOMHTMLTableElement** aTable)
{
  NS_ENSURE_ARG_POINTER(aTable);
  *aTable = nsnull;

  nsCOMPtr<nsIDOMNode> sectionNode;
  nsresult rv = GetParentNode(getter_AddRefs(sectionNode));
  if (!sectionNode) {
    return rv;
  }

  nsCOMPtr<nsIDOMNode> tableNode;
  rv = sectionNode->GetParentNode(getter_AddRefs(tableNode));
  if (!tableNode) {
    return rv;
  }
  
  return CallQueryInterface(tableNode, aTable);
}

NS_IMETHODIMP
nsHTMLTableRowElement::GetRowIndex(PRInt32* aValue)
{
  *aValue = -1;
  nsCOMPtr<nsIDOMHTMLTableElement> table;

  nsresult result = GetTable(getter_AddRefs(table));

  if (NS_SUCCEEDED(result) && table) {
    nsCOMPtr<nsIDOMHTMLCollection> rows;

    table->GetRows(getter_AddRefs(rows));

    PRUint32 numRows;
    rows->GetLength(&numRows);

    PRBool found = PR_FALSE;

    for (PRUint32 i = 0; (i < numRows) && !found; i++) {
      nsCOMPtr<nsIDOMNode> node;

      rows->Item(i, getter_AddRefs(node));

      if (node.get() == NS_STATIC_CAST(nsIDOMNode *, this)) {
        *aValue = i;
        found = PR_TRUE;
      }
    }
  }

  return result;
}

NS_IMETHODIMP
nsHTMLTableRowElement::GetSectionRowIndex(PRInt32* aValue)
{
  *aValue = -1;

  nsCOMPtr<nsIDOMHTMLTableSectionElement> section;

  nsresult result = GetSection(getter_AddRefs(section));

  if (NS_SUCCEEDED(result) && section) {
    nsCOMPtr<nsIDOMHTMLCollection> rows;

    section->GetRows(getter_AddRefs(rows));

    PRBool found = PR_FALSE;
    PRUint32 numRows;

    rows->GetLength(&numRows);

    for (PRUint32 i = 0; (i < numRows) && !found; i++) {
      nsCOMPtr<nsIDOMNode> node;
      rows->Item(i, getter_AddRefs(node));

      if (node.get() == NS_STATIC_CAST(nsIDOMNode *, this)) {
        *aValue = i;
        found = PR_TRUE;
      }
    } 
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableRowElement::GetCells(nsIDOMHTMLCollection** aValue)
{
  if (!mCells) {
    mCells = new nsTableCellCollection(this, nsHTMLAtoms::td);

    NS_ENSURE_TRUE(mCells, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(mCells); // this table's reference, released in the destructor
  }

  return CallQueryInterface(mCells, aValue);
}

NS_IMETHODIMP
nsHTMLTableRowElement::InsertCell(PRInt32 aIndex, nsIDOMHTMLElement** aValue)
{
  *aValue = nsnull;

  if (aIndex < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  
  nsCOMPtr<nsIDOMHTMLCollection> cells;
  GetCells(getter_AddRefs(cells));

  PRUint32 cellCount;
  cells->GetLength(&cellCount);

  if (aIndex > PRInt32(cellCount)) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  PRBool doInsert = (aIndex < PRInt32(cellCount)) && (aIndex != -1);

  // create the cell
  nsCOMPtr<nsINodeInfo> nodeInfo;
  mNodeInfo->NameChanged(nsHTMLAtoms::td, getter_AddRefs(nodeInfo));

  nsCOMPtr<nsIHTMLContent> cellContent;
  nsresult rv = NS_NewHTMLTableCellElement(getter_AddRefs(cellContent),
                                           nodeInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> cellNode(do_QueryInterface(cellContent));
  NS_ASSERTION(cellNode, "Should implement nsIDOMNode!");

  nsCOMPtr<nsIDOMNode> retChild;

  if (doInsert) {
    nsCOMPtr<nsIDOMNode> refCell;
    cells->Item(aIndex, getter_AddRefs(refCell));

    rv = InsertBefore(cellNode, refCell, getter_AddRefs(retChild));
  } else {
    rv = AppendChild(cellNode, getter_AddRefs(retChild));
  }

  if (retChild) {
    CallQueryInterface(retChild, aValue);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableRowElement::DeleteCell(PRInt32 aValue)
{
  if (aValue < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHTMLCollection> cells;
  GetCells(getter_AddRefs(cells));

  nsresult rv;
  PRUint32 refIndex;
  if (aValue == -1) {
    rv = cells->GetLength(&refIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    if (refIndex == 0) {
      return NS_OK;
    }

    --refIndex;
  }
  else {
    refIndex = (PRUint32)aValue;
  }

  nsCOMPtr<nsIDOMNode> cell;
  rv = cells->Item(refIndex, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!cell) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMNode> retChild;
  return RemoveChild(cell, getter_AddRefs(retChild));
}

NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableRowElement, Align, align, "left")
NS_IMPL_STRING_ATTR(nsHTMLTableRowElement, BgColor, bgcolor)
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableRowElement, Ch, _char, ".")
NS_IMPL_STRING_ATTR(nsHTMLTableRowElement, ChOff, charoff)
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableRowElement, VAlign, valign, "middle")


PRBool
nsHTMLTableRowElement::ParseAttribute(nsIAtom* aAttribute,
                                      const nsAString& aValue,
                                      nsAttrValue& aResult)
{
  /*
   * ignore these attributes, stored simply as strings
   *
   * ch
   */

  if (aAttribute == nsHTMLAtoms::charoff) {
    return aResult.ParseIntWithBounds(aValue, 0);
  }
  if (aAttribute == nsHTMLAtoms::height) {
    return aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE);
  }
  if (aAttribute == nsHTMLAtoms::width) {
    return aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE);
  }
  if (aAttribute == nsHTMLAtoms::align) {
    return ParseTableCellHAlignValue(aValue, aResult);
  }
  if (aAttribute == nsHTMLAtoms::bgcolor) {
    return aResult.ParseColor(aValue, nsGenericHTMLElement::GetOwnerDocument());
  }
  if (aAttribute == nsHTMLAtoms::valign) {
    return ParseTableVAlignValue(aValue, aResult);
  }

  return nsGenericHTMLElement::ParseAttribute(aAttribute, aValue, aResult);
}

NS_IMETHODIMP
nsHTMLTableRowElement::AttributeToString(nsIAtom* aAttribute,
                                  const nsHTMLValue& aValue,
                                  nsAString& aResult) const
{
  /* ignore these attributes, stored already as strings
     ch
   */
  /* ignore attributes that are of standard types
     charoff, height, width, background, bgcolor
   */
  if (aAttribute == nsHTMLAtoms::align) {
    if (TableCellHAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::valign) {
    if (TableVAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return nsGenericHTMLElement::AttributeToString(aAttribute, aValue, aResult);
}

static 
void MapAttributesIntoRule(const nsMappedAttributes* aAttributes, nsRuleData* aData)
{
  if (aData->mSID == eStyleStruct_Position) {
    // height: value
    nsHTMLValue value;
    if (aData->mPositionData->mHeight.GetUnit() == eCSSUnit_Null) {
      aAttributes->GetAttribute(nsHTMLAtoms::height, value);
      if (value.GetUnit() == eHTMLUnit_Integer)
        aData->mPositionData->mHeight.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel);
      else if (value.GetUnit() == eHTMLUnit_Percent) 
        aData->mPositionData->mHeight.SetPercentValue(value.GetPercentValue());
    }
  }
  else if (aData->mSID == eStyleStruct_Text) {
    if (aData->mTextData->mTextAlign.GetUnit() == eCSSUnit_Null) {
      // align: enum
      nsHTMLValue value;
      aAttributes->GetAttribute(nsHTMLAtoms::align, value);
      if (value.GetUnit() == eHTMLUnit_Enumerated)
        aData->mTextData->mTextAlign.SetIntValue(value.GetIntValue(), eCSSUnit_Enumerated);
    }
  }
  else if (aData->mSID == eStyleStruct_TextReset) {
    if (aData->mTextData->mVerticalAlign.GetUnit() == eCSSUnit_Null) {
      // valign: enum
      nsHTMLValue value;
      aAttributes->GetAttribute(nsHTMLAtoms::valign, value);
      if (value.GetUnit() == eHTMLUnit_Enumerated) 
        aData->mTextData->mVerticalAlign.SetIntValue(value.GetIntValue(), eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLTableRowElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsHTMLAtoms::align },
    { &nsHTMLAtoms::valign }, 
    { &nsHTMLAtoms::height },
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
nsHTMLTableRowElement::GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const
{
  aMapRuleFunc = &MapAttributesIntoRule;
  return NS_OK;
}
