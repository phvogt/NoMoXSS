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
 * Portions created by the Initial Developer are Copyright (C) 2000
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

#include "nsIComponentManager.h"
#include "nsLanguageAtomService.h"
#include "nsICharsetConverterManager.h"
#include "nsILocaleService.h"
#include "nsXPIDLString.h"
#include "nsUnicharUtils.h"
#include "nsIServiceManager.h"

class nsLanguageAtom : public nsILanguageAtom
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILANGUAGEATOM

  nsLanguageAtom();
  virtual ~nsLanguageAtom();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  virtual void Init(const nsString& aLanguage, nsIAtom* aLangGroup);

protected:
  nsAutoString      mLang;
  nsCOMPtr<nsIAtom> mLangGroup;
};

NS_IMPL_ISUPPORTS1(nsLanguageAtom, nsILanguageAtom)

nsLanguageAtom::nsLanguageAtom()
{
}

nsLanguageAtom::~nsLanguageAtom()
{
}

void
nsLanguageAtom::Init(const nsString& aLanguage, nsIAtom* aLangGroup)
{
  mLang = aLanguage;
  mLangGroup = aLangGroup;
}

NS_IMETHODIMP
nsLanguageAtom::GetLanguage(PRUnichar** aLanguage)
{
  NS_ENSURE_ARG_POINTER(aLanguage);
  *aLanguage = nsnull;

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsLanguageAtom::GetLanguageGroup(nsIAtom** aLanguageGroup)
{
  NS_ENSURE_ARG_POINTER(aLanguageGroup);

  *aLanguageGroup = mLangGroup;
  NS_IF_ADDREF(*aLanguageGroup);

  return NS_OK;
}

NS_IMETHODIMP
nsLanguageAtom::LanguageIs(const PRUnichar* aLanguage, PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aLanguage);
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = mLang.Equals(nsDependentString(aLanguage));

  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsLanguageAtomService, nsILanguageAtomService)

nsLanguageAtomService::nsLanguageAtomService()
{
}

nsLanguageAtomService::~nsLanguageAtomService()
{
}

