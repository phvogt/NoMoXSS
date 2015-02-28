/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 1999
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

#ifndef _nsMsgLocalSearch_H
#define _nsMsgLocalSearch_H

// inherit interface here
#include "nsIMsgSearchAdapter.h"

// inherit base implementation
#include "nsMsgSearchAdapter.h"
#include "nsISimpleEnumerator.h"

class nsIMsgDBHdr;
class nsMsgMailboxParser;
class nsIMsgSearchScopeTerm;
class nsIMsgFolder;
class nsMsgSearchBoolExpression;

class nsMsgSearchOfflineMail : public nsMsgSearchAdapter, public nsIUrlListener
{
public:
	nsMsgSearchOfflineMail (nsIMsgSearchScopeTerm*, nsISupportsArray *);
	virtual ~nsMsgSearchOfflineMail ();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIURLLISTENER

  NS_IMETHOD ValidateTerms ();
  NS_IMETHOD Search (PRBool *aDone);
  NS_IMETHOD Abort ();
  NS_IMETHOD AddResultElement (nsIMsgDBHdr *);

	static nsresult  MatchTermsForFilter(nsIMsgDBHdr * msgToMatch,
                                         nsISupportsArray *termList,
                                         const char *defaultCharset,
                                         nsIMsgSearchScopeTerm *scope, 
                                         nsIMsgDatabase * db, 
                                         const char * headers,
                                         PRUint32 headerSize,
										 PRBool *pResult);

  static nsresult MatchTermsForSearch(nsIMsgDBHdr * msgTomatch,
                                      nsISupportsArray * termList,
                                      const char *defaultCharset,
                                      nsIMsgSearchScopeTerm *scope,
                                      nsIMsgDatabase *db, PRBool *pResult);

	virtual nsresult OpenSummaryFile ();

protected:
	static nsresult MatchTerms(nsIMsgDBHdr *msgToMatch,
                                nsISupportsArray *termList,
                                const char *defaultCharset,
                                nsIMsgSearchScopeTerm *scope, 
                                nsIMsgDatabase * db, 
                                const char * headers,
                                PRUint32 headerSize,
                                PRBool ForFilters,
								PRBool *pResult);

    static nsresult ConstructExpressionTree(nsIMsgDBHdr *msgToMatch,
                                      nsISupportsArray * termList,
                                      PRUint32 &aStartPosInList,
                                      const char *defaultCharset,
                                      nsIMsgSearchScopeTerm * scope,
                                      nsIMsgDatabase * db, 
                                      const char * headers,
                                      PRUint32 headerSize,
                                      PRBool Filtering,
                                      nsMsgSearchBoolExpression ** aExpressionTree,
				        			  PRBool *pResult);
    
     static nsresult ProcessSearchTerm(nsIMsgDBHdr *msgToMatch,
                               nsIMsgSearchTerm * aTerm,
                               const char *defaultCharset,
                               nsIMsgSearchScopeTerm * scope,
                               nsIMsgDatabase * db, 
                               const char * headers,
                               PRUint32 headerSize,
                               PRBool Filtering,
							   PRBool *pResult); 
	nsCOMPtr <nsIMsgDatabase> m_db;
	nsCOMPtr<nsISimpleEnumerator> m_listContext;
	void CleanUpScope();
};


class nsMsgSearchOfflineNews : public nsMsgSearchOfflineMail
{
public:
	nsMsgSearchOfflineNews (nsIMsgSearchScopeTerm*, nsISupportsArray *);
	virtual ~nsMsgSearchOfflineNews ();
	NS_IMETHOD ValidateTerms ();

	virtual nsresult OpenSummaryFile ();
};



#endif

