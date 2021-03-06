
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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/* Implement shared vtbl methods. */

#include "xptcprivate.h"

#if _HPUX
#error "This code is for HP-PA RISC 32 bit mode only"
#endif

extern "C" nsresult
PrepareAndDispatch(nsXPTCStubBase* self, PRUint32 methodIndex,
  PRUint32* args, PRUint32* floatargs)
{

  typedef struct {
    uint32 hi;
    uint32 lo;
  } DU;

#define PARAM_BUFFER_COUNT     16

  nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
  nsXPTCMiniVariant* dispatchParams = NULL;
  nsIInterfaceInfo* iface_info = NULL;
  const nsXPTMethodInfo* info;
  PRInt32 regwords = 1; /* self pointer is not in the variant records */
  nsresult result = NS_ERROR_FAILURE;
  PRUint8 paramCount;
  PRUint8 i;

  NS_ASSERTION(self,"no self");

  self->GetInterfaceInfo(&iface_info);
  NS_ASSERTION(iface_info,"no interface info");

  iface_info->GetMethodInfo(PRUint16(methodIndex), &info);
  NS_ASSERTION(info,"no interface info");

  paramCount = info->GetParamCount();

  // setup variant array pointer
  if(paramCount > PARAM_BUFFER_COUNT)
    dispatchParams = new nsXPTCMiniVariant[paramCount];
  else
    dispatchParams = paramBuffer;
  NS_ASSERTION(dispatchParams,"no place for params");

  for(i = 0; i < paramCount; ++i, --args)
  {
    const nsXPTParamInfo& param = info->GetParam(i);
    const nsXPTType& type = param.GetType();
    nsXPTCMiniVariant* dp = &dispatchParams[i];

    if(param.IsOut() || !type.IsArithmetic())
    {
      dp->val.p = (void*) *args;
      ++regwords;
      continue;
    }
    switch(type)
    {
    case nsXPTType::T_I8     : dp->val.i8  = *((PRInt32*) args); break;
    case nsXPTType::T_I16    : dp->val.i16 = *((PRInt32*) args); break;
    case nsXPTType::T_I32    : dp->val.i32 = *((PRInt32*) args); break;
    case nsXPTType::T_DOUBLE :
                               if (regwords & 1)
                               {
                                 ++regwords; /* align on double word */
                                 --args;
                               }
                               if (regwords == 0 || regwords == 2)
                               {
                                 dp->val.d=*((double*) (floatargs + regwords));
                                 --args;
                               }
                               else
                               {
                                 dp->val.d = *((double*) --args);
                               }
                               regwords += 2;
                               continue;
    case nsXPTType::T_U64    :
    case nsXPTType::T_I64    :
                               if (regwords & 1)
                               {
                                 ++regwords; /* align on double word */
                                 --args;
                               }
                               ((DU *)dp)->lo = *((PRUint32*) args);
                               ((DU *)dp)->hi = *((PRUint32*) --args);
                               regwords += 2;
                               continue;
    case nsXPTType::T_FLOAT  :
                               if (regwords >= 4)
                                 dp->val.f = *((float*) args);
                               else
                                 dp->val.f = *((float*) floatargs+4+regwords);
                               break;
    case nsXPTType::T_U8     : dp->val.u8  = *((PRUint32*) args); break;
    case nsXPTType::T_U16    : dp->val.u16 = *((PRUint32*) args); break;
    case nsXPTType::T_U32    : dp->val.u32 = *((PRUint32*) args); break;
    case nsXPTType::T_BOOL   : dp->val.b   = *((PRBool*)   args); break;
    case nsXPTType::T_CHAR   : dp->val.c   = *((PRUint32*) args); break;
    case nsXPTType::T_WCHAR  : dp->val.wc  = *((PRInt32*)  args); break;
    default:
      NS_ASSERTION(0, "bad type");
      break;
    }
    ++regwords;
  }

  result = self->CallMethod((PRUint16) methodIndex, info, dispatchParams);

  NS_RELEASE(iface_info);

  if(dispatchParams != paramBuffer)
    delete [] dispatchParams;

  return result;
}

extern "C" int SharedStub(int);

#define STUB_ENTRY(n)       \
nsresult nsXPTCStubBase::Stub##n()  \
{                           \
    return SharedStub(n);   \
}

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ASSERTION(0,"nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"

