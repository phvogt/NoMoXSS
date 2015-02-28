/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributors:
 *   
 *   
 */

#include <stdio.h>
#include "nsSpellCheckUtils.h"

#include "nsReadableUtils.h"
#include "nsICharsetConverterManager.h"
#include "nsIPlatformCharset.h"
#include "nsIServiceManager.h"

#include "nsISpellChecker.h"
#include "nsITextServicesDocument.h"

#include "nsIServiceManager.h"
#include "nsIWordBreakerFactory.h" // nsIWordBreaker
#include "nsLWBrkCIID.h"


/* XXX The platform-specific #defines of IS_NSBSP_CHAR are unnecessary and
 *     inaccurate. We should be doing the whitespace conversion on the UTF-16
 *     buffer before the call to aUnicodeEncoder->Convert().
 *
 * XXX Should we be converting any other whitespace characters to spaces?
 *
 *     See comments in bug 211343.
 */
#ifdef XP_MAC
#define IS_NBSP_CHAR(c) (((unsigned char)0xca)==(c))
#else
#define IS_NBSP_CHAR(c) (((unsigned char)0xa0)==(c))
#endif

nsresult
nsSpellCheckUtils::ReadStringIntoBuffer(nsIUnicodeEncoder* aUnicodeEncoder,
                                        const PRUnichar*   aStr, 
                                        CharBuffer*        aBuf)
{
  NS_ENSURE_ARG_POINTER(aUnicodeEncoder);
  NS_ENSURE_ARG_POINTER(aBuf);

  if (!aStr || !*aStr) {
    return NS_OK;
  }

  aBuf->mDataLength = 0;

  PRInt32 unicodeLength = nsCRT::strlen(aStr);

  // Estimate a string length after the conversion.
  PRInt32 estimatedLength, stringLength;
  nsresult result = aUnicodeEncoder->GetMaxLength(aStr, unicodeLength, &estimatedLength);
  NS_ENSURE_SUCCESS(result, result);

  result = aBuf->AssureCapacity(estimatedLength + 1);
  NS_ENSURE_SUCCESS(result, result);

  // Convert from unicode.
  stringLength = estimatedLength;
  result = aUnicodeEncoder->Convert(aStr, &unicodeLength, aBuf->mData, &stringLength);
  NS_ENSURE_SUCCESS(result, result);

  // Terminate the conversion (e.g. put escape sequence for JIS).
  PRInt32 finLen = estimatedLength - stringLength;
  if (finLen)
  {
    NS_ENSURE_TRUE(finLen > 0, NS_ERROR_FAILURE);
    result = aUnicodeEncoder->Finish(&aBuf->mData[stringLength], &finLen);
    NS_ENSURE_SUCCESS(result, result);
  }

  aBuf->mDataLength = stringLength + finLen;
  aBuf->mData[aBuf->mDataLength] = '\0';
  for (unsigned char* p = (unsigned char*) aBuf->mData; *p ; p++)
    if( IS_NBSP_CHAR(*p) )  
      *p = ' '; 

  return NS_OK;
}

nsresult
nsSpellCheckUtils::ReadStringIntoBuffer(nsIUnicodeEncoder* aUnicodeEncoder,
                                        const nsString*    aStr, 
                                        CharBuffer*        aBuf)
{
  NS_ENSURE_ARG_POINTER(aUnicodeEncoder);
  NS_ENSURE_ARG_POINTER(aStr);
  NS_ENSURE_ARG_POINTER(aBuf);
  return ReadStringIntoBuffer(aUnicodeEncoder, aStr->get(), aBuf);
}

nsresult
nsSpellCheckUtils::CreateUnicodeConverters(const PRUnichar*    aCharset,
                                           nsIUnicodeEncoder** aUnicodeEncoder,
                                           nsIUnicodeDecoder** aUnicodeDecoder)
{
  NS_ENSURE_ARG_POINTER(aCharset);
  NS_ENSURE_ARG_POINTER(aUnicodeEncoder);
  NS_ENSURE_ARG_POINTER(aUnicodeDecoder);

  nsresult rv;

  nsCOMPtr <nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_LossyConvertUCS2toASCII charset(aCharset);

  rv = ccm->GetUnicodeDecoder(charset.get(), aUnicodeDecoder);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ccm->GetUnicodeEncoder(charset.get(), aUnicodeEncoder);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the error behavior, in case a character cannot be mapped.
  rv = (*aUnicodeEncoder)->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, ' ');

  return rv;
}

