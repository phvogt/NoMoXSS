/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
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

/*
 * The DLL entry point (DllMain) for NSPR.
 *
 * The only reason we use DLLMain() now is to find out whether
 * the NSPR DLL is statically or dynamically loaded.  When
 * dynamically loaded, we cannot use static thread-local storage.
 * However, static TLS is faster than the TlsXXX() functions.
 * So we want to use static TLS whenever we can.  A global
 * variable _pr_use_static_tls is set in DllMain() during process
 * attachment to indicate whether it is safe to use static TLS
 * or not.
 */

#include <windows.h>
#include <primpl.h>

extern BOOL _pr_use_static_tls;  /* defined in ntthread.c */

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
PRThread *me;

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            /*
             * If lpvReserved is NULL, we are dynamically loaded
             * and therefore can't use static thread-local storage.
             */
            if (lpvReserved == NULL) {
                _pr_use_static_tls = FALSE;
            } else {
                _pr_use_static_tls = TRUE;
            }
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            if (_pr_initialized) {
                me = _MD_GET_ATTACHED_THREAD();
                if ((me != NULL) && (me->flags & _PR_ATTACHED))
                    _PRI_DetachThread();
            }
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
