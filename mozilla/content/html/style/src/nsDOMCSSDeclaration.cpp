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

#include "nsDOMCSSDeclaration.h"
#include "nsIDOMCSSRule.h"
#include "nsICSSParser.h"
#include "nsICSSLoader.h"
#include "nsIStyleRule.h"
#include "nsCSSDeclaration.h"
#include "nsCSSProps.h"
#include "nsCOMPtr.h"
#include "nsIURL.h"
#include "nsReadableUtils.h"

#include "nsContentUtils.h"


nsDOMCSSDeclaration::nsDOMCSSDeclaration()
  : mInner(this)
{
}

nsDOMCSSDeclaration::~nsDOMCSSDeclaration()
{
}


// QueryInterface implementation for nsDOMCSSDeclaration
NS_INTERFACE_MAP_BEGIN(nsDOMCSSDeclaration)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSStyleDeclaration)
  NS_INTERFACE_MAP_ENTRY_AGGREGATED(nsIDOMCSS2Properties, &mInner)
  NS_INTERFACE_MAP_ENTRY_AGGREGATED(nsIDOMNSCSS2Properties, &mInner)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMCSSStyleDeclaration)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(CSSStyleDeclaration)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsDOMCSSDeclaration::GetCssText(nsAString& aCssText)
{
  nsCSSDeclaration* decl;
  aCssText.Truncate();
  GetCSSDeclaration(&decl, PR_FALSE);

  if (decl) {
    decl->ToString(aCssText);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::SetCssText(const nsAString& aCssText)
{
  return ParseDeclaration(aCssText, PR_FALSE, PR_TRUE);
}

NS_IMETHODIMP
nsDOMCSSDeclaration::GetLength(PRUint32* aLength)
{
  nsCSSDeclaration *decl;
  nsresult result = GetCSSDeclaration(&decl, PR_FALSE);
 
  if (decl) {
    *aLength = decl->Count();
  } else {
    *aLength = 0;
  }

  return result;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::GetPropertyCSSValue(const nsAString& aPropertyName,
                                         nsIDOMCSSValue** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  // We don't support CSSValue yet so we'll just return null...
  *aReturn = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::Item(PRUint32 aIndex, nsAString& aReturn)
{
  nsCSSDeclaration *decl;
  nsresult result = GetCSSDeclaration(&decl, PR_FALSE);

  aReturn.SetLength(0);
  if (decl) {
    result = decl->GetNthProperty(aIndex, aReturn);
  }

  return result;
}

NS_IMETHODIMP    
nsDOMCSSDeclaration::GetPropertyValue(const nsAString& aPropertyName, 
                                      nsAString& aReturn)
{
  nsCSSValue val;
  nsCSSDeclaration *decl;
  nsresult result = GetCSSDeclaration(&decl, PR_FALSE);

  aReturn.Truncate();
  if (decl) {
    result = decl->GetValue(aPropertyName, aReturn);
  }

  return result;
}

NS_IMETHODIMP    
nsDOMCSSDeclaration::GetPropertyPriority(const nsAString& aPropertyName, 
                                         nsAString& aReturn)
{
  nsCSSDeclaration *decl;
  nsresult result = GetCSSDeclaration(&decl, PR_FALSE);

  aReturn.Truncate();
  if (decl && decl->GetValueIsImportant(aPropertyName)) {
    aReturn.Assign(NS_LITERAL_STRING("important"));    
  }

  return result;
}

NS_IMETHODIMP    
nsDOMCSSDeclaration::SetProperty(const nsAString& aPropertyName, 
                                 const nsAString& aValue, 
                                 const nsAString& aPriority)
{
  if (aValue.IsEmpty()) {
     // If the new value of the property is an empty string we remove the
     // property.
    nsAutoString tmp;
    return RemoveProperty(aPropertyName, tmp);
  }

  if (aPriority.IsEmpty()) {
    return ParsePropertyValue(aPropertyName, aValue);
  }

  // ParsePropertyValue does not handle priorities correctly -- it's
  // optimized for speed.  And the priority is not part of the
  // property value anyway.... So we have to use the full-blown
  // ParseDeclaration()
  return ParseDeclaration(aPropertyName + NS_LITERAL_STRING(":") +
                          aValue + NS_LITERAL_STRING("!") + aPriority,
                          PR_TRUE, PR_FALSE);
}

NS_IMETHODIMP
nsDOMCSSDeclaration::RemoveProperty(const nsAString& aPropertyName,
                                    nsAString& aReturn)
{
  aReturn.Truncate();

  nsCSSDeclaration* decl;
  nsresult rv = GetCSSDeclaration(&decl, PR_FALSE);
  if (!decl) {
    return rv;
  }

  nsCSSProperty prop = nsCSSProps::LookupProperty(aPropertyName);
  if (prop == eCSSProperty_UNKNOWN) {
    return NS_OK;
  }

  decl->GetValue(prop, aReturn);

  rv = decl->RemoveProperty(prop);

  if (NS_SUCCEEDED(rv)) {
    rv = DeclarationChanged();
  } else {
    // RemoveProperty used to throw in all sorts of situations -- e.g.
    // if the property was a shorthand one.  Do not propagate its return
    // value to callers.  (XXX or should we propagate it again now?)
    rv = NS_OK;
  }

  return rv;
}

nsresult
nsDOMCSSDeclaration::ParsePropertyValue(const nsAString& aPropName,
                                        const nsAString& aPropValue)
{
  nsCSSDeclaration* decl;
  nsresult result = GetCSSDeclaration(&decl, PR_TRUE);
  if (!decl) {
    return result;
  }
  
  nsCOMPtr<nsICSSLoader> cssLoader;
  nsCOMPtr<nsICSSParser> cssParser;
  nsCOMPtr<nsIURI> baseURI;
  
  result = GetCSSParsingEnvironment(getter_AddRefs(baseURI),
                                    getter_AddRefs(cssLoader),
                                    getter_AddRefs(cssParser));
  if (NS_FAILED(result)) {
    return result;
  }

  PRBool changed;
  result = cssParser->ParseProperty(aPropName, aPropValue, baseURI, decl,
                                    &changed);
  if (NS_SUCCEEDED(result) && changed) {
    result = DeclarationChanged();
  }

  if (cssLoader) {
    cssLoader->RecycleParser(cssParser);
  }

  return result;
}

nsresult
nsDOMCSSDeclaration::ParseDeclaration(const nsAString& aDecl,
                                      PRBool aParseOnlyOneDecl,
                                      PRBool aClearOldDecl)
{
  nsCSSDeclaration* decl;
  nsresult result = GetCSSDeclaration(&decl, PR_TRUE);
  if (!decl) {
    return result;
  }

  nsCOMPtr<nsICSSLoader> cssLoader;
  nsCOMPtr<nsICSSParser> cssParser;
  nsCOMPtr<nsIURI> baseURI;

  result = GetCSSParsingEnvironment(getter_AddRefs(baseURI),
                                    getter_AddRefs(cssLoader),
                                    getter_AddRefs(cssParser));

  if (NS_FAILED(result)) {
    return result;
  }

  PRBool changed;
  result = cssParser->ParseAndAppendDeclaration(aDecl, baseURI, decl,
                                                aParseOnlyOneDecl,
                                                &changed,
                                                aClearOldDecl);
  
  if (NS_SUCCEEDED(result) && changed) {
    result = DeclarationChanged();
  }

  if (cssLoader) {
    cssLoader->RecycleParser(cssParser);
  }

  return result;
}


//////////////////////////////////////////////////////////////////////////////

CSS2PropertiesTearoff::CSS2PropertiesTearoff(nsIDOMCSSStyleDeclaration *aOuter)
  : mOuter(aOuter)
{
  NS_ASSERTION(mOuter, "must have outer");
}

CSS2PropertiesTearoff::~CSS2PropertiesTearoff()
{
}

NS_IMETHODIMP_(nsrefcnt)
CSS2PropertiesTearoff::AddRef(void)
{
  return mOuter->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
CSS2PropertiesTearoff::Release(void)
{
  return mOuter->Release();
}

NS_IMETHODIMP
CSS2PropertiesTearoff::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  return mOuter->QueryInterface(aIID, aInstancePtr);
}

// nsIDOMCSS2Properties
// nsIDOMNSCSS2Properties

#define CSS_PROP(name_, id_, method_, datastruct_, member_, type_, iscoord_, kwtable_) \
  NS_IMETHODIMP                                                              \
  CSS2PropertiesTearoff::Get##method_(nsAString& aValue)                     \
  {                                                                          \
    return mOuter->GetPropertyValue(NS_LITERAL_STRING(#name_), aValue);      \
  }                                                                          \
                                                                             \
  NS_IMETHODIMP                                                              \
  CSS2PropertiesTearoff::Set##method_(const nsAString& aValue)               \
  {                                                                          \
    return mOuter->SetProperty(NS_LITERAL_STRING(#name_), aValue,            \
                               EmptyString());                               \
  }

#define CSS_PROP_LIST_EXCLUDE_INTERNAL
#define CSS_PROP_NOTIMPLEMENTED(name_, id_, method_) \
  CSS_PROP(name_, id_, method_, , , , ,)
#define CSS_PROP_SHORTHAND(name_, id_, method_) \
  CSS_PROP(name_, id_, method_, , , , ,)
#include "nsCSSPropList.h"

// Aliases
CSS_PROP(opacity, X, MozOpacity, X, X, X, X, X)

#undef CSS_PROP_SHORTHAND
#undef CSS_PROP_NOTIMPLEMENTED
#undef CSS_PROP_LIST_EXCLUDE_INTERNAL
#undef CSS_PROP
