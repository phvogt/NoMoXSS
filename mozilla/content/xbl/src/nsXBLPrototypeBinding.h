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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
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

#ifndef nsXBLPrototypeBinding_h__
#define nsXBLPrototypeBinding_h__

#include "nsCOMPtr.h"
#include "nsXBLPrototypeResources.h"
#include "nsXBLPrototypeHandler.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSLoaderObserver.h"
#include "nsWeakReference.h"
#include "nsIContent.h"
#include "nsHashtable.h"
#include "nsIXBLDocumentInfo.h"
#include "nsCOMArray.h"
#include "nsIURL.h"

class nsIAtom;
class nsIDocument;
class nsIScriptContext;
class nsISupportsArray;
class nsSupportsHashtable;
class nsIXBLService;
class nsFixedSizeAllocator;
class nsXBLProtoImpl;
class nsIXBLBinding;

// *********************************************************************/
// The XBLPrototypeBinding class

// References to the prototype binding are held by each nsXBLBinding instance
// that uses this prototype binding, and also by the XBLDocumentInfo's
// binding table (with the XUL cache disabled).

class nsXBLPrototypeBinding
{
public:
  already_AddRefed<nsIContent> GetBindingElement();
  void SetBindingElement(nsIContent* aElement);

  nsIURI* BindingURI() const { return mBindingURI; }
  nsIURI* DocURI() const { return mXBLDocInfoWeak->DocumentURI(); }
  nsresult GetID(nsACString& aResult) const { return mBindingURI->GetRef(aResult); }

  nsresult GetAllowScripts(PRBool* aResult);

  nsresult BindingAttached(nsIDOMEventReceiver* aRec);
  nsresult BindingDetached(nsIDOMEventReceiver* aRec);

  PRBool LoadResources();
  nsresult AddResource(nsIAtom* aResourceType, const nsAString& aSrc);

  PRBool InheritsStyle() { return mInheritStyle; }

  nsXBLPrototypeHandler* GetPrototypeHandlers() { return mPrototypeHandler; }
  void SetPrototypeHandlers(nsXBLPrototypeHandler* aHandler) { mPrototypeHandler = aHandler; }

  nsXBLPrototypeHandler* GetConstructor();
  nsresult SetConstructor(nsXBLPrototypeHandler* aConstructor);
  nsXBLPrototypeHandler* GetDestructor();
  nsresult SetDestructor(nsXBLPrototypeHandler* aDestructor);

  nsresult InitClass(const nsCString& aClassName, nsIScriptContext * aContext, void * aScriptObject, void ** aClassObject);
  
  nsresult ConstructInterfaceTable(const nsAString& aImpls);
  
  void SetImplementation(nsXBLProtoImpl* aImpl) { mImplementation = aImpl; }
  nsresult InstallImplementation(nsIContent* aBoundElement);

  void AttributeChanged(nsIAtom* aAttribute, PRInt32 aNameSpaceID,
                        PRBool aRemoveFlag, nsIContent* aChangedElement,
                        nsIContent* aAnonymousContent, PRBool aNotify);

  void SetBasePrototype(nsXBLPrototypeBinding* aBinding);
  nsXBLPrototypeBinding* GetBasePrototype() { return mBaseBinding; }

  nsIXBLDocumentInfo* XBLDocumentInfo() const { return mXBLDocInfoWeak; }
  
  PRBool HasBasePrototype() { return mHasBaseProto; }
  void SetHasBasePrototype(PRBool aHasBase) { mHasBaseProto = aHasBase; }

  void SetInitialAttributes(nsIContent* aBoundElement, nsIContent* aAnonymousContent);

  nsCOMArray<nsIStyleRuleProcessor>* GetRuleProcessors();
  nsCOMArray<nsICSSStyleSheet>* GetStyleSheets();

  PRBool HasInsertionPoints() { return mInsertionPointTable != nsnull; }
  
  PRBool HasStyleSheets() {
    return mResources && mResources->mStyleSheetList.Count() > 0;
  }

  nsresult FlushSkinSheets();

  void InstantiateInsertionPoints(nsIXBLBinding* aBinding);

  void GetInsertionPoint(nsIContent* aBoundElement, nsIContent* aCopyRoot,
                         nsIContent* aChild, nsIContent** aResult,
                         PRUint32* aIndex, nsIContent** aDefaultContent);

