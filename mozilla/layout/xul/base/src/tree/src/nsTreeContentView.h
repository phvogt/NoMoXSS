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
 *   Jan Varga (varga@nixcorp.com)
 *   Brian Ryner <bryner@brianryner.com>
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

#ifndef nsTreeContentView_h__
#define nsTreeContentView_h__

#include "nsCOMPtr.h"
#include "nsFixedSizeAllocator.h"
#include "nsVoidArray.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsStubDocumentObserver.h"
#include "nsITreeView.h"
#include "nsITreeBoxObject.h"
#include "nsITreeSelection.h"
#include "nsITreeContentView.h"
#include "nsStyleConsts.h"

class Property;

nsresult NS_NewTreeContentView(nsITreeContentView** aResult);

class nsTreeContentView : public nsITreeView,
                          public nsITreeContentView,
                          public nsStubDocumentObserver
{
  public:
    nsTreeContentView(void);

    virtual ~nsTreeContentView(void);

    friend nsresult NS_NewTreeContentView(nsITreeContentView** aResult);

    NS_DECL_ISUPPORTS

    NS_DECL_NSITREEVIEW

    NS_DECL_NSITREECONTENTVIEW

    // nsIDocumentObserver
    virtual void ContentStatesChanged(nsIDocument* aDocument,
                                      nsIContent* aContent1,
                                      nsIContent* aContent2,
                                      PRInt32 aStateMask);
    virtual void AttributeChanged(nsIDocument *aDocument, nsIContent* aContent,
                                  PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                  PRInt32 aModType);
    virtual void ContentAppended(nsIDocument *aDocument,
                                 nsIContent* aContainer,
                                 PRInt32 aNewIndexInContainer);
    virtual void ContentInserted(nsIDocument *aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aChild,
                                 PRInt32 aIndexInContainer);
    virtual void ContentReplaced(nsIDocument *aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aOldChild, nsIContent* aNewChild,
                                 PRInt32 aIndexInContainer);
    virtual void ContentRemoved(nsIDocument *aDocument, nsIContent* aContainer,
                                nsIContent* aChild, PRInt32 aIndexInContainer);
    virtual void DocumentWillBeDestroyed(nsIDocument *aDocument);

  protected:
    // Recursive methods which deal with serializing of nested content.
    void Serialize(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex, nsVoidArray& aRows);

    void SerializeItem(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex, nsVoidArray& aRows);

    void SerializeSeparator(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex, nsVoidArray& aRows);

    void SerializeOption(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex,
                         nsVoidArray& aRows);

    void SerializeOptGroup(nsIContent* aContent, PRInt32 aParentIndex, PRInt32* aIndex,
                           nsVoidArray& aRows);

    void GetIndexInSubtree(nsIContent* aContainer, nsIContent* aContent, PRInt32* aResult);
    
    // Helper methods which we use to manage our plain array of rows.
    void EnsureSubtree(PRInt32 aIndex, PRInt32* aCount);

    void RemoveSubtree(PRInt32 aIndex, PRInt32* aCount);

    void InsertRowFor(nsIContent* aParent, nsIContent* aContainer, nsIContent* aChild);

    void InsertRow(PRInt32 aParentIndex, PRInt32 aIndex, nsIContent* aContent, PRInt32* aCount);

    void RemoveRow(PRInt32 aIndex, PRInt32* aCount);

    void ClearRows();
    
    void OpenContainer(PRInt32 aIndex);

    void CloseContainer(PRInt32 aIndex);

    PRInt32 FindContent(nsIContent* aContent);

    void UpdateSubtreeSizes(PRInt32 aIndex, PRInt32 aCount);

    void UpdateParentIndexes(PRInt32 aIndex, PRInt32 aSkip, PRInt32 aCount);

    // Content helpers.
    nsresult GetNamedCell(nsIContent* aContainer, const PRUnichar* aColID, nsIContent** aResult);

  private:
    nsCOMPtr<nsITreeBoxObject>          mBoxObject;
    nsCOMPtr<nsITreeSelection>          mSelection;
    nsCOMPtr<nsIContent>                mRoot;
    nsIDocument*                        mDocument;      // WEAK
    nsFixedSizeAllocator                mAllocator;
    nsVoidArray                         mRows;

    PRPackedBool                        mUpdateSelection;
};

#endif // nsTreeContentView_h__
