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
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: libc.c,v $ $Revision: 1.1 $ $Date: 2000/03/31 19:50:16 $ $Name: FIREFOX_0_10_1_RELEASE $";
#endif /* DEBUG */

/*
 * libc.c
 *
 * This file contains our wrappers/reimplementations for "standard" 
 * libc functions.  Things like "memcpy."  We add to this as we need 
 * it.  Oh, and let's keep it in alphabetical order, should it ever 
 * get large.  Most string/character stuff should be in utf8.c, not 
 * here.  This file (and maybe utf8.c) should be the only ones in
 * NSS to include files with angle brackets.
 */

#ifndef BASE_H
#include "base.h"
#endif /* BASE_H */

#include <string.h> /* memcpy, memset */

/*
 * nsslibc_memcpy
 * nsslibc_memset
 * nsslibc_offsetof
 * nsslibc_memequal
 */

/*
 * nsslibc_memcpy
 *
 * Errors:
 *  NSS_ERROR_INVALID_POINTER
 *
 * Return value:
 *  NULL on error
 *  The destination pointer on success
 */

NSS_IMPLEMENT void *
nsslibc_memcpy
(
  void *dest,
  const void *source,
  PRUint32 n
)
{
#ifdef NSSDEBUG
  if( ((void *)NULL == dest) || ((const void *)NULL == source) ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return (void *)NULL;
  }
#endif /* NSSDEBUG */

  return memcpy(dest, source, (size_t)n);
}

/*
 * nsslibc_memset
 *
 * Errors:
 *  NSS_ERROR_INVALID_POINTER
 *
 * Return value:
 *  NULL on error
 *  The destination pointer on success
 */

NSS_IMPLEMENT void *
nsslibc_memset
(
  void *dest,
  PRUint8 byte,
  PRUint32 n
)
{
#ifdef NSSDEBUG
  if( ((void *)NULL == dest) ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return (void *)NULL;
  }
#endif /* NSSDEBUG */

  return memset(dest, (int)byte, (size_t)n);
}

/*
 * nsslibc_memequal
 *
 * Errors:
 *  NSS_ERROR_INVALID_POINTER
 *
 * Return value:
 *  PR_TRUE if they match
 *  PR_FALSE if they don't
 *  PR_FALSE upon error
 */

NSS_IMPLEMENT PRBool
nsslibc_memequal
(
  const void *a,
  const void *b,
  PRUint32 len,
  PRStatus *statusOpt
)
{
#ifdef NSSDEBUG
  if( (((void *)NULL == a) || ((void *)NULL == b)) ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    if( (PRStatus *)NULL != statusOpt ) {
      *statusOpt = PR_FAILURE;
    }
    return PR_FALSE;
  }
#endif /* NSSDEBUG */

  if( (PRStatus *)NULL != statusOpt ) {
    *statusOpt = PR_SUCCESS;
  }

  if( 0 == memcmp(a, b, len) ) {
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}

/*
 * nsslibc_memcmp
 */

NSS_IMPLEMENT PRInt32
nsslibc_memcmp
(
  const void *a,
  const void *b,
  PRUint32 len,
  PRStatus *statusOpt
)
{
  int v;

#ifdef NSSDEBUG
  if( (((void *)NULL == a) || ((void *)NULL == b)) ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    if( (PRStatus *)NULL != statusOpt ) {
      *statusOpt = PR_FAILURE;
    }
    return -2;
  }
#endif /* NSSDEBUG */

  if( (PRStatus *)NULL != statusOpt ) {
    *statusOpt = PR_SUCCESS;
  }

  v = memcmp(a, b, len);
  return (PRInt32)v;
}

/*
 * offsetof is a preprocessor definition
 */
