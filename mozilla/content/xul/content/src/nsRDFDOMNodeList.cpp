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
 *   Pierre Phaneuf <pp@ludusdesign.com>
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

/*

    Helper class to implement the nsIDOMNodeList interface.

    XXX It's probably wrong in some sense, as it uses the "naked"
    content interface to look for kids. (I assume in general this is
    bad because there may be pseudo-elements created for presentation
    that aren't visible to the DOM.)

*/

#include "nsIDOMNode.h"
#include "nsRDFDOMNodeList.h"
#include "nsContentUtils.h"

////////////////////////////////////////////////////////////////////////
// ctors & dtors


nsRDFDOMNodeList::nsRDFDOMNodeList(void)
{
}

nsRDFDOMNodeList::~nsRDFDOMNodeList(void)
{
}

////////////////////////////////////////////////////////////////////////
// nsISupports interface


// QueryInterface implementation for nsRDFDOMNodeList
NS_INTERFACE_MAP_BEGIN(nsRDFDOMNodeList)
    NS_INTERFACE_MAP_ENTRY(nsIDOMNodeList)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMNodeList)
    NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XULNodeList)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsRDFDOMNodeList)
NS_IMPL_RELEASE(nsRDFDOMNodeList)


////////////////////////////////////////////////////////////////////////
// nsIDOMNodeList interface

NS_IMETHODIMP
nsRDFDOMNodeList::GetLength(PRUint32* aLength)
{
    NS_ASSERTION(aLength != nsnull, "null ptr");
    if (! aLength)
        return NS_ERROR_NULL_POINTER;

    *aLength = mElements.Count();
    return NS_OK;
}


NS_IMETHODIMP
nsRDFDOMNodeList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
    if (aIndex >= (PRUint32) mElements.Count()) {
        // We simply return nsnull, as per the DOM spec.
        *aReturn = nsnull;
        return NS_OK;
    }

    NS_ADDREF(*aReturn = mElements[aIndex]);
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// Implementation methods

nsresult
nsRDFDOMNodeList::AppendNode(nsIDOMNode* aNode)
{
    NS_PRECONDITION(aNode != nsnull, "null ptr");
    if (! aNode)
        return NS_ERROR_NULL_POINTER;

    return mElements.AppendObject(aNode);
}

nsresult
nsRDFDOMNodeList::RemoveNode(nsIDOMNode* aNode)
{
    NS_PRECONDITION(aNode != nsnull, "null ptr");
    if (! aNode)
        return NS_ERROR_NULL_POINTER;

    return mElements.RemoveObject(aNode);
}
