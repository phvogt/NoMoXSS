/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "MPL"); you may not use this file
 * except in compliance with the MPL. You may obtain a copy of
 * the MPL at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the MPL is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the MPL for the specific language governing
 * rights and limitations under the MPL.
 * 
 * The Original Code is XMLterm.
 * 
 * The Initial Developer of the Original Code is Ramalingam Saravanan.
 * Portions created by Ramalingam Saravanan <svn@xmlterm.org> are
 * Copyright (C) 1999 Ramalingam Saravanan. All Rights Reserved.
 * 
 * Contributor(s):
 */

// mozXMLTermStream.h: declaration of mozXMLTermStream
// which implements the mozIXMLTermStream interface
// to display HTML/XML streams as documents

#include "nscore.h"
#include "nspr.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIGenericFactory.h"

#include "nsIDOMWindowInternal.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDocument.h"

#include "nsIDocumentLoader.h"
#include "nsIDocumentLoaderFactory.h"

#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIIOService.h"

#include "nsIWebShell.h"
#include "nsIPresShell.h"
#include "nsIScriptContext.h"

#include "mozXMLT.h"
#include "mozIXMLTermStream.h"

#define NO_WORKAROUND

class mozXMLTermStream : public mozIXMLTermStream
{
  public:

  mozXMLTermStream();
  virtual ~mozXMLTermStream();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_MOZIXMLTERMSTREAM

 protected:

  /** Adjusts height of frame displaying stream to fit content
   * @param maxHeight maximum height of resized frame (pixels)
   *                  (zero value implies no maximum)
   */
  NS_IMETHOD SizeToContentHeight(PRInt32 maxHeight);

  /** UTF8 data buffer to hold to be read by rendering engine */
  nsCString mUTF8Buffer;

  /** offset at which to start reading the UTF8 data buffer */
  PRInt32 mUTF8Offset;

  /** maximum frame height for resizing */
  PRInt32 mMaxResizeHeight;

  /** DOM window in which to display stream */
  nsCOMPtr<nsIDOMWindowInternal> mDOMWindow;

  /** Frame element in which to display stream */
  nsCOMPtr<nsIDOMElement> mDOMIFrameElement;

#ifdef NO_WORKAROUND
  /** Context for stream display */
  nsCOMPtr<nsISupports> mContext;

  /** Load group for stream display */
  nsCOMPtr<nsILoadGroup> mLoadGroup;

  /** Channel for stream display */
  nsCOMPtr<nsIChannel> mChannel;

  /** Stream listener object */
  nsCOMPtr<nsIStreamListener> mStreamListener;
#else // !NO_WORKAROUND
  nsCOMPtr<nsIDOMHTMLDocument> mDOMHTMLDocument;
#endif // !NO_WORKAROUND
};

