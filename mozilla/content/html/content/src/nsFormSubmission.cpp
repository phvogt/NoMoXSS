/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsIFormSubmission.h"

#include "nsIPresContext.h"
#include "nsCOMPtr.h"
#include "nsIForm.h"
#include "nsILinkHandler.h"
#include "nsIDocument.h"
#include "nsHTMLAtoms.h"
#include "nsIHTMLDocument.h"
#include "nsIFormControl.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsDOMError.h"
#include "nsHTMLValue.h"
#include "nsGenericElement.h"
#include "nsISaveAsCharset.h"

// JBK added for submit move from content frame
#include "nsIFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFormProcessor.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsLinebreakConverter.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"
#include "nsEscape.h"
#include "nsUnicharUtils.h"
#include "nsIMultiplexInputStream.h"
#include "nsIMIMEInputStream.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIStringBundle.h"

//BIDI
#include "nsBidiUtils.h"
//end

static NS_DEFINE_CID(kFormProcessorCID, NS_FORMPROCESSOR_CID);
static NS_DEFINE_CID(kCharsetAliasCID, NS_CHARSETALIAS_CID);

/**
 * Helper superclass implementation of nsIFormSubmission, providing common
 * methods that most of the specific implementations need and use.
 */
class nsFormSubmission : public nsIFormSubmission {

public:

  /**
   * @param aCharset the charset of the form as a string
   * @param aEncoder an encoder that will encode Unicode names and values into
   *        bytes to be sent over the wire (usually a charset transformation)
   * @param aFormProcessor a form processor who can listen to 
   * @param aBidiOptions the BIDI options flags for the current pres context
   */
  nsFormSubmission(const nsACString& aCharset,
                   nsISaveAsCharset* aEncoder,
                   nsIFormProcessor* aFormProcessor,
                   PRInt32 aBidiOptions)
    : mCharset(aCharset),
      mEncoder(aEncoder),
      mFormProcessor(aFormProcessor),
      mBidiOptions(aBidiOptions)
  {
  };
  virtual ~nsFormSubmission()
  {
  };

  NS_DECL_ISUPPORTS

  //
  // nsIFormSubmission
  //
  virtual nsresult SubmitTo(nsIURI* aActionURI, const nsAString& aTarget,
                            nsIContent* aSource, nsIPresContext* aPresContext,
                            nsIDocShell** aDocShell, nsIRequest** aRequest);

  /**
   * Called to initialize the submission.  Perform any initialization that may
   * fail here.  Subclasses *must* implement this.
   */
  NS_IMETHOD Init() = 0;

protected:
  // this is essentially the nsFormSubmission interface (to be overridden)
  /**
   * Given a URI and the current submission, create the final URI and data
   * stream that will be submitted.  Subclasses *must* implement this.
   *
   * @param aURI the URI being submitted to [INOUT]
   * @param aPostDataStream a data stream for POST data [OUT]
   */
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream) = 0;

  // Helpers
  /**
   * Call to have the form processor listen in when a name/value pair is found
   * to be submitted.
   *
   * @param aSource the HTML element the name/value is associated with
   * @param aName the name that will be submitted
   * @param aValue the value that will be submitted
   * @return the new value (or null if the value was not changed)
   */
  nsString* ProcessValue(nsIDOMHTMLElement* aSource,
                         const nsAString& aName, const nsAString& aValue);

  // Encoding Helpers
  /**
   * Encode a Unicode string to bytes using the encoder (or just do ToNewCString
   * if there is no encoder).
   * @param aStr the string to encode
   * @return a char* pointing to the encoded bytes (use nsMemory::Free to free)
   */
  char* EncodeVal(const nsAString& aStr);
  /**
   * Encode a Unicode string to bytes using an encoder.  (Used by EncodeVal)
   * @param aStr the string to encode
   * @param aLen the length of aStr
   * @param aEncoder the encoder to encode the bytes with (cannot be null)
   * @return a char* pointing to the encoded bytes (use nsMemory::Free to free)
   */
  char* UnicodeToNewBytes(const PRUnichar* aStr, PRUint32 aLen,
                          nsISaveAsCharset* aEncoder);

  /** The name of the encoder charset */
  nsCString mCharset;
  /** The encoder that will encode Unicode names and values into
   *  bytes to be sent over the wire (usually a charset transformation)
   */
  nsCOMPtr<nsISaveAsCharset> mEncoder;
  /** A form processor who can listen to values */
  nsCOMPtr<nsIFormProcessor> mFormProcessor;
  /** The BIDI options flags for the current pres context */
  PRInt32 mBidiOptions;

public:
  // Static helpers

  /**
   * Get the submit charset for a form (suitable to pass in to the constructor).
   * @param aForm the form in question
   * @param aCtrlsModAtSubmit BIDI controls text mode.  Unused in non-BIDI
   *        builds.
   * @param aCharset the returned charset [OUT]
   */
  static void GetSubmitCharset(nsIHTMLContent* aForm,
                               PRUint8 aCtrlsModAtSubmit,
                               nsACString& aCharset);
  /**
   * Get the encoder for a form (suitable to pass in to the constructor).
   * @param aForm the form in question
   * @param aPresContext the pres context in which we are submitting
   * @param aCharset the charset of the form
   * @param aEncoder the returned encoder [OUT]
   */
  static nsresult GetEncoder(nsIHTMLContent* aForm,
                             nsIPresContext* aPresContext,
                             const nsACString& aCharset,
                             nsISaveAsCharset** aEncoder);
  /**
   * Get an attribute of a form as int, provided that it is an enumerated value.
   * @param aForm the form in question
   * @param aAtom the attribute (for example, nsHTMLAtoms::enctype) to get
   * @param aValue the result (will not be set at all if the attribute does not
   *        exist on the form, so *make sure you provide a default value*.)
   *        [OUT]
   */
  static void GetEnumAttr(nsIHTMLContent* aForm,
                          nsIAtom* aAtom, PRInt32* aValue);
};

