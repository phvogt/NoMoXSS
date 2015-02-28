/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
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
#include "txAtoms.h"
#include "txXPathTreeWalker.h"
#include "txIXPathContext.h"

txNameTest::txNameTest(nsIAtom* aPrefix, nsIAtom* aLocalName, PRInt32 aNSID,
                       PRUint16 aNodeType)
    :mPrefix(aPrefix), mLocalName(aLocalName), mNamespace(aNSID),
     mNodeType(aNodeType)
{
    if (aPrefix == txXMLAtoms::_empty)
        mPrefix = 0;
    NS_ASSERTION(aLocalName, "txNameTest without a local name?");
}

txNameTest::~txNameTest()
{
}

PRBool txNameTest::matches(const txXPathNode& aNode, txIMatchContext* aContext)
{
    if (txXPathNodeUtils::getNodeType(aNode) != mNodeType)
        return MB_FALSE;

    // Totally wild?
    if (mLocalName == txXPathAtoms::_asterix && !mPrefix)
        return MB_TRUE;

    // Compare namespaces
    if (txXPathNodeUtils::getNamespaceID(aNode) != mNamespace)
        return MB_FALSE;

    // Name wild?
    if (mLocalName == txXPathAtoms::_asterix)
        return MB_TRUE;

    // Compare local-names
    nsCOMPtr<nsIAtom> localName = txXPathNodeUtils::getLocalName(aNode);
    return localName == mLocalName;
}

/*
 * Returns the default priority of this txNodeTest
 */
double txNameTest::getDefaultPriority()
{
    if (mLocalName == txXPathAtoms::_asterix) {
        if (!mPrefix)
            return -0.5;
        return -0.25;
    }
    return 0;
}

/*
 * Returns the String representation of this txNodeTest.
 * @param aDest the String to use when creating the string representation.
 *              The string representation will be appended to the string.
 */
void txNameTest::toString(nsAString& aDest)
{
    if (mPrefix) {
        nsAutoString prefix;
        mPrefix->ToString(prefix);
        aDest.Append(prefix);
        aDest.Append(PRUnichar(':'));
    }
    nsAutoString localName;
    mLocalName->ToString(localName);
    aDest.Append(localName);
}