  void GetSingleInsertionPoint(nsIContent* aBoundElement,
                               nsIContent* aCopyRoot, nsIContent** aResult,
                               PRUint32* aIndex, PRBool* aMultiple,
                               nsIContent** aDefaultContent);

  void GetBaseTag(PRInt32* aNamespaceID, nsIAtom** aTag);
  void SetBaseTag(PRInt32 aNamespaceID, nsIAtom* aTag);

  PRBool ImplementsInterface(REFNSIID aIID);

  PRBool ShouldBuildChildFrames();

  nsresult AddResourceListener(nsIContent* aBoundElement);

  void Initialize();

  const nsCOMArray<nsXBLKeyEventHandler>* GetKeyEventHandlers()
  {
    if (!mKeyHandlersRegistered) {
      CreateKeyHandlers();
      mKeyHandlersRegistered = PR_TRUE;
    }

    return &mKeyHandlers;
  }

public:
  nsXBLPrototypeBinding();
  ~nsXBLPrototypeBinding();

  // Init must be called after construction to initialize the prototype
  // binding.  It may well throw errors (eg on out-of-memory).  Do not confuse
  // this with the Initialize() method, which must be called after the
  // binding's handlers, properties, etc are all set.
  nsresult Init(const nsACString& aRef,
                nsIXBLDocumentInfo* aInfo,
                nsIContent* aElement);
  
// Static members
  static PRUint32 gRefCnt;
 
  static nsFixedSizeAllocator* kAttrPool;
  static nsFixedSizeAllocator* kInsPool;

// Internal member functions
public:
  already_AddRefed<nsIContent> GetImmediateChild(nsIAtom* aTag);
  already_AddRefed<nsIContent> LocateInstance(nsIContent* aBoundElt,
                                              nsIContent* aTemplRoot,
                                              nsIContent* aCopyRoot,
                                              nsIContent* aTemplChild);

protected:  
  void ConstructAttributeTable(nsIContent* aElement);
  void ConstructInsertionTable(nsIContent* aElement);
  void GetNestedChildren(nsIAtom* aTag, nsIContent* aContent,
                         nsISupportsArray** aList);
  void CreateKeyHandlers();

protected:
  // Internal helper class for managing our IID table.
  class nsIIDKey : public nsHashKey {
    protected:
      nsIID mKey;
  
    public:
      nsIIDKey(REFNSIID key) : mKey(key) {}
      ~nsIIDKey(void) {}

      PRUint32 HashCode(void) const {
        // Just use the 32-bit m0 field.
        return mKey.m0;
      }

      PRBool Equals(const nsHashKey *aKey) const {
        return mKey.Equals( ((nsIIDKey*) aKey)->mKey);
      }

      nsHashKey *Clone(void) const {
        return new nsIIDKey(mKey);
      }
  };

// MEMBER VARIABLES
protected:
  nsCOMPtr<nsIURL> mBindingURI;
  nsCOMPtr<nsIContent> mBinding; // Strong. We own a ref to our content element in the binding doc.
  nsAutoPtr<nsXBLPrototypeHandler> mPrototypeHandler; // Strong. DocInfo owns us, and we own the handlers.
  
  nsXBLProtoImpl* mImplementation; // Our prototype implementation (includes methods, properties, fields,
                                   // the constructor, and the destructor).

  nsXBLPrototypeBinding* mBaseBinding; // Weak.  The docinfo will own our base binding.
  PRPackedBool mInheritStyle;
  PRPackedBool mHasBaseProto;
  PRPackedBool mKeyHandlersRegistered;
 
  nsXBLPrototypeResources* mResources; // If we have any resources, this will be non-null.
                                      
  nsIXBLDocumentInfo* mXBLDocInfoWeak; // A pointer back to our doc info.  Weak, since it owns us.

  nsObjectHashtable* mAttributeTable; // A table for attribute entries.  Used to efficiently
                                      // handle attribute changes.

  nsObjectHashtable* mInsertionPointTable; // A table of insertion points for placing explicit content
                                           // underneath anonymous content.

  nsSupportsHashtable* mInterfaceTable; // A table of cached interfaces that we support.

  PRInt32 mBaseNameSpaceID;    // If we extend a tagname/namespace, then that information will
  nsCOMPtr<nsIAtom> mBaseTag;  // be stored in here.

  nsAutoRefCnt mRefCnt;

  nsCOMArray<nsXBLKeyEventHandler> mKeyHandlers;
};

#endif

