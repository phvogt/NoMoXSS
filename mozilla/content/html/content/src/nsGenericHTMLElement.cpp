/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set tw=80 expandtab softtabstop=2 ts=2 sw=2: */
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
#include "nscore.h"
#include "nsGenericHTMLElement.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsINodeInfo.h"
#include "nsICSSParser.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleRule.h"
#include "nsCSSStruct.h"
#include "nsCSSDeclaration.h"
#include "nsIDocument.h"
#include "nsIDocumentEncoder.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMAttr.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIEventListenerManager.h"
#include "nsIFocusController.h"
#include "nsMappedAttributes.h"
#include "nsIHTMLStyleSheet.h"
#include "nsIHTMLDocument.h"
#include "nsIHTMLContent.h"
#include "nsILink.h"
#include "nsILinkHandler.h"
#include "nsPIDOMWindow.h"
#include "nsIStyleRule.h"
#include "nsISupportsArray.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsEscape.h"
#include "nsStyleConsts.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIScrollableView.h"
#include "nsIScrollableViewProvider.h"
#include "nsRange.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIDocShell.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsINameSpaceManager.h"
#include "nsDOMError.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"

#include "nsIPresState.h"
#include "nsILayoutHistoryState.h"

#include "nsHTMLParts.h"
#include "nsContentUtils.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsLayoutAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsIEventStateManager.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsDOMCSSDeclaration.h"
#include "nsICSSOMFactory.h"
#include "prprf.h"
#include "prmem.h"
#include "nsIFormControlFrame.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsILanguageAtomService.h"

#include "nsIDOMMutationEvent.h"
#include "nsMutationEvent.h"

#include "nsIBindingManager.h"
#include "nsIXBLBinding.h"

#include "nsRuleWalker.h"

#include "nsIObjectFrame.h"
#include "nsLayoutAtoms.h"
#include "xptinfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIServiceManager.h"

#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsIHTMLContentSink.h"
#include "nsLayoutCID.h"
#include "nsContentCID.h"

#include "nsIDOMText.h"
#include "nsITextContent.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"

#ifdef XSS /* XSS */
#include "nsPrintfCString.h"
#include "prlog.h"
#endif /* XSS */

static NS_DEFINE_CID(kPresStateCID,  NS_PRESSTATE_CID);
// XXX todo: add in missing out-of-memory checks

//----------------------------------------------------------------------

#ifdef GATHER_ELEMENT_USEAGE_STATISTICS

// static objects that have constructors are kinda bad, but we don't
// care here, this is only debugging code!

static nsHashtable sGEUS_ElementCounts;

void GEUS_ElementCreated(nsINodeInfo *aNodeInfo)
{
  nsAutoString name;
  aNodeInfo->GetLocalName(name);

  nsStringKey key(name);

  PRInt32 count = (PRInt32)sGEUS_ElementCounts.Get(&key);

  count++;

  sGEUS_ElementCounts.Put(&key, (void *)count);
}

PRBool GEUS_enum_func(nsHashKey *aKey, void *aData, void *aClosure)
{
  const PRUnichar *name_chars = ((nsStringKey *)aKey)->GetString();
  NS_ConvertUCS2toUTF8 name(name_chars);

  printf ("%s %d\n", name.get(), aData);

  return PR_TRUE;
}

void GEUS_DumpElementCounts()
{
  printf ("Element count statistics:\n");

  sGEUS_ElementCounts.Enumerate(GEUS_enum_func, nsnull);

  printf ("End of element count statistics:\n");
}

nsresult
nsGenericHTMLElement::Init(nsINodeInfo *aNodeInfo)
{
  GEUS_ElementCreated(aNodeInfo);

  return nsGenericElement::Init(aNodeInfo);
}

#endif


class nsGenericHTMLElementTearoff : public nsIDOMNSHTMLElement,
                                    public nsIDOMElementCSSInlineStyle
{
  NS_DECL_ISUPPORTS

  nsGenericHTMLElementTearoff(nsGenericHTMLElement *aElement)
    : mElement(aElement)
  {
    NS_ADDREF(mElement);
  }

  virtual ~nsGenericHTMLElementTearoff()
  {
    NS_RELEASE(mElement);
  }

  NS_FORWARD_NSIDOMNSHTMLELEMENT(mElement->)
  NS_FORWARD_NSIDOMELEMENTCSSINLINESTYLE(mElement->)

private:
  nsGenericHTMLElement *mElement;
};


NS_IMPL_ADDREF(nsGenericHTMLElementTearoff)
NS_IMPL_RELEASE(nsGenericHTMLElementTearoff)

NS_INTERFACE_MAP_BEGIN(nsGenericHTMLElementTearoff)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElementCSSInlineStyle)
NS_INTERFACE_MAP_END_AGGREGATED(mElement)


static nsICSSOMFactory* gCSSOMFactory = nsnull;
static NS_DEFINE_CID(kCSSOMFactoryCID, NS_CSSOMFACTORY_CID);

NS_INTERFACE_MAP_BEGIN(nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIHTMLContent)
NS_INTERFACE_MAP_END_INHERITING(nsGenericElement)
 

nsresult
nsGenericHTMLElement::DOMQueryInterface(nsIDOMHTMLElement *aElement,
                                        REFNSIID aIID, void **aInstancePtr)
{
  nsISupports *inst = nsnull;

  if (aIID.Equals(NS_GET_IID(nsIDOMNode))) {
    inst = NS_STATIC_CAST(nsIDOMNode *, aElement);
  } else if (aIID.Equals(NS_GET_IID(nsIDOMElement))) {
    inst = NS_STATIC_CAST(nsIDOMElement *, aElement);
  } else if (aIID.Equals(NS_GET_IID(nsIDOMHTMLElement))) {
    inst = NS_STATIC_CAST(nsIDOMHTMLElement *, aElement);
  } else if (aIID.Equals(NS_GET_IID(nsIDOMNSHTMLElement))) {
    inst = NS_STATIC_CAST(nsIDOMNSHTMLElement *,
                          new nsGenericHTMLElementTearoff(this));
    NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
  } else if (aIID.Equals(NS_GET_IID(nsIDOMElementCSSInlineStyle))) {
    inst = NS_STATIC_CAST(nsIDOMElementCSSInlineStyle *,
                          new nsGenericHTMLElementTearoff(this));
    NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
  } else {
    return NS_NOINTERFACE;
  }

  NS_ADDREF(inst);

  *aInstancePtr = inst;

  return NS_OK;
}

/* static */ void
nsGenericHTMLElement::Shutdown()
{
  NS_IF_RELEASE(gCSSOMFactory);
}