//
// Static helper methods that don't really have nothing to do with nsFormSub
//

/**
 * Send a warning to the JS console
 * @param aContent the content the warning is about
 * @param aWarningName the internationalized name of the warning within
 *        layout/html/forms/src/HtmlProperties.js
 */
static nsresult
SendJSWarning(nsIHTMLContent* aContent,
              const nsAFlatString& aWarningName);
/**
 * Send a warning to the JS console
 * @param aContent the content the warning is about
 * @param aWarningName the internationalized name of the warning within
 *        layout/html/forms/src/HtmlProperties.js
 * @param aWarningArg1 an argument to replace a %S in the warning
 */
static nsresult
SendJSWarning(nsIHTMLContent* aContent,
              const nsAFlatString& aWarningName,
              const nsAFlatString& aWarningArg1);
/**
 * Send a warning to the JS console
 * @param aContent the content the warning is about
 * @param aWarningName the internationalized name of the warning within
 *        layout/html/forms/src/HtmlProperties.js
 * @param aWarningArgs an array of strings to replace %S's in the warning
 * @param aWarningArgsLen the number of strings in the array
 */
static nsresult
SendJSWarning(nsIHTMLContent* aContent,
              const nsAFlatString& aWarningName,
              const PRUnichar** aWarningArgs, PRUint32 aWarningArgsLen);


class nsFSURLEncoded : public nsFormSubmission
{
public:
  /**
   * @param aCharset the charset of the form as a string
   * @param aEncoder an encoder that will encode Unicode names and values into
   *        bytes to be sent over the wire (usually a charset transformation)
   * @param aFormProcessor a form processor who can listen to 
   * @param aBidiOptions the BIDI options flags for the current pres context
   * @param aMethod the method of the submit (either NS_FORM_METHOD_GET or
   *        NS_FORM_METHOD_POST).
   */
  nsFSURLEncoded(const nsACString& aCharset,
                 nsISaveAsCharset* aEncoder,
                 nsIFormProcessor* aFormProcessor,
                 PRInt32 aBidiOptions,
                 PRInt32 aMethod)
    : nsFormSubmission(aCharset, aEncoder, aFormProcessor, aBidiOptions),
      mMethod(aMethod)
  {
  }
  virtual ~nsFSURLEncoded()
  {
  }

  NS_DECL_ISUPPORTS_INHERITED

  // nsIFormSubmission
  virtual nsresult AddNameValuePair(nsIDOMHTMLElement* aSource,
                                    const nsAString& aName,
                                    const nsAString& aValue);
  virtual nsresult AddNameFilePair(nsIDOMHTMLElement* aSource,
                                   const nsAString& aName,
                                   const nsAString& aFilename,
                                   nsIInputStream* aStream,
                                   const nsACString& aContentType,
                                   PRBool aMoreFilesToCome);
  virtual PRBool AcceptsFiles() const
  {
    return PR_FALSE;
  }

  NS_IMETHOD Init();

protected:
  // nsFormSubmission
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream);

  // Helpers
  /**
   * URL encode a Unicode string by encoding it to bytes, converting linebreaks
   * properly, and then escaping many bytes as %xx.
   *
   * @param aStr the string to encode
   * @param aEncoded the encoded string [OUT]
   * @throws NS_ERROR_OUT_OF_MEMORY if we run out of memory
   */
  nsresult URLEncode(const nsAString& aStr, nsCString& aEncoded);

private:
  /**
   * The method of the submit (either NS_FORM_METHOD_GET or
   * NS_FORM_METHOD_POST).
   */
  PRInt32 mMethod;

  /** The query string so far (the part after the ?) */
  nsCString mQueryString;

  /** Whether or not we have warned about a file control not being submitted */
  PRBool mWarnedFileControl;
};

NS_IMPL_RELEASE_INHERITED(nsFSURLEncoded, nsFormSubmission)
NS_IMPL_ADDREF_INHERITED(nsFSURLEncoded, nsFormSubmission)
NS_IMPL_QUERY_INTERFACE_INHERITED0(nsFSURLEncoded, nsFormSubmission)

