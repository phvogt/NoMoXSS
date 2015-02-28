/*
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
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 * Keith Visco, kvisco@ziplink.net
 *   -- original author.
 * 
 */

#include "Expr.h"
#include "txNodeSet.h"
#include "txIXPathContext.h"

/**
 * Creates a new RootExpr
 * @param aSerialize should this RootExpr be serialized
 */
RootExpr::RootExpr(MBool aSerialize) {
    mSerialize = aSerialize;
}

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
**/
nsresult
RootExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    const txXPathNode& context = aContext->getContextNode();
    nsAutoPtr<txXPathNode> document(txXPathNodeUtils::getDocument(context));
    if (!document) {
        nsRefPtr<txNodeSet> nodes;
        aContext->recycler()->getNodeSet(getter_AddRefs(nodes));
        if (!nodes) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        NS_ADDREF(*aResult = nodes);

        return NS_OK;
    }

    return aContext->recycler()->getNodeSet(*document, aResult);
} //-- evaluate

/**
 * Returns the String representation of this Expr.
 * @param dest the String to use when creating the String
 * representation. The String representation will be appended to
 * any data in the destination String, to allow cascading calls to
 * other #toString() methods for Expressions.
 * @return the String representation of this Expr.
**/
void RootExpr::toString(nsAString& dest) {
    if (mSerialize)
        dest.Append(PRUnichar('/'));
} //-- toString
