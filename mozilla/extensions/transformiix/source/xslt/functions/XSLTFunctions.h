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
 * The Original Code is XSL:P XSLT processor.
 *
 * The Initial Developer of the Original Code is Keith Visco.
 * Portions created by Keith Visco (C) 1999, 2000 Keith Visco.
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 * Olivier Gerardin,
 *    -- added document() function definition
 *
 * Jonas Sicking,
 *    -- added txXSLKey class
 *
 */

#ifndef TRANSFRMX_XSLT_FUNCTIONS_H
#define TRANSFRMX_XSLT_FUNCTIONS_H

#include "Expr.h"
#include "XMLUtils.h"
#include "nsAutoPtr.h"

class txPattern;
class txStylesheet;
class txKeyValueHashKey;
class txExecutionState;
class txNamespaceMap;

/**
 * The definition for the XSLT document() function
**/
class DocumentFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new document() function call
    **/
    DocumentFunctionCall(const nsAString& aBaseURI);

    TX_DECL_FUNCTION;

private:
    nsString mBaseURI;
};

/*
 * The definition for the XSLT key() function
 */
class txKeyFunctionCall : public FunctionCall {

public:

    /*
     * Creates a new key() function call
     */
    txKeyFunctionCall(txNamespaceMap* aMappings);

    TX_DECL_FUNCTION;

private:
    nsRefPtr<txNamespaceMap> mMappings;
};

/**
 * The definition for the XSLT format-number() function
**/
class txFormatNumberFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new format-number() function call
    **/
    txFormatNumberFunctionCall(txStylesheet* aStylesheet, txNamespaceMap* aMappings);

    TX_DECL_FUNCTION;

private:
    static const PRUnichar FORMAT_QUOTE;

    enum FormatParseState {
        Prefix,
        IntDigit,
        IntZero,
        FracZero,
        FracDigit,
        Suffix,
        Finished
    };
    
    txStylesheet* mStylesheet;
    nsRefPtr<txNamespaceMap> mMappings;
};

/**
 * DecimalFormat
 * A representation of the XSLT element <xsl:decimal-format>
 */
class txDecimalFormat : public TxObject {

public:
    /*
     * Creates a new decimal format and initilizes all properties with
     * default values
     */
    txDecimalFormat();
    MBool isEqual(txDecimalFormat* other);
    
    PRUnichar       mDecimalSeparator;
    PRUnichar       mGroupingSeparator;
    nsString        mInfinity;
    PRUnichar       mMinusSign;
    nsString        mNaN;
    PRUnichar       mPercent;
    PRUnichar       mPerMille;
    PRUnichar       mZeroDigit;
    PRUnichar       mDigit;
    PRUnichar       mPatternSeparator;
};

/**
 * The definition for the XSLT current() function
**/
class CurrentFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new current() function call
    **/
    CurrentFunctionCall();

    TX_DECL_FUNCTION;
};

/**
 * The definition for the XSLT unparsed-entity-uri() function
**/
class UnparsedEntityUriFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new unparsed-entity-uri() function call
    **/
    UnparsedEntityUriFunctionCall();

    TX_DECL_FUNCTION;

private:
};

/**
 * The definition for the XSLT generate-id() function
**/
class GenerateIdFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new generate-id() function call
    **/
    GenerateIdFunctionCall();

    TX_DECL_FUNCTION;
};

/**
 * The definition for the XSLT system-property() function
**/
class SystemPropertyFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new system-property() function call
     * aNode is the Element in the stylesheet containing the 
     * Expr and is used for namespaceID resolution
    **/
    SystemPropertyFunctionCall(txNamespaceMap* aMappings);

    TX_DECL_FUNCTION;

private:
    /*
     * resolve namespaceIDs with this node
     */
    nsRefPtr<txNamespaceMap> mMappings;
};

/**
 * The definition for the XSLT element-available() function
**/
class ElementAvailableFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new element-available() function call
     * aNode is the Element in the stylesheet containing the 
     * Expr and is used for namespaceID resolution
    **/
    ElementAvailableFunctionCall(txNamespaceMap* aMappings);

    TX_DECL_FUNCTION;

private:
    /*
     * resolve namespaceIDs with this node
     */
    nsRefPtr<txNamespaceMap> mMappings;
};

/**
 * The definition for the XSLT function-available() function
**/
class FunctionAvailableFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new function-available() function call
    **/
    FunctionAvailableFunctionCall(txNamespaceMap* aMappings);

    TX_DECL_FUNCTION;

private:
    /*
     * resolve namespaceIDs with this node
     */
    nsRefPtr<txNamespaceMap> mMappings;
};

#endif