NS_IMETHODIMP
nsLanguageAtomService::InitLangTable()
{
  if (!mLangs) {
    NS_ENSURE_SUCCESS(NS_NewISupportsArray(getter_AddRefs(mLangs)),
                      NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLanguageAtomService::InitLangGroupTable()
{
  if (mLangGroups) return NS_OK;
  nsresult rv;
  
  nsCOMPtr<nsIStringBundleService> bundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;


  rv = bundleService->CreateBundle("resource://gre/res/langGroups.properties",
                                   getter_AddRefs(mLangGroups));
  return rv;
}

NS_IMETHODIMP
nsLanguageAtomService::LookupLanguage(const PRUnichar* aLanguage,
  nsILanguageAtom** aResult)
{
  nsresult res;
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;
  NS_ENSURE_ARG_POINTER(aLanguage);

  if (!mLangs) {
    NS_ENSURE_SUCCESS(InitLangTable(), NS_ERROR_OUT_OF_MEMORY);
  }
  nsAutoString lowered(aLanguage);
  ToLowerCase(lowered);
  nsCOMPtr<nsILanguageAtom> lang;
  PRUint32 n;
  NS_ENSURE_SUCCESS(mLangs->Count(&n), NS_ERROR_FAILURE);
  for (PRUint32 i = 0; i < n; i++) {
    res = mLangs->QueryElementAt(i, NS_GET_IID(nsILanguageAtom),
                                 getter_AddRefs(lang));
    if (NS_SUCCEEDED(res)) {
      PRBool same = PR_FALSE;
      NS_ENSURE_SUCCESS(lang->LanguageIs(lowered.get(), &same),
        NS_ERROR_FAILURE);
      if (same) {
        break;
      }
      else {
        lang = nsnull;
      }
    }
  }
  if (!lang) {
    nsLanguageAtom* language = new nsLanguageAtom();
    NS_ENSURE_TRUE(language, NS_ERROR_OUT_OF_MEMORY);
    nsXPIDLString langGroupStr;

    if (lowered.Equals(NS_LITERAL_STRING("en-us"))) {
      langGroupStr.Assign(NS_LITERAL_STRING("x-western"));
    } else if (lowered.Equals(NS_LITERAL_STRING("de-de"))) {
      langGroupStr.Assign(NS_LITERAL_STRING("x-western"));
    } else if (lowered.Equals(NS_LITERAL_STRING("ja-jp"))) {
      langGroupStr.Assign(NS_LITERAL_STRING("ja"));
    } else {
      if (!mLangGroups) {
        NS_ENSURE_SUCCESS(InitLangGroupTable(), NS_ERROR_FAILURE);
      }
      res = mLangGroups->GetStringFromName(lowered.get(), getter_Copies(langGroupStr));
      if (NS_FAILED(res)) {
        PRInt32 hyphen = lowered.FindChar('-');
        if (hyphen >= 0) {
          nsAutoString truncated(lowered);
          truncated.Truncate(hyphen);
          res = mLangGroups->GetStringFromName(truncated.get(), getter_Copies(langGroupStr));
          if (NS_FAILED(res)) {
            langGroupStr.Assign(NS_LITERAL_STRING("x-western"));
          }
        } else {
          langGroupStr.Assign(NS_LITERAL_STRING("x-western"));
        }
      }
    }
    nsCOMPtr<nsIAtom> langGroup = do_GetAtom(langGroupStr);
    language->Init(lowered, langGroup);
    lang = language;
    mLangs->AppendElement(lang);
  }
  *aResult = lang;
  NS_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsLanguageAtomService::LookupCharSet(const char* aCharSet,
                                     nsILanguageAtom** aResult)
{
  nsresult res;
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;
  NS_ENSURE_ARG_POINTER(aCharSet);

  if (!mLangs) {
    NS_ENSURE_SUCCESS(InitLangTable(), NS_ERROR_OUT_OF_MEMORY);
  }
  if (!mCharSets) {
    mCharSets = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID);
    NS_ENSURE_TRUE(mCharSets, NS_ERROR_FAILURE);
  }
  if (!mUnicode) {
    mUnicode = do_GetAtom("x-unicode");
  }

  nsCOMPtr<nsIAtom> langGroup;
  mCharSets->GetCharsetLangGroup(aCharSet,
                                 getter_AddRefs(langGroup));
  if (!langGroup) {
    return NS_ERROR_FAILURE;
  }
#if !defined(XP_BEOS)
  if (langGroup.get() == mUnicode.get()) {
    res = GetLocaleLanguageGroup(getter_AddRefs(langGroup));
    NS_ENSURE_SUCCESS(res, res);
  }
#endif
  nsCOMPtr<nsILanguageAtom> lang;
  PRUint32 n;
  NS_ENSURE_SUCCESS(mLangs->Count(&n), NS_ERROR_FAILURE);
  for (PRUint32 i = 0; i < n; i++) {
    res = mLangs->QueryElementAt(i, NS_GET_IID(nsILanguageAtom),
                                 getter_AddRefs(lang));
    if (NS_SUCCEEDED(res)) {
      nsCOMPtr<nsIAtom> group;
      NS_ENSURE_SUCCESS(lang->GetLanguageGroup(getter_AddRefs(group)),
        NS_ERROR_FAILURE);
      if (langGroup.get() == group.get()) {
        break;
      }
      else {
        lang = nsnull;
      }
    }
  }
  if (!lang) {
    nsLanguageAtom* language = new nsLanguageAtom();
    NS_ENSURE_TRUE(language, NS_ERROR_OUT_OF_MEMORY);
    nsAutoString empty;
    language->Init(empty, langGroup);
    lang = language;
    mLangs->AppendElement(lang);
  }
  *aResult = lang;
  NS_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsLanguageAtomService::GetLocaleLanguageGroup(nsIAtom** aResult)
{
  nsresult res;
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

  if (!mLocaleLangGroup) {
    nsCOMPtr<nsILocaleService> localeService;
    localeService = do_GetService(NS_LOCALESERVICE_CONTRACTID);
    NS_ENSURE_TRUE(localeService, NS_ERROR_FAILURE);
    nsCOMPtr<nsILocale> locale;
    res = localeService->GetApplicationLocale(getter_AddRefs(locale));
    NS_ENSURE_SUCCESS(res, res);
    nsAutoString category;
    category.AssignWithConversion(NSILOCALE_MESSAGE);
    nsAutoString loc;
    res = locale->GetCategory(category, loc);
    NS_ENSURE_SUCCESS(res, res);
    nsCOMPtr<nsILanguageAtom> langAtom;
    res = LookupLanguage(loc.get(), getter_AddRefs(langAtom));
    NS_ENSURE_SUCCESS(res, res);
    res = langAtom->GetLanguageGroup(getter_AddRefs(mLocaleLangGroup));
    NS_ENSURE_SUCCESS(res, res);
  }

  *aResult = mLocaleLangGroup;
  NS_ADDREF(*aResult);

  return NS_OK;
}
