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
 * nsINodeInfo is an interface to node info, such as name, prefix, namespace
 * ID and possibly other data that is shared between nodes (elements
 * and attributes) that have the same name, prefix and namespace ID within
 * the same document.
 *
 * nsINodeInfoManager is an interface to an object that manages a list of
 * nsINodeInfo's, every document object should hold a strong reference to
 * a nsINodeInfoManager and every nsINodeInfo also holds a strong reference
 * to their owning manager. When a nsINodeInfo is no longer used it will
 * automatically remove itself from its owner manager, and when all
 * nsINodeInfo's have been removed from a nsINodeInfoManager and all external
 * references are released the nsINodeInfoManager deletes itself.
 *
 * -- jst@netscape.com
 */

#ifndef nsINodeInfo_h___
#define nsINodeInfo_h___

#include "nsISupports.h"
#include "nsIAtom.h"
#include "nsAString.h"
#include "nsDOMString.h"
#include "nsINameSpaceManager.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"

// Forward declarations
class nsINodeInfo;
class nsIDocument;
class nsIURI;
class nsIPrincipal;


// IID for the nsINodeInfo interface
#define NS_INODEINFO_IID       \
{ 0x93dbfd8c, 0x2fb3, 0x4ef5, \
  {0xa2, 0xa0, 0xcf, 0xf2, 0x69, 0x6f, 0x07, 0x88} }

// IID for the nsINodeInfoManager interface
#define NS_INODEINFOMANAGER_IID \
{ 0xb622469b, 0x4dcf, 0x45c4, \
  {0xb0, 0xb9, 0xa7, 0x32, 0xbc, 0xee, 0xa5, 0xcc} }

#define NS_NODEINFOMANAGER_CONTRACTID "@mozilla.org/layout/nodeinfomanager;1"


class nsINodeInfoManager : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_INODEINFOMANAGER_IID)

  nsINodeInfoManager()
    : mDocument(nsnull)
  {
  }

  virtual ~nsINodeInfoManager() { }

  /*
   * Initialize the nodeinfo manager with a document.
   */
  virtual nsresult Init(nsIDocument *aDocument) = 0;

  /*
   * Release the reference to the document, this will be called when
   * the document is going away.
   */
  virtual void DropDocumentReference() = 0;

  /*
   * Methods for creating nodeinfo's from atoms and/or strings.
   */
  virtual nsresult GetNodeInfo(nsIAtom *aName, nsIAtom *aPrefix,
                               PRInt32 aNamespaceID,
                               nsINodeInfo** aNodeInfo) = 0;
  virtual nsresult GetNodeInfo(const nsAString& aName, nsIAtom *aPrefix,
                               PRInt32 aNamespaceID,
                               nsINodeInfo** aNodeInfo) = 0;
  virtual nsresult GetNodeInfo(const nsAString& aQualifiedName,
                               const nsAString& aNamespaceURI,
                               nsINodeInfo** aNodeInfo) = 0;
  virtual nsresult GetNodeInfo(const nsAString& aName, nsIAtom *aPrefix,
                               const nsAString& aNamespaceURI,
                               nsINodeInfo** aNodeInfo) = 0;

  /*
   * Retrieve a pointer to the document that owns this node info
   * manager.
   */
  nsIDocument* GetDocument() const
  {
    return mDocument;
  }

  /**
   * Gets the principal of the document associated with this.
   */
  virtual nsresult GetDocumentPrincipal(nsIPrincipal** aPrincipal) = 0;
  
  /**
   * Sets the principal of the nodeinfo manager. This should only be called
   * when this nodeinfo manager isn't connected to an nsIDocument.
   */
  virtual nsresult SetDocumentPrincipal(nsIPrincipal* aPrincipal) = 0;
  
protected:
  nsIDocument *mDocument; // WEAK
};


