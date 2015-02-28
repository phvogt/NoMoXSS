/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 *    Alex Fritze <alex.fritze@crocodile-clips.com>
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
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "nsSVGElement.h"
#include "nsIDocument.h"
#include "nsRange.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMMutationEvent.h"
#include "nsMutationEvent.h"
#include "nsIBindingManager.h"
#include "nsIXBLBinding.h"
#include "nsStyleConsts.h"
#include "nsDOMError.h"
#include "nsIPresShell.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIServiceManager.h"
#include "nsIXBLService.h"
#include "nsSVGAtoms.h"
#include "nsICSSStyleRule.h"
#include "nsISVGSVGElement.h"
#include "nsRuleWalker.h"
#include "nsSVGStyleValue.h"
#include "nsCSSDeclaration.h"
#include "nsICSSParser.h"

nsSVGElement::nsSVGElement()
{
}

nsSVGElement::~nsSVGElement()
{
  PRUint32 i, count = mMappedAttributes.AttrCount();
  for (i = 0; i < count; ++i) {
    mMappedAttributes.AttrAt(i)->GetSVGValue()->RemoveObserver(this);
  }
}

//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_ADDREF_INHERITED(nsSVGElement,nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsSVGElement,nsGenericElement)

NS_INTERFACE_MAP_BEGIN(nsSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIXMLContent)
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOM3Node, new nsNode3Tearoff(this))
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY(nsISVGContent)
// provided by nsGenericElement:
//  NS_INTERFACE_MAP_ENTRY(nsIStyledContent)
//  NS_INTERFACE_MAP_ENTRY(nsIContent)
NS_INTERFACE_MAP_END_INHERITING(nsGenericElement)

//----------------------------------------------------------------------
// Implementation
  
nsresult
nsSVGElement::Init(nsINodeInfo* aNodeInfo)
{
  nsresult rv = nsGenericElement::Init(aNodeInfo);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Create mapped properties:
  
  // style #IMPLIED
  rv = NS_NewSVGStyleValue(getter_AddRefs(mStyle));
  NS_ENSURE_SUCCESS(rv,rv);
  rv = AddMappedSVGValue(nsSVGAtoms::style, mStyle);
  NS_ENSURE_SUCCESS(rv,rv);
  
  return NS_OK;
}

//----------------------------------------------------------------------
// nsIContent methods

void
nsSVGElement::SetParent(nsIContent* aParent)
{
  nsGenericElement::SetParent(aParent);

  ParentChainChanged();
}

nsIAtom *
nsSVGElement::GetIDAttributeName() const
{
  return nsSVGAtoms::id;
}

