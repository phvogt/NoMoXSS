/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Sun Microsystems, Inc.
 * Portions created by Sun Microsystems are Copyright (C) 2002 Sun
 * Microsystems, Inc. All Rights Reserved.
 *
 * Original Author: Kyle Yuan (kyle.yuan@sun.com)
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

#include "nsHTMLBlockAccessible.h"

NS_IMPL_ISUPPORTS_INHERITED2(nsHTMLBlockAccessible, nsBlockAccessible, nsIAccessibleText, nsIAccessibleHyperText)

nsHTMLBlockAccessible::nsHTMLBlockAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell):
nsBlockAccessible(aDomNode, aShell), nsAccessibleHyperText(aDomNode, aShell)
{ 
}

NS_IMETHODIMP nsHTMLBlockAccessible::GetName(nsAString& aName)
{
  return nsBlockAccessible::GetName(aName);
}

NS_IMETHODIMP nsHTMLBlockAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = ROLE_TEXT;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLBlockAccessible::GetState(PRUint32 *aState)
{
  nsAccessible::GetState(aState);
  *aState &= ~STATE_FOCUSABLE;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLBlockAccessible::GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  return nsAccessibleHyperText::GetBounds(mWeakShell, x, y, width, height);
}

NS_IMETHODIMP nsHTMLBlockAccessible::Shutdown()
{
  nsAccessibleHyperText::Shutdown();
  return nsBlockAccessible::Shutdown();
}