class nsINodeInfo : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_INODEINFO_IID)

  nsINodeInfo()
    : mInner(nsnull, nsnull, kNameSpaceID_None),
      mOwnerManager(nsnull)
  {
  }

  virtual ~nsINodeInfo() { }

  /*
   * Get the name from this node as a string, this does not include the prefix.
   *
   * For the HTML element "<body>" this will return "body" and for the XML
   * element "<html:body>" this will return "body".
   */
  void GetName(nsAString& aName) const
  {
    mInner.mName->ToString(aName);
  }

  /*
   * Get the name from this node as an atom, this does not include the prefix.
   * This function never returns a null atom.
   *
   * For the HTML element "<body>" this will return the "body" atom and for
   * the XML element "<html:body>" this will return the "body" atom.
   */
  nsIAtom* NameAtom() const
  {
    return mInner.mName;
  }

  /*
   * Get the qualified name from this node as a string, the qualified name
   * includes the prefix, if one exists.
   *
   * For the HTML element "<body>" this will return "body" and for the XML
   * element "<html:body>" this will return "html:body".
   */
  virtual void GetQualifiedName(nsAString& aQualifiedName) const = 0;

  /*
   * Get the local name from this node as a string, GetLocalName() gets the
   * same string as GetName() but only if the node has a prefix and/or a
   * namespace URI. If the node has neither a prefix nor a namespace URI the
   * local name is a null string.
   *
   * For the HTML element "<body>" in a HTML document this will return a null
   * string and for the XML element "<html:body>" this will return "body".
   */
  virtual void GetLocalName(nsAString& aLocalName) const = 0;

  /*
   * Get the prefix from this node as a string.
   *
   * For the HTML element "<body>" this will return a null string and for
   * the XML element "<html:body>" this will return the string "html".
   */
  void GetPrefix(nsAString& aPrefix) const
  {
    if (mInner.mPrefix) {
      mInner.mPrefix->ToString(aPrefix);
    } else {
      SetDOMStringToNull(aPrefix);
    }
  }

  /*
   * Get the prefix from this node as an atom.
   *
   * For the HTML element "<body>" this will return a null atom and for
   * the XML element "<html:body>" this will return the "html" atom.
   */
  nsIAtom* GetPrefixAtom() const
  {
    return mInner.mPrefix;
  }

  /*
   * Get the namespace URI for a node, if the node has a namespace URI.
   *
   * For the HTML element "<body>" in a HTML document this will return a null
   * string and for the XML element "<html:body>" (assuming that this element,
   * or one of it's ancestors has an
   * xmlns:html='http://www.w3.org/1999/xhtml' attribute) this will return
   * the string "http://www.w3.org/1999/xhtml".
   */
  virtual nsresult GetNamespaceURI(nsAString& aNameSpaceURI) const = 0;

  /*
   * Get the namespace ID for a node if the node has a namespace, if not this
   * returns kNameSpaceID_None.
   *
   * For the HTML element "<body>" in a HTML document this will return
   * kNameSpaceID_None and for the XML element "<html:body>" (assuming that
   * this element, or one of it's ancestors has an
   * xmlns:html='http://www.w3.org/1999/xhtml' attribute) this will return
   * the namespace ID for "http://www.w3.org/1999/xhtml".
   */
  PRInt32 NamespaceID() const
  {
    return mInner.mNamespaceID;
  }

  /*
   * Get and set the ID attribute atom for this node.
   * See http://www.w3.org/TR/1998/REC-xml-19980210#sec-attribute-types
   * for the definition of an ID attribute.
   *
   */
  nsIAtom* GetIDAttributeAtom() const
  {
    return mIDAttributeAtom;
  }

  void SetIDAttributeAtom(nsIAtom* aID)
  {
    mIDAttributeAtom = aID;
  }

  /*
   * Get the owning node info manager, this will never return null.
   */
  nsINodeInfoManager* NodeInfoManager() const
  {
    return mOwnerManager;
  }

  /*
   * Utility functions that can be used to check if a nodeinfo holds a specific
   * name, name and prefix, name and prefix and namespace ID, or just
   * namespace ID.
   */
  PRBool Equals(nsINodeInfo *aNodeInfo) const
  {
    return aNodeInfo == this || aNodeInfo->Equals(mInner.mName, mInner.mPrefix,
                                                  mInner.mNamespaceID);
  }

  PRBool NameAndNamespaceEquals(nsINodeInfo *aNodeInfo) const
  {
    return aNodeInfo == this || aNodeInfo->Equals(mInner.mName,
                                                  mInner.mNamespaceID);
  }

  PRBool Equals(nsIAtom *aNameAtom) const
  {
    return mInner.mName == aNameAtom;
  }

  PRBool Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom) const
  {
    return (mInner.mName == aNameAtom) && (mInner.mPrefix == aPrefixAtom);
  }

  PRBool Equals(nsIAtom *aNameAtom, PRInt32 aNamespaceID) const
  {
    return ((mInner.mName == aNameAtom) &&
            (mInner.mNamespaceID == aNamespaceID));
  }

  PRBool Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom,
                             PRInt32 aNamespaceID) const
  {
    return ((mInner.mName == aNameAtom) &&
            (mInner.mPrefix == aPrefixAtom) &&
            (mInner.mNamespaceID == aNamespaceID));
  }

  PRBool NamespaceEquals(PRInt32 aNamespaceID) const
  {
    return mInner.mNamespaceID == aNamespaceID;
  }

  virtual PRBool Equals(const nsAString& aName) const = 0;
  virtual PRBool Equals(const nsAString& aName,
                        const nsAString& aPrefix) const = 0;
  virtual PRBool Equals(const nsAString& aName,
                        PRInt32 aNamespaceID) const = 0;
  virtual PRBool Equals(const nsAString& aName, const nsAString& aPrefix,
                        PRInt32 aNamespaceID) const = 0;
  virtual PRBool NamespaceEquals(const nsAString& aNamespaceURI) const = 0;
  // switch to UTF8 - this allows faster access for consumers
  virtual PRBool QualifiedNameEquals(const nsACString& aQualifiedName) const = 0;

  /*
   * This is a convinience method that creates a new nsINodeInfo that differs
   * only by name from the one this is called on.
   */
  nsresult NameChanged(nsIAtom *aName, nsINodeInfo** aResult)
  {
    return mOwnerManager->GetNodeInfo(aName, mInner.mPrefix,
                                      mInner.mNamespaceID, aResult);
  }

  /*
   * This is a convinience method that creates a new nsINodeInfo that differs
   * only by prefix from the one this is called on.
   */
  nsresult PrefixChanged(nsIAtom *aPrefix, nsINodeInfo** aResult)
  {
    return mOwnerManager->GetNodeInfo(mInner.mName, aPrefix,
                                      mInner.mNamespaceID, aResult);
  }

  /*
   * Retrieve a pointer to the document that owns this node info.
   */
  nsIDocument* GetDocument() const
  {
    return mOwnerManager->GetDocument();
  }

  /*
   * Retrieve a pointer to the principal for the document of this node info.
   */
  nsresult GetDocumentPrincipal(nsIPrincipal** aPrincipal) const
  {
    return mOwnerManager->GetDocumentPrincipal(aPrincipal);
  }