nsresult
nsSVGElement::SetAttr(PRInt32 aNamespaceID, nsIAtom* aName, nsIAtom* aPrefix,
                      const nsAString& aValue,
                      PRBool aNotify)
{
  NS_ENSURE_ARG_POINTER(aName);
  NS_ASSERTION(aNamespaceID != kNameSpaceID_Unknown,
               "Don't call SetAttr with unknown namespace");
  
  nsresult rv;
  nsAutoString oldValue;
  PRBool hasListeners = PR_FALSE;
  PRBool modification = PR_FALSE;
  
  PRInt32 index = mAttrsAndChildren.IndexOfAttr(aName, aNamespaceID);
  
  if (mDocument) {
    hasListeners = nsGenericElement::HasMutationListeners(this,
      NS_EVENT_BITS_MUTATION_ATTRMODIFIED);
    
    // If we have no listeners and aNotify is false, we are almost certainly
    // coming from the content sink and will almost certainly have no previous
    // value.  Even if we do, setting the value is cheap when we have no
    // listeners and don't plan to notify.  The check for aNotify here is an
    // optimization, the check for haveListeners is a correctness issue.
    if (index >= 0 && (hasListeners || aNotify)) {
      modification = PR_TRUE;
      // don't do any update if old == new.
      mAttrsAndChildren.AttrAt(index)->ToString(oldValue);
      if (aValue.Equals(oldValue) &&
          aPrefix == mAttrsAndChildren.GetSafeAttrNameAt(index)->GetPrefix()) {
        return NS_OK;
      }
    }
  }
  
  // If this is an svg presentation attribute we need to map it into
  // the content stylerule.
  // XXX For some reason incremental mapping doesn't work, so for now
  // just delete the style rule and lazily reconstruct it in
  // GetContentStyleRule()
  if(aNamespaceID == kNameSpaceID_None && IsAttributeMapped(aName)) 
    mContentStyleRule = nsnull;
  
  // Parse value
  nsAttrValue attrValue;
  nsCOMPtr<nsISVGValue> svg_value;
  if (index >= 0) {
    // Found the attr in the list.
    const nsAttrValue* currVal = mAttrsAndChildren.AttrAt(index);
    if (currVal->Type() == nsAttrValue::eSVGValue) {
      svg_value = currVal->GetSVGValue();
    }
  }
  else {
    // Could be a mapped attribute.
    svg_value = GetMappedAttribute(aNamespaceID, aName);
  }
  
  if (svg_value) {
    if (NS_FAILED(svg_value->SetValueString(aValue))) {
      // The value was rejected. This happens e.g. in a XUL template
      // when trying to set a value like "?x" on a value object that
      // expects a length.
      // To accomodate this "erronous" value, we'll insert a proxy
      // object between ourselves and the actual value object:
      nsCOMPtr<nsISVGValue> proxy;
      rv = NS_CreateSVGStringProxyValue(svg_value, getter_AddRefs(proxy));
      NS_ENSURE_SUCCESS(rv, rv);

      svg_value->RemoveObserver(this);
      proxy->SetValueString(aValue);
      proxy->AddObserver(this);
      attrValue.SetTo(proxy);
    }
    else {
      attrValue.SetTo(svg_value);
    }
  }
  else {
    // We don't have an nsISVGValue attribute.
    attrValue.SetTo(aValue);
  }
  
  return SetAttrAndNotify(aNamespaceID, aName, aPrefix, oldValue, attrValue,
                          modification, hasListeners, aNotify);
}

nsresult
nsSVGElement::SetBindingParent(nsIContent* aParent)
{
  nsresult rv = nsGenericElement::SetBindingParent(aParent);

  // XXX Are parent and bindingparent always in sync? (in which case
  // we don't have to call ParentChainChanged() here)
  ParentChainChanged();
  return rv;
}

//----------------------------------------------------------------------
// nsIStyledContent methods

NS_IMETHODIMP
nsSVGElement::GetID(nsIAtom** aId)const
{
  nsAutoString value;
  
  nsresult rv = NS_CONST_CAST(nsSVGElement*,this)->
                    GetAttribute(NS_LITERAL_STRING("id"), value);
  if (NS_SUCCEEDED(rv))
    *aId = NS_NewAtom(value);
  else
    *aId = nsnull;
  
  return rv;
}

NS_IMETHODIMP
nsSVGElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
#ifdef DEBUG
//  printf("nsSVGElement(%p)::WalkContentStyleRules()\n", this);
#endif
  if (!mContentStyleRule)
    UpdateContentStyleRule();

  if (mContentStyleRule)  
    aRuleWalker->Forward(mContentStyleRule);

  return NS_OK;
}

NS_IMETHODIMP
nsSVGElement::GetInlineStyleRule(nsICSSStyleRule** aStyleRule)
{
  return mStyle->GetStyleRule(this, aStyleRule);
}