nsresult
nsFSURLEncoded::AddNameValuePair(nsIDOMHTMLElement* aSource,
                                 const nsAString& aName,
                                 const nsAString& aValue)
{
  //
  // Check if there is an input type=file so that we can warn
  //
  if (!mWarnedFileControl) {
    nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(aSource);
    if (formControl->GetType() == NS_FORM_INPUT_FILE) {
      nsCOMPtr<nsIHTMLContent> content = do_QueryInterface(aSource);
      SendJSWarning(content, NS_LITERAL_STRING("ForgotFileEnctypeWarning"));
      mWarnedFileControl = PR_TRUE;
    }
  }

  //
  // Let external code process (and possibly change) value
  //
  nsString* processedValue = ProcessValue(aSource, aName, aValue);

  //
  // Encode name
  //
  nsCString convName;
  nsresult rv = URLEncode(aName, convName);
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // Encode value
  //
  nsCString convValue;
  if (processedValue) {
    rv = URLEncode(*processedValue, convValue);
  } else {
    rv = URLEncode(aValue, convValue);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // Append data to string
  //
  if (mQueryString.IsEmpty()) {
    mQueryString += convName + NS_LITERAL_CSTRING("=") + convValue;
  } else {
    mQueryString += NS_LITERAL_CSTRING("&") + convName
                  + NS_LITERAL_CSTRING("=") + convValue;
  }

  delete processedValue;

  return NS_OK;
}

nsresult
nsFSURLEncoded::AddNameFilePair(nsIDOMHTMLElement* aSource,
                                const nsAString& aName,
                                const nsAString& aFilename,
                                nsIInputStream* aStream,
                                const nsACString& aContentType,
                                PRBool aMoreFilesToCome)
{
  return AddNameValuePair(aSource, aName, aFilename);
}

//
// nsFormSubmission
//
NS_IMETHODIMP
nsFSURLEncoded::Init()
{
  mQueryString.Truncate();
  mWarnedFileControl = PR_FALSE;
  return NS_OK;
}

static void
HandleMailtoSubject(nsCString& aPath) {

  // Walk through the string and see if we have a subject already.
  PRBool hasSubject = PR_FALSE;
  PRBool hasParams = PR_FALSE;
  PRInt32 paramSep = aPath.FindChar('?');
  while (paramSep != kNotFound && paramSep < (PRInt32)aPath.Length()) {
    hasParams = PR_TRUE;

    // Get the end of the name at the = op.  If it is *after* the next &,
    // assume that someone made a parameter without an = in it
    PRInt32 nameEnd = aPath.FindChar('=', paramSep+1);
    PRInt32 nextParamSep = aPath.FindChar('&', paramSep+1);
    if (nextParamSep == kNotFound) {
      nextParamSep = aPath.Length();
    }

    // If the = op is after the &, this parameter is a name without value.
    // If there is no = op, same thing.
    if (nameEnd == kNotFound || nextParamSep < nameEnd) {
      nameEnd = nextParamSep;
    }

    if (nameEnd != kNotFound) {
      if (Substring(aPath, paramSep+1, nameEnd-(paramSep+1)) ==
          NS_LITERAL_CSTRING("subject")) {
        hasSubject = PR_TRUE;
        break;
      }
    }

    paramSep = nextParamSep;
  }

  // If there is no subject, append a preformed subject to the mailto line
  if (!hasSubject) {
    if (hasParams) {
      aPath.Append('&');
    } else {
      aPath.Append('?');
    }

    aPath += NS_LITERAL_CSTRING("subject=Form%20Post%20From%20Mozilla&");
  }
}

NS_IMETHODIMP
nsFSURLEncoded::GetEncodedSubmission(nsIURI* aURI,
                                     nsIInputStream** aPostDataStream)
{
  nsresult rv = NS_OK;

  *aPostDataStream = nsnull;

  if (mMethod == NS_FORM_METHOD_POST) {

    PRBool isMailto = PR_FALSE;
    aURI->SchemeIs("mailto", &isMailto);
    if (isMailto) {

      nsCAutoString path;
      rv = aURI->GetPath(path);
      NS_ENSURE_SUCCESS(rv, rv);

      HandleMailtoSubject(path);

      // Append the body to and force-plain-text args to the mailto line
      nsCString escapedBody;
      escapedBody.Adopt(nsEscape(mQueryString.get(), url_XAlphas));

      path += NS_LITERAL_CSTRING("&force-plain-text=Y&body=") + escapedBody;

      rv = aURI->SetPath(path);

    } else {

      nsCOMPtr<nsIInputStream> dataStream;
      // XXX We *really* need to either get the string to disown its data (and
      // not destroy it), or make a string input stream that owns the CString
      // that is passed to it.  Right now this operation does a copy.
      rv = NS_NewCStringInputStream(getter_AddRefs(dataStream), mQueryString);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIMIMEInputStream> mimeStream(
        do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv));
      NS_ENSURE_SUCCESS(rv, rv);

#ifdef SPECIFY_CHARSET_IN_CONTENT_TYPE
      mimeStream->AddHeader("Content-Type",
                            PromiseFlatString(
                              "application/x-www-form-urlencoded; charset="
                              + mCharset
                            ).get());
#else
      mimeStream->AddHeader("Content-Type",
                            "application/x-www-form-urlencoded");
#endif
      mimeStream->SetAddContentLength(PR_TRUE);
      mimeStream->SetData(dataStream);

      *aPostDataStream = mimeStream;
      NS_ADDREF(*aPostDataStream);
    }

  } else {
    //
    // Get the full query string
    //
    PRBool schemeIsJavaScript;
    rv = aURI->SchemeIs("javascript", &schemeIsJavaScript);
    NS_ENSURE_SUCCESS(rv, rv);
    if (schemeIsJavaScript) {
      return NS_OK;
    }

    nsCAutoString path;
    rv = aURI->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);
    // Bug 42616: Trim off named anchor and save it to add later
    PRInt32 namedAnchorPos = path.FindChar('#');
    nsCAutoString namedAnchor;
    if (kNotFound != namedAnchorPos) {
      path.Right(namedAnchor, (path.Length() - namedAnchorPos));
      path.Truncate(namedAnchorPos);
    }

    // Chop off old query string (bug 25330, 57333)
    // Only do this for GET not POST (bug 41585)
    PRInt32 queryStart = path.FindChar('?');
    if (kNotFound != queryStart) {
      path.Truncate(queryStart);
    }

    path.Append('?');
    // Bug 42616: Add named anchor to end after query string
    path.Append(mQueryString + namedAnchor);

    aURI->SetPath(path);
  }

  return rv;
}

// i18n helper routines
nsresult
nsFSURLEncoded::URLEncode(const nsAString& aStr, nsCString& aEncoded)
{
  char* inBuf = EncodeVal(aStr);

  if (!inBuf)
    inBuf = ToNewCString(aStr);

  NS_ENSURE_TRUE(inBuf, NS_ERROR_OUT_OF_MEMORY);

  // convert to CRLF breaks
  char* convertedBuf = nsLinebreakConverter::ConvertLineBreaks(inBuf,
                           nsLinebreakConverter::eLinebreakAny,
                           nsLinebreakConverter::eLinebreakNet);
  nsMemory::Free(inBuf);

  char* escapedBuf = nsEscape(convertedBuf, url_XPAlphas);
  nsMemory::Free(convertedBuf);

  aEncoded.Adopt(escapedBuf);

  return NS_OK;
}



/**
 * Handle multipart/form-data encoding, which does files as well as normal
 * inputs.  This always does POST.
 */
