/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): 
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date             Modified by     Description of modification
 *
 * 07/05/2000       IBM Corp.      renamed file to nsOS2Locale.cpp from ns LocaleOS2.cpp
 *                                 added implementation for GetPlatformLocale, and GetXPlatformLocale using Unix model
 *                                 created ParseLocaleString method
 */

#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include "nsILocale.h"
#include "nsOS2Locale.h"
#include "nsLocaleCID.h"
#include "prprf.h"
#include "nsReadableUtils.h"
#include <unidef.h>

extern "C" {
#include <callconv.h>
int APIENTRY UniQueryLocaleValue ( const LocaleObject locale_object,
                                   LocaleItem item,
                                  int *info_item);
}

/* nsOS2Locale ISupports */
NS_IMPL_ISUPPORTS1(nsOS2Locale,nsIOS2Locale)

nsOS2Locale::nsOS2Locale(void)
{
}

nsOS2Locale::~nsOS2Locale(void)
{

}

/* Workaround for GCC problem */
#ifndef LOCI_sName
#define LOCI_sName ((LocaleItem)100)
#endif

NS_IMETHODIMP 
nsOS2Locale::GetPlatformLocale(const nsAString& locale, PULONG os2Codepage)
{
  LocaleObject locObj = NULL;
  int codePage;
  nsAutoString tempLocale(locale);
  tempLocale.ReplaceChar('-', '_');

 
  int  ret = UniCreateLocaleObject(UNI_UCS_STRING_POINTER, (UniChar *)PromiseFlatString(tempLocale).get(), &locObj);
  if (ret != ULS_SUCCESS)
    UniCreateLocaleObject(UNI_UCS_STRING_POINTER, (UniChar *)L"C", &locObj);

  ret = UniQueryLocaleValue(locObj, LOCI_iCodepage, &codePage);
  if (ret != ULS_SUCCESS)
    return NS_ERROR_FAILURE;

  if (codePage == 437) {
    *os2Codepage = 850;
  } else {
    *os2Codepage = codePage;
  }
  UniFreeLocaleObject(locObj);

  return NS_OK;
}

NS_IMETHODIMP
nsOS2Locale::GetXPLocale(const char* os2Locale, nsAString& locale)
{
  char  country_code[3];
  char  lang_code[3];
  char  extra[65];
  char  os2_locale[128];

  if (os2Locale!=nsnull) {
    if (strcmp(os2Locale,"C")==0 || strcmp(os2Locale,"OS2")==0) {
      locale.Assign(NS_LITERAL_STRING("en-US"));
      return NS_OK;
    }
    if (!ParseLocaleString(os2Locale,lang_code,country_code,extra,'_')) {
//      * locale = "x-user-defined";
      // use os2 if parse failed
      CopyASCIItoUTF16(nsDependentCString(os2Locale), locale);  
      return NS_OK;
    }

    if (*country_code) {
      if (*extra) {
        PR_snprintf(os2_locale,128,"%s-%s.%s",lang_code,country_code,extra);
      }
      else {
        PR_snprintf(os2_locale,128,"%s-%s",lang_code,country_code);
      }
    } 
    else {
      if (*extra) {
        PR_snprintf(os2_locale,128,"%s.%s",lang_code,extra);
      }
      else {
        PR_snprintf(os2_locale,128,"%s",lang_code);
      }
    }

    CopyASCIItoUTF16(nsDependentCString(os2_locale), locale);  
    return NS_OK;

  }

    return NS_ERROR_FAILURE;

}

//
// returns PR_FALSE/PR_TRUE depending on if it was of the form LL-CC.Extra
PRBool
nsOS2Locale::ParseLocaleString(const char* locale_string, char* language, char* country, char* extra, char separator)
{
  PRUint32 len = strlen(locale_string);

  *language = '\0';
  *country = '\0';
  *extra = '\0';

  if (2 == len) {
    language[0]=locale_string[0];
    language[1]=locale_string[1];
    language[2]='\0';
    country[0]='\0';
  } 
  else if (5 == len) {
    language[0]=locale_string[0];
    language[1]=locale_string[1];
    language[2]='\0';
    country[0]=locale_string[3];
    country[1]=locale_string[4];
    country[2]='\0';
  }
  else if (4 <= len && '.' == locale_string[2]) {
    strcpy(extra, &locale_string[3]);
    language[0]=locale_string[0];
    language[1]=locale_string[1];
    language[2]='\0';
    country[0]=locale_string[3];
    country[1]=locale_string[4];
    country[2]='\0';
  }
  else if (7 <= len && '.' == locale_string[5]) {
    strcpy(extra, &locale_string[6]);
    language[0]=locale_string[0];
    language[1]=locale_string[1];
    language[2]='\0';
    country[0]=locale_string[3];
    country[1]=locale_string[4];
    country[2]='\0';
  }
  else {
    return PR_FALSE;
  }

  return PR_TRUE;
}
