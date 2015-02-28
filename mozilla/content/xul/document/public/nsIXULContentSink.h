/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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


#ifndef nsIXULContentSink_h__
#define nsIXULContentSink_h__

#include "nsIXMLContentSink.h"

class nsIDocument;
class nsIXULPrototypeDocument;

// {E49AA620-C16C-11d2-A6AA-00104BDE6048}
#define NS_IXULCONTENTSINK_IID \
{ 0xe49aa620, 0xc16c, 0x11d2, { 0xa6, 0xaa, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }


class nsIXULContentSink : public nsIXMLContentSink
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXULCONTENTSINK_IID)

    /**
     * Initialize the content sink, giving it an nsIDocument object
     * with which to communicate with the outside world, and an
     * nsIXULPrototypeDocument to build.
     */
    NS_IMETHOD Init(nsIDocument* aDocument, nsIXULPrototypeDocument* aPrototype) = 0;
};


nsresult
NS_NewXULContentSink(nsIXULContentSink** aResult);

#endif // nsIXULContentSink_h__