class nsFSMultipartFormData : public nsFormSubmission
{
public:
  /**
   * @param aCharset the charset of the form as a string
   * @param aEncoder an encoder that will encode Unicode names and values into
   *        bytes to be sent over the wire (usually a charset transformation)
   * @param aFormProcessor a form processor who can listen to 
   * @param aBidiOptions the BIDI options flags for the current pres context
   */
  nsFSMultipartFormData(const nsACString& aCharset,
                        nsISaveAsCharset* aEncoder,
                        nsIFormProcessor* aFormProcessor,
                        PRInt32 aBidiOptions);
  virtual ~nsFSMultipartFormData() { }
 
  NS_DECL_ISUPPORTS_INHERITED

  // nsIFormSubmission
  virtual nsresult AddNameValuePair(nsIDOMHTMLElement* aSource,
                                    const nsAString& aName,
                                    const nsAString& aValue);
  virtual nsresult AddNameFilePair(nsIDOMHTMLElement* aSource,
                                   const nsAString& aName,
                                   const nsAString& aFilename,
                                   nsIInputStream* aStream,
                                   const nsACString& aContentType,
                                   PRBool aMoreFilesToCome);
  virtual PRBool AcceptsFiles() const
  {
    return PR_TRUE;
  }

  NS_IMETHOD Init();

protected:
  // nsFormSubmission
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream);

  // Helpers
  /**
   * Roll up the data we have so far and add it to the multiplexed data stream.
   */
  nsresult AddPostDataStream();
  /**
   * Call ProcessValue() and EncodeVal() on name and value.
   *
   * @param aSource the source of the name/value pair
   * @param aName the name to be sent
   * @param aValue the value to be sent
   * @param aProcessedName the name, after being encoded [OUT]
   * @param aProcessedValue the value, after being processed / encoded [OUT]
   * @throws NS_ERROR_OUT_OF_MEMORY if out of memory
   */
  nsresult ProcessAndEncode(nsIDOMHTMLElement* aSource,
                            const nsAString& aName,
                            const nsAString& aValue,
                            nsCString& aProcessedName,
                            nsCString& aProcessedValue);

private:
  /**
   * Get whether we are supposed to be doing backwards compatible submit, which
   * causes us to leave off the mandatory Content-Transfer-Encoding header.
   * This used to cause Bad Things, including server crashes.
   *
   * It is hoped that we can get rid of this at some point, but that will take
   * a lot of testing or some other browsers that send the header and have not
   * had problems.
   */
  PRBool mBackwardsCompatibleSubmit;

  /**
   * The post data stream as it is so far.  This is a collection of smaller
   * chunks--string streams and file streams interleaved to make one big POST
   * stream.
   */
  nsCOMPtr<nsIMultiplexInputStream> mPostDataStream;

  /**
   * The current string chunk.  When a file is hit, the string chunk gets
   * wrapped up into an input stream and put into mPostDataStream so that the
   * file input stream can then be appended and everything is in the right
   * order.  Then the string chunk gets appended to again as we process more
   * name/value pairs.
   */
  nsCString mPostDataChunk;

  /**
   * The boundary string to use after each "part" (the boundary that marks the
   * end of a value).  This is computed randomly and is different for each
   * submission.
   */
  nsCString mBoundary;
};

NS_IMPL_RELEASE_INHERITED(nsFSMultipartFormData, nsFormSubmission)
NS_IMPL_ADDREF_INHERITED(nsFSMultipartFormData, nsFormSubmission)
NS_IMPL_QUERY_INTERFACE_INHERITED0(nsFSMultipartFormData, nsFormSubmission)

//
// Constructor
//
nsFSMultipartFormData::nsFSMultipartFormData(const nsACString& aCharset,
                                             nsISaveAsCharset* aEncoder,
                                             nsIFormProcessor* aFormProcessor,
                                             PRInt32 aBidiOptions)
    : nsFormSubmission(aCharset, aEncoder, aFormProcessor, aBidiOptions)
{
  // XXX I can't *believe* we have a pref for this.  ifdef, anyone?
  mBackwardsCompatibleSubmit = PR_FALSE;
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefBranch) {
    prefBranch->GetBoolPref("browser.forms.submit.backwards_compatible",
                            &mBackwardsCompatibleSubmit);
  }
}