protected:
  /*
   * nsNodeInfoInner is used for two things:
   *
   *   1. as a member in nsNodeInfo for holding the name, prefix and
   *      namespace ID
   *   2. as the hash key in the hash table in nsNodeInfoManager
   *
   * nsNodeInfoInner does not do any kind of reference counting,
   * that's up to the user of this class. Since nsNodeInfoInner is
   * typically used as a member of nsNodeInfo, the hash table doesn't
   * need to delete the keys. When the value (nsNodeInfo) is deleted
   * the key is automatically deleted.
   */

  class nsNodeInfoInner
  {
  public:
    nsNodeInfoInner(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID)
      : mName(aName), mPrefix(aPrefix), mNamespaceID(aNamespaceID)
    {
    }

    nsIAtom*            mName;
    nsIAtom*            mPrefix;
    PRInt32             mNamespaceID;
  };

  // nsNodeInfoManager needs to pass mInner to the hash table.
  friend class nsNodeInfoManager;

  nsNodeInfoInner mInner;

  nsCOMPtr<nsIAtom> mIDAttributeAtom;
  nsINodeInfoManager* mOwnerManager; // Strong reference!
};

nsresult
NS_NewNodeInfoManager(nsINodeInfoManager** aResult);

#endif /* nsINodeInfo_h___ */