// PresentationAttributes-FillStroke
/* static */ const nsGenericElement::MappedAttributeEntry
nsSVGElement::sFillStrokeMap[] = {
  { &nsSVGAtoms::fill },
  { &nsSVGAtoms::fill_opacity },
  { &nsSVGAtoms::fill_rule },
  { &nsSVGAtoms::stroke },
  { &nsSVGAtoms::stroke_dasharray },
  { &nsSVGAtoms::stroke_dashoffset },
  { &nsSVGAtoms::stroke_linecap },
  { &nsSVGAtoms::stroke_linejoin },
  { &nsSVGAtoms::stroke_miterlimit },
  { &nsSVGAtoms::stroke_opacity },
  { &nsSVGAtoms::stroke_width },
  { nsnull }
};

// PresentationAttributes-Graphics
/* static */ const nsGenericElement::MappedAttributeEntry
nsSVGElement::sGraphicsMap[] = {
  { &nsSVGAtoms::clip_path },
  { &nsSVGAtoms::clip_rule },
  { &nsSVGAtoms::cursor },
  { &nsSVGAtoms::display },
  { &nsSVGAtoms::filter },
  { &nsSVGAtoms::image_rendering },
  { &nsSVGAtoms::mask },
  { &nsSVGAtoms::opacity },
  { &nsSVGAtoms::pointer_events },
  { &nsSVGAtoms::shape_rendering },
  { &nsSVGAtoms::text_rendering },
  { &nsSVGAtoms::visibility },
  { nsnull }
};

// PresentationAttributes-TextContentElements
/* static */ const nsGenericElement::MappedAttributeEntry
nsSVGElement::sTextContentElementsMap[] = {
  { &nsSVGAtoms::alignment_baseline },
  { &nsSVGAtoms::baseline_shift },
  { &nsSVGAtoms::direction },
  { &nsSVGAtoms::dominant_baseline },
  { &nsSVGAtoms::glyph_orientation_horizontal },
  { &nsSVGAtoms::glyph_orientation_vertical },
  { &nsSVGAtoms::kerning },
  { &nsSVGAtoms::letter_spacing },
  { &nsSVGAtoms::text_anchor },
  { &nsSVGAtoms::text_decoration },
  { &nsSVGAtoms::unicode_bidi },
  { &nsSVGAtoms::word_spacing },
  { nsnull }
};

// PresentationAttributes-FontSpecification
/* static */ const nsGenericElement::MappedAttributeEntry
nsSVGElement::sFontSpecificationMap[] = {
  { &nsSVGAtoms::font_family },
  { &nsSVGAtoms::font_size },
  { &nsSVGAtoms::font_size_adjust },
  { &nsSVGAtoms::font_stretch },
  { &nsSVGAtoms::font_style },
  { &nsSVGAtoms::font_variant },
  { &nsSVGAtoms::font_weight },  
  { nsnull }
};

//----------------------------------------------------------------------
// nsIDOMNode methods

NS_IMETHODIMP
nsSVGElement::IsSupported(const nsAString& aFeature, const nsAString& aVersion, PRBool* aReturn)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------
// nsIDOMElement methods

// forwarded to nsGenericElement implementations


//----------------------------------------------------------------------
// nsIDOMSVGElement methods

/* attribute DOMString id; */
NS_IMETHODIMP nsSVGElement::GetId(nsAString & aId)
{
  return GetAttr(kNameSpaceID_None, nsSVGAtoms::id, aId);
}

NS_IMETHODIMP nsSVGElement::SetId(const nsAString & aId)
{
  return SetAttr(kNameSpaceID_None, nsSVGAtoms::id, aId, PR_TRUE);
}