nsresult
nsFSMultipartFormData::ProcessAndEncode(nsIDOMHTMLElement* aSource,
                                        const nsAString& aName,
                                        const nsAString& aValue,
                                        nsCString& aProcessedName,
                                        nsCString& aProcessedValue)
{
  //
  // Let external code process (and possibly change) value
  //
  nsString* processedValue = ProcessValue(aSource, aName, aValue);

  //
  // Get name
  //
  char * encodedVal = EncodeVal(aName);
  if (!encodedVal) {
    delete processedValue;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aProcessedName.Adopt(encodedVal);

  //
  // Get value
  //
  if (processedValue) {
    encodedVal = EncodeVal(*processedValue);
    delete processedValue;
  } else {
    encodedVal = EncodeVal(aValue);
  }
  if (!encodedVal) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aProcessedValue.Adopt(encodedVal);

  //
  // Convert linebreaks in value
  //
  aProcessedValue.Adopt(nsLinebreakConverter::ConvertLineBreaks(aProcessedValue.get(),
                        nsLinebreakConverter::eLinebreakAny,
                        nsLinebreakConverter::eLinebreakNet));
  return NS_OK;
}

//
// nsIFormSubmission
//
nsresult
nsFSMultipartFormData::AddNameValuePair(nsIDOMHTMLElement* aSource,
                                        const nsAString& aName,
                                        const nsAString& aValue)
{
  nsCString nameStr;
  nsCString valueStr;
  nsresult rv = ProcessAndEncode(aSource, aName, aValue, nameStr, valueStr);
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // Make MIME block for name/value pair
  //
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                 + NS_LITERAL_CSTRING(CRLF)
                 + NS_LITERAL_CSTRING("Content-Disposition: form-data; name=\"")
                 + nameStr + NS_LITERAL_CSTRING("\"" CRLF CRLF)
                 + valueStr + NS_LITERAL_CSTRING(CRLF);

  return NS_OK;
}

nsresult
nsFSMultipartFormData::AddNameFilePair(nsIDOMHTMLElement* aSource,
                                       const nsAString& aName,
                                       const nsAString& aFilename,
                                       nsIInputStream* aStream,
                                       const nsACString& aContentType,
                                       PRBool aMoreFilesToCome)
{
  nsCString nameStr;
  nsCString filenameStr;
  nsresult rv = ProcessAndEncode(aSource, aName, aFilename, nameStr, filenameStr);
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // Make MIME block for name/value pair
  //
  // more appropriate than always using binary?
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                 + NS_LITERAL_CSTRING(CRLF);
  if (!mBackwardsCompatibleSubmit) {
    // XXX Is there any way to tell when "8bit" or "7bit" etc may be
    mPostDataChunk +=
          NS_LITERAL_CSTRING("Content-Transfer-Encoding: binary" CRLF);
  }
  mPostDataChunk +=
         NS_LITERAL_CSTRING("Content-Disposition: form-data; name=\"")
       + nameStr + NS_LITERAL_CSTRING("\"; filename=\"")
       + filenameStr + NS_LITERAL_CSTRING("\"" CRLF)
       + NS_LITERAL_CSTRING("Content-Type: ") + aContentType
       + NS_LITERAL_CSTRING(CRLF CRLF);

  //
  // Add the file to the stream
  //
  if (aStream) {
    // We need to dump the data up to this point into the POST data stream here,
    // since we're about to add the file input stream
    AddPostDataStream();

    mPostDataStream->AppendStream(aStream);
  }

  //
  // CRLF after file
  //
  mPostDataChunk += NS_LITERAL_CSTRING(CRLF);

  return NS_OK;
}

//
// nsFormSubmission
//
NS_IMETHODIMP
nsFSMultipartFormData::Init()
{
  nsresult rv;

  //
  // Create the POST stream
  //
  mPostDataStream =
    do_CreateInstance("@mozilla.org/io/multiplex-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mPostDataStream) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  //
  // Build boundary
  //
  mBoundary = NS_LITERAL_CSTRING("---------------------------");
  mBoundary.AppendInt(rand());
  mBoundary.AppendInt(rand());
  mBoundary.AppendInt(rand());

  return NS_OK;
}

NS_IMETHODIMP
nsFSMultipartFormData::GetEncodedSubmission(nsIURI* aURI,
                                            nsIInputStream** aPostDataStream)
{
  nsresult rv;

  //
  // Finish data
  //
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                  + NS_LITERAL_CSTRING("--" CRLF);

  //
  // Add final data input stream
  //
  AddPostDataStream();

  //
  // Make header
  //
  nsCOMPtr<nsIMIMEInputStream> mimeStream
    = do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString boundaryHeaderValue(
    NS_LITERAL_CSTRING("multipart/form-data; boundary=") + mBoundary);

  mimeStream->AddHeader("Content-Type", boundaryHeaderValue.get());
  mimeStream->SetAddContentLength(PR_TRUE);
  mimeStream->SetData(mPostDataStream);

  *aPostDataStream = mimeStream;

  NS_ADDREF(*aPostDataStream);

  return NS_OK;
}

nsresult
nsFSMultipartFormData::AddPostDataStream()
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIInputStream> postDataChunkStream;
  rv = NS_NewCStringInputStream(getter_AddRefs(postDataChunkStream),
                                mPostDataChunk);
  NS_ASSERTION(postDataChunkStream, "Could not open a stream for POST!");
  if (postDataChunkStream) {
    mPostDataStream->AppendStream(postDataChunkStream);
  }

  mPostDataChunk.Truncate();

  return rv;
}


//
// CLASS nsFSTextPlain
//
class nsFSTextPlain : public nsFormSubmission
{
public:
  nsFSTextPlain(const nsACString& aCharset,
                nsISaveAsCharset* aEncoder,
                nsIFormProcessor* aFormProcessor,
                PRInt32 aBidiOptions)
    : nsFormSubmission(aCharset, aEncoder, aFormProcessor, aBidiOptions)
  {
  }
  virtual ~nsFSTextPlain()
  {
  }

  NS_DECL_ISUPPORTS_INHERITED

  // nsIFormSubmission
  virtual nsresult AddNameValuePair(nsIDOMHTMLElement* aSource,
                                    const nsAString& aName,
                                    const nsAString& aValue);
  virtual nsresult AddNameFilePair(nsIDOMHTMLElement* aSource,
                                   const nsAString& aName,
                                   const nsAString& aFilename,
                                   nsIInputStream* aStream,
                                   const nsACString& aContentType,
                                   PRBool aMoreFilesToCome);

  NS_IMETHOD Init();

protected:
  // nsFormSubmission
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream);
  virtual PRBool AcceptsFiles() const
  {
    return PR_FALSE;
  }

private:
  nsString mBody;
};

NS_IMPL_RELEASE_INHERITED(nsFSTextPlain, nsFormSubmission)
NS_IMPL_ADDREF_INHERITED(nsFSTextPlain, nsFormSubmission)
NS_IMPL_QUERY_INTERFACE_INHERITED0(nsFSTextPlain, nsFormSubmission)

nsresult
nsFSTextPlain::AddNameValuePair(nsIDOMHTMLElement* aSource,
                                const nsAString& aName,
                                const nsAString& aValue)
{
  //
  // Let external code process (and possibly change) value
  //
  nsString* processedValue = ProcessValue(aSource, aName, aValue);

  // XXX This won't work well with a name like "a=b" or "a\nb" but I suppose
  // text/plain doesn't care about that.  Parsers aren't built for escaped
  // values so we'll have to live with it.
  if (processedValue) {
    mBody.Append(aName + NS_LITERAL_STRING("=") + *processedValue +
                 NS_LITERAL_STRING(CRLF));

    delete processedValue;
  } else {
    mBody.Append(aName + NS_LITERAL_STRING("=") + aValue +
                 NS_LITERAL_STRING(CRLF));
  }

  return NS_OK;
}

