/* -*- Mode: C++; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Contributor(s):
 *   Scott MacGregor <mscott@netscape.com>
 */

#include "nsOSHelperAppService.h"
#include "nsISupports.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIURL.h"
#include "nsIMIMEInfo.h"
#include "nsMIMEInfoWin.h"
#include "nsMimeTypes.h"
#include "nsILocalFile.h"
#include "nsIProcess.h"
#include "plstr.h"
#include "nsAutoPtr.h"
#include "nsNativeCharsetUtils.h"

// we need windows.h to read out registry information...
#include <windows.h>

// shellapi.h is needed to build with WIN32_LEAN_AND_MEAN
#include <shellapi.h>

#define LOG(args) PR_LOG(mLog, PR_LOG_DEBUG, args)

// helper methods: forward declarations...
static BYTE * GetValueBytes( HKEY hKey, const char *pValueName, DWORD *pLen=0);
static nsresult GetExtensionFrom4xRegistryInfo(const char * aMimeType, nsCString& aFileExtension);
static nsresult GetExtensionFromWindowsMimeDatabase(const char * aMimeType, nsCString& aFileExtension);

// static member
PRBool nsOSHelperAppService::mIsNT = PR_FALSE;

nsOSHelperAppService::nsOSHelperAppService() : nsExternalHelperAppService()
{
  OSVERSIONINFO osversion;
  ::ZeroMemory(&osversion, sizeof(OSVERSIONINFO));
  osversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  if (!GetVersionEx(&osversion)) {
    // If the call failed, better be safe and assume *W functions don't work
    mIsNT = PR_FALSE;
  }
  else {
    mIsNT = (osversion.dwPlatformId == VER_PLATFORM_WIN32_NT);
  }
}

nsOSHelperAppService::~nsOSHelperAppService()
{}

// The windows registry provides a mime database key which lists a set of mime types and corresponding "Extension" values. 
// we can use this to look up our mime type to see if there is a preferred extension for the mime type.
static nsresult GetExtensionFromWindowsMimeDatabase(const char * aMimeType, nsCString& aFileExtension)
{
   HKEY hKey;
   nsCAutoString mimeDatabaseKey ("MIME\\Database\\Content Type\\");

   mimeDatabaseKey += aMimeType;
   
   LONG err = ::RegOpenKeyEx( HKEY_CLASSES_ROOT, mimeDatabaseKey.get(), 0, KEY_QUERY_VALUE, &hKey);
   if (err == ERROR_SUCCESS)
   {
      LPBYTE pBytes = GetValueBytes( hKey, "Extension");
      if (pBytes)
      { 
        aFileExtension = (char * )pBytes;
      }

      delete[] pBytes;

      ::RegCloseKey(hKey);
   }

   return NS_OK;
}

// We have a serious problem!! I have this content type and the windows registry only gives me
// helper apps based on extension. Right now, we really don't have a good place to go for 
// trying to figure out the extension for a particular mime type....One short term hack is to look
// this information in 4.x (it's stored in the windows regsitry). 
static nsresult GetExtensionFrom4xRegistryInfo(const char * aMimeType, nsCString& aFileExtension)
{
   nsCAutoString command ("Software\\Netscape\\Netscape Navigator\\Suffixes");
   nsresult rv = NS_OK;
   HKEY hKey;
   LONG err = ::RegOpenKeyEx( HKEY_CURRENT_USER, command.get(), 0, KEY_QUERY_VALUE, &hKey);
   if (err == ERROR_SUCCESS)
   {
      LPBYTE pBytes = GetValueBytes( hKey, aMimeType);
      if (pBytes) // only try to get the extension if we have a value!
      {
        aFileExtension = ".";
        aFileExtension.Append( (char *) pBytes);
      
        // this may be a comma separate list of extensions...just take the first one
        // for now...

        PRInt32 pos = aFileExtension.FindChar(',');
        if (pos > 0) // we have a comma separated list of languages...
          aFileExtension.Truncate(pos); // truncate everything after the first comma (including the comma)
      }
   
      delete [] pBytes;
      // close the key
      ::RegCloseKey(hKey);
   }
   else
     rv = NS_ERROR_FAILURE; // no 4.x extension mapping found!

   return rv;
}

