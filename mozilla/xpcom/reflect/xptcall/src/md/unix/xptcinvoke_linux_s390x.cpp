/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/* Platform specific code to invoke XPCOM methods on native objects */

#include "xptcprivate.h"


static PRUint32
invoke_count_words(PRUint32 paramCount, nsXPTCVariant* s)
{
    PRUint32 overflow = 0, gpr = 1 /*this*/, fpr = 0;
    for(PRUint32 i = 0; i < paramCount; i++, s++)
    {
        if(s->IsPtrData())
        {
            if (gpr < 5) gpr++; else overflow++;
            continue;
        }
        switch(s->type)
        {
        case nsXPTType::T_I8     :
        case nsXPTType::T_I16    :
        case nsXPTType::T_I32    :
        case nsXPTType::T_I64    :
            if (gpr < 5) gpr++; else overflow++;
            break;
        case nsXPTType::T_U8     :
        case nsXPTType::T_U16    :
        case nsXPTType::T_U32    :
        case nsXPTType::T_U64    :
            if (gpr < 5) gpr++; else overflow++;
            break;
        case nsXPTType::T_FLOAT  :
        case nsXPTType::T_DOUBLE :
            if (fpr < 4) fpr++; else overflow++;
            break;
        case nsXPTType::T_BOOL   :
        case nsXPTType::T_CHAR   :
        case nsXPTType::T_WCHAR  :
            if (gpr < 5) gpr++; else overflow++;
            break;
        default:
            // all the others are plain pointer types
            if (gpr < 5) gpr++; else overflow++;
            break;
        }
    }
    /* Round up number of overflow words to ensure stack
       stays aligned to 8 bytes.  */
    return (overflow + 1) & ~1;
}