nsresult
nsFSTextPlain::AddNameFilePair(nsIDOMHTMLElement* aSource,
                               const nsAString& aName,
                               const nsAString& aFilename,
                               nsIInputStream* aStream,
                               const nsACString& aContentType,
                               PRBool aMoreFilesToCome)
{
  AddNameValuePair(aSource,aName,aFilename);
  return NS_OK;
}

//
// nsFormSubmission
//
NS_IMETHODIMP
nsFSTextPlain::Init()
{
  mBody.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsFSTextPlain::GetEncodedSubmission(nsIURI* aURI,
                                    nsIInputStream** aPostDataStream)
{
  nsresult rv = NS_OK;

  // XXX HACK We are using the standard URL mechanism to give the body to the
  // mailer instead of passing the post data stream to it, since that sounds
  // hard.
  PRBool isMailto = PR_FALSE;
  aURI->SchemeIs("mailto", &isMailto);
  if (isMailto) {
    nsCAutoString path;
    rv = aURI->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);

    HandleMailtoSubject(path);

    // Append the body to and force-plain-text args to the mailto line
    nsCString escapedBody;
    escapedBody.Adopt(nsEscape(NS_ConvertUTF16toUTF8(mBody).get(),
                               url_XAlphas));

    path += NS_LITERAL_CSTRING("&force-plain-text=Y&body=") + escapedBody;

    rv = aURI->SetPath(path);

  } else {

    // Create data stream
    nsCOMPtr<nsIInputStream> bodyStream;
    rv = NS_NewStringInputStream(getter_AddRefs(bodyStream),
                                          mBody);
    if (!bodyStream) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    // Create mime stream with headers and such
    nsCOMPtr<nsIMIMEInputStream> mimeStream
        = do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mimeStream->AddHeader("Content-Type", "text/plain");
    mimeStream->SetAddContentLength(PR_TRUE);
    mimeStream->SetData(bodyStream);
    CallQueryInterface(mimeStream, aPostDataStream);
    NS_ADDREF(*aPostDataStream);
  }

  return rv;
}


//
// CLASS nsFormSubmission
//

//
// nsISupports stuff
//

NS_IMPL_ADDREF(nsFormSubmission)
NS_IMPL_RELEASE(nsFormSubmission)

NS_INTERFACE_MAP_BEGIN(nsFormSubmission)
  NS_INTERFACE_MAP_ENTRY(nsIFormSubmission)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


// JBK moved from nsFormFrame - bug 34297
// submission

static nsresult
SendJSWarning(nsIHTMLContent* aContent,
               const nsAFlatString& aWarningName)
{
  return SendJSWarning(aContent, aWarningName, nsnull, 0);
}

static nsresult
SendJSWarning(nsIHTMLContent* aContent,
               const nsAFlatString& aWarningName,
               const nsAFlatString& aWarningArg1)
{
  const PRUnichar* formatStrings[1] = { aWarningArg1.get() };
  return SendJSWarning(aContent, aWarningName, formatStrings, 1);
}