static BYTE * GetValueBytes( HKEY hKey, const char *pValueName, DWORD *pLen)
{
  LONG err;
  DWORD bufSz;
  LPBYTE pBytes = NULL;

  err = ::RegQueryValueEx( hKey, pValueName, NULL, NULL, NULL, &bufSz); 
  if (err == ERROR_SUCCESS) {
    pBytes = new BYTE[bufSz];
    err = ::RegQueryValueEx( hKey, pValueName, NULL, NULL, pBytes, &bufSz);
    if (err != ERROR_SUCCESS) {
      delete [] pBytes;
      pBytes = NULL;
    } else {
      // Return length if caller wanted it.
      if ( pLen ) {
        *pLen = bufSz;
      }
    }
  }

  return( pBytes);
}

/* static */
PRBool nsOSHelperAppService::GetValueString(HKEY hKey, PRUnichar* pValueName, nsAString& result)
{
  if (!mIsNT) {
    nsCAutoString cValueName;
    if (pValueName)
      NS_CopyUnicodeToNative(nsDependentString(pValueName), cValueName);
    char* pBytes = (char*)GetValueBytes(hKey, cValueName.get());
    if (pBytes) {
      nsresult rv = NS_CopyNativeToUnicode(nsDependentCString(pBytes), result);
      delete[] pBytes;
      return NS_SUCCEEDED(rv);
    }
    return PR_FALSE;
  }

  DWORD bufSz;
  LONG err = ::RegQueryValueExW( hKey, pValueName, NULL, NULL, NULL, &bufSz); 
  if (err == ERROR_SUCCESS) {
    PRUnichar* pBytes = new PRUnichar[bufSz];
    if (!pBytes)
      return PR_FALSE;

    err = ::RegQueryValueExW( hKey, pValueName, NULL, NULL, (BYTE*)pBytes, &bufSz);
    if (err != ERROR_SUCCESS) {
      delete [] pBytes;
      return PR_FALSE;
    } else {
      result.Assign(pBytes);
      delete [] pBytes;
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

NS_IMETHODIMP nsOSHelperAppService::ExternalProtocolHandlerExists(const char * aProtocolScheme, PRBool * aHandlerExists)
{
  // look up the protocol scheme in the windows registry....if we find a match then we have a handler for it...
  *aHandlerExists = PR_FALSE;
  if (aProtocolScheme && *aProtocolScheme)
  {
     HKEY hKey;
     LONG err = ::RegOpenKeyEx( HKEY_CLASSES_ROOT, aProtocolScheme, 0, KEY_QUERY_VALUE, &hKey);
     if (err == ERROR_SUCCESS)
     {
       *aHandlerExists = PR_TRUE;
       // close the key
       ::RegCloseKey(hKey);
     }
  }

  return NS_OK;
}

// this implementation was pretty much copied verbatime from Tony Robinson's code in nsExternalProtocolWin.cpp

NS_IMETHODIMP nsOSHelperAppService::LoadUrl(nsIURI * aURL)
{
  nsresult rv = NS_OK;

  // 1. Find the default app for this protocol
  // 2. Set up the command line
  // 3. Launch the app.

  // For now, we'll just cheat essentially, check for the command line
  // then just call ShellExecute()!

  if (aURL)
  {
    // extract the url spec from the url
    nsCAutoString urlSpec;
    aURL->GetAsciiSpec(urlSpec);

    // Some versions of windows (Win2k before SP3, Win XP before SP1)
    // crash in ShellExecute on long URLs (bug 161357).
    // IE 5 and 6 support URLS of 2083 chars in length, 2K is safe
    const PRUint32 maxSafeURL(2048);
    if (urlSpec.Length() > maxSafeURL)
      return NS_ERROR_FAILURE;

    LONG r = (LONG) ::ShellExecute( NULL, "open", urlSpec.get(), NULL, NULL, SW_SHOWNORMAL);
    rv = NS_OK;
  }

  return rv;
}

// GetMIMEInfoFromRegistry: This function obtains the values of some of the nsIMIMEInfo
// attributes for the mimeType/extension associated with the input registry key.  The default
// entry for that key is the name of a registry key under HKEY_CLASSES_ROOT.  The default
// value for *that* key is the descriptive name of the type.  The EditFlags value is a binary
// value; the low order bit of the third byte of which indicates that the user does not need
// to be prompted.
//
// This function sets only the Description attribute of the input nsIMIMEInfo.
/* static */
nsresult nsOSHelperAppService::GetMIMEInfoFromRegistry( const nsAFlatString& fileType, nsIMIMEInfo *pInfo )
{
    nsresult rv = NS_OK;

    NS_ENSURE_ARG( pInfo );

    // Get registry key for the pointed-to "file type."
    HKEY fileTypeKey = 0;
    LONG rc;
    if (mIsNT) {
      rc = ::RegOpenKeyExW( HKEY_CLASSES_ROOT, fileType.get(), 0, KEY_QUERY_VALUE, &fileTypeKey );
    }
    else {
      nsCAutoString ansiKey;
      rv = NS_CopyUnicodeToNative(fileType, ansiKey);
      if (NS_FAILED(rv))
        return rv;

      rc = ::RegOpenKeyEx( HKEY_CLASSES_ROOT, ansiKey.get(), 0, KEY_QUERY_VALUE, &fileTypeKey );
    }
    if ( rc != ERROR_SUCCESS )
        return NS_ERROR_FAILURE;

    // OK, the default value here is the description of the type.
    nsAutoString description;
    PRBool found = GetValueString(fileTypeKey, NULL, description);
    if (found)
        pInfo->SetDescription(description.get());
    ::RegCloseKey(fileTypeKey);

    return NS_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// method overrides used to gather information from the windows registry for
// various mime types. 
////////////////////////////////////////////////////////////////////////////////////////////////

/// Looks up the type for the extension aExt and compares it to aType
static PRBool typeFromExtEquals(const char *aExt, const char *aType)
{
  if (!aType)
    return PR_FALSE;
  nsCAutoString fileExtToUse;
  if (aExt[0] != '.')
    fileExtToUse = '.';

  fileExtToUse.Append(aExt);

  HKEY hKey;
  PRBool eq = PR_FALSE;
  LONG err = ::RegOpenKeyEx( HKEY_CLASSES_ROOT, fileExtToUse.get(), 0, KEY_QUERY_VALUE, &hKey);
  if (err == ERROR_SUCCESS)
  {
     LPBYTE pBytes = GetValueBytes(hKey, "Content Type");
     if (pBytes)
     {
       eq = strcmp((const char *)pBytes, aType) == 0;
       delete[] pBytes;
     }
     ::RegCloseKey(hKey);
  }
  return eq;
}

already_AddRefed<nsMIMEInfoWin> nsOSHelperAppService::GetByExtension(const char *aFileExt, const char *aTypeHint)
{
  if (!aFileExt || !*aFileExt)
    return nsnull;

  // windows registry assumes your file extension is going to include the '.'.
  // so make sure it's there...
  nsCAutoString fileExtToUse;
  if (*aFileExt != '.')
    fileExtToUse = '.';

  fileExtToUse.Append(aFileExt);

  // o.t. try to get an entry from the windows registry.
  HKEY hKey;
  LONG err = ::RegOpenKeyEx( HKEY_CLASSES_ROOT, fileExtToUse.get(), 0, KEY_QUERY_VALUE, &hKey);
  if (err == ERROR_SUCCESS)
  {
    nsCAutoString typeToUse;
    if (aTypeHint) {
      typeToUse.Assign(aTypeHint);
    }
    else {
      LPBYTE pBytes = GetValueBytes( hKey, "Content Type");
      if (pBytes)
        typeToUse.Assign((const char*)pBytes);
      delete [] pBytes;
    }
    nsAutoString description;
    PRBool found = GetValueString(hKey, NULL, description);

    nsMIMEInfoWin* mimeInfo = new nsMIMEInfoWin();
    if (mimeInfo)
    {
      NS_ADDREF(mimeInfo);
      if (!typeToUse.IsEmpty())
        mimeInfo->SetMIMEType(typeToUse.get());
      // don't append the '.'
      mimeInfo->AppendExtension(fileExtToUse.get() + 1);

      mimeInfo->SetPreferredAction(nsIMIMEInfo::useSystemDefault);

      nsAutoString visibleDesc(description);
      PRInt32 pos = visibleDesc.FindChar('.');
      if (pos > 0) 
        visibleDesc.Truncate(pos); 
      // the format of the description usually looks like appname.version.something.
      // for now, let's try to make it pretty and just show you the appname.

      mimeInfo->SetDefaultDescription(visibleDesc.get());

      // Get other nsIMIMEInfo fields from registry, if possible.
      if ( found )
      {
          GetMIMEInfoFromRegistry( description, mimeInfo );
      }
    }
    else {
      NS_IF_RELEASE(mimeInfo); // we failed to really find an entry in the registry
    }

    // close the key
    ::RegCloseKey(hKey);

    return mimeInfo;
  }

  // we failed to find a mime type.
  return nsnull;
}

already_AddRefed<nsIMIMEInfo> nsOSHelperAppService::GetMIMEInfoFromOS(const char *aMIMEType, const char *aFileExt, PRBool *aFound)
{
  *aFound = PR_TRUE;
  nsCAutoString fileExtension;
  /* XXX The strcasecmp is a gross hack to wallpaper over the most common Win32
   * extension issues caused by the fix for bug 116938.  See bug
   * 120327, comment 271 for why this is needed.  Not even sure we
   * want to remove this once we have fixed all this stuff to work
   * right; any info we get from the OS on this type is pretty much
   * useless....
   * We'll do extension-based lookup for this type later in this function.
   */
  if (aMIMEType && *aMIMEType && PL_strcasecmp(aMIMEType, APPLICATION_OCTET_STREAM) != 0) {
    // (1) try to use the windows mime database to see if there is a mapping to a file extension
    // (2) try to see if we have some left over 4.x registry info we can peek at...
    GetExtensionFromWindowsMimeDatabase(aMIMEType, fileExtension);
    LOG(("Windows mime database: extension '%s'\n", fileExtension.get()));
    if (fileExtension.IsEmpty()) {
      GetExtensionFrom4xRegistryInfo(aMIMEType, fileExtension);
      LOG(("4.x Registry: extension '%s'\n", fileExtension.get()));
    }
  }
  // If we found an extension for the type, do the lookup
  nsMIMEInfoWin* mi = nsnull;
  if (!fileExtension.IsEmpty())
    mi = GetByExtension(fileExtension.get(), aMIMEType).get();
  LOG(("Extension lookup on '%s' found: 0x%p\n", fileExtension.get(), mi));

  PRBool hasDefault = PR_FALSE;
  if (mi) {
    mi->GetHasDefaultHandler(&hasDefault);
    // OK. We might have the case that |aFileExt| is a valid extension for the
    // mimetype we were given. In that case, we do want to append aFileExt
    // to the mimeinfo that we have. (E.g.: We are asked for video/mpeg and
    // .mpg, but the primary extension for video/mpeg is .mpeg. But because
    // .mpg is an extension for video/mpeg content, we want to append it)
    if (aFileExt && *aFileExt && typeFromExtEquals(aFileExt, aMIMEType)) {
      LOG(("Appending extension '%s' to mimeinfo, because its mimetype is '%s'\n",
           aFileExt, aMIMEType));
      PRBool extExist = PR_FALSE;
      mi->ExtensionExists(aFileExt, &extExist);
      if (!extExist)
        mi->AppendExtension(aFileExt);
    }
  }
  if (!mi || !hasDefault) {
    nsRefPtr<nsMIMEInfoWin> miByExt = GetByExtension(aFileExt, aMIMEType);
    LOG(("Ext. lookup for '%s' found 0x%p\n", aFileExt, miByExt.get()));
    if (!miByExt && mi)
      return mi;
    if (miByExt && !mi) {
      miByExt.swap(mi);
      return mi;
    }
    if (!miByExt && !mi) {
      *aFound = PR_FALSE;
      mi = new nsMIMEInfoWin();
      if (mi) {
        NS_ADDREF(mi);
        if (aMIMEType && *aMIMEType)
          mi->SetMIMEType(aMIMEType);
        if (aFileExt && *aFileExt)
          mi->AppendExtension(aFileExt);
      }
      
      return mi;
    }

    // if we get here, mi has no default app. copy from extension lookup.
    nsCOMPtr<nsIFile> defaultApp;
    nsXPIDLString desc;
    miByExt->GetDefaultDescription(getter_Copies(desc));

    mi->SetDefaultDescription(desc.get());
  }
  return mi;
}
