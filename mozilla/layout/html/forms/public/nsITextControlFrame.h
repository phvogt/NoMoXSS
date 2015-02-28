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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef nsITextControlFrame_h___
#define nsITextControlFrame_h___
 
#include "nsIFormControlFrame.h"

class nsIEditor;
class nsIDocShell;
class nsISelectionController;

#define NS_IGFXTEXTCONTROLFRAME2_IID \
{/* A744CFC9-2DA8-416d-A058-ADB1D4B3B534*/ \
0xa744cfc9, 0x2da8, 0x416d, \
{ 0xa0, 0x58, 0xad, 0xb1, 0xd4, 0xb3, 0xb5, 0x34 } }

class nsITextControlFrame : public nsIFormControlFrame
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IGFXTEXTCONTROLFRAME2_IID)

  NS_IMETHOD    GetEditor(nsIEditor **aEditor) = 0;

  /**
   * Tell whether the frame currently owns the value or the content does (for
   * edge cases where the frame has just been created or is just going away).
   *
   * @param aOwnsValue whether the frame owns the value [out]
   */
  NS_IMETHOD    OwnsValue(PRBool* aOwnsValue) = 0;

  /**
   * Get the current value, either from the editor or from the textarea.
   *
   * @param aValue the value [out]
   * @param aIgnoreWrap whether to ignore the wrap attribute when getting the
   *        value.  If this is true, linebreaks will not be inserted even if
   *        wrap=hard.
   */
  NS_IMETHOD    GetValue(nsAString& aValue, PRBool aIgnoreWrap) = 0;
  
  NS_IMETHOD    GetTextLength(PRInt32* aTextLength) = 0;
  
  /**
   * Fire onChange if the value has changed since it was focused or since it
   * was last fired.
   */
  NS_IMETHOD    CheckFireOnChange() = 0;
  NS_IMETHOD    SetSelectionStart(PRInt32 aSelectionStart) = 0;
  NS_IMETHOD    SetSelectionEnd(PRInt32 aSelectionEnd) = 0;
  
  NS_IMETHOD    SetSelectionRange(PRInt32 aSelectionStart, PRInt32 aSelectionEnd) = 0;
  NS_IMETHOD    GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd) = 0;

  NS_IMETHOD    GetSelectionContr(nsISelectionController **aSelCon) = 0;
};

#endif
