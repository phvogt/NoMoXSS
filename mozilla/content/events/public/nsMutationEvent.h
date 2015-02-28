/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
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
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsGUIEvent.h"
#include "nsIDOMNode.h"
#include "nsIAtom.h"
#include "nsIDOMEventTarget.h"
#include "nsIContent.h"

#define NS_MUTATION_EVENT     20

struct nsMutationEvent : public nsEvent
{             
  nsMutationEvent(PRUint32 msg = 0, PRUint8 structType = NS_MUTATION_EVENT)
    : nsEvent(msg, structType),
      mAttrChange(0)
  {
  }

  nsMutationEvent(PRUint32 msg,
                  nsIDOMEventTarget *target,
                  PRUint8 structType = NS_MUTATION_EVENT)
    : nsEvent(msg, structType),
      mTarget(target),
      mAttrChange(0)
  {
  }

  nsMutationEvent(PRUint32 msg,
                  nsIContent *target,
                  PRUint8 structType = NS_MUTATION_EVENT)
    : nsEvent(msg, structType),
      mTarget(do_QueryInterface(target)),
      mAttrChange(0)
  {
  }

  nsCOMPtr<nsIDOMNode> mRelatedNode;
  nsCOMPtr<nsIDOMEventTarget> mTarget;
  
  nsCOMPtr<nsIAtom> mAttrName;
  nsCOMPtr<nsIAtom> mPrevAttrValue;
  nsCOMPtr<nsIAtom> mNewAttrValue;
  
  unsigned short mAttrChange;
};

#define NS_MUTATION_START           1800
#define NS_MUTATION_SUBTREEMODIFIED                   (NS_MUTATION_START)
#define NS_MUTATION_NODEINSERTED                      (NS_MUTATION_START+1)
#define NS_MUTATION_NODEREMOVED                       (NS_MUTATION_START+2)
#define NS_MUTATION_NODEREMOVEDFROMDOCUMENT           (NS_MUTATION_START+3)
#define NS_MUTATION_NODEINSERTEDINTODOCUMENT          (NS_MUTATION_START+4)
#define NS_MUTATION_ATTRMODIFIED                      (NS_MUTATION_START+5)
#define NS_MUTATION_CHARACTERDATAMODIFIED             (NS_MUTATION_START+6)

// Bits are actually checked to optimize mutation event firing.
// That's why I don't number from 0x00.  The first event should
// always be 0x01.
#define NS_EVENT_BITS_MUTATION_SUBTREEMODIFIED                0x01
#define NS_EVENT_BITS_MUTATION_NODEINSERTED                   0x02
#define NS_EVENT_BITS_MUTATION_NODEREMOVED                    0x04
#define NS_EVENT_BITS_MUTATION_NODEREMOVEDFROMDOCUMENT        0x08
#define NS_EVENT_BITS_MUTATION_NODEINSERTEDINTODOCUMENT       0x10
#define NS_EVENT_BITS_MUTATION_ATTRMODIFIED                   0x20
#define NS_EVENT_BITS_MUTATION_CHARACTERDATAMODIFIED          0x40