static nsresult
SendJSWarning(nsIHTMLContent* aContent,
              const nsAFlatString& aWarningName,
              const PRUnichar** aWarningArgs, PRUint32 aWarningArgsLen)
{
  nsresult rv = NS_OK;

  //
  // Get the document URL to use as the filename
  //
  nsCAutoString documentURISpec;

  nsIDocument* document = aContent->GetDocument();
  if (document) {
    nsIURI *documentURI = document->GetDocumentURI();
    NS_ENSURE_TRUE(documentURI, NS_ERROR_UNEXPECTED);
    documentURI->GetPath(documentURISpec);
  }

  //
  // Get the error string
  //
  nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIStringBundle> bundle;
  rv = bundleService->CreateBundle(
      "chrome://global/locale/layout/HtmlForm.properties",
      getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLString warningStr;
  if (aWarningArgsLen > 0) {
    bundle->FormatStringFromName(aWarningName.get(),
                                 aWarningArgs, aWarningArgsLen,
                                 getter_Copies(warningStr));
  } else {
    bundle->GetStringFromName(aWarningName.get(), getter_Copies(warningStr));
  }

  //
  // Create the error
  //
  nsCOMPtr<nsIScriptError>
      scriptError(do_CreateInstance(NS_SCRIPTERROR_CONTRACTID));
  NS_ENSURE_TRUE(scriptError, NS_ERROR_UNEXPECTED);

  rv = scriptError->Init(warningStr.get(),
                         NS_ConvertUTF8toUTF16(documentURISpec).get(),
                         nsnull, (uintN)0,
                         0, nsIScriptError::warningFlag,
                         "HTML");
  NS_ENSURE_SUCCESS(rv,rv);

  //
  // Send the error to the console
  //
  nsCOMPtr<nsIConsoleService>
      consoleService(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
  NS_ENSURE_TRUE(consoleService, NS_ERROR_UNEXPECTED);

  return consoleService->LogMessage(scriptError);
}

nsresult
GetSubmissionFromForm(nsIHTMLContent* aForm,
                      nsIPresContext* aPresContext,
                      nsIFormSubmission** aFormSubmission)
{
  nsresult rv = NS_OK;

  //
  // Get all the information necessary to encode the form data
  //

  // Get BIDI options
  PRUint32 bidiOptions = 0;
  PRUint8 ctrlsModAtSubmit = 0;
  aPresContext->GetBidi(&bidiOptions);
  ctrlsModAtSubmit = GET_BIDI_OPTION_CONTROLSTEXTMODE(bidiOptions);

  // Get encoding type (default: urlencoded)
  PRInt32 enctype = NS_FORM_ENCTYPE_URLENCODED;
  nsFormSubmission::GetEnumAttr(aForm, nsHTMLAtoms::enctype, &enctype);

  // Get method (default: GET)
  PRInt32 method = NS_FORM_METHOD_GET;
  nsFormSubmission::GetEnumAttr(aForm, nsHTMLAtoms::method, &method);

  // Get charset
  nsCAutoString charset;
  nsFormSubmission::GetSubmitCharset(aForm, ctrlsModAtSubmit, charset);

  // Get unicode encoder
  nsCOMPtr<nsISaveAsCharset> encoder;
  nsFormSubmission::GetEncoder(aForm, aPresContext, charset,
                               getter_AddRefs(encoder));

  // Get form processor
  nsCOMPtr<nsIFormProcessor> formProcessor =
    do_GetService(kFormProcessorCID, &rv);

  //
  // Choose encoder
  //
  // If enctype=multipart/form-data and method=post, do multipart
  // Else do URL encoded
  // NOTE:
  // The rule used to be, if enctype=multipart/form-data, do multipart
  // Else do URL encoded
  if (method == NS_FORM_METHOD_POST &&
      enctype == NS_FORM_ENCTYPE_MULTIPART) {
    *aFormSubmission = new nsFSMultipartFormData(charset, encoder,
                                                 formProcessor, bidiOptions);
  } else if (method == NS_FORM_METHOD_POST &&
             enctype == NS_FORM_ENCTYPE_TEXTPLAIN) {
    *aFormSubmission = new nsFSTextPlain(charset, encoder,
                                         formProcessor, bidiOptions);
  } else {
    if (enctype == NS_FORM_ENCTYPE_MULTIPART ||
        enctype == NS_FORM_ENCTYPE_TEXTPLAIN) {
      nsAutoString enctypeStr;
      aForm->GetAttr(kNameSpaceID_None, nsHTMLAtoms::enctype, enctypeStr);
      SendJSWarning(aForm, NS_LITERAL_STRING("ForgotPostWarning"), PromiseFlatString(enctypeStr));
    }
    *aFormSubmission = new nsFSURLEncoded(charset, encoder,
                                          formProcessor, bidiOptions, method);
  }
  NS_ENSURE_TRUE(*aFormSubmission, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aFormSubmission);


  // This ASSUMES that all encodings above inherit from nsFormSubmission, which
  // they currently do.  If that changes, change this too.
  NS_STATIC_CAST(nsFormSubmission*, *aFormSubmission)->Init();

  return NS_OK;
}

nsresult
nsFormSubmission::SubmitTo(nsIURI* aActionURI, const nsAString& aTarget,
                           nsIContent* aSource, nsIPresContext* aPresContext,
                           nsIDocShell** aDocShell, nsIRequest** aRequest)
{
  nsresult rv;

  //
  // Finish encoding (get post data stream and URI)
  //
  nsCOMPtr<nsIInputStream> postDataStream;
  rv = GetEncodedSubmission(aActionURI, getter_AddRefs(postDataStream));
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // Actually submit the data
  //
  nsILinkHandler *handler = aPresContext->GetLinkHandler();
  NS_ENSURE_TRUE(handler, NS_ERROR_FAILURE);

  return handler->OnLinkClickSync(aSource, eLinkVerb_Replace,
                                  aActionURI,
                                  PromiseFlatString(aTarget).get(),
                                  postDataStream, nsnull,
                                  aDocShell, aRequest);
}

// JBK moved from nsFormFrame - bug 34297
// static
void
nsFormSubmission::GetSubmitCharset(nsIHTMLContent* aForm,
                                   PRUint8 aCtrlsModAtSubmit,
                                   nsACString& oCharset)
{
  oCharset = NS_LITERAL_CSTRING("UTF-8"); // default to utf-8

  nsresult rv = NS_OK;
  nsAutoString acceptCharsetValue;
  nsHTMLValue value;
  rv = aForm->GetHTMLAttribute(nsHTMLAtoms::acceptcharset, value);
  if (rv == NS_CONTENT_ATTR_HAS_VALUE && value.GetUnit() == eHTMLUnit_String) {
    value.GetStringValue(acceptCharsetValue);
  }

  PRInt32 charsetLen = acceptCharsetValue.Length();
  if (charsetLen > 0) {
    PRInt32 offset=0;
    PRInt32 spPos=0;
    // get charset from charsets one by one
    nsCOMPtr<nsICharsetAlias> calias(do_GetService(kCharsetAliasCID, &rv));
    if (NS_FAILED(rv)) {
      return;
    }
    if (calias) {
      do {
        spPos = acceptCharsetValue.FindChar(PRUnichar(' '), offset);
        PRInt32 cnt = ((-1==spPos)?(charsetLen-offset):(spPos-offset));
        if (cnt > 0) {
          nsAutoString uCharset;
          acceptCharsetValue.Mid(uCharset, offset, cnt);

          nsCAutoString charset; charset.AssignWithConversion(uCharset);
          if (NS_SUCCEEDED(calias->GetPreferred(charset, oCharset)))
            return;
        }
        offset = spPos + 1;
      } while (spPos != -1);
    }
  }
  // if there are no accept-charset or all the charset are not supported
  // Get the charset from document
  nsIDocument* doc = aForm->GetDocument();
  if (doc) {
    oCharset = doc->GetDocumentCharacterSet();
  }

  if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_VISUAL
     && oCharset.Equals(NS_LITERAL_CSTRING("windows-1256"),
                        nsCaseInsensitiveCStringComparator())) {
//Mohamed
    oCharset = NS_LITERAL_CSTRING("IBM864");
  }
  else if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_LOGICAL
          && oCharset.Equals(NS_LITERAL_CSTRING("IBM864"),
                             nsCaseInsensitiveCStringComparator())) {
    oCharset = NS_LITERAL_CSTRING("IBM864i");
  }
  else if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_VISUAL
          && oCharset.Equals(NS_LITERAL_CSTRING("ISO-8859-6"),
                             nsCaseInsensitiveCStringComparator())) {
    oCharset = NS_LITERAL_CSTRING("IBM864");
  }
  else if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_VISUAL
          && oCharset.Equals(NS_LITERAL_CSTRING("UTF-8"),
                             nsCaseInsensitiveCStringComparator())) {
    oCharset = NS_LITERAL_CSTRING("IBM864");
  }

}

