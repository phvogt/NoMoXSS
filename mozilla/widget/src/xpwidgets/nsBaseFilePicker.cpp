/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Stuart Parmenter <pavlov@netscape.com>
 *   Mike Pinkerton   <pinkerton@netscape.com>
 */

#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIViewManager.h"
#include "nsIView.h"
#include "nsIWidget.h"

#include "nsIStringBundle.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsILocalFile.h"
#include "nsEnumeratorUtils.h"

#include "nsBaseFilePicker.h"


static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
#define FILEPICKER_PROPERTIES "chrome://global/locale/filepicker.properties"


nsBaseFilePicker::nsBaseFilePicker()
{

}

nsBaseFilePicker::~nsBaseFilePicker()
{

}

/* XXX aaaarrrrrrgh! */
nsIWidget *nsBaseFilePicker::DOMWindowToWidget(nsIDOMWindow *dw)
{
  nsCOMPtr<nsIWidget> widget;

  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(dw);
  if (sgo) {
    nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(sgo->GetDocShell()));

    if (baseWin) {
      baseWin->GetParentWidget(getter_AddRefs(widget));
    }
  }

  // This will return a pointer that we're about to release, but
  // that's ok since the docshell (nsIBaseWindow) holds the widget
  // alive.
  return widget.get();
}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsBaseFilePicker::Init(nsIDOMWindow *aParent,
                                     const nsAString& aTitle,
                                     PRInt16 aMode)
{
  NS_PRECONDITION(aParent, "Null parent passed to filepicker, no file "
                  "picker for you!");
  nsIWidget *widget = DOMWindowToWidget(aParent);
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

  InitNative(widget, aTitle, aMode);

  return NS_OK;
}


NS_IMETHODIMP
nsBaseFilePicker::AppendFilters(PRInt32 aFilterMask)
{
  nsresult rv;
  nsCOMPtr<nsIStringBundleService> stringService = do_GetService(kStringBundleServiceCID);
  nsCOMPtr<nsIStringBundle> stringBundle;

  rv = stringService->CreateBundle(FILEPICKER_PROPERTIES, getter_AddRefs(stringBundle));
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  nsXPIDLString title;
  nsXPIDLString filter;

  if (aFilterMask & filterAll) {
    stringBundle->GetStringFromName(NS_LITERAL_STRING("allTitle").get(), getter_Copies(title));
    stringBundle->GetStringFromName(NS_LITERAL_STRING("allFilter").get(), getter_Copies(filter));
    AppendFilter(title,filter);
  }
  if (aFilterMask & filterHTML) {
    stringBundle->GetStringFromName(NS_LITERAL_STRING("htmlTitle").get(), getter_Copies(title));
    stringBundle->GetStringFromName(NS_LITERAL_STRING("htmlFilter").get(), getter_Copies(filter));
    AppendFilter(title,filter);
  }
  if (aFilterMask & filterText) {
    stringBundle->GetStringFromName(NS_LITERAL_STRING("textTitle").get(), getter_Copies(title));
    stringBundle->GetStringFromName(NS_LITERAL_STRING("textFilter").get(), getter_Copies(filter));
    AppendFilter(title,filter);
  }
  if (aFilterMask & filterImages) {
    stringBundle->GetStringFromName(NS_LITERAL_STRING("imageTitle").get(), getter_Copies(title));
    stringBundle->GetStringFromName(NS_LITERAL_STRING("imageFilter").get(), getter_Copies(filter));
    AppendFilter(title,filter);
  }
  if (aFilterMask & filterXML) {
    stringBundle->GetStringFromName(NS_LITERAL_STRING("xmlTitle").get(), getter_Copies(title));
    stringBundle->GetStringFromName(NS_LITERAL_STRING("xmlFilter").get(), getter_Copies(filter));
    AppendFilter(title,filter);
  }
  if (aFilterMask & filterXUL) {
    stringBundle->GetStringFromName(NS_LITERAL_STRING("xulTitle").get(), getter_Copies(title));
    stringBundle->GetStringFromName(NS_LITERAL_STRING("xulFilter").get(), getter_Copies(filter));
    AppendFilter(title, filter);
  }
  if (aFilterMask & filterApps) {
    stringBundle->GetStringFromName(NS_LITERAL_STRING("appsTitle").get(), getter_Copies(title));
    // Pass the magic string "..apps" to the platform filepicker, which it
    // should recognize and do the correct platform behavior for.
    AppendFilter(title, NS_LITERAL_STRING("..apps"));
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set the filter index
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsBaseFilePicker::GetFilterIndex(PRInt32 *aFilterIndex)
{
  *aFilterIndex = 0;
  return NS_OK;
}

NS_IMETHODIMP nsBaseFilePicker::SetFilterIndex(PRInt32 aFilterIndex)
{
  return NS_OK;
}

NS_IMETHODIMP nsBaseFilePicker::GetFiles(nsISimpleEnumerator **aFiles)
{
  NS_ENSURE_ARG_POINTER(aFiles);
  nsCOMPtr <nsISupportsArray> files;
  nsresult rv = NS_NewISupportsArray(getter_AddRefs(files));
  NS_ENSURE_SUCCESS(rv,rv);

  // if we get into the base class, the platform
  // doesn't implement GetFiles() yet.
  // so we fake it.
  nsCOMPtr <nsILocalFile> file;
  rv = GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = files->AppendElement(file);
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_NewArrayEnumerator(aFiles, files);
}
