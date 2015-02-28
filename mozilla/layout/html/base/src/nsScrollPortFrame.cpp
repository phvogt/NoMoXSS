/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Author: Eric D Vaughan (evaughan@netscape.com)
 *   Pierre Phaneuf <pp@ludusdesign.com>
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

#include "nsScrollPortFrame.h"
#include "nsIFormControlFrame.h"
#include "nsGfxScrollFrame.h"

nsresult
NS_NewScrollPortFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsScrollPortFrame* it = new (aPresShell) nsScrollPortFrame (aPresShell);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

PRBool
nsScrollPortFrame::NeedsClipWidget()
{
  // Scrollports contained in form controls (e.g., listboxes) don't get
  // widgets.
  for (nsIFrame* parentFrame = GetParent(); parentFrame;
       parentFrame = parentFrame->GetParent()) {
    nsIFormControlFrame* fcFrame;
    if ((NS_SUCCEEDED(parentFrame->QueryInterface(NS_GET_IID(nsIFormControlFrame), (void**)&fcFrame)))) {
      return PR_FALSE;
    }
  }

  // Scrollports that don't ever show associated scrollbars don't get
  // widgets, because they will seldom actually be scrolled.
  nsGfxScrollFrame* scrollFrame = nsGfxScrollFrame::GetScrollFrameForPort(this);
  if (scrollFrame) {
    nsGfxScrollFrame::ScrollbarStyles scrollbars
      = scrollFrame->GetScrollbarStyles();
    if ((scrollbars.mHorizontal == NS_STYLE_OVERFLOW_HIDDEN
         || scrollbars.mHorizontal == NS_STYLE_OVERFLOW_VISIBLE)
        && (scrollbars.mVertical == NS_STYLE_OVERFLOW_HIDDEN
            || scrollbars.mVertical == NS_STYLE_OVERFLOW_VISIBLE)) {
      return PR_FALSE;
    }
  }
 
  return PR_TRUE;
}

#ifdef NS_DEBUG
NS_IMETHODIMP
nsScrollPortFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ScrollPortFrame"), aResult);
}
#endif

nsScrollPortFrame::nsScrollPortFrame(nsIPresShell* aShell):nsScrollBoxFrame(aShell)
{
}
