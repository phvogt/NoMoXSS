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

#ifndef nsXMLContentSerializer_h__
#define nsXMLContentSerializer_h__

#include "nsIContentSerializer.h"
#include "nsISupportsUtils.h"
#include "nsCOMPtr.h"
#include "nsVoidArray.h"

class nsIDOMNode;

class nsXMLContentSerializer : public nsIContentSerializer {
 public:
  nsXMLContentSerializer();
  virtual ~nsXMLContentSerializer();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(PRUint32 flags, PRUint32 aWrapColumn,
                  const char* aCharSet, PRBool aIsCopying);

  NS_IMETHOD AppendText(nsIDOMText* aText, PRInt32 aStartOffset,
                        PRInt32 aEndOffset, nsAString& aStr);

  NS_IMETHOD AppendCDATASection(nsIDOMCDATASection* aCDATASection,
                                PRInt32 aStartOffset, PRInt32 aEndOffset,
                                nsAString& aStr);

  NS_IMETHOD AppendProcessingInstruction(nsIDOMProcessingInstruction* aPI,
                                         PRInt32 aStartOffset,
                                         PRInt32 aEndOffset,
                                         nsAString& aStr);

  NS_IMETHOD AppendComment(nsIDOMComment* aComment, PRInt32 aStartOffset,
                           PRInt32 aEndOffset, nsAString& aStr);
  
  NS_IMETHOD AppendDoctype(nsIDOMDocumentType *aDoctype,
                           nsAString& aStr);

  NS_IMETHOD AppendElementStart(nsIDOMElement *aElement,
                                PRBool aHasChildren,
                                nsAString& aStr);
  
  NS_IMETHOD AppendElementEnd(nsIDOMElement *aElement,
                              nsAString& aStr);

  NS_IMETHOD Flush(nsAString& aStr) { return NS_OK; }

  NS_IMETHOD AppendDocumentStart(nsIDOMDocument *aDocument,
                                 nsAString& aStr);

 protected:
  virtual void AppendToString(const PRUnichar* aStr,
                              PRInt32 aLength,
                              nsAString& aOutputStr);
  virtual void AppendToString(const PRUnichar aChar, nsAString& aOutputStr);
  virtual void AppendToString(const nsAString& aStr,
                              nsAString& aOutputStr,
                              PRBool aTranslateEntities = PR_FALSE,
                              PRBool aIncrColumn = PR_TRUE);
  nsresult AppendTextData(nsIDOMNode* aNode, 
                          PRInt32 aStartOffset,
                          PRInt32 aEndOffset,
                          nsAString& aStr,
                          PRBool aTranslateEntities,
                          PRBool aIncrColumn);
  virtual nsresult PushNameSpaceDecl(const nsAString& aPrefix,
                                     const nsAString& aURI,
                                     nsIDOMElement* aOwner);
  void PopNameSpaceDeclsFor(nsIDOMElement* aOwner);
  PRBool ConfirmPrefix(nsAString& aPrefix,
                       const nsAString& aURI);
  void SerializeAttr(const nsAString& aPrefix,
                     const nsAString& aName,
                     const nsAString& aValue,
                     nsAString& aStr,
                     PRBool aDoEscapeEntities);
  PRBool IsShorthandAttr(const nsIAtom* aAttrName,
                         const nsIAtom* aElementName);

  // Functions to check for newlines that needs to be added between nodes in
  // the root of a document.
  void MaybeAddNewline(nsAString& aStr);
  void MaybeFlagNewline(nsIDOMNode* aNode);

  PRInt32 mPrefixIndex;
  nsVoidArray mNameSpaceStack;
  PRPackedBool mInAttribute;
  PRPackedBool mAddNewline;
};

nsresult
NS_NewXMLContentSerializer(nsIContentSerializer** aSerializer);

#endif 
