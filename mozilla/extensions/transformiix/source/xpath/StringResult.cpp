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
 */

/**
 * StringResult
 * Represents a String as a Result of evaluating an Expr
**/
#include "ExprResult.h"

/**
 * Default Constructor
**/
StringResult::StringResult(txResultRecycler* aRecycler)
    : txAExprResult(aRecycler)
{
}

/**
 * Creates a new StringResult with the value of the given String parameter
 * @param str the String to use for initialization of this StringResult's value
**/
StringResult::StringResult(const nsAString& aValue, txResultRecycler* aRecycler)
    : txAExprResult(aRecycler), mValue(aValue)
{
}

/*
 * Virtual Methods from ExprResult
*/

short StringResult::getResultType() {
    return txAExprResult::STRING;
} //-- getResultType

void StringResult::stringValue(nsAString& str)  {
    str.Append(mValue);
} //-- stringValue

nsAString*
StringResult::stringValuePointer()
{
    return &mValue;
}

MBool StringResult::booleanValue() {
   return !mValue.IsEmpty();
} //-- booleanValue

double StringResult::numberValue() {
    return Double::toDouble(mValue);
} //-- numberValue

