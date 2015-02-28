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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Brian Ryner.
 * Portions created by Brian Ryner are Copyright (C) 2000 Brian Ryner.
 * All Rights Reserved.
 *
 * Contributor(s): 
 *  Brian Ryner <bryner@brianryner.com>
 */

#include "nsIGenericFactory.h"
#include "nsFingerHandler.h"

static const nsModuleComponentInfo gResComponents[] = {
    { "The Finger Protocol Handler", 
      NS_FINGERHANDLER_CID,
      NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "finger",
      nsFingerHandler::Create
    }
};

NS_IMPL_NSGETMODULE(finger, gResComponents)
