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
#include "nsIHTMLTableCellElement.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsIDOMHTMLTableRowElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsMappedAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsRuleNode.h"
#include "nsIDocument.h"

class nsHTMLTableCellElement : public nsGenericHTMLElement,
                               public nsIHTMLTableCellElement,
                               public nsIDOMHTMLTableCellElement
{
public:
  nsHTMLTableCellElement();
  virtual ~nsHTMLTableCellElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  // nsIDOMHTMLTableCellElement
  NS_DECL_NSIDOMHTMLTABLECELLELEMENT

  // nsIHTMLTableCellElement
  NS_METHOD GetColIndex (PRInt32* aColIndex);
  NS_METHOD SetColIndex (PRInt32 aColIndex);

  virtual PRBool ParseAttribute(nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAString& aResult) const;
  NS_IMETHOD GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

protected:
  // This does not return a nsresult since all we care about is if we
  // found the row element that this cell is in or not.
  void GetRow(nsIDOMHTMLTableRowElement** aRow);
  nsIContent * GetTable();

  PRInt32 mColIndex;
};

nsresult
NS_NewHTMLTableCellElement(nsIHTMLContent** aInstancePtrResult,
                           nsINodeInfo *aNodeInfo, PRBool aFromParser)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLTableCellElement* it = new nsHTMLTableCellElement();

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


nsHTMLTableCellElement::nsHTMLTableCellElement()
{
  mColIndex=0;
}

nsHTMLTableCellElement::~nsHTMLTableCellElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTableCellElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableCellElement, nsGenericElement) 


// QueryInterface implementation for nsHTMLTableCellElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLTableCellElement, nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLTableCellElement)
  NS_INTERFACE_MAP_ENTRY(nsIHTMLTableCellElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLTableCellElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLTableCellElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLTableCellElement* it = new nsHTMLTableCellElement();

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

/** @return the starting column for this cell in aColIndex.  Always >= 1 */
NS_METHOD nsHTMLTableCellElement::GetColIndex (PRInt32* aColIndex)
{ 
  *aColIndex = mColIndex;
  return NS_OK;
}

/** set the starting column for this cell.  Always >= 1 */
NS_METHOD nsHTMLTableCellElement::SetColIndex (PRInt32 aColIndex)
{ 
  mColIndex = aColIndex;
  return NS_OK;
}

// protected method
void
nsHTMLTableCellElement::GetRow(nsIDOMHTMLTableRowElement** aRow)
{
  *aRow = nsnull;

  nsCOMPtr<nsIDOMNode> rowNode;
  GetParentNode(getter_AddRefs(rowNode));

  if (rowNode) {
    CallQueryInterface(rowNode, aRow);
  }
}

// protected method
nsIContent*
nsHTMLTableCellElement::GetTable()
{
  nsIContent *result = nsnull;

  if (GetParent()) {  // GetParent() should be a row
    nsIContent* section = GetParent()->GetParent();
    if (section) {
      if (section->IsContentOfType(eHTML) &&
          section->GetNodeInfo()->Equals(nsHTMLAtoms::table)) {
        // XHTML, without a row group
        result = section;
      } else {
        // we have a row group.
        result = section->GetParent();
      }
    }
  }
  return result;
}

NS_IMETHODIMP
nsHTMLTableCellElement::GetCellIndex(PRInt32* aCellIndex)
{
  *aCellIndex = -1;

  nsCOMPtr<nsIDOMHTMLTableRowElement> row;

  GetRow(getter_AddRefs(row));

  if (!row) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLCollection> cells;

  row->GetCells(getter_AddRefs(cells));

  if (!cells) {
    return NS_OK;
  }

  PRUint32 numCells;
  cells->GetLength(&numCells);

  PRBool found = PR_FALSE;
  PRUint32 i;

  for (i = 0; (i < numCells) && !found; i++) {
    nsCOMPtr<nsIDOMNode> node;
    cells->Item(i, getter_AddRefs(node));

    if (node.get() == NS_STATIC_CAST(nsIDOMNode *, this)) {
      *aCellIndex = i;
      found = PR_TRUE;
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableCellElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
  nsresult rv = nsGenericHTMLElement::WalkContentStyleRules(aRuleWalker);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add style information from the mapped attributes of the table
  // element.  This depends on the strange behavior of the
  // |MapAttributesIntoRule| in nsHTMLTableElement.cpp, which is
  // technically incorrect since it's violating the nsIStyleRule
  // contract.  However, things are OK (except for the incorrect
  // dependence on display type rather than tag) since tables and cells
  // match different, less specific, rules.
  nsCOMPtr<nsIStyledContent> styledTable = do_QueryInterface(GetTable());
  if (styledTable) {
    rv = styledTable->WalkContentStyleRules(aRuleWalker);
  }

  return rv;
}


NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Abbr, abbr)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Axis, axis)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, BgColor, bgcolor)
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableCellElement, Ch, _char, ".")
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, ChOff, charoff)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLTableCellElement, ColSpan, colspan, 1)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Headers, headers)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Height, height)
NS_IMPL_BOOL_ATTR(nsHTMLTableCellElement, NoWrap, nowrap)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLTableCellElement, RowSpan, rowspan, 1)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Scope, scope)
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableCellElement, VAlign, valign, "middle")
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Width, width)