nsresult
nsGenericHTMLElement::CopyInnerTo(nsGenericElement* aDst, PRBool aDeep)
{
  nsresult rv = NS_OK;
  PRInt32 i, count = GetAttrCount();
  nsCOMPtr<nsIAtom> name, prefix;
  PRInt32 namespace_id;
  nsAutoString value;

  for (i = 0; i < count; ++i) {
    rv = GetAttrNameAt(i, &namespace_id, getter_AddRefs(name),
                       getter_AddRefs(prefix));
    NS_ENSURE_SUCCESS(rv, rv);

    if (name == nsHTMLAtoms::style && namespace_id == kNameSpaceID_None) {
      // We can't just set this as a string, because that will fail
      // to reparse the string into style data until the node is
      // inserted into the document.  Clone the HTMLValue instead.
      const nsAttrValue* styleVal =
        mAttrsAndChildren.GetAttr(nsHTMLAtoms::style);
      if (styleVal && styleVal->Type() == nsAttrValue::eCSSStyleRule) {
        nsCOMPtr<nsICSSRule> ruleClone;
        rv = styleVal->GetCSSStyleRuleValue()->
          Clone(*getter_AddRefs(ruleClone));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsICSSStyleRule> styleRule = do_QueryInterface(ruleClone);
        NS_ENSURE_TRUE(styleRule, NS_ERROR_UNEXPECTED);

        rv = aDst->SetInlineStyleRule(styleRule, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);

        continue;
      }
    }

    rv = GetAttr(namespace_id, name, value);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aDst->SetAttr(namespace_id, name, prefix, value, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsIDocument *doc = mNodeInfo->GetDocument();

  PRInt32 id;
  if (doc) {
    id = doc->GetAndIncrementContentID();
  } else {
    id = PR_INT32_MAX;
  }

  aDst->SetContentID(id);

  if (aDeep) {
    PRInt32 i;
    PRInt32 count = mAttrsAndChildren.ChildCount();
    for (i = 0; i < count; ++i) {
      nsCOMPtr<nsIDOMNode> node = 
          do_QueryInterface(mAttrsAndChildren.ChildAt(i));
      NS_ASSERTION(node, "child doesn't implement nsIDOMNode");

      nsCOMPtr<nsIDOMNode> newNode;
      rv = node->CloneNode(aDeep, getter_AddRefs(newNode));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIContent> newContent(do_QueryInterface(newNode));
      NS_ASSERTION(newContent, "clone doesn't implement nsIContent");
      rv = aDst->AppendChildTo(newContent, PR_FALSE, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetTagName(nsAString& aTagName)
{
  return GetNodeName(aTagName);
}

NS_IMETHODIMP
nsGenericHTMLElement::SetAttribute(const nsAString& aName,
                                   const nsAString& aValue)
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);

  if (!name) {
    nsresult rv = nsContentUtils::CheckQName(aName, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIAtom> nameAtom;
    if (mNodeInfo->NamespaceEquals(kNameSpaceID_None)) {
      nsAutoString lower;
      ToLowerCase(aName, lower);
      nameAtom = do_GetAtom(lower);
    }
    else {
      nameAtom = do_GetAtom(aName);
    }
    NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

    return SetAttr(kNameSpaceID_None, nameAtom, aValue, PR_TRUE);
  }

  return SetAttr(name->NamespaceID(), name->LocalName(), name->GetPrefix(),
                 aValue, PR_TRUE);
}

nsresult
nsGenericHTMLElement::GetNodeName(nsAString& aNodeName)
{
  mNodeInfo->GetQualifiedName(aNodeName);

  if (mNodeInfo->NamespaceEquals(kNameSpaceID_None))
    ToUpperCase(aNodeName);

  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetLocalName(nsAString& aLocalName)
{
  mNodeInfo->GetLocalName(aLocalName);

  if (mNodeInfo->NamespaceEquals(kNameSpaceID_None)) {
    // No namespace, this means we're dealing with a good ol' HTML
    // element, so uppercase the local name.

    ToUpperCase(aLocalName);
  }

  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetElementsByTagName(const nsAString& aTagname,
                                           nsIDOMNodeList** aReturn)
{
  nsAutoString tagName(aTagname);

  // Only lowercase the name if this element has no namespace (i.e.
  // it's a HTML element, not an XHTML element).
  if (mNodeInfo && mNodeInfo->NamespaceEquals(kNameSpaceID_None))
    ToLowerCase(tagName);

  return nsGenericElement::GetElementsByTagName(tagName, aReturn);
}

// Implementation for nsIDOMHTMLElement
nsresult
nsGenericHTMLElement::GetId(nsAString& aId)
{
  GetAttr(kNameSpaceID_None, nsHTMLAtoms::id, aId);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetId(const nsAString& aId)
{
  SetAttr(kNameSpaceID_None, nsHTMLAtoms::id, aId, PR_TRUE);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetTitle(nsAString& aTitle)
{
  GetAttr(kNameSpaceID_None, nsHTMLAtoms::title, aTitle);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetTitle(const nsAString& aTitle)
{
  SetAttr(kNameSpaceID_None, nsHTMLAtoms::title, aTitle, PR_TRUE);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetLang(nsAString& aLang)
{
  GetAttr(kNameSpaceID_None, nsHTMLAtoms::lang, aLang);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetLang(const nsAString& aLang)
{
  SetAttr(kNameSpaceID_None, nsHTMLAtoms::lang, aLang, PR_TRUE);
  return NS_OK;
}

static const nsHTMLValue::EnumTable kDirTable[] = {
  { "ltr", NS_STYLE_DIRECTION_LTR },
  { "rtl", NS_STYLE_DIRECTION_RTL },
  { 0 }
};

nsresult
nsGenericHTMLElement::GetDir(nsAString& aDir)
{
  nsHTMLValue value;
  nsresult result = GetHTMLAttribute(nsHTMLAtoms::dir, value);

  if (NS_CONTENT_ATTR_HAS_VALUE == result) {
    value.EnumValueToString(kDirTable, aDir);
  }

  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetDir(const nsAString& aDir)
{
  SetAttr(kNameSpaceID_None, nsHTMLAtoms::dir, aDir, PR_TRUE);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetClassName(nsAString& aClassName)
{
  GetAttr(kNameSpaceID_None, nsHTMLAtoms::kClass, aClassName);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetClassName(const nsAString& aClassName)
{
  SetAttr(kNameSpaceID_None, nsHTMLAtoms::kClass, aClassName, PR_TRUE);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  nsDOMSlots *slots = GetDOMSlots();

  if (!slots->mStyle) {
    // Just in case...
    ReparseStyleAttribute();

    nsresult rv;
    if (!gCSSOMFactory) {
      rv = CallGetService(kCSSOMFactoryCID, &gCSSOMFactory);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }

    rv = gCSSOMFactory->CreateDOMCSSAttributeDeclaration(this,
                                                         getter_AddRefs(slots->mStyle));
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  // Why bother with QI?
  NS_IF_ADDREF(*aStyle = slots->mStyle);
  return NS_OK;
}

void
nsGenericHTMLElement::RecreateFrames()
{
  if (!mDocument) {
    return;
  }

  PRInt32 numShells = mDocument->GetNumberOfShells();
  for (PRInt32 i = 0; i < numShells; ++i) {
    nsIPresShell *shell = mDocument->GetShellAt(i);
    if (shell) {
      nsIFrame* frame = nsnull;
      shell->GetPrimaryFrameFor(this, &frame);
      if (frame) {
        shell->RecreateFramesFor(this);
      }
    }
  }
}

static PRBool
IsBody(nsIContent *aContent)
{
  nsINodeInfo *ni = aContent->GetNodeInfo();

  return (ni && ni->Equals(nsHTMLAtoms::body) &&
          aContent->IsContentOfType(nsIContent::eHTML));
}

static PRBool
IsOffsetParent(nsIContent *aContent)
{
  nsINodeInfo *ni = aContent->GetNodeInfo();

  return (ni && (ni->Equals(nsHTMLAtoms::td) ||
                 ni->Equals(nsHTMLAtoms::table) ||
                 ni->Equals(nsHTMLAtoms::th)) &&
          aContent->IsContentOfType(nsIContent::eHTML));
}

void
nsGenericHTMLElement::GetOffsetRect(nsRect& aRect, nsIContent** aOffsetParent)
{
  *aOffsetParent = nsnull;

  aRect.x = aRect.y = 0;
  aRect.Empty();

  if (!mDocument) {
    return;
  }

  // Get Presentation shell 0
  nsIPresShell *presShell = mDocument->GetShellAt(0);

  if (!presShell) {
    return;
  }

  // Get the Presentation Context from the Shell
  nsCOMPtr<nsIPresContext> context;
  presShell->GetPresContext(getter_AddRefs(context));

  if (!context) {
    return;
  }

  // Flush all pending notifications so that our frames are uptodate
  mDocument->FlushPendingNotifications();

  // Get the Frame for our content
  nsIFrame* frame = nsnull;
  presShell->GetPrimaryFrameFor(this, &frame);

  if (!frame) {
    return;
  }

  // Get the union of all rectangles in this and continuation frames
  nsRect rcFrame;
  nsIFrame* next = frame;

  do {
    rcFrame.UnionRect(rcFrame, next->GetRect());
    next->GetNextInFlow(&next);
  } while (next);


  nsIContent *docElement = mDocument->GetRootContent();

  // Find the frame parent whose content's tagName either matches
  // the tagName passed in or is the document element.
  nsIFrame* parent = nsnull;
  PRBool done = PR_FALSE;

  nsIContent* content = frame->GetContent();

  if (content) {
    if (IsBody(content) || content == docElement) {
      done = PR_TRUE;

      parent = frame;
    }
  }

  nsPoint origin(0, 0);

  if (!done) {
    PRBool is_absolutely_positioned = PR_FALSE;
    PRBool is_positioned = PR_FALSE;

    origin = frame->GetPosition();

    const nsStyleDisplay* display = frame->GetStyleDisplay();

    if (display->IsPositioned()) {
      if (display->IsAbsolutelyPositioned()) {
        // If the primary frame or a parent is absolutely positioned
        // (fixed or absolute) we stop walking up the frame parent
        // chain

        is_absolutely_positioned = PR_TRUE;
      }

      // We need to know if the primary frame is positioned later on.
      is_positioned = PR_TRUE;
    }

    parent = frame->GetParent();

    while (parent) {
      display = parent->GetStyleDisplay();

      if (display->IsPositioned()) {
        // Stop at the first *parent* that is positioned (fixed,
        // absolute, or relatiive)

        *aOffsetParent = parent->GetContent();
        NS_IF_ADDREF(*aOffsetParent);

        break;
      }

      // Add the parent's origin to our own to get to the
      // right coordinate system

      if (!is_absolutely_positioned) {
        origin += parent->GetPosition();
      }

      content = parent->GetContent();

      if (content) {
        // If we've hit the document element, break here
        if (content == docElement) {
          break;
        }

        // If the tag of this frame is a offset parent tag and this
        // element is *not* positioned, break here. Also break if we
        // hit the body element.
        if ((!is_positioned && IsOffsetParent(content)) || IsBody(content)) {
          *aOffsetParent = content;
          NS_ADDREF(*aOffsetParent);

          break;
        }
      }

      parent = parent->GetParent();
    }

    if (is_absolutely_positioned && !*aOffsetParent) {
      // If this element is absolutely positioned, but we don't have
      // an offset parent it means this element is an absolutely
      // positioned child that's not nested inside another positioned
      // element, in this case the element's frame's parent is the
      // frame for the HTML element so we fail to find the body in the
      // parent chain. We want the offset parent in this case to be
      // the body, so we just get the body element from the document.

      nsCOMPtr<nsIDOMHTMLDocument> html_doc(do_QueryInterface(mDocument));

      if (html_doc) {
        nsCOMPtr<nsIDOMHTMLElement> html_element;

        html_doc->GetBody(getter_AddRefs(html_element));

        if (html_element) {
          CallQueryInterface(html_element, aOffsetParent);
        }
      }
    }
  }

  // For the origin, add in the border for the frame
  nsStyleCoord coord;

#if 0
  // We used to do this to include the border of the frame in the
  // calculations, but I think that's wrong. My tests show that we
  // work more like IE if we don't do this, so lets try this and see
  // if people agree.
  const nsStyleBorder* border = frame->GetStyleBorder();

  if (eStyleUnit_Coord == border->mBorder.GetLeftUnit()) {
    origin.x += border->mBorder.GetLeft(coord).GetCoordValue();
  }
  if (eStyleUnit_Coord == border->mBorder.GetTopUnit()) {
    origin.y += border->mBorder.GetTop(coord).GetCoordValue();
  }
#endif

  // And subtract out the border for the parent
  if (parent) {
    PRBool includeBorder = PR_TRUE;  // set to false if border-box sizing is used
    const nsStylePosition* position = parent->GetStylePosition();
    if (position->mBoxSizing == NS_STYLE_BOX_SIZING_BORDER) {
      includeBorder = PR_FALSE;
    }
    
    if (includeBorder) {
      const nsStyleBorder* border = parent->GetStyleBorder();
      if (eStyleUnit_Coord == border->mBorder.GetLeftUnit()) {
        origin.x -= border->mBorder.GetLeft(coord).GetCoordValue();
      }
      if (eStyleUnit_Coord == border->mBorder.GetTopUnit()) {
        origin.y -= border->mBorder.GetTop(coord).GetCoordValue();
      }
    }
  }

  // XXX We should really consider subtracting out padding for
  // content-box sizing, but we should see what IE does....
  
  // Get the scale from that Presentation Context
  float scale;
  scale = context->TwipsToPixels();

  // Convert to pixels using that scale
  aRect.x = NSTwipsToIntPixels(origin.x, scale);
  aRect.y = NSTwipsToIntPixels(origin.y, scale);
  aRect.width = NSTwipsToIntPixels(rcFrame.width, scale);
  aRect.height = NSTwipsToIntPixels(rcFrame.height, scale);
}

nsresult
nsGenericHTMLElement::GetOffsetTop(PRInt32* aOffsetTop)
{
  nsRect rcFrame;
  nsCOMPtr<nsIContent> parent;
  GetOffsetRect(rcFrame, getter_AddRefs(parent));

  *aOffsetTop = rcFrame.y;

  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetOffsetLeft(PRInt32* aOffsetLeft)
{
  nsRect rcFrame;
  nsCOMPtr<nsIContent> parent;
  GetOffsetRect(rcFrame, getter_AddRefs(parent));

  *aOffsetLeft = rcFrame.x;

  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetOffsetWidth(PRInt32* aOffsetWidth)
{
  nsRect rcFrame;
  nsCOMPtr<nsIContent> parent;
  GetOffsetRect(rcFrame, getter_AddRefs(parent));

  *aOffsetWidth = rcFrame.width;

  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetOffsetHeight(PRInt32* aOffsetHeight)
{
  nsRect rcFrame;
  nsCOMPtr<nsIContent> parent;
  GetOffsetRect(rcFrame, getter_AddRefs(parent));

  *aOffsetHeight = rcFrame.height;

  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetOffsetParent(nsIDOMElement** aOffsetParent)
{
  nsRect rcFrame;
  nsCOMPtr<nsIContent> parent;
  GetOffsetRect(rcFrame, getter_AddRefs(parent));

  if (parent) {
    CallQueryInterface(parent, aOffsetParent);
  } else {
    *aOffsetParent = nsnull;
  }

  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetInnerHTML(nsAString& aInnerHTML)
{
  aInnerHTML.Truncate();

  nsCOMPtr<nsIDocument> doc = GetOwnerDocument();
  if (!doc) {
    return NS_OK; // We rely on the document for doing HTML conversion
  }

  nsCOMPtr<nsIDOMNode> thisNode(do_QueryInterface(NS_STATIC_CAST(nsIContent *,
                                                                 this)));
  nsresult rv = NS_OK;

  nsCOMPtr<nsIDocumentEncoder> docEncoder;
  docEncoder = do_CreateInstance(NS_DOC_ENCODER_CONTRACTID_BASE "text/html");

  NS_ENSURE_TRUE(docEncoder, NS_ERROR_FAILURE);

  docEncoder->Init(doc, NS_LITERAL_STRING("text/html"),
                   nsIDocumentEncoder::OutputEncodeBasicEntities);

  nsCOMPtr<nsIDOMRange> range(new nsRange);
  NS_ENSURE_TRUE(range, NS_ERROR_OUT_OF_MEMORY);

  rv = range->SelectNodeContents(thisNode);
  NS_ENSURE_SUCCESS(rv, rv);

  docEncoder->SetRange(range);

  docEncoder->EncodeToString(aInnerHTML);

  return rv;
}

nsresult
nsGenericHTMLElement::SetInnerHTML(const nsAString& aInnerHTML)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIDOMRange> range = new nsRange;
  NS_ENSURE_TRUE(range, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> thisNode(do_QueryInterface(NS_STATIC_CAST(nsIContent *,
                                                                 this)));
  rv = range->SelectNodeContents(thisNode);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = range->DeleteContents();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocumentFragment> df;

  nsCOMPtr<nsIDocument> doc = GetOwnerDocument();

  nsIScriptContext *scx = nsnull;
  PRBool scripts_enabled = PR_FALSE;

  if (doc) {
    nsIScriptGlobalObject *sgo = doc->GetScriptGlobalObject();

    if (sgo) {
      scx = sgo->GetContext();

      if (scx) {
        scripts_enabled = scx->GetScriptsEnabled();
      }
    }
  }

  if (scripts_enabled) {
    // Don't let scripts execute while setting .innerHTML.

    scx->SetScriptsEnabled(PR_FALSE, PR_FALSE);
  }

  rv = nsrange->CreateContextualFragment(aInnerHTML, getter_AddRefs(df));

  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDOMNode> tmpNode;
    rv = thisNode->AppendChild(df, getter_AddRefs(tmpNode));
  }

  if (scripts_enabled) {
    // If we disabled scripts, re-enable them now that we're
    // done. Don't fire JS timeouts when enabling the context here.

    scx->SetScriptsEnabled(PR_TRUE, PR_FALSE);
  }

  return rv;
}

void
nsGenericHTMLElement::GetScrollInfo(nsIScrollableView **aScrollableView,
                                    float *aP2T, float *aT2P,
                                    nsIFrame **aFrame)
{
  *aScrollableView = nsnull;
  *aP2T = 0.0f;
  *aT2P = 0.0f;

  if (!mDocument) {
    return;
  }

  mDocument->FlushPendingNotifications(PR_TRUE, PR_FALSE);

  // Get the presentation shell
  nsIPresShell *presShell = mDocument->GetShellAt(0);
  if (!presShell) {
    return;
  }

  // Get the presentation context
  nsCOMPtr<nsIPresContext> presContext;
  presShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext) {
    return;
  }

  // Get the primary frame for this element
  nsIFrame *frame = nsnull;
  presShell->GetPrimaryFrameFor(this, &frame);
  if (!frame) {
    return;
  }

  if (aFrame) {
    *aFrame = frame;
  }

  *aP2T = presContext->PixelsToTwips();
  *aT2P = presContext->TwipsToPixels();

  // Get the scrollable frame
  nsIScrollableFrame *scrollFrame = nsnull;
  CallQueryInterface(frame, &scrollFrame);

  if (!scrollFrame) {
    nsIScrollableViewProvider *scrollProvider = nsnull;
    CallQueryInterface(frame, &scrollProvider);
    if (scrollProvider) {
      scrollProvider->GetScrollableView(presContext, aScrollableView);
      if (*aScrollableView) {
        return;
      }
    }

    PRBool quirksMode = InNavQuirksMode(mDocument);
    if ((quirksMode && mNodeInfo->Equals(nsHTMLAtoms::body)) ||
        (!quirksMode && mNodeInfo->Equals(nsHTMLAtoms::html))) {
      // In quirks mode, the scroll info for the body element should map to the
      // scroll info for the nearest scrollable frame above the body element
      // (i.e. the root scrollable frame).  This is what IE6 does in quirks
      // mode.  In strict mode the root scrollable frame corresponds to the
      // html element in IE6, so we map the scroll info for the html element to
      // the root scrollable frame.

      do {
        frame = frame->GetParent();

        if (!frame) {
          break;
        }

        CallQueryInterface(frame, &scrollFrame);
      } while (!scrollFrame);
    }

    if (!scrollFrame) {
      return;
    }
  }

  // Get the scrollable view
  scrollFrame->GetScrollableView(presContext, aScrollableView);

  return;
}


nsresult
nsGenericHTMLElement::GetScrollTop(PRInt32* aScrollTop)
{
  NS_ENSURE_ARG_POINTER(aScrollTop);
  *aScrollTop = 0;

  nsIScrollableView *view = nsnull;
  nsresult rv = NS_OK;
  float p2t, t2p;

  GetScrollInfo(&view, &p2t, &t2p);

  if (view) {
    nscoord xPos, yPos;
    rv = view->GetScrollPosition(xPos, yPos);

    *aScrollTop = NSTwipsToIntPixels(yPos, t2p);
  }

  return rv;
}

nsresult
nsGenericHTMLElement::SetScrollTop(PRInt32 aScrollTop)
{
  nsIScrollableView *view = nsnull;
  nsresult rv = NS_OK;
  float p2t, t2p;

  GetScrollInfo(&view, &p2t, &t2p);

  if (view) {
    nscoord xPos, yPos;

    rv = view->GetScrollPosition(xPos, yPos);

    if (NS_SUCCEEDED(rv)) {
      rv = view->ScrollTo(xPos, NSIntPixelsToTwips(aScrollTop, p2t),
                          NS_VMREFRESH_IMMEDIATE);
    }
  }

  return rv;
}

nsresult
nsGenericHTMLElement::GetScrollLeft(PRInt32* aScrollLeft)
{
  NS_ENSURE_ARG_POINTER(aScrollLeft);
  *aScrollLeft = 0;

  nsIScrollableView *view = nsnull;
  nsresult rv = NS_OK;
  float p2t, t2p;

  GetScrollInfo(&view, &p2t, &t2p);

  if (view) {
    nscoord xPos, yPos;
    rv = view->GetScrollPosition(xPos, yPos);

    *aScrollLeft = NSTwipsToIntPixels(xPos, t2p);
  }

  return rv;
}

nsresult
nsGenericHTMLElement::SetScrollLeft(PRInt32 aScrollLeft)
{
  nsIScrollableView *view = nsnull;
  nsresult rv = NS_OK;
  float p2t, t2p;

  GetScrollInfo(&view, &p2t, &t2p);

  if (view) {
    nscoord xPos, yPos;
    rv = view->GetScrollPosition(xPos, yPos);

    if (NS_SUCCEEDED(rv)) {
      rv = view->ScrollTo(NSIntPixelsToTwips(aScrollLeft, p2t),
                          yPos, NS_VMREFRESH_IMMEDIATE);
    }
  }

  return rv;
}

nsresult
nsGenericHTMLElement::GetScrollHeight(PRInt32* aScrollHeight)
{
  NS_ENSURE_ARG_POINTER(aScrollHeight);
  *aScrollHeight = 0;

  nsIScrollableView *scrollView = nsnull;
  nsresult rv = NS_OK;
  float p2t, t2p;

  GetScrollInfo(&scrollView, &p2t, &t2p);

  if (!scrollView) {
    return GetOffsetHeight(aScrollHeight);
  }

  // xMax and yMax is the total length of our container
  nscoord xMax, yMax;
  rv = scrollView->GetContainerSize(&xMax, &yMax);

  *aScrollHeight = NSTwipsToIntPixels(yMax, t2p);

  return rv;
}

nsresult
nsGenericHTMLElement::GetScrollWidth(PRInt32* aScrollWidth)
{
  NS_ENSURE_ARG_POINTER(aScrollWidth);
  *aScrollWidth = 0;

  nsIScrollableView *scrollView = nsnull;
  nsresult rv = NS_OK;
  float p2t, t2p;

  GetScrollInfo(&scrollView, &p2t, &t2p);

  if (!scrollView) {
    return GetOffsetWidth(aScrollWidth);
  }

  nscoord xMax, yMax;
  rv = scrollView->GetContainerSize(&xMax, &yMax);

  *aScrollWidth = NSTwipsToIntPixels(xMax, t2p);

  return rv;
}

// static
const nsSize
nsGenericHTMLElement::GetClientAreaSize(nsIFrame *aFrame)
{
  nsRect rect = aFrame->GetRect();

  const nsStyleBorder* border = aFrame->GetStyleBorder();

  nsMargin border_size;
  border->CalcBorderFor(aFrame, border_size);

  rect.Deflate(border_size);

  return nsSize(rect.width, rect.height);
}

nsresult
nsGenericHTMLElement::GetClientHeight(PRInt32* aClientHeight)
{
  NS_ENSURE_ARG_POINTER(aClientHeight);
  *aClientHeight = 0;

  nsIScrollableView *scrollView = nsnull;
  float p2t, t2p;
  nsIFrame *frame = nsnull;

  GetScrollInfo(&scrollView, &p2t, &t2p, &frame);

  if (scrollView) {
    const nsIView *view = nsnull;

    scrollView->GetClipView(&view);
    nsRect r = view->GetBounds();

    *aClientHeight = NSTwipsToIntPixels(r.height, t2p);
  } else if (frame &&
             (frame->GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_INLINE ||
              (frame->GetStateBits() & NS_FRAME_REPLACED_ELEMENT))) {
    // Special case code to make clientHeight work even when there isn't
    // a scroll view, see bug 180552 and bug 227567.

    *aClientHeight = NSTwipsToIntPixels(GetClientAreaSize(frame).height, t2p);
  }

  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetClientWidth(PRInt32* aClientWidth)
{
  NS_ENSURE_ARG_POINTER(aClientWidth);
  *aClientWidth = 0;

  nsIScrollableView *scrollView = nsnull;
  float p2t, t2p;
  nsIFrame *frame = nsnull;

  GetScrollInfo(&scrollView, &p2t, &t2p, &frame);

  if (scrollView) {
    const nsIView *view = nsnull;

    scrollView->GetClipView(&view);
    nsRect r = view->GetBounds();

    *aClientWidth = NSTwipsToIntPixels(r.width, t2p);
  } else if (frame &&
             (frame->GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_INLINE ||
              (frame->GetStateBits() & NS_FRAME_REPLACED_ELEMENT))) {
    // Special case code to make clientWidth work even when there isn't
    // a scroll view, see bug 180552 and bug 227567.

    *aClientWidth = NSTwipsToIntPixels(GetClientAreaSize(frame).width, t2p);
  }

  return NS_OK;
}

nsresult
nsGenericHTMLElement::ScrollIntoView(PRBool aTop)
{
  if (!mDocument) {
    return NS_OK;
  }

  // Get the presentation shell
  nsIPresShell *presShell = mDocument->GetShellAt(0);
  if (!presShell) {
    return NS_OK;
  }

  // Now flush to make sure things are up to date
  presShell->FlushPendingNotifications(PR_FALSE);
  
  // Get the primary frame for this element
  nsIFrame *frame = nsnull;
  presShell->GetPrimaryFrameFor(this, &frame);
  if (!frame) {
    return NS_OK;
  }

  PRIntn vpercent = aTop ? NS_PRESSHELL_SCROLL_TOP :
    NS_PRESSHELL_SCROLL_ANYWHERE;

  presShell->ScrollFrameIntoView(frame, vpercent,
                                 NS_PRESSHELL_SCROLL_ANYWHERE);

  return NS_OK;
}


PRBool
nsGenericHTMLElement::InNavQuirksMode(nsIDocument* aDoc)
{
  nsCOMPtr<nsIHTMLDocument> doc(do_QueryInterface(aDoc));
  if (!doc) {
    return PR_FALSE;
  }

  return doc->GetCompatibilityMode() == eCompatibility_NavQuirks;
}

void
nsGenericHTMLElement::SetDocument(nsIDocument* aDocument, PRBool aDeep,
                                  PRBool aCompileEventHandlers)
{
  PRBool doNothing = aDocument == mDocument; // short circuit useless work

  nsGenericElement::SetDocument(aDocument, aDeep, aCompileEventHandlers);

  ReparseStyleAttribute();
  if (!doNothing && mDocument) {
    nsIHTMLStyleSheet* sheet = mDocument->GetAttributeStyleSheet();
    if (sheet) {
      mAttrsAndChildren.SetMappedAttrStyleSheet(sheet);
    }
  }
}

already_AddRefed<nsIDOMHTMLFormElement>
nsGenericHTMLElement::FindForm()
{
  nsIContent* content = this;
  while (content) {
    // If the current ancestor is a form, return it as our form
    if (content->IsContentOfType(nsIContent::eHTML) &&
        content->GetNodeInfo()->Equals(nsHTMLAtoms::form)) {
      nsIDOMHTMLFormElement* form;
      CallQueryInterface(content, &form);

      return form;
    }

    nsIContent *tmp = content;
    content = tmp->GetParent();

    if (content) {
      PRInt32 i = content->IndexOf(tmp);

      if (i < 0) {
        // This means 'tmp' is anonymous content, form controls in
        // anonymous content can't refer to the real form, if they do
        // they end up in form.elements n' such, and that's wrong...

        return NS_OK;
      }
    }
  }

  return NS_OK;
}

static PRBool
IsArea(nsIContent *aContent)
{
  nsINodeInfo *ni = aContent->GetNodeInfo();

  return (ni && ni->Equals(nsHTMLAtoms::area) &&
          aContent->IsContentOfType(nsIContent::eHTML));
}

nsresult
nsGenericHTMLElement::HandleDOMEventForAnchors(nsIPresContext* aPresContext,
                                               nsEvent* aEvent,
                                               nsIDOMEvent** aDOMEvent,
                                               PRUint32 aFlags,
                                               nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  NS_PRECONDITION(nsCOMPtr<nsILink>(do_QueryInterface(this)),
                  "should be called only when |this| implements |nsILink|");

  // Try script event handlers first
  nsresult ret = nsGenericHTMLElement::HandleDOMEvent(aPresContext, aEvent,
                                                      aDOMEvent, aFlags,
                                                      aEventStatus);

  if (!aPresContext) {
    // No pres context when event generated by other parts of DOM code,
    // as in mutation events
    return NS_OK; 
  }

  //Need to check if we hit an imagemap area and if so see if we're handling
  //the event on that map or on a link farther up the tree.  If we're on a
  //link farther up, do nothing.
  if (NS_SUCCEEDED(ret)) {
    nsCOMPtr<nsIContent> target;

    aPresContext->EventStateManager()->
      GetEventTargetContent(aEvent, getter_AddRefs(target));

    if (target && IsArea(target) && !IsArea(this)) {
      // We are over an area and our element is not one.  Return without
      // running anchor code.
      return ret;
    }
  }

  if (NS_FAILED(ret))
    return ret;

  if ((*aEventStatus == nsEventStatus_eIgnore ||
       (*aEventStatus != nsEventStatus_eConsumeNoDefault &&
        (aEvent->message == NS_MOUSE_ENTER_SYNTH ||
         aEvent->message == NS_MOUSE_EXIT_SYNTH))) &&
      !(aFlags & NS_EVENT_FLAG_CAPTURE) && !(aFlags & NS_EVENT_FLAG_SYSTEM_EVENT)) {

    // We'll use the equivalent of |GetHrefUTF8| on the
    // nsILink interface to get a canonified URL that has been
    // correctly escaped and URL-encoded for the document's charset.

    nsCOMPtr<nsIURI> hrefURI;
    GetHrefURIForAnchors(getter_AddRefs(hrefURI));

    // Only bother to handle the mouse event if there was an href
    // specified.
    if (hrefURI) {
      switch (aEvent->message) {
      case NS_MOUSE_LEFT_BUTTON_DOWN:
        {
          // don't make the link grab the focus if there is no link handler
          nsILinkHandler *handler = aPresContext->GetLinkHandler();
          if (handler && mDocument) {
            // If the window is not active, do not allow the focus to bring the
            // window to the front.  We update the focus controller, but do
            // nothing else.
            nsCOMPtr<nsIFocusController> focusController;
            nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(mDocument->GetScriptGlobalObject()));
            win->GetRootFocusController(getter_AddRefs(focusController));
            PRBool isActive = PR_FALSE;
            focusController->GetActive(&isActive);
            if (!isActive) {
              nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(this);
              if(domElement)
                focusController->SetFocusedElement(domElement);
              break;
            }
  
            aPresContext->EventStateManager()->
              SetContentState(this,
                              NS_EVENT_STATE_ACTIVE | NS_EVENT_STATE_FOCUS);

          }
        }
        break;

      case NS_MOUSE_LEFT_CLICK:
      {
        if (nsEventStatus_eConsumeNoDefault != *aEventStatus) {
          nsInputEvent* inputEvent = NS_STATIC_CAST(nsInputEvent*, aEvent);
          nsAutoString target;
          nsCOMPtr<nsIURI> baseURI = GetBaseURI();
          GetAttr(kNameSpaceID_None, nsHTMLAtoms::target, target);
          if (target.IsEmpty()) {
            GetBaseTarget(target);
          }
          if (inputEvent->isControl || inputEvent->isMeta ||
              inputEvent->isAlt ||inputEvent->isShift) {
            break;  // let the click go through so we can handle it in JS/XUL
          }

          ret = TriggerLink(aPresContext, eLinkVerb_Replace, baseURI, hrefURI,
                            target, PR_TRUE, PR_TRUE);

          *aEventStatus = nsEventStatus_eConsumeDoDefault;
        }
      }
      break;

      case NS_KEY_PRESS:
        if (aEvent->eventStructType == NS_KEY_EVENT) {
          nsKeyEvent* keyEvent = NS_STATIC_CAST(nsKeyEvent*, aEvent);
          if (keyEvent->keyCode == NS_VK_RETURN) {
            nsEventStatus status = nsEventStatus_eIgnore;
            nsCOMPtr<nsIContent> mouseContent;

            //fire click
            nsGUIEvent* guiEvent = NS_STATIC_CAST(nsGUIEvent*, aEvent);
            nsMouseEvent event(NS_MOUSE_LEFT_CLICK, guiEvent->widget);
            event.point = aEvent->point;
            event.refPoint = aEvent->refPoint;
            event.clickCount = 1;
            event.isShift = keyEvent->isShift;
            event.isControl = keyEvent->isControl;
            event.isAlt = keyEvent->isAlt;
            event.isMeta = keyEvent->isMeta;

            nsIPresShell *presShell = aPresContext->GetPresShell();
            if (presShell) {
              ret = presShell->HandleDOMEventWithTarget(this, &event, &status);
              // presShell may no longer be alive, don't use it here
              // unless you keep a reference.
            }
          }
        }
        break;

      // Set the status bar the same for focus and mouseover
      case NS_MOUSE_ENTER_SYNTH:
        *aEventStatus = nsEventStatus_eConsumeNoDefault;
      case NS_FOCUS_CONTENT:
      {
        nsAutoString target;
        nsCOMPtr<nsIURI> baseURI = GetBaseURI();
        GetAttr(kNameSpaceID_None, nsHTMLAtoms::target, target);
        if (target.IsEmpty()) {
          GetBaseTarget(target);
        }
        ret = TriggerLink(aPresContext, eLinkVerb_Replace, baseURI,
                          hrefURI, target, PR_FALSE, PR_TRUE);
      }
      break;

      case NS_MOUSE_EXIT_SYNTH:
      {
        *aEventStatus = nsEventStatus_eConsumeNoDefault;
        ret = LeaveLink(aPresContext);
      }
      break;

      default:
        break;
      }
    }
  }
  return ret;
}

nsresult
nsGenericHTMLElement::GetHrefURIForAnchors(nsIURI** aURI)
{
  // This is used by the three nsILink implementations and
  // nsHTMLStyleElement.

  // Get href= attribute (relative URI).
  nsAutoString relURISpec;

  if (NS_CONTENT_ATTR_HAS_VALUE ==
      GetAttr(kNameSpaceID_None, nsHTMLAtoms::href, relURISpec)) {
    // Get base URI.
    nsCOMPtr<nsIURI> baseURI = GetBaseURI();

    // Get absolute URI.
    nsresult rv = nsContentUtils::NewURIWithDocumentCharset(aURI,
                                                            relURISpec,
                                                            mDocument,
                                                            baseURI);
    if (NS_FAILED(rv)) {
      *aURI = nsnull;
    }
  }
  else {
    // Absolute URI is null to say we have no HREF.
    *aURI = nsnull;
  }

  return NS_OK;
}

void
nsGenericHTMLElement::GetNameSpaceID(PRInt32* aID) const
{
  // XXX
  // XXX This is incorrect!!!!!!!!!!!!!!!!
  // XXX
  *aID = kNameSpaceID_XHTML;
}

nsresult
nsGenericHTMLElement::SetAttr(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                              nsIAtom* aPrefix, const nsAString& aValue,
                              PRBool aNotify)
{
  NS_ASSERTION(aNamespaceID != kNameSpaceID_XHTML,
               "Error, attribute on [X]HTML element set with XHTML namespace, "
               "this is wrong, trust me! Lose the prefix on the attribute!");

  nsresult rv;
  nsAutoString oldValue;
  PRBool hasListeners = PR_FALSE;
  PRBool modification = PR_FALSE;

  if (mDocument) {
    hasListeners = nsGenericElement::HasMutationListeners(this,
      NS_EVENT_BITS_MUTATION_ATTRMODIFIED);

    // If we have no listeners and aNotify is false, we are almost certainly
    // coming from the content sink and will almost certainly have no previous
    // value.  Even if we do, setting the value is cheap when we have no
    // listeners and don't plan to notify.  The check for aNotify here is an
    // optimization, the check for haveListeners is a correctness issue.
    if (hasListeners || aNotify) {
      // don't do any update if old == new.
      // It would be nice to not have to call GetAttr here but to rather just
      // grab the nsAttrValue from mAttrsAndChildren and compare that to
      // aValue. However not all nsAttrValues can currently be converted to
      // strings (specifically enums and nsISupports can't) so we have to take
      // the detour through GetAttr for now.
      rv = GetAttr(aNamespaceID, aAttribute, oldValue);
      modification = rv != NS_CONTENT_ATTR_NOT_THERE;
      if (modification && aValue.Equals(oldValue)) {
        return NS_OK;
      }
    }
  }

#ifdef XSS /* XSS */
  // first added for anchor.href
  if (aValue.xssGetTainted()) {
	  {
		  nsCString xss_doc_uri;
          nsCOMPtr<nsIURI> baseURI = GetBaseURI();
		  if (baseURI) {
			  baseURI->GetSpec(xss_doc_uri);
		  }	  
		  XSS_LOG("xsstaintstring nsGenericHTMLElement::SetAttr: %s\n",
			  ToNewCString(
			  NS_LITERAL_STRING("'") +
			  NS_ConvertUTF8toUTF16(nsPrintfCString("%d", aNamespaceID)) + 
			  NS_LITERAL_STRING("' '") + 
			  aValue + 
			  NS_LITERAL_STRING("' ") + 
			  NS_ConvertUTF8toUTF16(xss_doc_uri)));
	  } while (0);
	  xssSetTainted(aValue.xssGetTainted());
  }
#endif /* XSS */

  // Parse into a nsAttrValue
  nsAttrValue attrValue;
  if (aNamespaceID == kNameSpaceID_None) {
    if (!ParseAttribute(aAttribute, aValue, attrValue)) {
      attrValue.SetTo(aValue);
    }

    if (IsEventName(aAttribute)) {
      AddScriptEventListener(aAttribute, aValue);
    }
  }
  else {
    attrValue.SetTo(aValue);
  }

  return SetAttrAndNotify(aNamespaceID, aAttribute, aPrefix, oldValue,
                          attrValue, modification, hasListeners, aNotify);
}

nsresult
nsGenericHTMLElement::SetAttrAndNotify(PRInt32 aNamespaceID,
                                       nsIAtom* aAttribute,
                                       nsIAtom* aPrefix,
                                       const nsAString& aOldValue,
                                       nsAttrValue& aParsedValue,
                                       PRBool aModification,
                                       PRBool aFireMutation,
                                       PRBool aNotify)
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
    if (IsAttributeMapped(aAttribute)) {
      nsIHTMLStyleSheet* sheet = mDocument ?
        mDocument->GetAttributeStyleSheet() : nsnull;
      rv = mAttrsAndChildren.SetAndTakeMappedAttr(aAttribute, aParsedValue,
                                                  this, sheet);
    }
    else {
      rv = mAttrsAndChildren.SetAndTakeAttr(aAttribute, aParsedValue);
    }
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
      nsCOMPtr<nsIDOMEventTarget> node =
        do_QueryInterface(NS_STATIC_CAST(nsIContent *, this));
      nsMutationEvent mutation(NS_MUTATION_ATTRMODIFIED, node);

      nsAutoString attrName;
      aAttribute->ToString(attrName);
      nsCOMPtr<nsIDOMAttr> attrNode;
      GetAttributeNode(attrName, getter_AddRefs(attrNode));
      mutation.mRelatedNode = attrNode;

      mutation.mAttrName = aAttribute;
      nsAutoString newValue;
      // It sucks that we have to call GetAttr just to get the stringvalue
      // here, but in SetInlineStyleRule we don't have the stringvalue
      // directly and it would be a waste of time to get it since in the
      // common case it's not needed. If we make nsHTMLValue->string
      // conversion possible we wouldn't have to do this.
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

PRBool nsGenericHTMLElement::IsEventName(nsIAtom* aName)
{
  const char* name;
  aName->GetUTF8String(&name);

  if (name[0] != 'o' || name[1] != 'n') {
    return PR_FALSE;
  }

  return (aName == nsLayoutAtoms::onclick                       ||
          aName == nsLayoutAtoms::ondblclick                    ||
          aName == nsLayoutAtoms::onmousedown                   ||
          aName == nsLayoutAtoms::onmouseup                     ||
          aName == nsLayoutAtoms::onmouseover                   ||
          aName == nsLayoutAtoms::onmouseout                    ||
          aName == nsLayoutAtoms::onkeydown                     ||
          aName == nsLayoutAtoms::onkeyup                       ||
          aName == nsLayoutAtoms::onkeypress                    ||
          aName == nsLayoutAtoms::onmousemove                   ||
          aName == nsLayoutAtoms::onload                        ||
          aName == nsLayoutAtoms::onunload                      ||
          aName == nsLayoutAtoms::onbeforeunload                ||
          aName == nsLayoutAtoms::onabort                       ||
          aName == nsLayoutAtoms::onerror                       ||
          aName == nsLayoutAtoms::onfocus                       ||
          aName == nsLayoutAtoms::onblur                        ||
          aName == nsLayoutAtoms::onsubmit                      ||
          aName == nsLayoutAtoms::onreset                       ||
          aName == nsLayoutAtoms::onchange                      ||
          aName == nsLayoutAtoms::onselect                      || 
          aName == nsLayoutAtoms::onpaint                       ||
          aName == nsLayoutAtoms::onresize                      ||
          aName == nsLayoutAtoms::onscroll                      ||
          aName == nsLayoutAtoms::oninput                       ||
          aName == nsLayoutAtoms::oncontextmenu                 ||
          aName == nsLayoutAtoms::onDOMAttrModified             ||
          aName == nsLayoutAtoms::onDOMCharacterDataModified    || 
          aName == nsLayoutAtoms::onDOMSubtreeModified          ||
          aName == nsLayoutAtoms::onDOMNodeInsertedIntoDocument || 
          aName == nsLayoutAtoms::onDOMNodeRemovedFromDocument  ||
          aName == nsLayoutAtoms::onDOMNodeInserted             || 
          aName == nsLayoutAtoms::onDOMNodeRemoved);
}

nsresult
nsGenericHTMLElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                PRBool aNotify)
{
  // Check for event handlers
  if (aNameSpaceID == kNameSpaceID_None &&
      IsEventName(aAttribute)) {
    nsCOMPtr<nsIEventListenerManager> manager;
    GetListenerManager(getter_AddRefs(manager));

    if (manager) {
      manager->RemoveScriptEventListener(aAttribute);
    }
  }

  return nsGenericElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

nsresult
nsGenericHTMLElement::GetAttr(PRInt32 aNameSpaceID, nsIAtom *aAttribute,
                              nsAString& aResult) const
{
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown,
               "must have a real namespace ID!");

  aResult.Truncate();

  const nsAttrValue* attrValue =
    mAttrsAndChildren.GetAttr(aAttribute, aNameSpaceID);
  if (!attrValue) {
    return NS_CONTENT_ATTR_NOT_THERE;
  }

  // Use subclass to convert enums to string.
  if (attrValue->Type() == nsAttrValue::eEnum) {
    nsHTMLValue htmlVal(attrValue->GetEnumValue(), eHTMLUnit_Enumerated);
    if (aNameSpaceID != kNameSpaceID_None ||
        AttributeToString(aAttribute, htmlVal, aResult) !=
        NS_CONTENT_ATTR_HAS_VALUE) {
      NS_NOTREACHED("no enum to string conversion found");

      return NS_CONTENT_ATTR_NOT_THERE;
    }
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  attrValue->ToString(aResult);

#ifdef XSS /* XSS */
  // first added for anchor.href
  if (xss_istainted != XSS_NOT_TAINTED) {
	  {
		  nsCString xss_doc_uri;
          nsCOMPtr<nsIURI> baseURI = GetBaseURI();
		  if (baseURI) {
			  baseURI->GetSpec(xss_doc_uri);
		  }	  
		  XSS_LOG("xsstaintstring nsGenericHTMLElement::GetAttr: %s\n",
			  ToNewCString(
			  NS_LITERAL_STRING("'") +
			  NS_ConvertUTF8toUTF16(nsPrintfCString("%d", aNameSpaceID)) + 
			  NS_LITERAL_STRING("' '") + 
			  aResult + 
			  NS_LITERAL_STRING("' ") + 
			  NS_ConvertUTF8toUTF16(xss_doc_uri)));
	  } while (0);
	  aResult.xssSetTainted(XSS_TAINTED);
  }
#endif /* XSS */

  return NS_CONTENT_ATTR_HAS_VALUE;
}

nsresult
nsGenericHTMLElement::GetHTMLAttribute(nsIAtom* aAttribute,
                                       nsHTMLValue& aValue) const
{
  const nsAttrValue* val = mAttrsAndChildren.GetAttr(aAttribute);

  if (!val) {
    aValue.Reset();
    return NS_CONTENT_ATTR_NOT_THERE;
  }

  val->ToHTMLValue(aValue);

  return NS_CONTENT_ATTR_HAS_VALUE;
}

nsresult
nsGenericHTMLElement::GetID(nsIAtom** aResult) const
{
  *aResult = nsnull;

  const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(nsHTMLAtoms::id);
  NS_ASSERTION(!attrVal ||
               attrVal->Type() == nsAttrValue::eAtom ||
               (attrVal->Type() == nsAttrValue::eString &&
                attrVal->GetStringValue().IsEmpty()),
               "unexpected attribute type");
  if (attrVal && attrVal->Type() == nsAttrValue::eAtom) {
    NS_ADDREF(*aResult = attrVal->GetAtomValue());
  }

  return NS_OK;
}

const nsAttrValue*
nsGenericHTMLElement::GetClasses() const
{
  return mAttrsAndChildren.GetAttr(nsHTMLAtoms::kClass);
}

nsIAtom *
nsGenericHTMLElement::GetIDAttributeName() const
{
  return nsHTMLAtoms::id;
}

nsIAtom *
nsGenericHTMLElement::GetClassAttributeName() const
{
  return nsHTMLAtoms::kClass;
}

NS_IMETHODIMP_(PRBool)
nsGenericHTMLElement::HasClass(nsIAtom* aClass, PRBool aCaseSensitive) const
{
  const nsAttrValue* val = mAttrsAndChildren.GetAttr(nsHTMLAtoms::kClass);
  if (val) {
    if (val->Type() == nsAttrValue::eAtom) {
      if (aCaseSensitive) {
        return aClass == val->GetAtomValue();
      }

      const char *class1, *class2;
      aClass->GetUTF8String(&class1);
      val->GetAtomValue()->GetUTF8String(&class2);

      return nsCRT::strcasecmp(class1, class2) == 0;
    }
    if (val->Type() == nsAttrValue::eAtomArray) {
      nsCOMArray<nsIAtom>* array = val->GetAtomArrayValue();
      if (aCaseSensitive) {
        return array->IndexOf(aClass) >= 0;
      }

      const char *class1, *class2;
      aClass->GetUTF8String(&class1);

      PRInt32 i, count = array->Count();
      for (i = 0; i < count; ++i) {
        array->ObjectAt(i)->GetUTF8String(&class2);
        if (nsCRT::strcasecmp(class1, class2) == 0) {
          return PR_TRUE;
        }
      }
    }
  }

  return PR_FALSE;
}

nsresult
nsGenericHTMLElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
  mAttrsAndChildren.WalkMappedAttributeStyleRules(aRuleWalker);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetInlineStyleRule(nsICSSStyleRule** aStyleRule)
{
  *aStyleRule = nsnull;
  
  const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(nsHTMLAtoms::style);
  
  if (attrVal) {
    if (attrVal->Type() != nsAttrValue::eCSSStyleRule) {
      ReparseStyleAttribute();
      attrVal = mAttrsAndChildren.GetAttr(nsHTMLAtoms::style);
      // hopefully value.GetUnit() is now eHTMLUnit_CSSStyleRule
    }

    if (attrVal->Type() == nsAttrValue::eCSSStyleRule) {
      NS_ADDREF(*aStyleRule = attrVal->GetCSSStyleRuleValue());
    }
  }

  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetInlineStyleRule(nsICSSStyleRule* aStyleRule,
                                         PRBool aNotify)
{
  PRBool hasListeners = PR_FALSE;
  PRBool modification = PR_FALSE;
  nsAutoString oldValueStr;

  if (mDocument) {
    hasListeners = nsGenericElement::HasMutationListeners(this,
      NS_EVENT_BITS_MUTATION_ATTRMODIFIED);

    // There's no point in comparing the stylerule pointers since we're always
    // getting a new stylerule here. And we can't compare the stringvalues of
    // the old and the new rules since both will point to the same declaration
    // and thus will be the same.
    if (hasListeners) {
      // save the old attribute so we can set up the mutation event properly
      modification = GetAttr(kNameSpaceID_None, nsHTMLAtoms::style,
                             oldValueStr) != NS_CONTENT_ATTR_NOT_THERE;
    }
    else if (aNotify) {
      modification = !!mAttrsAndChildren.GetAttr(nsHTMLAtoms::style);
    }
  }

  nsAttrValue attrValue(aStyleRule);

  return SetAttrAndNotify(kNameSpaceID_None, nsHTMLAtoms::style, nsnull, oldValueStr,
                          attrValue, modification, hasListeners, aNotify);
}

already_AddRefed<nsIURI>
nsGenericHTMLElement::GetBaseURI() const
{
  nsIDocument* doc = GetOwnerDocument();

  const nsAttrValue* val = mAttrsAndChildren.GetAttr(nsHTMLAtoms::_baseHref);
  if (val) {
    // We have a _baseHref attribute; that will determine our base URI
    nsAutoString str;
    val->ToString(str);

    nsIURI* docBaseURL = nsnull;
    if (doc) {
      docBaseURL = doc->GetBaseURI();
    }

    nsIURI* uri = nsnull;
    NS_NewURI(&uri, str, nsnull, docBaseURL);

    return uri;
  }

  // If we are a plain old HTML element (not XHTML), don't bother asking the
  // base class -- our base URI is determined solely by the document base.
  if (mNodeInfo->NamespaceEquals(kNameSpaceID_None)) {
    if (doc) {
      nsIURI *uri = doc->GetBaseURI();
      NS_IF_ADDREF(uri);

      return uri;
    }

    return nsnull;
  }

  return nsGenericElement::GetBaseURI();
}

void
nsGenericHTMLElement::GetBaseTarget(nsAString& aBaseTarget) const
{
  const nsAttrValue* val = mAttrsAndChildren.GetAttr(nsHTMLAtoms::_baseTarget);
  if (val) {
    val->ToString(aBaseTarget);
    return;
  }

  if (mDocument) {
    mDocument->GetBaseTarget(aBaseTarget);
  }
  else {
    aBaseTarget.Truncate();
  }
}

#ifdef DEBUG
void
nsGenericHTMLElement::ListAttributes(FILE* out) const
{
  PRUint32 index, count = GetAttrCount();

  for (index = 0; index < count; index++) {
    // name
    nsCOMPtr<nsIAtom> attr;
    nsCOMPtr<nsIAtom> prefix;
    PRInt32 nameSpaceID;
    GetAttrNameAt(index, &nameSpaceID, getter_AddRefs(attr),
                  getter_AddRefs(prefix));

    nsAutoString buffer;
    attr->ToString(buffer);

    // value
    nsAutoString value;
    GetAttr(nameSpaceID, attr, value);
    buffer.Append(NS_LITERAL_STRING("=\""));
    for (int i = value.Length(); i >= 0; --i) {
      if (value[i] == PRUnichar('"'))
        value.Insert(PRUnichar('\\'), PRUint32(i));
    }
    buffer.Append(value);
    buffer.Append(NS_LITERAL_STRING("\""));

    fputs(" ", out);
    fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
  }
}

void
nsGenericHTMLElement::List(FILE* out, PRInt32 aIndent) const
{
  NS_PRECONDITION(nsnull != mDocument, "bad content");

  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buf;
  mNodeInfo->GetQualifiedName(buf);
  fputs(NS_LossyConvertUCS2toASCII(buf).get(), out);

  fprintf(out, "@%p", (void*)this);

  ListAttributes(out);

  fprintf(out, " refcount=%d<", mRefCnt.get());

  fputs("\n", out);
  PRInt32 kids = GetChildCount();
  for (index = 0; index < kids; index++) {
    nsIContent *kid = GetChildAt(index);
    kid->List(out, aIndent + 1);
  }
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs(">\n", out);
}

void
nsGenericHTMLElement::DumpContent(FILE* out, PRInt32 aIndent,
                                  PRBool aDumpAll) const
{
   NS_PRECONDITION(nsnull != mDocument, "bad content");

  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buf;
  mNodeInfo->GetQualifiedName(buf);
  fputs("<",out);
  fputs(NS_LossyConvertUCS2toASCII(buf).get(), out);

  if(aDumpAll) ListAttributes(out);

  fputs(">",out);

  if(aIndent) fputs("\n", out);
  PRInt32 kids = GetChildCount();
  for (index = 0; index < kids; index++) {
    nsIContent *kid = GetChildAt(index);
    PRInt32 indent = aIndent ? aIndent + 1 : 0;
    kid->DumpContent(out, indent, aDumpAll);
  }
  for (index = aIndent; --index >= 0; ) fputs("  ", out);
  fputs("</",out);
  fputs(NS_LossyConvertUCS2toASCII(buf).get(), out);
  fputs(">",out);

  if(aIndent) fputs("\n", out);
}
#endif


PRBool
nsGenericHTMLElement::IsContentOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eELEMENT | eHTML));
}

//----------------------------------------------------------------------


nsresult
nsGenericHTMLElement::AttributeToString(nsIAtom* aAttribute,
                                        const nsHTMLValue& aValue,
                                        nsAString& aResult) const
{
  if (nsHTMLAtoms::dir == aAttribute) {
    if (aValue.GetUnit() == eHTMLUnit_Enumerated) {
      aValue.EnumValueToString(kDirTable, aResult);

      return NS_OK;
    }
  }
  aResult.Truncate();
  return NS_CONTENT_ATTR_NOT_THERE;
}

PRBool
nsGenericHTMLElement::ParseAttribute(nsIAtom* aAttribute,
                                     const nsAString& aValue,
                                     nsAttrValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::dir) {
    return aResult.ParseEnumValue(aValue, kDirTable);
  }
  if (aAttribute == nsHTMLAtoms::style) {
    ParseStyleAttribute(this, mNodeInfo->NamespaceEquals(kNameSpaceID_XHTML),
                        aValue, aResult);
    return PR_TRUE;
  }
  if (aAttribute == nsHTMLAtoms::id && !aValue.IsEmpty()) {
    aResult.ParseAtom(aValue);

    return PR_TRUE;
  }
  if (aAttribute == nsHTMLAtoms::kClass) {
    aResult.ParseAtomArray(aValue);

    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sCommonAttributeMap
  };
  
  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

/**
 * Handle attributes on the BDO element
 */
static void
MapBdoAttributesInto(const nsMappedAttributes* aAttributes,
                     nsRuleData* aData)
{
  if (aData->mSID == eStyleStruct_TextReset &&
      aData->mTextData->mUnicodeBidi.GetUnit() == eCSSUnit_Null) {
    aData->mTextData->mUnicodeBidi.SetIntValue(NS_STYLE_UNICODE_BIDI_OVERRIDE, eCSSUnit_Enumerated);
  }
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP
nsGenericHTMLElement::GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const
{
  if (mNodeInfo->Equals(nsHTMLAtoms::bdo))
    aMapRuleFunc = &MapBdoAttributesInto;
  else
    aMapRuleFunc = &MapCommonAttributesInto;
  return NS_OK;
}

// static
nsIFrame *
nsGenericHTMLElement::GetPrimaryFrameFor(nsIContent* aContent,
                                         nsIDocument* aDocument,
                                         PRBool aFlushContent)
{
  if (aFlushContent) {
    // Cause a flush of content, so we get up-to-date frame
    // information
    aDocument->FlushPendingNotifications(PR_FALSE);
  }

  // Get presentation shell 0
  nsIPresShell *presShell = aDocument->GetShellAt(0);

  nsIFrame *frame = nsnull;

  if (presShell) {
    presShell->GetPrimaryFrameFor(aContent, &frame);
  }

  return frame;
}

// static
nsIFormControlFrame*
nsGenericHTMLElement::GetFormControlFrameFor(nsIContent* aContent,
                                             nsIDocument* aDocument,
                                             PRBool aFlushContent)
{
  nsIFrame* frame = GetPrimaryFrameFor(aContent, aDocument, aFlushContent);
  if (frame) {
    nsIFormControlFrame* form_frame = nsnull;
    CallQueryInterface(frame, &form_frame);
    if (form_frame) {
      return form_frame;
    }

    // If we have generated content, the primary frame will be a
    // wrapper frame..  out real frame will be in its child list.
    for (frame = frame->GetFirstChild(nsnull);
         frame;
         frame = frame->GetNextSibling()) {
      CallQueryInterface(frame, &form_frame);
      if (form_frame) {
        return form_frame;
      }
    }
        
    NS_ERROR("Form control has a frame, but it's not a form frame");
  }

  return nsnull;
}

nsresult
nsGenericHTMLElement::GetPrimaryPresState(nsIHTMLContent* aContent,
                                          nsIPresState** aPresState)
{
  NS_ENSURE_ARG_POINTER(aPresState);
  *aPresState = nsnull;

  nsresult result = NS_OK;

  nsCOMPtr<nsILayoutHistoryState> history;
  nsCAutoString key;
  GetLayoutHistoryAndKey(aContent, getter_AddRefs(history), key);

  if (history) {
    // Get the pres state for this key, if it doesn't exist, create one
    result = history->GetState(key, aPresState);
    if (!*aPresState) {
      result = CallCreateInstance(kPresStateCID, aPresState);
      if (NS_SUCCEEDED(result)) {
        result = history->AddState(key, *aPresState);
      }
    }
  }

  return result;
}


nsresult
nsGenericHTMLElement::GetLayoutHistoryAndKey(nsIHTMLContent* aContent,
                                             nsILayoutHistoryState** aHistory,
                                             nsACString& aKey)
{
  //
  // Get the pres shell
  //
  nsCOMPtr<nsIDocument> doc = aContent->GetDocument();
  if (!doc) {
    return NS_OK;
  }

  //
  // Get the history (don't bother with the key if the history is not there)
  //
  nsresult rv;
  nsCOMPtr<nsISupports> container = doc->GetContainer();
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
  if (docShell) {
    rv = docShell->GetLayoutHistoryState(aHistory);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (!*aHistory) {
    return NS_OK;
  }

  //
  // Get the state key
  //
  rv = nsContentUtils::GenerateStateKey(aContent, nsIStatefulFrame::eNoID, aKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the state key is blank, this is anonymous content or for
  // whatever reason we are not supposed to save/restore state.
  if (aKey.IsEmpty()) {
    NS_RELEASE(*aHistory);
    return NS_OK;
  }

  // Add something unique to content so layout doesn't muck us up
  aKey += "-C";

  return rv;
}

PRBool
nsGenericHTMLElement::RestoreFormControlState(nsIHTMLContent* aContent,
                                              nsIFormControl* aControl)
{
  nsCOMPtr<nsILayoutHistoryState> history;
  nsCAutoString key;
  nsresult rv = GetLayoutHistoryAndKey(aContent, getter_AddRefs(history), key);
  if (!history) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIPresState> state;
  // Get the pres state for this key
  rv = history->GetState(key, getter_AddRefs(state));
  if (state) {
    rv = aControl->RestoreState(state);
    history->RemoveState(key);
    return NS_SUCCEEDED(rv);
  }

  return PR_FALSE;
}

// XXX This creates a dependency between content and frames
nsresult
nsGenericHTMLElement::GetPresContext(nsIHTMLContent* aContent,
                                     nsIPresContext** aPresContext)
{
  // Get the document
  nsIDocument* doc = aContent->GetDocument();
  if (doc) {
    // Get presentation shell 0
    nsIPresShell *presShell = doc->GetShellAt(0);
    if (presShell) {
      return presShell->GetPresContext(aPresContext);
    }
  }

  return NS_NOINTERFACE;
}

// XXX check all mappings against ebina's usage
static const nsHTMLValue::EnumTable kAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_RIGHT },

  { "texttop", NS_STYLE_VERTICAL_ALIGN_TEXT_TOP },// verified
  { "baseline", NS_STYLE_VERTICAL_ALIGN_BASELINE },// verified
  { "center", NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { "bottom", NS_STYLE_VERTICAL_ALIGN_BASELINE },//verified
  { "top", NS_STYLE_VERTICAL_ALIGN_TOP },//verified
  { "middle", NS_STYLE_VERTICAL_ALIGN_MIDDLE },//verified
  { "absbottom", NS_STYLE_VERTICAL_ALIGN_BOTTOM },//verified
  { "abscenter", NS_STYLE_VERTICAL_ALIGN_MIDDLE },/* XXX not the same as ebina */
  { "absmiddle", NS_STYLE_VERTICAL_ALIGN_MIDDLE },/* XXX ditto */
  { 0 }
};

// Elements that should return vertical align values "middle", "bottom", and "top" 
//  instead of "center", "baseline", and "texttop" from GetAttribute() should use this
static const nsHTMLValue::EnumTable kVAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_RIGHT },
  { "top", NS_STYLE_VERTICAL_ALIGN_TOP },//verified
  { "texttop", NS_STYLE_VERTICAL_ALIGN_TEXT_TOP },// verified
  { "bottom", NS_STYLE_VERTICAL_ALIGN_BASELINE },//verified
  { "baseline", NS_STYLE_VERTICAL_ALIGN_BASELINE },// verified
  { "middle", NS_STYLE_VERTICAL_ALIGN_MIDDLE },//verified
  { "center", NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { "absbottom", NS_STYLE_VERTICAL_ALIGN_BOTTOM },//verified
  { "abscenter", NS_STYLE_VERTICAL_ALIGN_MIDDLE },/* XXX not the same as ebina */
  { "absmiddle", NS_STYLE_VERTICAL_ALIGN_MIDDLE },/* XXX ditto */
  { 0 }
};

static const nsHTMLValue::EnumTable kDivAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_MOZ_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_MOZ_CENTER },
  { "middle", NS_STYLE_TEXT_ALIGN_MOZ_CENTER },
  { "justify", NS_STYLE_TEXT_ALIGN_JUSTIFY },
  { 0 }
};

static const nsHTMLValue::EnumTable kFrameborderTable[] = {
  { "yes", NS_STYLE_FRAME_YES },
  { "no", NS_STYLE_FRAME_NO },
  { "1", NS_STYLE_FRAME_1 },
  { "0", NS_STYLE_FRAME_0 },
  { 0 }
};

static const nsHTMLValue::EnumTable kScrollingTable[] = {
  { "yes", NS_STYLE_FRAME_YES },
  { "no", NS_STYLE_FRAME_NO },
  { "on", NS_STYLE_FRAME_ON },
  { "off", NS_STYLE_FRAME_OFF },
  { "scroll", NS_STYLE_FRAME_SCROLL },
  { "noscroll", NS_STYLE_FRAME_NOSCROLL },
  { "auto", NS_STYLE_FRAME_AUTO },
  { 0 }
};

static const nsHTMLValue::EnumTable kTableVAlignTable[] = {
  { "top",     NS_STYLE_VERTICAL_ALIGN_TOP },
  { "middle",  NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { "bottom",  NS_STYLE_VERTICAL_ALIGN_BOTTOM },
  { "baseline",NS_STYLE_VERTICAL_ALIGN_BASELINE },
  { 0 }
};

PRBool
nsGenericHTMLElement::ParseAlignValue(const nsAString& aString,
                                      nsAttrValue& aResult)
{
  return aResult.ParseEnumValue(aString, kAlignTable);
}

//----------------------------------------

// Vanilla table as defined by the html4 spec...
static const nsHTMLValue::EnumTable kTableHAlignTable[] = {
  { "left",   NS_STYLE_TEXT_ALIGN_LEFT },
  { "right",  NS_STYLE_TEXT_ALIGN_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { "char",   NS_STYLE_TEXT_ALIGN_CHAR },
  { "justify",NS_STYLE_TEXT_ALIGN_JUSTIFY },
  { 0 }
};

// This table is used for TABLE when in compatability mode
static const nsHTMLValue::EnumTable kCompatTableHAlignTable[] = {
  { "left",   NS_STYLE_TEXT_ALIGN_LEFT },
  { "right",  NS_STYLE_TEXT_ALIGN_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { "char",   NS_STYLE_TEXT_ALIGN_CHAR },
  { "justify",NS_STYLE_TEXT_ALIGN_JUSTIFY },
  { "abscenter", NS_STYLE_TEXT_ALIGN_CENTER },
  { "absmiddle", NS_STYLE_TEXT_ALIGN_CENTER },
  { "middle", NS_STYLE_TEXT_ALIGN_CENTER },
  { 0 }
};

PRBool
nsGenericHTMLElement::ParseTableHAlignValue(const nsAString& aString,
                                            nsAttrValue& aResult) const
{
  if (InNavQuirksMode(mDocument)) {
    return aResult.ParseEnumValue(aString, kCompatTableHAlignTable);
  }
  return aResult.ParseEnumValue(aString, kTableHAlignTable);
}

PRBool
nsGenericHTMLElement::TableHAlignValueToString(const nsHTMLValue& aValue,
                                               nsAString& aResult) const
{
  if (InNavQuirksMode(mDocument)) {
    return aValue.EnumValueToString(kCompatTableHAlignTable, aResult);
  }
  return aValue.EnumValueToString(kTableHAlignTable, aResult);
}

//----------------------------------------

// These tables are used for TD,TH,TR, etc (but not TABLE)
static const nsHTMLValue::EnumTable kTableCellHAlignTable[] = {
  { "left",   NS_STYLE_TEXT_ALIGN_LEFT },
  { "right",  NS_STYLE_TEXT_ALIGN_MOZ_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_MOZ_CENTER },
  { "char",   NS_STYLE_TEXT_ALIGN_CHAR },
  { "justify",NS_STYLE_TEXT_ALIGN_JUSTIFY },
  { 0 }
};

static const nsHTMLValue::EnumTable kCompatTableCellHAlignTable[] = {
  { "left",   NS_STYLE_TEXT_ALIGN_LEFT },
  { "right",  NS_STYLE_TEXT_ALIGN_MOZ_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_MOZ_CENTER },
  { "char",   NS_STYLE_TEXT_ALIGN_CHAR },
  { "justify",NS_STYLE_TEXT_ALIGN_JUSTIFY },

  // The following are non-standard but necessary for Nav4 compatibility
  { "middle", NS_STYLE_TEXT_ALIGN_MOZ_CENTER },
  // allow center and absmiddle to map to NS_STYLE_TEXT_ALIGN_CENTER and
  // NS_STYLE_TEXT_ALIGN_CENTER to map to center by using the following order
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { "absmiddle", NS_STYLE_TEXT_ALIGN_CENTER },

  { 0 }
};

PRBool
nsGenericHTMLElement::ParseTableCellHAlignValue(const nsAString& aString,
                                                nsAttrValue& aResult) const
{
  if (InNavQuirksMode(mDocument)) {
    return aResult.ParseEnumValue(aString, kCompatTableCellHAlignTable);
  }
  return aResult.ParseEnumValue(aString, kTableCellHAlignTable);
}

PRBool
nsGenericHTMLElement::TableCellHAlignValueToString(const nsHTMLValue& aValue,
                                                   nsAString& aResult) const
{
  if (InNavQuirksMode(mDocument)) {
    return aValue.EnumValueToString(kCompatTableCellHAlignTable, aResult);
  }
  return aValue.EnumValueToString(kTableCellHAlignTable, aResult);
}

//----------------------------------------

PRBool
nsGenericHTMLElement::ParseTableVAlignValue(const nsAString& aString,
                                            nsAttrValue& aResult)
{
  return aResult.ParseEnumValue(aString, kTableVAlignTable);
}

PRBool
nsGenericHTMLElement::AlignValueToString(const nsHTMLValue& aValue,
                                         nsAString& aResult)
{
  return aValue.EnumValueToString(kAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::VAlignValueToString(const nsHTMLValue& aValue,
                                         nsAString& aResult)
{
  return aValue.EnumValueToString(kVAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::TableVAlignValueToString(const nsHTMLValue& aValue,
                                               nsAString& aResult)
{
  return aValue.EnumValueToString(kTableVAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::ParseDivAlignValue(const nsAString& aString,
                                         nsAttrValue& aResult) const
{
  return aResult.ParseEnumValue(aString, kDivAlignTable);
}

PRBool
nsGenericHTMLElement::DivAlignValueToString(const nsHTMLValue& aValue,
                                            nsAString& aResult) const
{
  return aValue.EnumValueToString(kDivAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::ParseImageAttribute(nsIAtom* aAttribute,
                                          const nsAString& aString,
                                          nsAttrValue& aResult)
{
  if ((aAttribute == nsHTMLAtoms::width) ||
      (aAttribute == nsHTMLAtoms::height)) {
    return aResult.ParseSpecialIntValue(aString, PR_TRUE, PR_FALSE);
  }
  else if ((aAttribute == nsHTMLAtoms::hspace) ||
           (aAttribute == nsHTMLAtoms::vspace) ||
           (aAttribute == nsHTMLAtoms::border)) {
    return aResult.ParseIntWithBounds(aString, 0);
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseFrameborderValue(const nsAString& aString,
                                            nsAttrValue& aResult)
{
  return aResult.ParseEnumValue(aString, kFrameborderTable);
}

PRBool
nsGenericHTMLElement::FrameborderValueToString(const nsHTMLValue& aValue,
                                               nsAString& aResult)
{
  return aValue.EnumValueToString(kFrameborderTable, aResult);
}

PRBool
nsGenericHTMLElement::ParseScrollingValue(const nsAString& aString,
                                          nsAttrValue& aResult)
{
  return aResult.ParseEnumValue(aString, kScrollingTable);
}

PRBool
nsGenericHTMLElement::ScrollingValueToString(const nsHTMLValue& aValue,
                                             nsAString& aResult)
{
  return aValue.EnumValueToString(kScrollingTable, aResult);
}

nsresult
nsGenericHTMLElement::ReparseStyleAttribute()
{
  const nsAttrValue* oldVal = mAttrsAndChildren.GetAttr(nsHTMLAtoms::style);
  
  if (oldVal && oldVal->Type() != nsAttrValue::eCSSStyleRule) {
    nsAttrValue attrValue;
    nsAutoString stringValue;
    oldVal->ToString(stringValue);
    ParseStyleAttribute(this, mNodeInfo->NamespaceEquals(kNameSpaceID_XHTML),
                        stringValue, attrValue);
    // Don't bother going through SetInlineStyleRule, we don't want to fire off
    // mutation events or document notifications anyway
    nsresult rv = mAttrsAndChildren.SetAndTakeAttr(nsHTMLAtoms::style, attrValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

void
nsGenericHTMLElement::ParseStyleAttribute(nsIContent* aContent,
                                          PRBool aCaseSensitive,
                                          const nsAString& aValue,
                                          nsAttrValue& aResult)
{
  nsresult result = NS_OK;
  NS_ASSERTION(aContent->GetNodeInfo(), "If we don't have a nodeinfo, we are very screwed");

  nsIDocument* doc = aContent->GetDocument();
  if (!doc) {
    doc = aContent->GetNodeInfo()->GetDocument();
  }

  if (doc) {
    PRBool isCSS = PR_TRUE; // assume CSS until proven otherwise

    if (!aContent->IsNativeAnonymous()) {  // native anonymous content
                                           // always assumes CSS
      nsAutoString styleType;
      doc->GetHeaderData(nsHTMLAtoms::headerContentStyleType, styleType);
      if (!styleType.IsEmpty()) {
        static const char textCssStr[] = "text/css";
        isCSS = (styleType.EqualsIgnoreCase(textCssStr, sizeof(textCssStr) - 1));
      }
    }

    if (isCSS) {
      nsICSSLoader* cssLoader = doc->GetCSSLoader();
      nsCOMPtr<nsICSSParser> cssParser;
      if (cssLoader) {
        result = cssLoader->GetParserFor(nsnull, getter_AddRefs(cssParser));
      }
      else {
        result = NS_NewCSSParser(getter_AddRefs(cssParser));
        if (cssParser) {
          // look up our namespace.  If we're XHTML, we need to be case-sensitive
          // Otherwise, we should not be.
          cssParser->SetCaseSensitive(aCaseSensitive);
        }
      }
      if (cssParser) {
        nsCOMPtr<nsIURI> baseURI = aContent->GetBaseURI();

        nsCOMPtr<nsICSSStyleRule> rule;
        result = cssParser->ParseStyleAttribute(aValue, baseURI,
                                                getter_AddRefs(rule));
        if (cssLoader) {
          cssLoader->RecycleParser(cssParser);
        }

        if (rule) {
          aResult.SetTo(rule);

          return;
        }
      }
    }
  }

  aResult.SetTo(aValue);
}

/**
 * Handle attributes common to all html elements
 */
void
nsGenericHTMLElement::MapCommonAttributesInto(const nsMappedAttributes* aAttributes,
                                              nsRuleData* aData)
{
  if (aData->mSID == eStyleStruct_TextReset) {
    if (aData->mTextData->mUnicodeBidi.GetUnit() == eCSSUnit_Null) {
      nsHTMLValue value;
      aAttributes->GetAttribute(nsHTMLAtoms::dir, value);
      if (value.GetUnit() == eHTMLUnit_Enumerated)
        aData->mTextData->mUnicodeBidi.SetIntValue(
            NS_STYLE_UNICODE_BIDI_EMBED, eCSSUnit_Enumerated);
    }
  } else if (aData->mSID == eStyleStruct_Visibility) {
    if (aData->mDisplayData->mDirection.GetUnit() == eCSSUnit_Null) {
      nsHTMLValue value;
      aAttributes->GetAttribute(nsHTMLAtoms::dir, value);
      if (value.GetUnit() == eHTMLUnit_Enumerated)
        aData->mDisplayData->mDirection.SetIntValue(value.GetIntValue(),
                                                    eCSSUnit_Enumerated);
    }
    nsHTMLValue value;
    if (aAttributes->GetAttribute(nsHTMLAtoms::lang, value) !=
        NS_CONTENT_ATTR_NOT_THERE &&
        value.GetUnit() == eHTMLUnit_String) {
      nsAutoString lang;
      value.GetStringValue(lang);
      aData->mDisplayData->mLang.SetStringValue(lang,
                                                eCSSUnit_String);
    }
  }
}



/* static */ const nsGenericHTMLElement::MappedAttributeEntry
nsGenericHTMLElement::sCommonAttributeMap[] = {
  { &nsHTMLAtoms::dir },
  { &nsHTMLAtoms::lang },
  { nsnull }
};

/* static */ const nsGenericElement::MappedAttributeEntry
nsGenericHTMLElement::sImageMarginSizeAttributeMap[] = {
  { &nsHTMLAtoms::width },
  { &nsHTMLAtoms::height },
  { &nsHTMLAtoms::hspace },
  { &nsHTMLAtoms::vspace },
  { nsnull }
};

/* static */ const nsGenericElement::MappedAttributeEntry
nsGenericHTMLElement::sImageAlignAttributeMap[] = {
  { &nsHTMLAtoms::align },
  { nsnull }
};

/* static */ const nsGenericElement::MappedAttributeEntry
nsGenericHTMLElement::sDivAlignAttributeMap[] = {
  { &nsHTMLAtoms::align },
  { nsnull }
};

/* static */ const nsGenericElement::MappedAttributeEntry
nsGenericHTMLElement::sImageBorderAttributeMap[] = {
  { &nsHTMLAtoms::border },
  { nsnull }
};

/* static */ const nsGenericElement::MappedAttributeEntry
nsGenericHTMLElement::sBackgroundAttributeMap[] = {
  { &nsHTMLAtoms::background },
  { &nsHTMLAtoms::bgcolor },
  { nsnull }
};

/* static */ const nsGenericElement::MappedAttributeEntry
nsGenericHTMLElement::sScrollingAttributeMap[] = {
  { &nsHTMLAtoms::scrolling },
  { nsnull }
};

void
nsGenericHTMLElement::MapImageAlignAttributeInto(const nsMappedAttributes* aAttributes,
                                                 nsRuleData* aRuleData)
{
  if (aRuleData->mSID == eStyleStruct_Display || aRuleData->mSID == eStyleStruct_TextReset) {
    nsHTMLValue value;
    aAttributes->GetAttribute(nsHTMLAtoms::align, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      PRUint8 align = (PRUint8)(value.GetIntValue());
      if (aRuleData->mSID == eStyleStruct_Display && aRuleData->mDisplayData->mFloat.GetUnit() == eCSSUnit_Null) {
        if (align == NS_STYLE_TEXT_ALIGN_LEFT)
          aRuleData->mDisplayData->mFloat.SetIntValue(NS_STYLE_FLOAT_LEFT, eCSSUnit_Enumerated);
        else if (align == NS_STYLE_TEXT_ALIGN_RIGHT)
          aRuleData->mDisplayData->mFloat.SetIntValue(NS_STYLE_FLOAT_RIGHT, eCSSUnit_Enumerated);
      }
      else if (aRuleData->mSID == eStyleStruct_TextReset && aRuleData->mTextData->mVerticalAlign.GetUnit() == eCSSUnit_Null) {
        switch (align) {
        case NS_STYLE_TEXT_ALIGN_LEFT:
        case NS_STYLE_TEXT_ALIGN_RIGHT:
          break;
        default:
          aRuleData->mTextData->mVerticalAlign.SetIntValue(align, eCSSUnit_Enumerated);
          break;
        }
      }
    }
  }
}

void
nsGenericHTMLElement::MapDivAlignAttributeInto(const nsMappedAttributes* aAttributes,
                                               nsRuleData* aRuleData)
{
  if (aRuleData->mSID == eStyleStruct_Text) {
    if (aRuleData->mTextData->mTextAlign.GetUnit() == eCSSUnit_Null) {
      // align: enum
      nsHTMLValue value;
      aAttributes->GetAttribute(nsHTMLAtoms::align, value);
      if (value.GetUnit() == eHTMLUnit_Enumerated)
        aRuleData->mTextData->mTextAlign.SetIntValue(value.GetIntValue(), eCSSUnit_Enumerated);
    }
  }
}


void
nsGenericHTMLElement::MapImageMarginAttributeInto(const nsMappedAttributes* aAttributes,
                                                  nsRuleData* aData)
{
  if (aData->mSID != eStyleStruct_Margin)
    return;

  nsHTMLValue value;

  // hspace: value
  aAttributes->GetAttribute(nsHTMLAtoms::hspace, value);
  nsCSSValue hval;
  if (value.GetUnit() == eHTMLUnit_Integer)
    hval.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel);
  else if (value.GetUnit() == eHTMLUnit_Percent)
    hval.SetPercentValue(value.GetPercentValue());

  if (hval.GetUnit() != eCSSUnit_Null) {
    nsCSSRect& margin = aData->mMarginData->mMargin;
    if (margin.mLeft.GetUnit() == eCSSUnit_Null)
      margin.mLeft = hval;
    if (margin.mRight.GetUnit() == eCSSUnit_Null)
      margin.mRight = hval;
  }

  // vspace: value
  aAttributes->GetAttribute(nsHTMLAtoms::vspace, value);
  nsCSSValue vval;
  if (value.GetUnit() == eHTMLUnit_Integer)
    vval.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Pixel);
  else if (value.GetUnit() == eHTMLUnit_Percent)
    vval.SetPercentValue(value.GetPercentValue());

  if (vval.GetUnit() != eCSSUnit_Null) {
    nsCSSRect& margin = aData->mMarginData->mMargin;
    if (margin.mTop.GetUnit() == eCSSUnit_Null)
      margin.mTop = vval;
    if (margin.mBottom.GetUnit() == eCSSUnit_Null)
      margin.mBottom = vval;
  }
}

void
nsGenericHTMLElement::MapImageSizeAttributesInto(const nsMappedAttributes* aAttributes,
                                                 nsRuleData* aData)
{
  if (aData->mSID != eStyleStruct_Position)
    return;

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

void
nsGenericHTMLElement::MapImageBorderAttributeInto(const nsMappedAttributes* aAttributes,
                                                  nsRuleData* aData)
{
  if (aData->mSID != eStyleStruct_Border)
    return;

  nsHTMLValue value;

  // border: pixels
  if (aAttributes->GetAttribute(nsHTMLAtoms::border, value) ==
      NS_CONTENT_ATTR_NOT_THERE) {
    return;
  }
  
  if (value.GetUnit() != eHTMLUnit_Integer)  // something other than Integer
    value.SetIntValue(0, eHTMLUnit_Integer);

  nscoord val = value.GetIntValue();

  nsCSSRect& borderWidth = aData->mMarginData->mBorderWidth;
  if (borderWidth.mLeft.GetUnit() == eCSSUnit_Null)
    borderWidth.mLeft.SetFloatValue((float)val, eCSSUnit_Pixel);
  if (borderWidth.mTop.GetUnit() == eCSSUnit_Null)
    borderWidth.mTop.SetFloatValue((float)val, eCSSUnit_Pixel);
  if (borderWidth.mRight.GetUnit() == eCSSUnit_Null)
    borderWidth.mRight.SetFloatValue((float)val, eCSSUnit_Pixel);
  if (borderWidth.mBottom.GetUnit() == eCSSUnit_Null)
    borderWidth.mBottom.SetFloatValue((float)val, eCSSUnit_Pixel);

  nsCSSRect& borderStyle = aData->mMarginData->mBorderStyle;
  if (borderStyle.mLeft.GetUnit() == eCSSUnit_Null)
    borderStyle.mLeft.SetIntValue(NS_STYLE_BORDER_STYLE_SOLID, eCSSUnit_Enumerated);
  if (borderStyle.mTop.GetUnit() == eCSSUnit_Null)
    borderStyle.mTop.SetIntValue(NS_STYLE_BORDER_STYLE_SOLID, eCSSUnit_Enumerated);
  if (borderStyle.mRight.GetUnit() == eCSSUnit_Null)
    borderStyle.mRight.SetIntValue(NS_STYLE_BORDER_STYLE_SOLID, eCSSUnit_Enumerated);
  if (borderStyle.mBottom.GetUnit() == eCSSUnit_Null)
    borderStyle.mBottom.SetIntValue(NS_STYLE_BORDER_STYLE_SOLID, eCSSUnit_Enumerated);

  nsCSSRect& borderColor = aData->mMarginData->mBorderColor;
  if (borderColor.mLeft.GetUnit() == eCSSUnit_Null)
    borderColor.mLeft.SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR, eCSSUnit_Enumerated);
  if (borderColor.mTop.GetUnit() == eCSSUnit_Null)
    borderColor.mTop.SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR, eCSSUnit_Enumerated);
  if (borderColor.mRight.GetUnit() == eCSSUnit_Null)
    borderColor.mRight.SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR, eCSSUnit_Enumerated);
  if (borderColor.mBottom.GetUnit() == eCSSUnit_Null)
    borderColor.mBottom.SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR, eCSSUnit_Enumerated);
}

void
nsGenericHTMLElement::MapBackgroundAttributesInto(const nsMappedAttributes* aAttributes,
                                                  nsRuleData* aData)
{
  if (aData->mSID != eStyleStruct_Background)
    return;

  if (aData->mColorData->mBackImage.GetUnit() == eCSSUnit_Null) {
    // background
    nsHTMLValue value;
    if (NS_CONTENT_ATTR_HAS_VALUE ==
        aAttributes->GetAttribute(nsHTMLAtoms::background, value) &&
        value.GetUnit() == eHTMLUnit_String) {
      nsAutoString spec;
      value.GetStringValue(spec);
      if (!spec.IsEmpty()) {
        // Resolve url to an absolute url
        // XXX this breaks if the HTML element has an xml:base
        // attribute (the xml:base will not be taken into account)
        // as well as elements with _baseHref set. We need to be able
        // to get to the element somehow, or store the base URI in the
        // attributes.
        nsIDocument* doc = aData->mPresContext->GetDocument();
        nsCOMPtr<nsIURI> uri;
        nsresult rv = nsContentUtils::NewURIWithDocumentCharset(
            getter_AddRefs(uri), spec, doc, doc->GetBaseURI());
        if (NS_SUCCEEDED(rv)) {
          nsCSSValue::Image *img = new nsCSSValue::Image(uri, spec.get(), doc);
          if (img) {
            if (img->mString) {
              aData->mColorData->mBackImage.SetImageValue(img);
            }
            else
              delete img;
          }
        }
      }
      else if (aData->mPresContext->CompatibilityMode() ==
               eCompatibility_NavQuirks) {
        // in NavQuirks mode, allow the empty string to set the
        // background to empty
        aData->mColorData->mBackImage.SetNoneValue();
      }
    }
  }

  // bgcolor
  if (aData->mColorData->mBackColor.GetUnit() == eCSSUnit_Null) {
    nsHTMLValue value;
    nscolor color;
    if (aAttributes->GetAttribute(nsHTMLAtoms::bgcolor, value) !=
        NS_CONTENT_ATTR_NOT_THERE &&
        value.GetColorValue(color)) {
      aData->mColorData->mBackColor.SetColorValue(color);
    }
  }
}

void
nsGenericHTMLElement::MapScrollingAttributeInto(const nsMappedAttributes* aAttributes,
                                                nsRuleData* aData)
{
  if (aData->mSID != eStyleStruct_Display)
    return;

  // scrolling
  if (aData->mDisplayData->mOverflow.GetUnit() == eCSSUnit_Null) {
    nsHTMLValue value;
    aAttributes->GetAttribute(nsHTMLAtoms::scrolling, value);
    if (eHTMLUnit_Enumerated == value.GetUnit()) {
      PRInt32 mappedValue;
      switch (value.GetIntValue()) {
        case NS_STYLE_FRAME_ON:
        case NS_STYLE_FRAME_SCROLL:
        case NS_STYLE_FRAME_YES:
          mappedValue = NS_STYLE_OVERFLOW_SCROLL;
          break;

        case NS_STYLE_FRAME_OFF:
        case NS_STYLE_FRAME_NOSCROLL:
        case NS_STYLE_FRAME_NO:
          mappedValue = NS_STYLE_OVERFLOW_SCROLLBARS_NONE;
          break;
      
        case NS_STYLE_FRAME_AUTO:
          mappedValue = NS_STYLE_OVERFLOW_AUTO;
          break;

        default:
          NS_NOTREACHED("unexpected value");
          mappedValue = NS_STYLE_OVERFLOW_AUTO;
          break;
      }
      aData->mDisplayData->mOverflow.SetIntValue(mappedValue,
                                                 eCSSUnit_Enumerated);
    }
  }
}

//----------------------------------------------------------------------

nsresult
nsGenericHTMLElement::ReplaceContentsWithText(const nsAString& aText,
                                              PRBool aNotify)
{
  PRUint32 count = GetChildCount();
  nsresult rv = NS_OK;

  nsCOMPtr<nsIDOMText> textChild;

  if (count > 0) {
    // if we already have a DOMText child, reuse it.
    textChild = do_QueryInterface(GetChildAt(0));

    PRUint32 lastChild = textChild ? 1 : 0;
    PRUint32 i = count - 1;
    while (i-- > lastChild) {
      RemoveChildAt(i, aNotify);
    }
  }

  if (textChild) {
    rv = textChild->SetData(aText);
  } else {
    nsCOMPtr<nsITextContent> text;
    rv = NS_NewTextNode(getter_AddRefs(text));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = text->SetText(aText, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = InsertChildAt(text, 0, aNotify, PR_FALSE);
  }    
      
  return rv;
}

nsresult
nsGenericHTMLElement::GetContentsAsText(nsAString& aText)
{
  aText.Truncate();
  PRInt32 children = GetChildCount();
  
  nsCOMPtr<nsIDOMText> tc;
  nsAutoString textData;

  PRInt32 i;
  for (i = 0; i < children; ++i) {
    tc = do_QueryInterface(GetChildAt(i));
    if (tc) {
      if (aText.IsEmpty()) {
        tc->GetData(aText);
      } else {
        tc->GetData(textData);
        aText.Append(textData);
      }
    }
  }
  
  return NS_OK;
}

void
nsGenericHTMLElement::GetStringAttrWithDefault(nsIAtom* aAttr,
                                               const nsAString& aDefault,
                                               nsAString& aResult)
{
  nsresult rv = GetAttr(kNameSpaceID_None, aAttr, aResult);
  if (rv == NS_CONTENT_ATTR_NOT_THERE) {
    aResult = aDefault;
  }
}

nsresult
nsGenericHTMLElement::SetBoolAttr(nsIAtom* aAttr, PRBool aValue)
{
  if (aValue) {
    return SetAttr(kNameSpaceID_None, aAttr, EmptyString(), PR_TRUE);
  }

  return UnsetAttr(kNameSpaceID_None, aAttr, PR_TRUE);
}

void
nsGenericHTMLElement::GetIntAttr(nsIAtom* aAttr, PRInt32 aDefault, PRInt32* aResult)
{
  const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(aAttr);
  if (attrVal && attrVal->Type() == nsAttrValue::eInteger) {
    *aResult = attrVal->GetIntegerValue();
  }
  else {
    *aResult = aDefault;
  }
}

nsresult
nsGenericHTMLElement::SetIntAttr(nsIAtom* aAttr, PRInt32 aValue)
{
  nsAutoString value;
  value.AppendInt(aValue);

  return SetAttr(kNameSpaceID_None, aAttr, value, PR_TRUE);
}

void
nsGenericHTMLElement::GetURIAttr(nsIAtom* aAttr, nsAString& aResult)
{
  nsAutoString attrValue;
  nsresult rv = GetAttr(kNameSpaceID_None, aAttr, attrValue);
  if (rv != NS_CONTENT_ATTR_HAS_VALUE) {
    aResult.Truncate();

    return;
  }

  nsCOMPtr<nsIURI> baseURI = GetBaseURI();
  nsCOMPtr<nsIURI> attrURI;
  rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(attrURI),
                                                 attrValue, GetOwnerDocument(),
                                                 baseURI);
  if (NS_FAILED(rv)) {
    // Just use the attr value as the result...
    aResult = attrValue;

    return;
  }

  NS_ASSERTION(attrURI,
               "nsContentUtils::NewURIWithDocumentCharset return value lied");

  nsCAutoString spec;
  attrURI->GetSpec(spec);
  CopyUTF8toUTF16(spec, aResult);
}

//----------------------------------------------------------------------

nsGenericHTMLFormElement::nsGenericHTMLFormElement()
{
  mForm = nsnull;
}

nsGenericHTMLFormElement::~nsGenericHTMLFormElement()
{
  // Clean up.  Set the form to nsnull so it knows we went away.
  SetForm(nsnull);
}

NS_INTERFACE_MAP_BEGIN(nsGenericHTMLFormElement)
  NS_INTERFACE_MAP_ENTRY(nsIFormControl)
NS_INTERFACE_MAP_END_INHERITING(nsGenericHTMLElement)


PRBool
nsGenericHTMLFormElement::IsContentOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eELEMENT | eHTML | eHTML_FORM_CONTROL));
}

NS_IMETHODIMP
nsGenericHTMLFormElement::SetForm(nsIDOMHTMLFormElement* aForm,
                                  PRBool aRemoveFromForm)
{
  nsAutoString nameVal, idVal;

  if (aForm || (mForm && aRemoveFromForm)) {
    GetAttr(kNameSpaceID_None, nsHTMLAtoms::name, nameVal);
    GetAttr(kNameSpaceID_None, nsHTMLAtoms::id, idVal);
  }

  if (mForm && aRemoveFromForm) {
    mForm->RemoveElement(this);

    if (!nameVal.IsEmpty()) {
      mForm->RemoveElementFromTable(this, nameVal);
    }

    if (!idVal.IsEmpty()) {
      mForm->RemoveElementFromTable(this, idVal);
    }
  }

  if (aForm) {
    // keep a *weak* ref to the form here
    CallQueryInterface(aForm, &mForm);
    mForm->Release();
  } else {
    mForm = nsnull;
  }

  if (mForm) {
    mForm->AddElement(this);

    if (!nameVal.IsEmpty()) {
      mForm->AddElementToTable(this, nameVal);
    }

    if (!idVal.IsEmpty()) {
      mForm->AddElementToTable(this, idVal);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGenericHTMLFormElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  NS_ENSURE_ARG_POINTER(aForm);
  *aForm = nsnull;

  if (mForm) {
    CallQueryInterface(mForm, aForm);
  }

  return NS_OK;
}

void
nsGenericHTMLFormElement::SetParent(nsIContent* aParent)
{
  if (!aParent && mForm) {
    SetForm(nsnull);
  } else if (mDocument && aParent && (GetParent() || !mForm)) {
    // If we have a new parent and either we had an old parent or we
    // don't have a form, search for a containing form.  If we didn't
    // have an old parent, but we do have a form, we shouldn't do the
    // search. In this case, someone (possibly the content sink) has
    // already set the form for us.

    FindAndSetForm();
  }

  nsGenericHTMLElement::SetParent(aParent);
}

void
nsGenericHTMLFormElement::SetDocument(nsIDocument* aDocument, PRBool aDeep,
                                      PRBool aCompileEventHandlers)
{
  // Save state before doing anything if the document is being removed
  if (!aDocument) {
    SaveState();
  }

  if (aDocument && GetParent() && !mForm) {
    FindAndSetForm();
  } else if (!aDocument && mForm) {
    // We got removed from document.  We have a parent form.  Check
    // that the form is still in the document, and if so remove
    // ourselves from the form.  This keeps ghosts from appearing in
    // the form's |elements| array
    nsCOMPtr<nsIContent> formContent(do_QueryInterface(mForm));
    if (formContent && formContent->GetDocument()) {
      SetForm(nsnull);
    }
  }

  nsGenericHTMLElement::SetDocument(aDocument, aDeep, aCompileEventHandlers);
}


nsresult
nsGenericHTMLFormElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                  nsIAtom* aPrefix, const nsAString& aValue,
                                  PRBool aNotify)
{
  if (aNameSpaceID != kNameSpaceID_None) {
    return nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                         aNotify);
  }

  nsCOMPtr<nsIFormControl> thisControl;
  nsAutoString tmp;
  nsresult rv = NS_OK;

  QueryInterface(NS_GET_IID(nsIFormControl), getter_AddRefs(thisControl));

  // Add & remove the control to and/or from the hash table
  if (mForm && (aName == nsHTMLAtoms::name || aName == nsHTMLAtoms::id)) {
    GetAttr(kNameSpaceID_None, aName, tmp);

    if (!tmp.IsEmpty()) {
      mForm->RemoveElementFromTable(thisControl, tmp);
    }
  }

  if (mForm && aName == nsHTMLAtoms::type) {
    GetAttr(kNameSpaceID_None, nsHTMLAtoms::name, tmp);

    if (!tmp.IsEmpty()) {
      mForm->RemoveElementFromTable(thisControl, tmp);
    }

    GetAttr(kNameSpaceID_None, nsHTMLAtoms::id, tmp);

    if (!tmp.IsEmpty()) {
      mForm->RemoveElementFromTable(thisControl, tmp);
    }

    mForm->RemoveElement(thisControl);
  }

  rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue, aNotify);

  if (mForm && (aName == nsHTMLAtoms::name || aName == nsHTMLAtoms::id)) {
    GetAttr(kNameSpaceID_None, aName, tmp);

    if (!tmp.IsEmpty()) {
      mForm->AddElementToTable(thisControl, tmp);
    }
  }

  if (mForm && aName == nsHTMLAtoms::type) {
    GetAttr(kNameSpaceID_None, nsHTMLAtoms::name, tmp);

    if (!tmp.IsEmpty()) {
      mForm->AddElementToTable(thisControl, tmp);
    }

    GetAttr(kNameSpaceID_None, nsHTMLAtoms::id, tmp);

    if (!tmp.IsEmpty()) {
      mForm->AddElementToTable(thisControl, tmp);
    }

    mForm->AddElement(thisControl);
  }

  return rv;
}

void
nsGenericHTMLFormElement::FindAndSetForm()
{
  nsCOMPtr<nsIDOMHTMLFormElement> form = FindForm();
  if (form) {
    SetForm(form);  // always succeeds
  }
}

void
nsGenericHTMLElement::SetElementFocus(PRBool aDoFocus)
{
  nsCOMPtr<nsIPresContext> presContext;
  GetPresContext(this, getter_AddRefs(presContext));
  if (!presContext)
    return;

  if (aDoFocus) {
    SetFocus(presContext);

    presContext->EventStateManager()->MoveCaretToFocus();
    return;
  }

  RemoveFocus(presContext);
}

nsresult nsGenericHTMLElement::RegUnRegAccessKey(PRBool aDoReg)
{
  // first check to see if we have an access key
  nsAutoString accessKey;
  nsresult rv = GetAttr(kNameSpaceID_None, nsHTMLAtoms::accesskey, accessKey);
  NS_ENSURE_SUCCESS(rv, rv);
  if (NS_CONTENT_ATTR_NOT_THERE == rv || accessKey.IsEmpty()) {
    return NS_OK;
  }

  // We have an access key, so get the ESM from the pres context.
  nsCOMPtr<nsIPresContext> presContext;
  GetPresContext(this, getter_AddRefs(presContext));
  NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

  nsIEventStateManager *esm = presContext->EventStateManager();

  // Register or unregister as appropriate.
  if (aDoReg) {
    rv = esm->RegisterAccessKey(this, (PRUint32)accessKey.First());
  } else {
    rv = esm->UnregisterAccessKey(this, (PRUint32)accessKey.First());
  }
  return rv;
}

// static
nsresult
nsGenericHTMLElement::SetProtocolInHrefString(const nsAString &aHref,
                                              const nsAString &aProtocol,
                                              nsAString &aResult)
{
  aResult.Truncate();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  nsAString::const_iterator start, end;
  aProtocol.BeginReading(start);
  aProtocol.EndReading(end);
  nsAString::const_iterator iter(start);
  FindCharInReadable(':', iter, end);
  uri->SetScheme(NS_ConvertUCS2toUTF8(Substring(start, iter)));
   
  nsCAutoString newHref;
  uri->GetSpec(newHref);

  CopyUTF8toUTF16(newHref, aResult);

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::SetHostnameInHrefString(const nsAString &aHref,
                                              const nsAString &aHostname,
                                              nsAString &aResult)
{
  aResult.Truncate();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  uri->SetHost(NS_ConvertUCS2toUTF8(aHostname));

  nsCAutoString newHref;
  uri->GetSpec(newHref);

  CopyUTF8toUTF16(newHref, aResult);

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::SetPathnameInHrefString(const nsAString &aHref,
                                              const nsAString &aPathname,
                                              nsAString &aResult)
{
  aResult.Truncate();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIURL> url(do_QueryInterface(uri, &rv));
  if (NS_FAILED(rv))
    return rv;

  url->SetFilePath(NS_ConvertUCS2toUTF8(aPathname));

  nsCAutoString newHref;
  uri->GetSpec(newHref);

  CopyUTF8toUTF16(newHref, aResult);

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::SetHostInHrefString(const nsAString &aHref,
                                          const nsAString &aHost,
                                          nsAString &aResult)
{
  // Can't simply call nsURI::SetHost, because that would treat the name as an
  // IPv6 address (like http://[server:443]/)

  aResult.Truncate();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString scheme, userpass, path;
  uri->GetScheme(scheme);
  uri->GetUserPass(userpass);
  uri->GetPath(path);

  CopyASCIItoUTF16(scheme, aResult);
  aResult.Append(NS_LITERAL_STRING("://"));
  if (!userpass.IsEmpty()) {
    AppendUTF8toUTF16(userpass, aResult);
    aResult.Append(PRUnichar('@'));
  }
  aResult.Append(aHost);
  AppendUTF8toUTF16(path, aResult);

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::SetSearchInHrefString(const nsAString &aHref,
                                            const nsAString &aSearch,
                                            nsAString &aResult)
{
  aResult.Truncate();
  nsCOMPtr<nsIURI> uri;

  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIURL> url(do_QueryInterface(uri, &rv));
  if (NS_FAILED(rv))
    return rv;

  url->SetQuery(NS_ConvertUCS2toUTF8(aSearch));

  nsCAutoString newHref;
  uri->GetSpec(newHref);

  CopyUTF8toUTF16(newHref, aResult);

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::SetHashInHrefString(const nsAString &aHref,
                                          const nsAString &aHash,
                                          nsAString &aResult)
{
  aResult.Truncate();
  nsCOMPtr<nsIURI> uri;

  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIURL> url(do_QueryInterface(uri, &rv));
  if (NS_FAILED(rv))
    return rv;

  rv = url->SetRef(NS_ConvertUCS2toUTF8(aHash));

  nsCAutoString newHref;
  uri->GetSpec(newHref);

  CopyUTF8toUTF16(newHref, aResult);

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::SetPortInHrefString(const nsAString &aHref,
                                          const nsAString &aPort,
                                          nsAString &aResult)
{
  aResult.Truncate();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);

  if (NS_FAILED(rv))
    return rv;

  PRInt32 port;
  port = nsString(aPort).ToInteger((PRInt32*)&rv);
  if (NS_FAILED(rv))
    return rv;

  uri->SetPort(port);

  nsCAutoString newHref;
  uri->GetSpec(newHref);

  CopyUTF8toUTF16(newHref, aResult);

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::GetProtocolFromHrefString(const nsAString& aHref,
                                                nsAString& aProtocol,
                                                nsIDocument *aDocument)
{
  aProtocol.Truncate();

  nsIIOService* ioService = nsContentUtils::GetIOServiceWeakRef();
  NS_ENSURE_TRUE(ioService, NS_ERROR_FAILURE);

  nsCAutoString protocol;

  nsresult rv =
    ioService->ExtractScheme(NS_ConvertUCS2toUTF8(aHref), protocol);

  if (NS_SUCCEEDED(rv)) {
    CopyASCIItoUTF16(protocol, aProtocol);
  } else {
    // set the protocol to the protocol of the base URI.

    if (aDocument) {
      nsIURI *uri = aDocument->GetBaseURI();
      if (uri) {
        uri->GetScheme(protocol);
      }
    }

    if (protocol.IsEmpty()) {
      // set the protocol to http since it is the most likely protocol
      // to be used.
      aProtocol.Assign(NS_LITERAL_STRING("http"));
    } else {
      CopyASCIItoUTF16(protocol, aProtocol);
    }
  }
  aProtocol.Append(PRUnichar(':'));

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::GetHostFromHrefString(const nsAString& aHref,
                                            nsAString& aHost)
{
  aHost.Truncate();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString hostport;
  rv = uri->GetHostPort(hostport);

  // Failure to get the hostport from the URI isn't necessarily an
  // error. Some URI's just don't have a hostport.

  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(hostport, aHost);
  }

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::GetHostnameFromHrefString(const nsAString& aHref,
                                                nsAString& aHostname)
{
  aHostname.Truncate();
  nsCOMPtr<nsIURI> url;
  nsresult rv = NS_NewURI(getter_AddRefs(url), aHref);
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString host;
  rv = url->GetHost(host);

  if (NS_SUCCEEDED(rv)) {
    // Failure to get the host from the URI isn't necessarily an
    // error. Some URI's just don't have a host.

    CopyUTF8toUTF16(host, aHostname);
  }

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::GetPathnameFromHrefString(const nsAString& aHref,
                                                nsAString& aPathname)
{
  aPathname.Truncate();
  nsCOMPtr<nsIURI> uri;

  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));

  if (!url) {
    // If this is not a URL, we can't get the pathname from the URI

    return NS_OK;
  }

  nsCAutoString file;
  rv = url->GetFilePath(file);
  if (NS_FAILED(rv))
    return rv;

  CopyUTF8toUTF16(file, aPathname);

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::GetSearchFromHrefString(const nsAString& aHref,
                                              nsAString& aSearch)
{
  aSearch.Truncate();
  nsCOMPtr<nsIURI> uri;

  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));

  if (!url) {
    // If this is not a URL, we can't get the query from the URI

    return NS_OK;
  }

  nsCAutoString search;
  rv = url->GetQuery(search);
  if (NS_FAILED(rv))
    return rv;

  if (!search.IsEmpty()) {
    CopyUTF8toUTF16(NS_LITERAL_CSTRING("?") + search, aSearch);
  }

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::GetPortFromHrefString(const nsAString& aHref,
                                            nsAString& aPort)
{
  aPort.Truncate();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  PRInt32 port;
  rv = uri->GetPort(&port);

  if (NS_SUCCEEDED(rv)) {
    // Failure to get the port from the URI isn't necessarily an
    // error. Some URI's just don't have a port.

    if (port == -1) {
      return NS_OK;
    }

    nsAutoString portStr;
    portStr.AppendInt(port, 10);
    aPort.Assign(portStr);
  }

  return NS_OK;
}

// static
nsresult
nsGenericHTMLElement::GetHashFromHrefString(const nsAString& aHref,
                                            nsAString& aHash)
{
  aHash.Truncate();
  nsCOMPtr<nsIURI> uri;

  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHref);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));

  if (!url) {
    // If this is not a URL, we can't get the hash part from the URI

    return NS_OK;
  }

  nsCAutoString ref;
  rv = url->GetRef(ref);
  if (NS_FAILED(rv))
    return rv;
  NS_UnescapeURL(ref); // XXX may result in random non-ASCII bytes!

  if (!ref.IsEmpty()) {
    aHash.Assign(PRUnichar('#'));
    AppendASCIItoUTF16(ref, aHash);
  }
  return NS_OK;
}

const nsAttrName*
nsGenericHTMLElement::InternalGetExistingAttrNameFromQName(const nsAString& aStr) const
{
  if (mNodeInfo->NamespaceEquals(kNameSpaceID_None)) {
    nsAutoString lower;
    ToLowerCase(aStr, lower);
    return mAttrsAndChildren.GetExistingAttrNameFromQName(
      NS_ConvertUTF16toUTF8(lower));
  }

  return mAttrsAndChildren.GetExistingAttrNameFromQName(
    NS_ConvertUTF16toUTF8(aStr));
}
