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

#ifndef nsIListControlFrame_h___
#define nsIListControlFrame_h___

#include "nsISupports.h"
#include "nsFont.h"
class nsIPresContext;
class nsAString;
class nsIContent;
class nsIPresState;

// IID for the nsIListControlFrame class
#define NS_ILISTCONTROLFRAME_IID    \
{ 0xf44db101, 0xa73c, 0x11d2,  \
  { 0x8d, 0xcf, 0x0, 0x60, 0x97, 0x3, 0xc1, 0x4e } }

/** 
  * nsIListControlFrame is the interface for frame-based listboxes.
  */
class nsIListControlFrame : public nsISupports {

public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ILISTCONTROLFRAME_IID)

  /**
   * Sets the ComboBoxFrame
   *
   */
  NS_IMETHOD SetComboboxFrame(nsIFrame* aComboboxFrame) = 0;

  /**
   * Get the display string for an item
   */
  NS_IMETHOD GetOptionText(PRInt32 aIndex, nsAString & aStr) = 0;

  /**
   * Get the Selected Item's index
   *
   */
  NS_IMETHOD GetSelectedIndex(PRInt32* aIndex) = 0;

  /**
   * Initiates mouse capture for the listbox
   *
   */
  NS_IMETHOD CaptureMouseEvents(nsIPresContext* aPresContext, PRBool aGrabMouseEvents) = 0;

  /**
   * Returns the maximum width and height of an item in the listbox
   */

  NS_IMETHOD GetMaximumSize(nsSize &aSize) = 0;

  /**
   * Returns the number of options in the listbox
   */

  NS_IMETHOD GetNumberOfOptions(PRInt32* aNumOptions) = 0; 

  /**
   * 
   */
  NS_IMETHOD SyncViewWithFrame(nsIPresContext* aPresContext) = 0;

  /**
   * Called by combobox when it's about to drop down
   */
  NS_IMETHOD AboutToDropDown() = 0;

  /**
   * Called by combobox when it's about to roll up
   */
  NS_IMETHOD AboutToRollup() = 0;

  /**
   * Fire on change (used by combobox)
   */
  NS_IMETHOD FireOnChange() = 0;

  /**
   *
   */
  NS_IMETHOD SetOverrideReflowOptimization(PRBool aValue) = 0;

  /**
   *  Return the the frame that the options will be inserted into
   */
  NS_IMETHOD GetOptionsContainer(nsIPresContext* aPresContext,
                                 nsIFrame** aFrame) = 0;

  /**
   * Tell the selected list to roll up and ensure that the proper index is
   * selected, possibly firing onChange if the index has changed
   *
   * @param aIndex the index to actually select
   */
  NS_IMETHOD ComboboxFinish(PRInt32 aIndex) = 0;
};

#endif

