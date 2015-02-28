/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is TransforMiiX XSLT processor.
 *
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Olivier Gerardin are  Copyright (C) 2000,
 * Olivier Gerardin. All rights reserved.
 *
 * Portions created by Keith Visco are  Copyright (C) 2000, Keith Visco.
 * All rights reserved.
 *
 * Contributor(s):
 * Olivier Gerardin, ogerardin@vo.lu
 *   -- original author.
 *
 * Keith Visco, kvisco@ziplink.net
 *   -- patched compilation error on due to improper usage of the
 *      3rd argument (error message) for URIUtils::getInputStream,
 *      in method DocumentFunctionCall::retrieveDocument
 */

/*
 * DocumentFunctionCall
 * A representation of the XSLT additional function: document()
 */

#include "txAtoms.h"
#include "txIXPathContext.h"
#include "XSLTFunctions.h"
#include "txExecutionState.h"
#include "txURIUtils.h"

/*
 * Creates a new DocumentFunctionCall.
 */
DocumentFunctionCall::DocumentFunctionCall(const nsAString& aBaseURI)
    : mBaseURI(aBaseURI)
{
}

static void
retrieveNode(txExecutionState* aExecutionState, const nsAString& aUri,
             const nsAString& aBaseUri, txNodeSet* aNodeSet)
{
    nsAutoString absUrl;
    URIUtils::resolveHref(aUri, aBaseUri, absUrl);

    PRInt32 hash = absUrl.RFindChar(PRUnichar('#'));
    PRUint32 urlEnd, fragStart, fragEnd;
    if (hash == kNotFound) {
        urlEnd = absUrl.Length();
        fragStart = 0;
        fragEnd = 0;
    }
    else {
        urlEnd = hash;
        fragStart = hash + 1;
        fragEnd = absUrl.Length();
    }

    nsDependentSubstring docUrl(absUrl, 0, urlEnd);
    nsDependentSubstring frag(absUrl, fragStart, fragEnd);

    const txXPathNode* loadNode = aExecutionState->retrieveDocument(docUrl);
    if (loadNode) {
        if (frag.IsEmpty()) {
            aNodeSet->add(*loadNode);
        }
        else {
            txXPathTreeWalker walker(*loadNode);
            if (walker.moveToElementById(frag)) {
                aNodeSet->add(walker.getCurrentPosition());
            }
        }
    }
}

/*
 * Evaluates this Expr based on the given context node and processor state
 * NOTE: the implementation is incomplete since it does not make use of the
 * second argument (base URI)
 * @param context the context node for evaluation of this Expr
 * @return the result of the evaluation
 */
nsresult
DocumentFunctionCall::evaluate(txIEvalContext* aContext,
                               txAExprResult** aResult)
{
    *aResult = nsnull;
    txExecutionState* es =
        NS_STATIC_CAST(txExecutionState*, aContext->getPrivateContext());

    nsRefPtr<txNodeSet> nodeSet;
    nsresult rv = aContext->recycler()->getNodeSet(getter_AddRefs(nodeSet));
    NS_ENSURE_SUCCESS(rv, rv);

    // document(object, node-set?)
    if (!requireParams(1, 2, aContext)) {
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
    }

    txListIterator iter(&params);
    Expr* param1 = (Expr*)iter.next();
    nsRefPtr<txAExprResult> exprResult1;
    rv = param1->evaluate(aContext, getter_AddRefs(exprResult1));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString baseURI;
    MBool baseURISet = MB_FALSE;

    if (iter.hasNext()) {
        // We have 2 arguments, get baseURI from the first node
        // in the resulting nodeset
        nsRefPtr<txNodeSet> nodeSet2;
        rv = evaluateToNodeSet(NS_STATIC_CAST(Expr*, iter.next()),
                               aContext, getter_AddRefs(nodeSet2));
        NS_ENSURE_SUCCESS(rv, rv);

        // Make this true, even if nodeSet2 is empty. For relative URLs,
        // we'll fail to load the document with an empty base URI, and for
        // absolute URLs, the base URI doesn't matter
        baseURISet = MB_TRUE;

        if (!nodeSet2->isEmpty()) {
            txXPathNodeUtils::getBaseURI(nodeSet2->get(0), baseURI);
        }
    }

    if (exprResult1->getResultType() == txAExprResult::NODESET) {
        // The first argument is a NodeSet, iterate on its nodes
        txNodeSet* nodeSet1 = NS_STATIC_CAST(txNodeSet*,
                                             NS_STATIC_CAST(txAExprResult*,
                                                            exprResult1));
        PRInt32 i;
        for (i = 0; i < nodeSet1->size(); ++i) {
            const txXPathNode& node = nodeSet1->get(i);
            nsAutoString uriStr;
            txXPathNodeUtils::appendNodeValue(node, uriStr);
            if (!baseURISet) {
                // if the second argument wasn't specified, use
                // the baseUri of node itself
                txXPathNodeUtils::getBaseURI(node, baseURI);
            }
            retrieveNode(es, uriStr, baseURI, nodeSet);
        }
        
        NS_ADDREF(*aResult = nodeSet);
        
        return NS_OK;
    }

    // The first argument is not a NodeSet
    nsAutoString uriStr;
    exprResult1->stringValue(uriStr);
    const nsAString* base = baseURISet ? &baseURI : &mBaseURI;
    retrieveNode(es, uriStr, *base, nodeSet);

    NS_ADDREF(*aResult = nodeSet);

    return NS_OK;
}

nsresult DocumentFunctionCall::getNameAtom(nsIAtom** aAtom)
{
    *aAtom = txXSLTAtoms::document;
    NS_ADDREF(*aAtom);
    return NS_OK;
}