/* readonly attribute nsIDOMSVGSVGElement ownerSVGElement; */
NS_IMETHODIMP
nsSVGElement::GetOwnerSVGElement(nsIDOMSVGSVGElement * *aOwnerSVGElement)
{
  *aOwnerSVGElement = nsnull;

  nsIBindingManager *bindingManager = nsnull;
  if (mDocument) {
    bindingManager = mDocument->GetBindingManager();
  }

  nsCOMPtr<nsIContent> parent;
  
  if (bindingManager) {
    // we have a binding manager -- do we have an anonymous parent?
    bindingManager->GetInsertionParent(this, getter_AddRefs(parent));
  }

  if (!parent) {
    // if we didn't find an anonymous parent, use the explicit one,
    // whether it's null or not...
    parent = GetParent();
  }

  while (parent) {    
    nsCOMPtr<nsIDOMSVGSVGElement> SVGSVGElement = do_QueryInterface(parent);
    if (SVGSVGElement) {
      *aOwnerSVGElement = SVGSVGElement;
      NS_ADDREF(*aOwnerSVGElement);
      return NS_OK;
    }
    nsCOMPtr<nsIContent> next;

    if (bindingManager) {
      bindingManager->GetInsertionParent(parent, getter_AddRefs(next));
    }

    if (!next) {
      // no anonymous parent, so use explicit one
      next = parent->GetParent();
    }
    
    parent = next;
  }

  // we don't have a parent SVG element...

  // are _we_ the outermost SVG element? If yes, return nsnull, but don't fail
  nsCOMPtr<nsIDOMSVGSVGElement> SVGSVGElement = do_QueryInterface((nsIDOMSVGElement*)this);
  if (SVGSVGElement) return NS_OK;
  
  // no owner found and we aren't the outermost SVG element either.
  // this situation can e.g. occur during content tree teardown. 
  return NS_ERROR_FAILURE;
}

