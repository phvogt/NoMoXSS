/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.
 * All Rights Reserved.
 * 
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#include "nsError.h"

/**
 * imagelib specific nsresult success and error codes
 */
#define NS_IMAGELIB_SUCCESS_LOAD_FINISHED   NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_IMGLIB, 0)
#define NS_IMAGELIB_CHANGING_OWNER          NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_IMGLIB, 1)

#define NS_IMAGELIB_ERROR_FAILURE           NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_IMGLIB, 5)
#define NS_IMAGELIB_ERROR_NO_DECODER        NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_IMGLIB, 6)
#define NS_IMAGELIB_ERROR_NOT_FINISHED      NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_IMGLIB, 7)
#define NS_IMAGELIB_ERROR_LOAD_ABORTED      NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_IMGLIB, 8)

