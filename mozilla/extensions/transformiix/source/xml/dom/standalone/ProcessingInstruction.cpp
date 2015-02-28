/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * (C) Copyright The MITRE Corporation 1999  All rights reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * The program provided "as is" without any warranty express or
 * implied, including the warranty of non-infringement and the implied
 * warranties of merchantibility and fitness for a particular purpose.
 * The Copyright owner will not be liable for any damages suffered by
 * you as a result of using the Program. In no event will the Copyright
 * owner be liable for any special, indirect or consequential damages or
 * lost profits even if the Copyright owner has been advised of the
 * possibility of their occurrence.
 *
 * Please see release.txt distributed with this file for more information.
 *
 */
// Tom Kneeland (3/29/99)
//
//  Implementation of the Document Object Model Level 1 Core
//    Implementation of the ProcessingInstruction class
//
// Modification History:
// Who  When      What
// TK   03/29/99  Created
//

#include "dom.h"
#include "nsIAtom.h"

//
//Construct a text object with the specified document owner and data
//
ProcessingInstruction::ProcessingInstruction(const nsAString& theTarget,
                                             const nsAString& theData,
                                             Document* owner) :
                       NodeDefinition(Node::PROCESSING_INSTRUCTION_NODE,
                                      theTarget, theData, owner)
{
  mLocalName = do_GetAtom(nodeName);
}

//
//Release the mLocalName
//
ProcessingInstruction::~ProcessingInstruction()
{
}

//
//ProcessingInstruction nodes can not have any children, so just return null
//from all child manipulation functions.
//

MBool ProcessingInstruction::getLocalName(nsIAtom** aLocalName)
{
  if (!aLocalName)
    return MB_FALSE;
  *aLocalName = mLocalName;
  NS_ADDREF(*aLocalName);
  return MB_TRUE;
}