static void
invoke_copy_to_stack(PRUint32 paramCount, nsXPTCVariant* s, PRUint64* d_ov, PRUint32 overflow)
{
    PRUint64 *d_gpr = d_ov + overflow; 
    PRUint64 *d_fpr = (PRUint64 *)(d_gpr + 4);
    PRUint32 gpr = 1 /*this*/, fpr = 0;

    for(PRUint32 i = 0; i < paramCount; i++, s++)
    {
        if(s->IsPtrData())
        {
            if (gpr < 5) 
                *((void**)d_gpr) = s->ptr, d_gpr++, gpr++;
            else
                *((void**)d_ov ) = s->ptr, d_ov++;
            continue;
        }
        switch(s->type)
        {
        case nsXPTType::T_I8     : 
            if (gpr < 5)
                *((PRInt64*) d_gpr) = s->val.i8, d_gpr++, gpr++;
            else
                *((PRInt64*) d_ov ) = s->val.i8, d_ov++;
            break;
        case nsXPTType::T_I16    : 
            if (gpr < 5)
                *((PRInt64*) d_gpr) = s->val.i16, d_gpr++, gpr++;
            else
                *((PRInt64*) d_ov ) = s->val.i16, d_ov++;
            break;
        case nsXPTType::T_I32    : 
            if (gpr < 5)
                *((PRInt64*) d_gpr) = s->val.i32, d_gpr++, gpr++;
            else
                *((PRInt64*) d_ov ) = s->val.i32, d_ov++;
            break;
        case nsXPTType::T_I64    : 
            if (gpr < 5)
                *((PRInt64*) d_gpr) = s->val.i64, d_gpr++, gpr++;
            else
                *((PRInt64*) d_ov ) = s->val.i64, d_ov++;
            break;
        case nsXPTType::T_U8     : 
            if (gpr < 5)
                *((PRUint64*) d_gpr) = s->val.u8, d_gpr++, gpr++;
            else
                *((PRUint64*) d_ov ) = s->val.u8, d_ov++;
            break;
        case nsXPTType::T_U16    : 
            if (gpr < 5)
                *((PRUint64*)d_gpr) = s->val.u16, d_gpr++, gpr++;
            else
                *((PRUint64*)d_ov ) = s->val.u16, d_ov++;
            break;
        case nsXPTType::T_U32    : 
            if (gpr < 5)
                *((PRUint64*)d_gpr) = s->val.u32, d_gpr++, gpr++;
            else
                *((PRUint64*)d_ov ) = s->val.u32, d_ov++;
            break;
        case nsXPTType::T_U64    : 
            if (gpr < 5)
                *((PRUint64*)d_gpr) = s->val.u64, d_gpr++, gpr++;
            else
                *((PRUint64*)d_ov ) = s->val.u64, d_ov++;
            break;
        case nsXPTType::T_FLOAT  : 
            if (fpr < 4)
                *((float*)   d_fpr)    = s->val.f, d_fpr++, fpr++;
            else
                *(((float*)  d_ov )+1) = s->val.f, d_ov++;
            break;
        case nsXPTType::T_DOUBLE : 
            if (fpr < 4)
                *((double*)  d_fpr) = s->val.d, d_fpr++, fpr++;
            else
                *((double*)  d_ov ) = s->val.d, d_ov++;
            break;
        case nsXPTType::T_BOOL   : 
            if (gpr < 5)
                *((PRUint64*)d_gpr) = s->val.b, d_gpr++, gpr++;
            else
                *((PRUint64*)d_ov ) = s->val.b, d_ov++;
            break;
        case nsXPTType::T_CHAR   : 
            if (gpr < 5)
                *((PRUint64*)d_gpr) = s->val.c, d_gpr++, gpr++;
            else
                *((PRUint64*)d_ov ) = s->val.c, d_ov++;
            break;
        case nsXPTType::T_WCHAR  : 
            if (gpr < 5)
                *((PRUint64*)d_gpr) = s->val.wc, d_gpr++, gpr++;
            else
                *((PRUint64*)d_ov ) = s->val.wc, d_ov++;
            break;
        default:
            // all the others are plain pointer types
            if (gpr < 5) 
                *((void**)   d_gpr) = s->val.p, d_gpr++, gpr++;
            else
                *((void**)   d_ov ) = s->val.p, d_ov++;
            break;
        }
    }
}

XPTC_PUBLIC_API(nsresult)
XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
    PRUint64 *vtable = *(PRUint64 **)that;
#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 /* G++ V3 ABI */
    PRUint64 method = vtable[methodIndex];
#else /* not G++ V3 ABI  */
    PRUint64 method = vtable[methodIndex + 2];
#endif /* G++ V3 ABI */
    PRUint64 overflow = invoke_count_words (paramCount, params);
    PRUint64 result;

    __asm__ __volatile__
    (
        "lgr    7,15\n\t"
        "aghi   7,-64\n\t"

        "lgr    3,%3\n\t"
        "sllg   3,3,3\n\t"
        "lcgr   3,3\n\t"
        "lg     2,0(15)\n\t"
        "la     15,0(3,7)\n\t"
        "stg    2,0(15)\n\t"

        "lgr    2,%1\n\t"
        "lgr    3,%2\n\t"
        "la     4,160(15)\n\t"
        "lgr    5,%3\n\t"
        "basr   14,%4\n\t"

        "lgr    2,%5\n\t"
        "ld     0,192(7)\n\t"
        "ld     2,200(7)\n\t"
        "ld     4,208(7)\n\t"
        "ld     6,216(7)\n\t"
        "lmg    3,6,160(7)\n\t"
        "basr   14,%6\n\t"

        "la     15,64(7)\n\t"

        "lgr    %0,2\n\t"
        : "=r" (result)
        : "r" ((PRUint64)paramCount),
          "r" (params),
          "r" (overflow),
          "a" (invoke_copy_to_stack),
          "a" (that),
          "a" (method)
        : "2", "3", "4", "5", "6", "7", "memory"
    );
  
    return result;
}    