// JBK moved from nsFormFrame - bug 34297
// static
nsresult
nsFormSubmission::GetEncoder(nsIHTMLContent* aForm,
                             nsIPresContext* aPresContext,
                             const nsACString& aCharset,
                             nsISaveAsCharset** aEncoder)
{
  *aEncoder = nsnull;
  nsresult rv = NS_OK;

  nsCAutoString charset(aCharset);
  if(charset.Equals(NS_LITERAL_CSTRING("ISO-8859-1")))
    charset.Assign(NS_LITERAL_CSTRING("windows-1252"));

  rv = CallCreateInstance( NS_SAVEASCHARSET_CONTRACTID, aEncoder);
  NS_ASSERTION(NS_SUCCEEDED(rv), "create nsISaveAsCharset failed");
  NS_ENSURE_SUCCESS(rv, rv);

  rv = (*aEncoder)->Init(charset.get(),
                         (nsISaveAsCharset::attr_EntityAfterCharsetConv + 
                          nsISaveAsCharset::attr_FallbackDecimalNCR),
                         0);
  NS_ASSERTION(NS_SUCCEEDED(rv), "initialize nsISaveAsCharset failed");
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// i18n helper routines
char*
nsFormSubmission::UnicodeToNewBytes(const PRUnichar* aStr, PRUint32 aLen,
                                    nsISaveAsCharset* aEncoder)
{
  nsresult rv = NS_OK;

  PRUint8 ctrlsModAtSubmit = GET_BIDI_OPTION_CONTROLSTEXTMODE(mBidiOptions);
  PRUint8 textDirAtSubmit = GET_BIDI_OPTION_DIRECTION(mBidiOptions);
  //ahmed 15-1
  nsAutoString temp;
  nsAutoString newBuffer;
  //This condition handle the RTL,LTR for a logical file
  if (ctrlsModAtSubmit == IBMBIDI_CONTROLSTEXTMODE_VISUAL
     && mCharset.Equals(NS_LITERAL_CSTRING("windows-1256"),
                        nsCaseInsensitiveCStringComparator())) {
    Conv_06_FE_WithReverse(nsString(aStr),
                           newBuffer,
                           textDirAtSubmit);
    aStr = newBuffer.get();
    aLen=newBuffer.Length();
  }
  else if (ctrlsModAtSubmit == IBMBIDI_CONTROLSTEXTMODE_LOGICAL
          && mCharset.Equals(NS_LITERAL_CSTRING("IBM864"),
                             nsCaseInsensitiveCStringComparator())) {
    //For 864 file, When it is logical, if LTR then only convert
    //If RTL will mak a reverse for the buffer
    Conv_FE_06(nsString(aStr), newBuffer);
    aStr = newBuffer.get();
    temp = newBuffer;
    aLen=newBuffer.Length();
    if (textDirAtSubmit == 2) { //RTL
    //Now we need to reverse the Buffer, it is by searching the buffer
      PRUint32 loop = aLen;
      PRUint32 z;
      for (z=0; z<=aLen; z++) {
        temp.SetCharAt((PRUnichar)aStr[loop], z);
        loop--;
      }
    }
    aStr = temp.get();
  }
  else if (ctrlsModAtSubmit == IBMBIDI_CONTROLSTEXTMODE_VISUAL
          && mCharset.Equals(NS_LITERAL_CSTRING("IBM864"),
                             nsCaseInsensitiveCStringComparator())
                  && textDirAtSubmit == IBMBIDI_TEXTDIRECTION_RTL) {

    Conv_FE_06(nsString(aStr), newBuffer);
    aStr = newBuffer.get();
    temp = newBuffer;
    aLen=newBuffer.Length();
    //Now we need to reverse the Buffer, it is by searching the buffer
    PRUint32 loop = aLen;
    PRUint32 z;
    for (z=0; z<=aLen; z++) {
      temp.SetCharAt((PRUnichar)aStr[loop], z);
      loop--;
    }
    aStr = temp.get();
  }

  
  char* res = nsnull;
  if(aStr && aStr[0]) {
    rv = aEncoder->Convert(aStr, &res);
    NS_ASSERTION(NS_SUCCEEDED(rv), "conversion failed");
  } 
  if(! res)
    res = nsCRT::strdup("");
  return res;
}


// static
void
nsFormSubmission::GetEnumAttr(nsIHTMLContent* aContent,
                              nsIAtom* atom, PRInt32* aValue)
{
  nsHTMLValue value;
  if (aContent->GetHTMLAttribute(atom, value) == NS_CONTENT_ATTR_HAS_VALUE) {
    if (eHTMLUnit_Enumerated == value.GetUnit()) {
      *aValue = value.GetIntValue();
    }
  }
}

char*
nsFormSubmission::EncodeVal(const nsAString& aStr)
{
  char* retval;
  if (mEncoder) {
    retval = UnicodeToNewBytes(PromiseFlatString(aStr).get(), aStr.Length(),
                               mEncoder);
  } else {
    retval = ToNewCString(aStr);
  }

  return retval;
}

nsString*
nsFormSubmission::ProcessValue(nsIDOMHTMLElement* aSource,
                               const nsAString& aName, const nsAString& aValue)
{
  // Hijack _charset_ (hidden inputs only) for internationalization (bug 18643)
  if (aName == NS_LITERAL_STRING("_charset_")) {
    nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(aSource);
    if (formControl) {
      if (formControl->GetType() == NS_FORM_INPUT_HIDDEN) {
        return new NS_ConvertASCIItoUTF16(mCharset);
      }
    }
  }

  nsString* retval = nsnull;
  if (mFormProcessor) {
    // XXX We need to change the ProcessValue interface to take nsAString
    nsString tmpNameStr(aName);
    retval = new nsString(aValue);
    if (!retval) {
      return nsnull;
    }

#ifdef DEBUG
    nsresult rv =
#endif
    mFormProcessor->ProcessValue(aSource, tmpNameStr, *retval);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to Notify form process observer");
  }

  return retval;
}
