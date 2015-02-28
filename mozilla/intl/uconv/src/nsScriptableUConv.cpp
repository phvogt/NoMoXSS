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
 *   Makoto Kato <m_kato@ga2.so-net.ne.jp >
 *   Ryoichi Furukawa <oliver@1000cp.com>
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

#include "pratom.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIScriptableUConv.h"
#include "nsScriptableUConv.h"
#include "nsCRT.h"

#include "nsIPlatformCharset.h"

static PRInt32          gInstanceCount = 0;

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsScriptableUnicodeConverter, nsIScriptableUnicodeConverter)

nsScriptableUnicodeConverter::nsScriptableUnicodeConverter()
{
  PR_AtomicIncrement(&gInstanceCount);
}

nsScriptableUnicodeConverter::~nsScriptableUnicodeConverter()
{
  PR_AtomicDecrement(&gInstanceCount);
}

/* string ConvertFromUnicode ([const] in wstring src); */
NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertFromUnicode(const PRUnichar *aSrc, char **_retval)
{
  if (!mEncoder)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  PRInt32 inLength = nsCRT::strlen(aSrc);
  PRInt32 outLength;
  rv = mEncoder->GetMaxLength(aSrc, inLength, &outLength);
  if (NS_SUCCEEDED(rv)) {
    *_retval = (char*) nsMemory::Alloc(outLength+1);
    if (!*_retval)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = mEncoder->Convert(aSrc, &inLength, *_retval, &outLength);
    if (NS_SUCCEEDED(rv))
    {
      (*_retval)[outLength] = '\0';
      return NS_OK;
    }
    nsMemory::Free(*_retval);
  }
  *_retval = nsnull;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsScriptableUnicodeConverter::Finish(char **_retval)
{
  if (!mEncoder)
    return NS_ERROR_FAILURE;

  PRInt32 finLength = 32;

  *_retval = (char *) nsMemory::Alloc(finLength);
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = mEncoder->Finish(*_retval, &finLength);
  if (NS_SUCCEEDED(rv))
    (*_retval)[finLength] = '\0';
  else
    nsMemory::Free(*_retval);

  return rv;

}

/* wstring ConvertToUnicode ([const] in string src); */
NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertToUnicode(const char *aSrc, PRUnichar **_retval)
{
  if (!mDecoder)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  PRInt32 inLength = strlen(aSrc);
  PRInt32 outLength;
  rv = mDecoder->GetMaxLength(aSrc, inLength, &outLength);
  if (NS_SUCCEEDED(rv))
  {
    *_retval = (PRUnichar*) nsMemory::Alloc((outLength+1)*sizeof(PRUnichar));
    if (!*_retval)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = mDecoder->Convert(aSrc, &inLength, *_retval, &outLength);
    if (NS_SUCCEEDED(rv))
    {
      (*_retval)[outLength] = 0;
      return NS_OK;
    }
    nsMemory::Free(*_retval);
  }
  *_retval = nsnull;
  return NS_ERROR_FAILURE;
}

/* attribute wstring charset; */
NS_IMETHODIMP
nsScriptableUnicodeConverter::GetCharset(char * *aCharset)
{
  *aCharset = ToNewCString(mCharset);
  if (!*aCharset)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
nsScriptableUnicodeConverter::SetCharset(const char * aCharset)
{
  mCharset.Assign(aCharset);
  return InitConverter();
}

nsresult
nsScriptableUnicodeConverter::InitConverter()
{
  nsresult rv = NS_OK;
  mEncoder = NULL ;

  nsCOMPtr<nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);

  if (NS_SUCCEEDED( rv) && (nsnull != ccm)) {
    // get charset atom due to getting unicode converter
    
    // get an unicode converter
    rv = ccm->GetUnicodeEncoder(mCharset.get(), getter_AddRefs(mEncoder));
    if(NS_SUCCEEDED(rv)) {
      rv = mEncoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, (PRUnichar)'?');
      if(NS_SUCCEEDED(rv)) {
        rv = ccm->GetUnicodeDecoder(mCharset.get(), getter_AddRefs(mDecoder));
      }
    }
  }

  return rv ;
}
