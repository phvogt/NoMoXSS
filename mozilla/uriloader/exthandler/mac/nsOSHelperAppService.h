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

#ifndef nsOSHelperAppService_h__
#define nsOSHelperAppService_h__

// The OS helper app service is a subclass of nsExternalHelperAppService and is implemented on each
// platform. It contains platform specific code for finding helper applications for a given mime type
// in addition to launching those applications. This is the Mac version.

#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsCOMPtr.h"

class nsOSHelperAppService : public nsExternalHelperAppService
{
public:
  nsOSHelperAppService();
  virtual ~nsOSHelperAppService();

  // override nsIExternalProtocolService methods
  NS_IMETHOD ExternalProtocolHandlerExists(const char * aProtocolScheme, PRBool * aHandlerExists);
  NS_IMETHOD LoadUrl(nsIURI * aURL);
  
  // method overrides --> used to hook the mime service into internet config....
  NS_IMETHOD GetFromTypeAndExtension(const char * aType, const char * aFileExt, nsIMIMEInfo ** aMIMEInfo);
  already_AddRefed<nsIMIMEInfo> GetMIMEInfoFromOS(const char * aMIMEType, const char * aFileExt, PRBool * aFound);

  // GetFileTokenForPath must be implemented by each platform. 
  // platformAppPath --> a platform specific path to an application that we got out of the 
  //                     rdf data source. This can be a mac file spec, a unix path or a windows path depending on the platform
  // aFile --> an nsIFile representation of that platform application path.
  virtual nsresult GetFileTokenForPath(const PRUnichar * platformAppPath, nsIFile ** aFile);
  
protected:
  // add any mac specific service state here
  void UpdateCreatorInfo(nsIMIMEInfo * aMIMEInfo);

};

#endif // nsOSHelperAppService_h__