/* readonly attribute nsIDOMSVGElement viewportElement; */
NS_IMETHODIMP
nsSVGElement::GetViewportElement(nsIDOMSVGElement * *aViewportElement)
{
  *aViewportElement = nsnull;
  nsCOMPtr<nsIDOMSVGSVGElement> SVGSVGElement;
  nsresult rv = GetOwnerSVGElement(getter_AddRefs(SVGSVGElement));
  NS_ENSURE_SUCCESS(rv,rv);
  if (SVGSVGElement) {
    nsCOMPtr<nsIDOMSVGElement> SVGElement = do_QueryInterface(SVGSVGElement);
    *aViewportElement = SVGElement;
    NS_IF_ADDREF(*aViewportElement);
  }
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGValueObserver methods:

NS_IMETHODIMP
nsSVGElement::WillModifySVGObservable(nsISVGValue* observable)
{
  return NS_OK;
}


NS_IMETHODIMP
nsSVGElement::DidModifySVGObservable(nsISVGValue* aObservable)
{
  PRUint32 i, count = mMappedAttributes.AttrCount();
  const nsAttrValue* attrValue = nsnull;
  for (i = 0; i < count; ++i) {
    attrValue = mMappedAttributes.AttrAt(i);
    if (attrValue->GetSVGValue() == aObservable) {
      break;
    }
  }

  if (i == count) {
    NS_NOTREACHED("unknown nsISVGValue");

    return NS_ERROR_UNEXPECTED;
  }

  const nsAttrName* attrName = mMappedAttributes.GetSafeAttrNameAt(i);
  PRBool modification = PR_FALSE;
  PRBool hasListeners = PR_FALSE;
  if (mDocument) {
    modification = !!mAttrsAndChildren.GetAttr(attrName->LocalName(),
                                               attrName->NamespaceID());
    hasListeners = nsGenericElement::HasMutationListeners(this,
      NS_EVENT_BITS_MUTATION_ATTRMODIFIED);
  }

  nsAttrValue newValue(aObservable);

  return SetAttrAndNotify(attrName->NamespaceID(), attrName->LocalName(),
                          attrName->GetPrefix(), EmptyString(), newValue,
                          modification, hasListeners, PR_TRUE);
}

//----------------------------------------------------------------------
// nsISVGContent methods:

// recursive helper to call ParentChainChanged across non-SVG elements
static void CallParentChainChanged(nsIContent*elem)
{
  NS_ASSERTION(elem, "null element");
  
  PRUint32 count = elem->GetChildCount();
  for (PRUint32 i=0; i<count; ++i) {
    nsIContent* child = elem->GetChildAt(i);

    nsCOMPtr<nsISVGContent> svgChild = do_QueryInterface(child);
    if (svgChild) {
      svgChild->ParentChainChanged();
    }
    else {
      // non-svg element might have an svg child, so recurse
      CallParentChainChanged(child);
    }
  }
}

void
nsSVGElement::ParentChainChanged()
{
  CallParentChainChanged(this);
}


//----------------------------------------------------------------------
// Implementation Helpers:

nsresult
nsSVGElement::SetAttrAndNotify(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                               nsIAtom* aPrefix, const nsAString& aOldValue,
                               nsAttrValue& aParsedValue, PRBool aModification,
                               PRBool aFireMutation, PRBool aNotify)
{
  nsresult rv;
  PRUint8 modType = aModification ?
    NS_STATIC_CAST(PRUint8, nsIDOMMutationEvent::MODIFICATION) :
    NS_STATIC_CAST(PRUint8, nsIDOMMutationEvent::ADDITION);

  mozAutoDocUpdate updateBatch(mDocument, UPDATE_CONTENT_MODEL, aNotify);
  if (aNotify && mDocument) {
    mDocument->AttributeWillChange(this, aNamespaceID, aAttribute);
  }

  if (aNamespaceID == kNameSpaceID_None) {
    // XXX doesn't check IsAttributeMapped here.
    rv = mAttrsAndChildren.SetAndTakeAttr(aAttribute, aParsedValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    nsCOMPtr<nsINodeInfo> ni;
    rv = mNodeInfo->NodeInfoManager()->GetNodeInfo(aAttribute, aPrefix,
                                                   aNamespaceID,
                                                   getter_AddRefs(ni));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mAttrsAndChildren.SetAndTakeAttr(ni, aParsedValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDocument) {
    nsCOMPtr<nsIXBLBinding> binding;
    mDocument->GetBindingManager()->GetBinding(this, getter_AddRefs(binding));
    if (binding) {
      binding->AttributeChanged(aAttribute, aNamespaceID, PR_FALSE, aNotify);
    }

    if (aFireMutation) {
      nsCOMPtr<nsIDOMEventTarget> node = do_QueryInterface(NS_STATIC_CAST(nsIContent *, this));
      nsMutationEvent mutation(NS_MUTATION_ATTRMODIFIED, node);

      nsAutoString attrName;
      aAttribute->ToString(attrName);
      nsCOMPtr<nsIDOMAttr> attrNode;
      GetAttributeNode(attrName, getter_AddRefs(attrNode));
      mutation.mRelatedNode = attrNode;

      mutation.mAttrName = aAttribute;
      nsAutoString newValue;
      // We don't really need to call GetAttr here, but lets try to keep this
      // code as similar to nsGenericHTMLElement::SetAttrAndNotify as possible
      // so that they can merge sometime in the future.
      GetAttr(aNamespaceID, aAttribute, newValue);
      if (!newValue.IsEmpty()) {
        mutation.mNewAttrValue = do_GetAtom(newValue);
      }
      if (!aOldValue.IsEmpty()) {
        mutation.mPrevAttrValue = do_GetAtom(aOldValue);
      }
      mutation.mAttrChange = modType;
      nsEventStatus status = nsEventStatus_eIgnore;
      HandleDOMEvent(nsnull, &mutation, nsnull,
                     NS_EVENT_FLAG_INIT, &status);
    }

    if (aNotify) {
      mDocument->AttributeChanged(this, aNamespaceID, aAttribute, modType);
    }
  }
  
  return NS_OK;
}

nsresult
nsSVGElement::CopyNode(nsSVGElement* aDest, PRBool aDeep)
{
  nsresult rv;

  // copy attributes:
  PRUint32 i, count = mAttrsAndChildren.AttrCount();
  for (i = 0; i < count; ++i) {
    const nsAttrName* name = mAttrsAndChildren.GetSafeAttrNameAt(i);
    const nsAttrValue* value = mAttrsAndChildren.AttrAt(i);
    nsAutoString valStr;
    value->ToString(valStr);
    rv = aDest->SetAttr(name->NamespaceID(), name->LocalName(),
                        name->GetPrefix(), valStr, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aDeep) {
    // copy children:
    PRInt32 count = mAttrsAndChildren.ChildCount();
    for (PRInt32 i = 0; i < count; ++i) {
      nsIContent* child = mAttrsAndChildren.ChildAt(i);
      
      nsCOMPtr<nsIDOMNode> domchild = do_QueryInterface(child);
      NS_ASSERTION(domchild != nsnull, "child is not a DOM node");
      if (! domchild)
        return NS_ERROR_UNEXPECTED;
      
      nsCOMPtr<nsIDOMNode> newdomchild;
      rv = domchild->CloneNode(PR_TRUE, getter_AddRefs(newdomchild));
      if (NS_FAILED(rv)) return rv;
      
      nsCOMPtr<nsIContent> newchild = do_QueryInterface(newdomchild);
      NS_ASSERTION(newchild != nsnull, "newdomchild is not an nsIContent");
      if (!newchild)
        return NS_ERROR_UNEXPECTED;
      
      rv = aDest->AppendChildTo(newchild, PR_FALSE, PR_FALSE);
      if (NS_FAILED(rv)) return rv;
    }
  }
  
  return rv;
}

void
nsSVGElement::UpdateContentStyleRule()
{
  NS_ASSERTION(!mContentStyleRule, "we already have a content style rule");

  nsCSSDeclaration* declaration = new nsCSSDeclaration();
  if (!declaration || !declaration->InitializeEmpty()) {
    NS_ERROR("could not initialize nsCSSDeclaration");
    return;
  }
  
  nsCOMPtr<nsIURI> baseURI = GetBaseURI();
  nsCOMPtr<nsICSSParser> parser;
  NS_NewCSSParser(getter_AddRefs(parser));
  if (!parser)
    return;

  PRUint32 attrCount = mAttrsAndChildren.AttrCount();
  for (int i = 0; i< attrCount; ++i) {
    const nsAttrName* attrName = mAttrsAndChildren.GetSafeAttrNameAt(i);
    if (!attrName->IsAtom() || !IsAttributeMapped(attrName->Atom()))
      continue;

    nsAutoString name;
    attrName->Atom()->ToString(name);

    nsAutoString value;
    mAttrsAndChildren.AttrAt(i)->ToString(value);

    PRBool changed;
    parser->ParseProperty(name, value, baseURI, declaration, &changed);
  }
  
  NS_NewCSSStyleRule(getter_AddRefs(mContentStyleRule), nsnull, declaration);
  NS_ASSERTION(mContentStyleRule, "could not create contentstylerule");
}

nsISVGValue*
nsSVGElement::GetMappedAttribute(PRInt32 aNamespaceID, nsIAtom* aName)
{
  const nsAttrValue* attrVal = mMappedAttributes.GetAttr(aName, aNamespaceID);
  if (!attrVal)
    return nsnull;

  return attrVal->GetSVGValue();
}

nsresult
nsSVGElement::AddMappedSVGValue(nsIAtom* aName, nsISupports* aValue,
                                PRInt32 aNamespaceID)
{
  nsresult rv;
  nsCOMPtr<nsISVGValue> svg_value = do_QueryInterface(aValue);
  svg_value->AddObserver(this);
  nsAttrValue attrVal(svg_value);

  if (aNamespaceID == kNameSpaceID_None) {
    rv = mMappedAttributes.SetAndTakeAttr(aName, attrVal);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    nsCOMPtr<nsINodeInfo> ni;
    rv = mNodeInfo->NodeInfoManager()->GetNodeInfo(aName, nsnull,
                                                   aNamespaceID,
                                                   getter_AddRefs(ni));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMappedAttributes.SetAndTakeAttr(ni, attrVal);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
