/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsJARURI.h"
#include "nsNetUtil.h"
#include "nsIIOService.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIZipReader.h"
#include "nsReadableUtils.h"
#include "nsURLHelper.h"
#include "nsStandardURL.h"

////////////////////////////////////////////////////////////////////////////////
 
nsJARURI::nsJARURI()
{
}
 
nsJARURI::~nsJARURI()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS4(nsJARURI, nsIJARURI, nsIURL, nsIURI, nsISerializable)

nsresult
nsJARURI::Init(const char *charsetHint)
{
    mCharsetHint = charsetHint;
    return NS_OK;
}

#define NS_JAR_SCHEME           NS_LITERAL_CSTRING("jar:")
#define NS_JAR_DELIMITER        NS_LITERAL_CSTRING("!/")
#define NS_BOGUS_ENTRY_SCHEME   NS_LITERAL_CSTRING("x:///")

// FormatSpec takes the entry spec (including the "x:///" at the
// beginning) and gives us a full JAR spec.
nsresult
nsJARURI::FormatSpec(const nsACString &entrySpec, nsACString &result,
                     PRBool aIncludeScheme)
{
    // The entrySpec MUST start with "x:///"
    NS_ASSERTION(StringBeginsWith(entrySpec, NS_BOGUS_ENTRY_SCHEME),
                 "bogus entry spec");

    nsCAutoString fileSpec;
    nsresult rv = mJARFile->GetSpec(fileSpec);
    if (NS_FAILED(rv)) return rv;

    if (aIncludeScheme)
        result = NS_JAR_SCHEME;
    else
        result.Truncate();

    result.Append(fileSpec + NS_JAR_DELIMITER +
                  Substring(entrySpec, 5, entrySpec.Length() - 5));
    return NS_OK;
}

nsresult
nsJARURI::CreateEntryURL(const nsACString& entryFilename,
                         const char* charset,
                         nsIURL** url)
{
    *url = nsnull;
    
    nsStandardURL* stdURL = new nsStandardURL();
    if (!stdURL) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    // Flatten the concatenation, just in case.  See bug 128288
    nsCAutoString spec(NS_BOGUS_ENTRY_SCHEME + entryFilename);
    nsresult rv = stdURL->Init(nsIStandardURL::URLTYPE_NO_AUTHORITY, -1,
                               spec, charset, nsnull);
    if (NS_FAILED(rv)) {
        delete stdURL;
        return rv;
    }

    NS_ADDREF(*url = stdURL);
    return NS_OK;
}
    
////////////////////////////////////////////////////////////////////////////////
// nsISerializable methods:

NS_IMETHODIMP
nsJARURI::Read(nsIObjectInputStream* aStream)
{
    NS_NOTREACHED("nsJARURI::Read");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsJARURI::Write(nsIObjectOutputStream* aStream)
{
    NS_NOTREACHED("nsJARURI::Write");
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// nsIURI methods:

NS_IMETHODIMP
nsJARURI::GetSpec(nsACString &aSpec)
{
    nsCAutoString entrySpec;
    mJAREntry->GetSpec(entrySpec);
    return FormatSpec(entrySpec, aSpec);
}

NS_IMETHODIMP
nsJARURI::SetSpec(const nsACString &aSpec)
{
    nsresult rv;
    nsCOMPtr<nsIIOService> ioServ(do_GetIOService(&rv));
    if (NS_FAILED(rv)) return rv;

    nsCAutoString scheme;
    rv = net_ExtractURLScheme(aSpec, nsnull, nsnull, &scheme);
    if (NS_FAILED(rv)) return rv;

    if (strcmp("jar", scheme.get()) != 0)
        return NS_ERROR_MALFORMED_URI;

    // Search backward from the end for the "!/" delimiter. Remember, jar URLs
    // can nest, e.g.:
    //    jar:jar:http://www.foo.com/bar.jar!/a.jar!/b.html
    // This gets the b.html document from out of the a.jar file, that's 
    // contained within the bar.jar file.

    nsACString::const_iterator begin, end, delim_begin, delim_end;
    aSpec.BeginReading(begin);
    aSpec.EndReading(end);

    delim_begin = begin;
    delim_end = end;

    if (!RFindInReadable(NS_JAR_DELIMITER, delim_begin, delim_end))
        return NS_ERROR_MALFORMED_URI;

    begin.advance(4);

    rv = ioServ->NewURI(Substring(begin, delim_begin), mCharsetHint.get(), nsnull, getter_AddRefs(mJARFile));
    if (NS_FAILED(rv)) return rv;

    // skip over any extra '/' chars
    while (*delim_end == '/')
        ++delim_end;

    return SetJAREntry(Substring(delim_end, end));
}

NS_IMETHODIMP
nsJARURI::GetPrePath(nsACString &prePath)
{
    prePath = NS_JAR_SCHEME;
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::GetScheme(nsACString &aScheme)
{
    aScheme = "jar";
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::SetScheme(const nsACString &aScheme)
{
    // doesn't make sense to set the scheme of a jar: URL
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetUserPass(nsACString &aUserPass)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::SetUserPass(const nsACString &aUserPass)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetUsername(nsACString &aUsername)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::SetUsername(const nsACString &aUsername)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetPassword(nsACString &aPassword)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::SetPassword(const nsACString &aPassword)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetHostPort(nsACString &aHostPort)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::SetHostPort(const nsACString &aHostPort)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetHost(nsACString &aHost)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::SetHost(const nsACString &aHost)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetPort(PRInt32 *aPort)
{
    return NS_ERROR_FAILURE;
}
 
NS_IMETHODIMP
nsJARURI::SetPort(PRInt32 aPort)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetPath(nsACString &aPath)
{
    nsCAutoString entrySpec;
    mJAREntry->GetSpec(entrySpec);
    return FormatSpec(entrySpec, aPath, PR_FALSE);
}

NS_IMETHODIMP
nsJARURI::SetPath(const nsACString &aPath)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetAsciiSpec(nsACString &aSpec)
{
    // XXX Shouldn't this like... make sure it returns ASCII or something?
    return GetSpec(aSpec);
}

NS_IMETHODIMP
nsJARURI::GetAsciiHost(nsACString &aHost)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetOriginCharset(nsACString &aOriginCharset)
{
    aOriginCharset = mCharsetHint;
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::Equals(nsIURI *other, PRBool *result)
{
    nsresult rv;
    *result = PR_FALSE;

    if (other == nsnull)
        return NS_OK;	// not equal

    nsCOMPtr<nsIJARURI> otherJAR(do_QueryInterface(other, &rv));
    if (NS_FAILED(rv))
        return NS_OK;   // not equal

    nsCOMPtr<nsIURI> otherJARFile;
    rv = otherJAR->GetJARFile(getter_AddRefs(otherJARFile));
    if (NS_FAILED(rv)) return rv;

    PRBool equal;
    rv = mJARFile->Equals(otherJARFile, &equal);
    if (NS_FAILED(rv)) return rv;
    if (!equal)
        return NS_OK;   // not equal

    nsCAutoString otherJAREntry;
    rv = otherJAR->GetJAREntry(otherJAREntry);
    if (NS_FAILED(rv)) return rv;

    nsCAutoString ourJAREntry;
    rv = GetJAREntry(ourJAREntry);
    if (NS_SUCCEEDED(rv))
        *result = (strcmp(ourJAREntry.get(), otherJAREntry.get()) == 0);
    return rv;
}

NS_IMETHODIMP
nsJARURI::SchemeIs(const char *i_Scheme, PRBool *o_Equals)
{
    NS_ENSURE_ARG_POINTER(o_Equals);
    if (!i_Scheme) return NS_ERROR_INVALID_ARG;

    if (*i_Scheme == 'j' || *i_Scheme == 'J') {
        *o_Equals = PL_strcasecmp("jar", i_Scheme) ? PR_FALSE : PR_TRUE;
    } else {
        *o_Equals = PR_FALSE;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::Clone(nsIURI **result)
{
    nsresult rv;

    nsCOMPtr<nsIURI> newJARFile;
    rv = mJARFile->Clone(getter_AddRefs(newJARFile));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIURI> newJAREntryURI;
    rv = mJAREntry->Clone(getter_AddRefs(newJAREntryURI));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIURL> newJAREntry(do_QueryInterface(newJAREntryURI));
    NS_ASSERTION(newJAREntry, "This had better QI to nsIURL!");
    
    nsJARURI* uri = new nsJARURI();
    if (uri) {
        NS_ADDREF(uri);
        uri->mJARFile = newJARFile;
        uri->mJAREntry = newJAREntry;
        *result = uri;
    } else {
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::Resolve(const nsACString &relativePath, nsACString &result)
{
    nsresult rv;

    rv = net_ExtractURLScheme(relativePath, nsnull, nsnull, nsnull);
    if (NS_SUCCEEDED(rv)) {
        // then aSpec is absolute
        result = relativePath;
        return NS_OK;
    }

    nsCAutoString resolvedPath;
    mJAREntry->Resolve(relativePath, resolvedPath);
    
    return FormatSpec(resolvedPath, result);
}

////////////////////////////////////////////////////////////////////////////////
// nsIURL methods:

NS_IMETHODIMP
nsJARURI::GetFilePath(nsACString& filePath)
{
    return mJAREntry->GetFilePath(filePath);
}

NS_IMETHODIMP
nsJARURI::SetFilePath(const nsACString& filePath)
{
    return mJAREntry->SetFilePath(filePath);
}

NS_IMETHODIMP
nsJARURI::GetParam(nsACString& param)
{
    param.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::SetParam(const nsACString& param)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsJARURI::GetQuery(nsACString& query)
{
    query.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::SetQuery(const nsACString& query)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsJARURI::GetRef(nsACString& ref)
{
    return mJAREntry->GetRef(ref);
}

NS_IMETHODIMP
nsJARURI::SetRef(const nsACString& ref)
{
    return mJAREntry->SetRef(ref);
}

NS_IMETHODIMP
nsJARURI::GetDirectory(nsACString& directory)
{
    return mJAREntry->GetDirectory(directory);
}

NS_IMETHODIMP
nsJARURI::SetDirectory(const nsACString& directory)
{
    return mJAREntry->SetDirectory(directory);
}

NS_IMETHODIMP
nsJARURI::GetFileName(nsACString& fileName)
{
    return mJAREntry->GetFileName(fileName);
}

NS_IMETHODIMP
nsJARURI::SetFileName(const nsACString& fileName)
{
    return mJAREntry->SetFileName(fileName);
}

NS_IMETHODIMP
nsJARURI::GetFileBaseName(nsACString& fileBaseName)
{
    return mJAREntry->GetFileBaseName(fileBaseName);
}

NS_IMETHODIMP
nsJARURI::SetFileBaseName(const nsACString& fileBaseName)
{
    return mJAREntry->SetFileBaseName(fileBaseName);
}

NS_IMETHODIMP
nsJARURI::GetFileExtension(nsACString& fileExtension)
{
    return mJAREntry->GetFileExtension(fileExtension);
}

NS_IMETHODIMP
nsJARURI::SetFileExtension(const nsACString& fileExtension)
{
    return mJAREntry->SetFileExtension(fileExtension);
}

NS_IMETHODIMP
nsJARURI::GetCommonBaseSpec(nsIURI* uriToCompare, nsACString& commonSpec)
{
    commonSpec.Truncate();

    NS_ENSURE_ARG_POINTER(uriToCompare);
    
    commonSpec.Truncate();
    nsCOMPtr<nsIJARURI> otherJARURI(do_QueryInterface(uriToCompare));
    if (!otherJARURI) {
        // Nothing in common
        return NS_OK;
    }

    nsCOMPtr<nsIURI> otherJARFile;
    nsresult rv = otherJARURI->GetJARFile(getter_AddRefs(otherJARFile));
    if (NS_FAILED(rv)) return rv;

    PRBool equal;
    rv = mJARFile->Equals(otherJARFile, &equal);
    if (NS_FAILED(rv)) return rv;

    if (!equal) {
        // See what the JAR file URIs have in common
        nsCOMPtr<nsIURL> ourJARFileURL(do_QueryInterface(mJARFile));
        if (!ourJARFileURL) {
            // Not a URL, so nothing in common
            return NS_OK;
        }
        nsCAutoString common;
        rv = ourJARFileURL->GetCommonBaseSpec(otherJARFile, common);
        if (NS_FAILED(rv)) return rv;

        commonSpec = NS_JAR_SCHEME + common;
        return NS_OK;
        
    }
    
    // At this point we have the same JAR file.  Compare the JAREntrys
    nsCAutoString otherEntry;
    rv = otherJARURI->GetJAREntry(otherEntry);
    if (NS_FAILED(rv)) return rv;

    nsCAutoString otherCharset;
    rv = uriToCompare->GetOriginCharset(otherCharset);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIURL> url;
    rv = CreateEntryURL(otherEntry, otherCharset.get(), getter_AddRefs(url));
    if (NS_FAILED(rv)) return rv;

    nsCAutoString common;
    rv = mJAREntry->GetCommonBaseSpec(url, common);
    if (NS_FAILED(rv)) return rv;

    rv = FormatSpec(common, commonSpec);
    return rv;
}

NS_IMETHODIMP
nsJARURI::GetRelativeSpec(nsIURI* uriToCompare, nsACString& relativeSpec)
{
    GetSpec(relativeSpec);

    NS_ENSURE_ARG_POINTER(uriToCompare);

    nsCOMPtr<nsIJARURI> otherJARURI(do_QueryInterface(uriToCompare));
    if (!otherJARURI) {
        // Nothing in common
        return NS_OK;
    }

    nsCOMPtr<nsIURI> otherJARFile;
    nsresult rv = otherJARURI->GetJARFile(getter_AddRefs(otherJARFile));
    if (NS_FAILED(rv)) return rv;

    PRBool equal;
    rv = mJARFile->Equals(otherJARFile, &equal);
    if (NS_FAILED(rv)) return rv;

    if (!equal) {
        // We live in different JAR files.  Nothing in common.
        return rv;
    }

    // Same JAR file.  Compare the JAREntrys
    nsCAutoString otherEntry;
    rv = otherJARURI->GetJAREntry(otherEntry);
    if (NS_FAILED(rv)) return rv;

    nsCAutoString otherCharset;
    rv = uriToCompare->GetOriginCharset(otherCharset);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIURL> url;
    rv = CreateEntryURL(otherEntry, otherCharset.get(), getter_AddRefs(url));
    if (NS_FAILED(rv)) return rv;

    nsCAutoString relativeEntrySpec;
    rv = mJAREntry->GetRelativeSpec(url, relativeEntrySpec);
    if (NS_FAILED(rv)) return rv;

    if (!StringBeginsWith(relativeEntrySpec, NS_BOGUS_ENTRY_SCHEME)) {
        // An actual relative spec!
        relativeSpec = relativeEntrySpec;
    }
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsIJARURI methods:

NS_IMETHODIMP
nsJARURI::GetJARFile(nsIURI* *jarFile)
{
    *jarFile = mJARFile;
    NS_ADDREF(*jarFile);
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::SetJARFile(nsIURI* jarFile)
{
    mJARFile = jarFile;
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::GetJAREntry(nsACString &entryPath)
{
    nsCAutoString filePath;
    mJAREntry->GetFilePath(filePath);
    NS_ASSERTION(filePath.Length() > 0, "path should never be empty!");
    // Trim off the leading '/'
    entryPath = Substring(filePath, 1, filePath.Length() - 1);
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::SetJAREntry(const nsACString &entryPath)
{
    return CreateEntryURL(entryPath, mCharsetHint.get(),
                          getter_AddRefs(mJAREntry));
}

////////////////////////////////////////////////////////////////////////////////