nsresult
nsSpellCheckUtils::LoadTextBlockIntoBuffer(nsITextServicesDocument* aTxtSvcDoc,
                                           nsISpellChecker*        aSpellChecker,
                                           CharBuffer&             aCharBuf,
                                           nsString&               aText, 
                                           PRUint32&               aOffset)
{
  NS_ENSURE_ARG_POINTER(aTxtSvcDoc);
  NS_ENSURE_ARG_POINTER(aSpellChecker);

  nsCOMPtr<nsIUnicodeEncoder> unicodeEncoder;
  nsXPIDLString charSet;
  nsresult result = aSpellChecker->GetCharset(getter_Copies(charSet));
  if (NS_SUCCEEDED(result)) 
  {
    nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder;
    result = nsSpellCheckUtils::CreateUnicodeConverters(charSet, 
                                                        getter_AddRefs(unicodeEncoder), 
                                                        getter_AddRefs(unicodeDecoder));
    NS_ENSURE_TRUE(unicodeEncoder, NS_ERROR_NULL_POINTER);
  }

  if (aCharBuf.mData)
    aCharBuf.mData[0]  = '\0';

  nsString str;
  aCharBuf.mDataLength = 0;

  result = aTxtSvcDoc->GetCurrentTextBlock(&str);
  NS_ENSURE_SUCCESS(result, result);

  result = nsSpellCheckUtils::ReadStringIntoBuffer(unicodeEncoder, &str, &aCharBuf);
  NS_ENSURE_SUCCESS(result, result);

  if (aCharBuf.mDataLength < 1)
  {
    // The document could be empty, so return
    // NS_OK!
    return NS_OK;
  }

  aText.AssignWithConversion(aCharBuf.mData);

  return NS_OK;
}

nsresult 
nsSpellCheckUtils::GetWordBreaker(nsIWordBreaker** aResult) 
{
  NS_ENSURE_ARG_POINTER(aResult);

  // no line breaker, find a default one
  nsresult result;
  nsCOMPtr<nsIWordBreakerFactory> wbf(do_GetService(NS_LWBRK_CONTRACTID, &result));
  if (NS_SUCCEEDED(result)) 
  {
    nsAutoString wbarg;
    result = wbf->GetBreaker(wbarg, aResult);
    NS_IF_ADDREF(*aResult);
  }
  return result;
}

#ifdef NS_DEBUG
nsresult
nsSpellCheckUtils::DumpWords(nsIWordBreaker* aWordBreaker, 
                             const PRUnichar*      aText, 
                             const PRUint32&       aTextLen)
{
  PRUint32 offset = 0;
  PRUint32 wlen   = 0;
  for (int i=0;i<7;i++) printf("**********");
  printf("\n");
  for (i=0;i<7;i++) printf("0123456789");
  printf("\n");
  char* line = strdup(NS_LossyConvertUCS2toASCII(aText).get());
  for (i=0;i<aTextLen;i++)
    if (line[i] < 32) 
      putc('_', stdout);
    else 
      putc(line[i], stdout);
  printf("\n");
  //printf("%s\n", NS_LossyConvertUCS2toASCII(aText).get());
  free(line);

  while (offset < aTextLen) 
  {
    PRUint32 begin = 0;
    PRUint32 end   = 0;
    nsresult result = aWordBreaker->FindWord(aText, aTextLen, offset, &begin, &end);
    NS_ENSURE_SUCCESS(result, result);
    wlen = end - begin;
    printf("%d  %d  l:%d ", begin, end, wlen);
    const PRUnichar* start = (const PRUnichar*)(aText+offset);
    PRUnichar* word = nsCRT::strndup(start, wlen);
    nsString str(word);
    printf("[%s]\n", NS_LossyConvertUCS2toASCII(str).get());
    nsMemory::Free(word);
    offset = end;
  }
  for (i=0;i<7;i++) printf("**********");
  printf("\n");
  return NS_OK;
}
#endif