NS_IMETHODIMP
nsHTMLTableCellElement::GetAlign(nsAString& aValue)
{
  nsresult rv = GetAttr(kNameSpaceID_None, nsHTMLAtoms::align, aValue);

  if (rv == NS_CONTENT_ATTR_NOT_THERE) {
    // There's no align attribute, ask the row for the alignment.

    nsCOMPtr<nsIDOMHTMLTableRowElement> row;
    GetRow(getter_AddRefs(row));

    if (row) {
      return row->GetAlign(aValue);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableCellElement::SetAlign(const nsAString& aValue)
{
  return SetAttr(kNameSpaceID_None, nsHTMLAtoms::align, aValue, PR_TRUE);
}


static const nsHTMLValue::EnumTable kCellScopeTable[] = {
  { "row",      NS_STYLE_CELL_SCOPE_ROW },
  { "col",      NS_STYLE_CELL_SCOPE_COL },
  { "rowgroup", NS_STYLE_CELL_SCOPE_ROWGROUP },
  { "colgroup", NS_STYLE_CELL_SCOPE_COLGROUP },
  { 0 }
};

#define MAX_COLROWSPAN 8190 // celldata.h can not handle more

PRBool
nsHTMLTableCellElement::ParseAttribute(nsIAtom* aAttribute,
                                       const nsAString& aValue,
                                       nsAttrValue& aResult)
{
  /* ignore these attributes, stored simply as strings
     abbr, axis, ch, headers
   */
  if (aAttribute == nsHTMLAtoms::charoff) {
    /* attributes that resolve to integers with a min of 0 */
    return aResult.ParseIntWithBounds(aValue, 0);
  }
  else if ((aAttribute == nsHTMLAtoms::colspan) ||
           (aAttribute == nsHTMLAtoms::rowspan)) {
    PRBool res = aResult.ParseIntWithBounds(aValue, -1, MAX_COLROWSPAN);
    if (res) {
      PRInt32 val = aResult.GetIntegerValue();
      // quirks mode does not honor the special html 4 value of 0
      if (val < 0 || (0 == val && InNavQuirksMode(mDocument))) {
        aResult.SetTo(1, nsAttrValue::eInteger);
      }
    }
    return res;
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
  if (aAttribute == nsHTMLAtoms::scope) {
    return aResult.ParseEnumValue(aValue, kCellScopeTable);
  }
  if (aAttribute == nsHTMLAtoms::valign) {
    return ParseTableVAlignValue(aValue, aResult);
  }

  return nsGenericHTMLElement::ParseAttribute(aAttribute, aValue, aResult);
}

NS_IMETHODIMP
nsHTMLTableCellElement::AttributeToString(nsIAtom* aAttribute,
                                          const nsHTMLValue& aValue,
                                          nsAString& aResult) const
{
  /* ignore these attributes, stored already as strings
     abbr, axis, ch, headers
   */
  /* ignore attributes that are of standard types
     charoff, colspan, rowspan, height, width, nowrap, background, bgcolor
   */
  if (aAttribute == nsHTMLAtoms::align) {
    if (TableCellHAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::scope) {
    if (aValue.EnumValueToString(kCellScopeTable, aResult)) {
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
void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                           nsRuleData* aData)
{
  if (aData->mSID == eStyleStruct_Position) {
    // width: value
    nsHTMLValue value;
    if (aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
      aAttributes->GetAttribute(nsHTMLAtoms::width, value);
      if (value.GetUnit() == eHTMLUnit_Integer) {
        if (value.GetIntValue() > 0)
          aData->mPositionData->mWidth.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel); 
        // else 0 implies auto for compatibility.
      }
      else if (value.GetUnit() == eHTMLUnit_Percent) {
        if (value.GetPercentValue() > 0.0f)
          aData->mPositionData->mWidth.SetPercentValue(value.GetPercentValue());
        // else 0 implies auto for compatibility
      }
    }

    // height: value
    if (aData->mPositionData->mHeight.GetUnit() == eCSSUnit_Null) {
      aAttributes->GetAttribute(nsHTMLAtoms::height, value);
      if (value.GetUnit() == eHTMLUnit_Integer) {
        if (value.GetIntValue() > 0)
          aData->mPositionData->mHeight.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel);
        // else 0 implies auto for compatibility.
      }
      else if (value.GetUnit() == eHTMLUnit_Percent) {
        if (value.GetPercentValue() > 0.0f)
          aData->mPositionData->mHeight.SetPercentValue(value.GetPercentValue());
        // else 0 implies auto for compatibility
      }
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

    if (aData->mTextData->mWhiteSpace.GetUnit() == eCSSUnit_Null) {
      // nowrap: enum
      nsHTMLValue value;
      if (aAttributes->GetAttribute(nsHTMLAtoms::nowrap, value) !=
          NS_CONTENT_ATTR_NOT_THERE) {
        // See if our width is not a integer width.
        nsHTMLValue widthValue;
        aAttributes->GetAttribute(nsHTMLAtoms::width, widthValue);
        if (widthValue.GetUnit() != eHTMLUnit_Integer)
          aData->mTextData->mWhiteSpace.SetIntValue(NS_STYLE_WHITESPACE_NOWRAP, eCSSUnit_Enumerated);
      }
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
nsHTMLTableCellElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsHTMLAtoms::align }, 
    { &nsHTMLAtoms::valign },
    { &nsHTMLAtoms::nowrap },
#if 0
    // XXXldb If these are implemented, they might need to move to
    // GetAttributeChangeHint (depending on how, and preferably not).
    { &nsHTMLAtoms::abbr },
    { &nsHTMLAtoms::axis },
    { &nsHTMLAtoms::headers },
    { &nsHTMLAtoms::scope },
#endif
    { &nsHTMLAtoms::width },
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
nsHTMLTableCellElement::GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const
{
  aMapRuleFunc = &MapAttributesIntoRule;
  return NS_OK;
}
