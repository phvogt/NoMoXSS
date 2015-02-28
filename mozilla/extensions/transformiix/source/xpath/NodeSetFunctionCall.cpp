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
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 * Keith Visco, kvisco@ziplink.net
 *   -- original author.
 *
 * Marina Mechtcheriakova, mmarina@mindspring.com
 *   -- changed some behavoir to be more compliant with spec
 * 
 */

/*
 * NodeSetFunctionCall
 * A representation of the XPath NodeSet funtions
 */

#include "FunctionLib.h"
#include "nsAutoPtr.h"
#include "txNodeSet.h"
#include "txAtoms.h"
#include "txIXPathContext.h"
#include "txTokenizer.h"
#ifndef TX_EXE
#include "nsIDOMNode.h"
#endif

/*
 * Creates a NodeSetFunctionCall of the given type
 */
NodeSetFunctionCall::NodeSetFunctionCall(NodeSetFunctions aType)
    : mType(aType)
{
}

/*
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
 */
nsresult
NodeSetFunctionCall::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    *aResult = nsnull;
    nsresult rv = NS_OK;
    txListIterator iter(&params);

    switch (mType) {
        case COUNT:
        {
            if (!requireParams(1, 1, aContext))
                return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;

            nsRefPtr<txNodeSet> nodes;
            rv = evaluateToNodeSet((Expr*)iter.next(), aContext,
                                   getter_AddRefs(nodes));
            NS_ENSURE_SUCCESS(rv, rv);

            return aContext->recycler()->getNumberResult(nodes->size(),
                                                         aResult);
        }
        case ID:
        {
            if (!requireParams(1, 1, aContext))
                return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;

            nsRefPtr<txAExprResult> exprResult;
            rv = ((Expr*)iter.next())->evaluate(aContext,
                                                getter_AddRefs(exprResult));
            NS_ENSURE_SUCCESS(rv, rv);

            nsRefPtr<txNodeSet> resultSet;
            rv = aContext->recycler()->getNodeSet(getter_AddRefs(resultSet));
            NS_ENSURE_SUCCESS(rv, rv);

            txXPathTreeWalker walker(aContext->getContextNode());
            
            if (exprResult->getResultType() == txAExprResult::NODESET) {
                txNodeSet* nodes = NS_STATIC_CAST(txNodeSet*,
                                                  NS_STATIC_CAST(txAExprResult*,
                                                                 exprResult));
                PRInt32 i;
                for (i = 0; i < nodes->size(); ++i) {
                    nsAutoString idList;
                    txXPathNodeUtils::appendNodeValue(nodes->get(i), idList);
                    txTokenizer tokenizer(idList);
                    while (tokenizer.hasMoreTokens()) {
                        if (walker.moveToElementById(tokenizer.nextToken())) {
                            resultSet->add(walker.getCurrentPosition());
                        }
                    }
                }
            }
            else {
                nsAutoString idList;
                exprResult->stringValue(idList);
                txTokenizer tokenizer(idList);
                while (tokenizer.hasMoreTokens()) {
                    if (walker.moveToElementById(tokenizer.nextToken())) {
                        resultSet->add(walker.getCurrentPosition());
                    }
                }
            }

            *aResult = resultSet;
            NS_ADDREF(*aResult);

            return NS_OK;
        }
        case LAST:
        {
            if (!requireParams(0, 0, aContext))
                return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;

            return aContext->recycler()->getNumberResult(aContext->size(),
                                                         aResult);
        }
        case LOCAL_NAME:
        case NAME:
        case NAMESPACE_URI:
        {
            if (!requireParams(0, 1, aContext)) {
                return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
            }

            // Check for optional arg
            nsRefPtr<txNodeSet> nodes;
            if (iter.hasNext()) {
                rv = evaluateToNodeSet((Expr*)iter.next(), aContext,
                                       getter_AddRefs(nodes));
                NS_ENSURE_SUCCESS(rv, rv);

                if (nodes->isEmpty()) {
                    aContext->recycler()->getEmptyStringResult(aResult);

                    return NS_OK;
                }
            }

            const txXPathNode& node = nodes ? nodes->get(0) :
                                              aContext->getContextNode();
            switch (mType) {
                case LOCAL_NAME:
                {
                    StringResult* strRes = nsnull;
                    rv = aContext->recycler()->getStringResult(&strRes);
                    NS_ENSURE_SUCCESS(rv, rv);

                    *aResult = strRes;
                    txXPathNodeUtils::getLocalName(node, strRes->mValue);

                    return NS_OK;
                }
                case NAMESPACE_URI:
                {
                    StringResult* strRes = nsnull;
                    rv = aContext->recycler()->getStringResult(&strRes);
                    NS_ENSURE_SUCCESS(rv, rv);

                    *aResult = strRes;
                    txXPathNodeUtils::getNamespaceURI(node, strRes->mValue);

                    return NS_OK;
                }
                case NAME:
                {
                    switch (txXPathNodeUtils::getNodeType(node)) {
                        case txXPathNodeType::ATTRIBUTE_NODE:
                        case txXPathNodeType::ELEMENT_NODE:
                        case txXPathNodeType::PROCESSING_INSTRUCTION_NODE:
                        // XXX Namespace: namespaces have a name
                        {
                            StringResult* strRes = nsnull;
                            rv = aContext->recycler()->getStringResult(&strRes);
                            NS_ENSURE_SUCCESS(rv, rv);

                            *aResult = strRes;
                            txXPathNodeUtils::getNodeName(node, strRes->mValue);

                            return NS_OK;
                        }
                        default:
                        {
                            aContext->recycler()->getEmptyStringResult(aResult);

                            return NS_OK;
                        }
                    }
                }
                default:
                {
                    break;
                }
            }
        }
        case POSITION:
        {
            if (!requireParams(0, 0, aContext))
                return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;

            return aContext->recycler()->getNumberResult(aContext->position(),
                                                         aResult);
        }
    }

    aContext->receiveError(NS_LITERAL_STRING("Internal error"),
                           NS_ERROR_UNEXPECTED);
    return NS_ERROR_UNEXPECTED;
}

nsresult NodeSetFunctionCall::getNameAtom(nsIAtom** aAtom)
{
    switch (mType) {
        case COUNT:
        {
            *aAtom = txXPathAtoms::count;
            break;
        }
        case ID:
        {
            *aAtom = txXPathAtoms::id;
            break;
        }
        case LAST:
        {
            *aAtom = txXPathAtoms::last;
            break;
        }
        case LOCAL_NAME:
        {
            *aAtom = txXPathAtoms::localName;
            break;
        }
        case NAME:
        {
            *aAtom = txXPathAtoms::name;
            break;
        }
        case NAMESPACE_URI:
        {
            *aAtom = txXPathAtoms::namespaceUri;
            break;
        }
        case POSITION:
        {
            *aAtom = txXPathAtoms::position;
            break;
        }
        default:
        {
            *aAtom = 0;
            return NS_ERROR_FAILURE;
        }
    }
    NS_ADDREF(*aAtom);
    return NS_OK;
}
