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
 * GC related routines
 *
 */
#include "primpl.h"

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np) 
{
    CONTEXTRECORD context;
    context.ContextFlags = CONTEXT_INTEGER;

    if (_PR_IS_NATIVE_THREAD(t)) {
        context.ContextFlags |= CONTEXT_CONTROL;
        if (QueryThreadContext(t->md.handle, CONTEXT_CONTROL, &context)) {
            t->md.gcContext[0] = context.ctx_RegEax;
            t->md.gcContext[1] = context.ctx_RegEbx;
            t->md.gcContext[2] = context.ctx_RegEcx;
            t->md.gcContext[3] = context.ctx_RegEdx;
            t->md.gcContext[4] = context.ctx_RegEsi;
            t->md.gcContext[5] = context.ctx_RegEdi;
            t->md.gcContext[6] = context.ctx_RegEsp;
            t->md.gcContext[7] = context.ctx_RegEbp;
            *np = PR_NUM_GCREGS;
        } else {
            PR_ASSERT(0);/* XXX */
        }
    }
    return (PRWord *)&t->md.gcContext;
}

/* This function is not used right now, but is left as a reference.
 * If you ever need to get the fiberID from the currently running fiber, 
 * this is it.
 */
void *
GetMyFiberID()
{
    void *fiberData = 0;

    /* A pointer to our tib entry is found at FS:[18]
     * At offset 10h is the fiberData pointer.  The context of the 
     * fiber is stored in there.  
     */
#ifdef HAVE_ASM
    __asm {
        mov    EDX, FS:[18h]
        mov    EAX, DWORD PTR [EDX+10h]
        mov    [fiberData], EAX
    }
#endif
  
    return fiberData;
}
