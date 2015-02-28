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
#include "nsIDOMHTMLBaseFontElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"


class nsHTMLBaseFontElement : public nsGenericHTMLElement,
                              public nsIDOMHTMLBaseFontElement
{
public:
  nsHTMLBaseFontElement();
  virtual ~nsHTMLBaseFontElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  // nsIDOMHTMLBaseElement
  NS_DECL_NSIDOMHTMLBASEFONTELEMENT
};

nsresult
NS_NewHTMLBaseFontElement(nsIHTMLContent** aInstancePtrResult,
                          nsINodeInfo *aNodeInfo, PRBool aFromParser)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLBaseFontElement* it = new nsHTMLBaseFontElement();

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


nsHTMLBaseFontElement::nsHTMLBaseFontElement()
{
}

nsHTMLBaseFontElement::~nsHTMLBaseFontElement()
{
}


NS_IMPL_ADDREF(nsHTMLBaseFontElement)
NS_IMPL_RELEASE(nsHTMLBaseFontElement)


// QueryInterface implementation for nsHTMLBaseFontElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLBaseFontElement,
                                    nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLBaseFontElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLBaseFontElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLBaseFontElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLBaseFontElement* it = new nsHTMLBaseFontElement();

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


NS_IMPL_STRING_ATTR(nsHTMLBaseFontElement, Color, color)
NS_IMPL_STRING_ATTR(nsHTMLBaseFontElement, Face, face)

NS_IMETHODIMP
nsHTMLBaseFontElement::GetSize(PRInt32 *aSize)
{
  *aSize = 3;

  nsHTMLValue value;
  if (NS_CONTENT_ATTR_HAS_VALUE !=
      GetHTMLAttribute(nsHTMLAtoms::size, value)) {
    return NS_OK;
  }

  if (value.GetUnit() == eHTMLUnit_Integer) {
    *aSize = value.GetIntValue();

    return NS_OK;
  }

  if (value.GetUnit() != eHTMLUnit_String) {
    return NS_OK;
  }

  nsAutoString stringValue;
  value.GetStringValue(stringValue);
  if (stringValue.IsEmpty()) {
    return NS_OK;
  }

  PRInt32 ec;
  PRInt32 intValue;
  intValue = stringValue.ToInteger(&ec);
  if (ec != 0) {
    return NS_ERROR_FAILURE;
  }

  if (stringValue[0] == PRUnichar('+') ||
      stringValue[0] == PRUnichar('-')) {
    *aSize += intValue;
  }
  else {
    *aSize = intValue;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLBaseFontElement::SetSize(PRInt32 aSize)
{
  return SetIntAttr(nsHTMLAtoms::size, aSize);
}
