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

#ifndef nsDeviceContextSpecMac_h___
#define nsDeviceContextSpecMac_h___

#include "nsIDeviceContextSpec.h"
#include "nsIPrintingContext.h"
#include "nsDeviceContextMac.h"
#include "nsIPrintSettings.h"
#include <Printing.h>

class nsDeviceContextSpecMac : public nsIDeviceContextSpec, public nsIPrintingContext {
public:
/**
 * Construct a nsDeviceContextSpecMac, which is an object which contains and manages a mac printrecord
 * @update  dc 12/02/98
 */
  nsDeviceContextSpecMac();

  NS_DECL_ISUPPORTS

/**
 * Initialize the nsDeviceContextSpecMac for use.  This will allocate a printrecord for use
 * @update   dc 12/02/98
 * @param aIsPrintPreview   TRUE if doing print preview, FALSE if normal printing.
 * @return error status
 */
  NS_IMETHOD Init(nsIPrintSettings* aPS, PRBool	aIsPrintPreview);
  
    
/**
 * This will tell if the printmanager is currently open
 * @update   dc 12/03/98
 * @param aIsOpen True or False depending if the printmanager is open
 * @return error status
 */
  NS_IMETHOD PrintManagerOpen(PRBool* aIsOpen) {*aIsOpen = mPrintManagerOpen; return NS_OK;}
  
/**
 * Closes the printmanager if it is open.
 * @update   dc 12/03/98
 * @return error status
 */
  NS_IMETHOD ClosePrintManager();

    NS_IMETHOD BeginDocument(PRInt32     aStartPage, 
                             PRInt32     aEndPage);
    
    NS_IMETHOD EndDocument();
    
    NS_IMETHOD BeginPage();
    
    NS_IMETHOD EndPage();

    NS_IMETHOD GetPrinterResolution(double* aResolution);
    
    NS_IMETHOD GetPageRect(double* aTop, double* aLeft, double* aBottom, double* aRight);
protected:
/**
 * Destuct a nsDeviceContextSpecMac, this will release the printrecord
 * @update  dc 12/02/98
 */
  virtual ~nsDeviceContextSpecMac();

protected:
	THPrint						mPrtRec;									// the print record
	PRBool						mPrintManagerOpen;				// tells if the print manager is open

	
	
#if !TARGET_CARBON
// pinkerton - obsolete APIs. Doesn't exist in carbon
	TPrPort*	mPrinterPort;
#endif

friend class nsDeviceContextMac;
};

#endif
