/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

/*
 * JS script operations.
 */
#include "jsstddef.h"
#include <string.h>
#include "jstypes.h"
#include "jsutil.h" /* Added by JSIFY */
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsdbgapi.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsscript.h"
#if JS_HAS_XDR
#include "jsxdrapi.h"
#endif

#if JS_HAS_SCRIPT_OBJECT

#if JS_HAS_TOSOURCE
static JSBool
script_toSource(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
    JSScript *script;
    size_t i, j, k, n;
    char buf[16];
    jschar *s, *t;
    uint32 indent;
    JSString *str;

    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;
    script = (JSScript *) JS_GetPrivate(cx, obj);

    /* Let n count the source string length, j the "front porch" length. */
    j = JS_snprintf(buf, sizeof buf, "(new %s(", js_ScriptClass.name);
    n = j + 2;
    if (!script) {
        /* Let k count the constructor argument string length. */
        k = 0;
        s = NULL;               /* quell GCC overwarning */
    } else {
        indent = 0;
        if (argc && !js_ValueToECMAUint32(cx, argv[0], &indent))
            return JS_FALSE;
        str = JS_DecompileScript(cx, script, "Script.prototype.toSource",
                                 (uintN)indent);
        if (!str)
            return JS_FALSE;
        str = js_QuoteString(cx, str, '\'');
        if (!str)
            return JS_FALSE;
        s = JSSTRING_CHARS(str);
        k = JSSTRING_LENGTH(str);
        n += k;
    }

    /* Allocate the source string and copy into it. */
    t = (jschar *) JS_malloc(cx, (n + 1) * sizeof(jschar));
    if (!t)
        return JS_FALSE;
    for (i = 0; i < j; i++)
        t[i] = buf[i];
    for (j = 0; j < k; i++, j++)
        t[i] = s[j];
    t[i++] = ')';
    t[i++] = ')';
    t[i] = 0;

    /* Create and return a JS string for t. */
    str = JS_NewUCString(cx, t, n);
    if (!str) {
        JS_free(cx, t);
        return JS_FALSE;
    }
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}
#endif /* JS_HAS_TOSOURCE */

static JSBool
script_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
    JSScript *script;
    uint32 indent;
    JSString *str;

    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;
    script = (JSScript *) JS_GetPrivate(cx, obj);
    if (!script) {
        *rval = STRING_TO_JSVAL(cx->runtime->emptyString);
        return JS_TRUE;
    }

    indent = 0;
    if (argc && !js_ValueToECMAUint32(cx, argv[0], &indent))
        return JS_FALSE;
    str = JS_DecompileScript(cx, script, "Script.prototype.toString",
                             (uintN)indent);
    if (!str)
        return JS_FALSE;
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
script_compile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
    JSScript *oldscript, *script;
    JSString *str;
    JSStackFrame *fp, *caller;
    JSObject *scopeobj;
    const char *file;
    uintN line;
    JSPrincipals *principals;

    /* Make sure obj is a Script object. */
    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;

    /* If no args, leave private undefined and return early. */
    if (argc == 0)
        goto out;

    /* Otherwise, the first arg is the script source to compile. */
    str = js_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;

    /* Compile using the caller's scope chain, which js_Invoke passes to fp. */
    fp = cx->fp;
    caller = JS_GetScriptedCaller(cx, fp);
    JS_ASSERT(!caller || fp->scopeChain == caller->scopeChain);

    scopeobj = NULL;
    if (argc >= 2) {
        if (!js_ValueToObject(cx, argv[1], &scopeobj))
            return JS_FALSE;
        argv[1] = OBJECT_TO_JSVAL(scopeobj);
    }
    if (caller) {
        if (!scopeobj)
            scopeobj = caller->scopeChain;

        file = caller->script->filename;
        line = js_PCToLineNumber(cx, caller->script, caller->pc);
        principals = JS_EvalFramePrincipals(cx, fp, caller);
    } else {
        file = NULL;
        line = 0;
        principals = NULL;
    }

    /* Compile the new script using the caller's scope chain, a la eval(). */
    fp->flags |= JSFRAME_EVAL;
    script = JS_CompileUCScriptForPrincipals(cx, scopeobj, principals,
                                             JSSTRING_CHARS(str),
                                             JSSTRING_LENGTH(str),
                                             file, line);
    if (!script)
        return JS_FALSE;

    /* Swap script for obj's old script, if any. */
    oldscript = (JSScript *) JS_GetPrivate(cx, obj);
    if (!JS_SetPrivate(cx, obj, script)) {
        js_DestroyScript(cx, script);
        return JS_FALSE;
    }
    if (oldscript)
        js_DestroyScript(cx, oldscript);

    script->object = obj;
out:
    /* Return the object. */
    *rval = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

static JSBool
script_exec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSScript *script;
    JSObject *scopeobj, *parent;
    JSStackFrame *fp, *caller;

    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;
    script = (JSScript *) JS_GetPrivate(cx, obj);
    if (!script)
        return JS_TRUE;

    scopeobj = NULL;
    if (argc) {
        if (!js_ValueToObject(cx, argv[0], &scopeobj))
            return JS_FALSE;
        argv[0] = OBJECT_TO_JSVAL(scopeobj);
    }

    /*
     * Emulate eval() by using caller's this, var object, sharp array, etc.,
     * all propagated by js_Execute via a non-null fourth (down) argument to
     * js_Execute.  If there is no scripted caller, js_Execute uses its second
     * (chain) argument to set the exec frame's varobj, thisp, and scopeChain.
     *
     * Unlike eval, which the compiler detects, Script.prototype.exec may be
     * called from a lightweight function, or even from native code (in which
     * case fp->varobj and fp->scopeChain are null).  If exec is called from
     * a lightweight function, we will need to get a Call object representing
     * its frame, to act as the var object and scope chain head.
     */
    fp = cx->fp;
    caller = JS_GetScriptedCaller(cx, fp);
    if (caller && !caller->varobj) {
        /* Called from a lightweight function. */
        JS_ASSERT(caller->fun && !(caller->fun->flags & JSFUN_HEAVYWEIGHT));

        /* Scope chain links from Call object to callee's parent. */
        parent = OBJ_GET_PARENT(cx, JSVAL_TO_OBJECT(caller->argv[-2]));
        if (!js_GetCallObject(cx, caller, parent))
            return JS_FALSE;
    }

    if (!scopeobj) {
        /* No scope object passed in: try to use the caller's scope chain. */
        if (caller) {
            /*
             * Load caller->scopeChain after the conditional js_GetCallObject
             * call above, which resets scopeChain as well as varobj.
             */
            scopeobj = caller->scopeChain;
        } else {
            /*
             * Called from native code, so we don't know what scope object to
             * use.  We could use parent (see above), but Script.prototype.exec
             * might be a shared/sealed "superglobal" method.  A more general
             * approach would use cx->globalObject, which will be the same as
             * exec.__parent__ in the non-superglobal case.  In the superglobal
             * case it's the right object: the global, not the superglobal.
             */
            scopeobj = cx->globalObject;
        }
    }

    return js_Execute(cx, scopeobj, script, caller, JSFRAME_EVAL, rval);
}

#if JS_HAS_XDR

static JSBool
XDRAtomListElement(JSXDRState *xdr, JSAtomListElement *ale)
{
    jsval value;
    jsatomid index;

    if (xdr->mode == JSXDR_ENCODE)
        value = ATOM_KEY(ALE_ATOM(ale));

    index = ALE_INDEX(ale);
    if (!JS_XDRUint32(xdr, &index))
        return JS_FALSE;
    ALE_SET_INDEX(ale, index);

    if (!JS_XDRValue(xdr, &value))
        return JS_FALSE;

    if (xdr->mode == JSXDR_DECODE) {
        if (!ALE_SET_ATOM(ale, js_AtomizeValue(xdr->cx, value, 0)))
            return JS_FALSE;
    }
    return JS_TRUE;
}

static JSBool
XDRAtomMap(JSXDRState *xdr, JSAtomMap *map)
{
    uint32 length;
    uintN i;
    JSBool ok;

    if (xdr->mode == JSXDR_ENCODE)
        length = map->length;

    if (!JS_XDRUint32(xdr, &length))
        return JS_FALSE;

    if (xdr->mode == JSXDR_DECODE) {
        JSContext *cx;
        void *mark;
        JSAtomList al;
        JSAtomListElement *ale;

        cx = xdr->cx;
        mark = JS_ARENA_MARK(&cx->tempPool);
        ATOM_LIST_INIT(&al);
        for (i = 0; i < length; i++) {
            JS_ARENA_ALLOCATE_TYPE(ale, JSAtomListElement, &cx->tempPool);
            if (!ale ||
                !XDRAtomListElement(xdr, ale)) {
                if (!ale)
                    JS_ReportOutOfMemory(cx);
                JS_ARENA_RELEASE(&cx->tempPool, mark);
                return JS_FALSE;
            }
            ALE_SET_NEXT(ale, al.list);
            al.count++;
            al.list = ale;
        }
        ok = js_InitAtomMap(cx, map, &al);
        JS_ARENA_RELEASE(&cx->tempPool, mark);
        return ok;
    }

    if (xdr->mode == JSXDR_ENCODE) {
        JSAtomListElement ale;

        for (i = 0; i < map->length; i++) {
            ALE_SET_ATOM(&ale, map->vector[i]);
            ALE_SET_INDEX(&ale, i);
            if (!XDRAtomListElement(xdr, &ale))
                return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSBool
js_XDRScript(JSXDRState *xdr, JSScript **scriptp, JSBool *hasMagic)
{
    JSContext *cx;
    JSScript *script, *newscript;
    uint32 length, lineno, depth, magic, nsrcnotes, ntrynotes;
    uint32 prologLength, version;
    JSBool filenameWasSaved;
    jssrcnote *notes, *sn;

    cx = xdr->cx;
    script = *scriptp;
    nsrcnotes = ntrynotes = 0;
    filenameWasSaved = JS_FALSE;
    notes = NULL;

    /*
     * Encode prologLength and version after script->length (_2 or greater),
     * but decode both new (>= _2) and old, prolog&version-free (_1) scripts.
     * Version _3 supports principals serialization.  Version _4 reorders the
     * nsrcnotes and ntrynotes fields to come before everything except magic,
     * length, prologLength, and version, so that srcnote and trynote storage
     * can be allocated as part of the JSScript (along with bytecode storage).
     */
    if (xdr->mode == JSXDR_ENCODE)
        magic = JSXDR_MAGIC_SCRIPT_CURRENT;
    if (!JS_XDRUint32(xdr, &magic))
        return JS_FALSE;
    if (magic != JSXDR_MAGIC_SCRIPT_4 &&
        magic != JSXDR_MAGIC_SCRIPT_3 &&
        magic != JSXDR_MAGIC_SCRIPT_2 &&
        magic != JSXDR_MAGIC_SCRIPT_1) {
        if (!hasMagic) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_SCRIPT_MAGIC);
            return JS_FALSE;
        }
        *hasMagic = JS_FALSE;
        return JS_TRUE;
    }
    if (hasMagic)
        *hasMagic = JS_TRUE;

    if (xdr->mode == JSXDR_ENCODE) {
        length = script->length;
        prologLength = PTRDIFF(script->main, script->code, jsbytecode);
        version = (int32)script->version;
        lineno = (uint32)script->lineno;
        depth = (uint32)script->depth;

        /* Count the srcnotes, keeping notes pointing at the first one. */
        notes = SCRIPT_NOTES(script);
        for (sn = notes; !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn))
            continue;
        nsrcnotes = PTRDIFF(sn, notes, jssrcnote);
        nsrcnotes++;            /* room for the terminator */

        /* Count the trynotes. */
        if (script->trynotes) {
            while (script->trynotes[ntrynotes].catchStart)
                ntrynotes++;
            ntrynotes++;        /* room for the end marker */
        }
    }

    if (!JS_XDRUint32(xdr, &length))
        return JS_FALSE;
    if (magic >= JSXDR_MAGIC_SCRIPT_2) {
        if (!JS_XDRUint32(xdr, &prologLength))
            return JS_FALSE;
        if (!JS_XDRUint32(xdr, &version))
            return JS_FALSE;

        /* To fuse allocations, we need srcnote and trynote counts early. */
        if (magic >= JSXDR_MAGIC_SCRIPT_4) {
            if (!JS_XDRUint32(xdr, &nsrcnotes))
                return JS_FALSE;
            if (!JS_XDRUint32(xdr, &ntrynotes))
                return JS_FALSE;
        }
    }

    if (xdr->mode == JSXDR_DECODE) {
        script = js_NewScript(cx, length, nsrcnotes, ntrynotes);
        if (!script)
            return JS_FALSE;
        if (magic >= JSXDR_MAGIC_SCRIPT_2) {
            script->main += prologLength;
            script->version = (JSVersion) version;

            /* If we know nsrcnotes, we allocated space for notes in script. */
            if (magic >= JSXDR_MAGIC_SCRIPT_4)
                notes = SCRIPT_NOTES(script);
        }
        *scriptp = script;
    }

    /*
     * Control hereafter must goto error on failure, in order for the DECODE
     * case to destroy script and conditionally free notes, which if non-null
     * in the (DECODE and version < _4) case must point at a temporary vector
     * allocated just below.
     */
    if (!JS_XDRBytes(xdr, (char *)script->code, length * sizeof(jsbytecode)) ||
        !XDRAtomMap(xdr, &script->atomMap)) {
        goto error;
    }

    if (magic < JSXDR_MAGIC_SCRIPT_4) {
        if (!JS_XDRUint32(xdr, &nsrcnotes))
            goto error;
        if (xdr->mode == JSXDR_DECODE) {
            notes = (jssrcnote *) JS_malloc(cx, nsrcnotes * sizeof(jssrcnote));
            if (!notes)
                goto error;
        }
    }

    if (!JS_XDRBytes(xdr, (char *)notes, nsrcnotes * sizeof(jssrcnote)) ||
        !JS_XDRCStringOrNull(xdr, (char **)&script->filename) ||
        !JS_XDRUint32(xdr, &lineno) ||
        !JS_XDRUint32(xdr, &depth) ||
        (magic < JSXDR_MAGIC_SCRIPT_4 && !JS_XDRUint32(xdr, &ntrynotes))) {
        goto error;
    }

    /* Script principals transcoding support comes with versions >= _3. */
    if (magic >= JSXDR_MAGIC_SCRIPT_3) {
        JSPrincipals *principals;
        uint32 encodeable;

        if (xdr->mode == JSXDR_ENCODE) {
            principals = script->principals;
            encodeable = (cx->runtime->principalsTranscoder != NULL);
            if (!JS_XDRUint32(xdr, &encodeable))
                goto error;
            if (encodeable &&
                !cx->runtime->principalsTranscoder(xdr, &principals)) {
                goto error;
            }
        } else {
            if (!JS_XDRUint32(xdr, &encodeable))
                goto error;
            if (encodeable) {
                if (!cx->runtime->principalsTranscoder) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_CANT_DECODE_PRINCIPALS);
                    goto error;
                }
                if (!cx->runtime->principalsTranscoder(xdr, &principals))
                    goto error;
                script->principals = principals;
            }
        }
    }

    if (xdr->mode == JSXDR_DECODE) {
        const char *filename = script->filename;
        if (filename) {
            filename = js_SaveScriptFilename(cx, filename);
            if (!filename)
                goto error;
            JS_free(cx, (void *) script->filename);
            script->filename = filename;
            filenameWasSaved = JS_TRUE;
        }
        script->lineno = (uintN)lineno;
        script->depth = (uintN)depth;

        if (magic < JSXDR_MAGIC_SCRIPT_4) {
            /*
             * Argh, we have to reallocate script, copy notes into the extra
             * space after the bytecodes, and free the temporary notes vector.
             * First, add enough slop to nsrcnotes so we can align the address
             * after the srcnotes of the first trynote.
             */
            uint32 osrcnotes = nsrcnotes;

            if (ntrynotes)
                nsrcnotes += JSTRYNOTE_ALIGNMASK;
            newscript = (JSScript *) JS_realloc(cx, script,
                                                sizeof(JSScript) +
                                                length * sizeof(jsbytecode) +
                                                nsrcnotes * sizeof(jssrcnote) +
                                                ntrynotes * sizeof(JSTryNote));
            if (!newscript)
                goto error;

            *scriptp = script = newscript;
            script->code = (jsbytecode *)(script + 1);
            script->main = script->code + prologLength;
            memcpy(script->code + length, notes, osrcnotes * sizeof(jssrcnote));
            JS_free(cx, (void *) notes);
            notes = NULL;
            if (ntrynotes) {
                script->trynotes = (JSTryNote *)
                                   ((jsword)(SCRIPT_NOTES(script) + nsrcnotes) &
                                    ~(jsword)JSTRYNOTE_ALIGNMASK);
            }
        }
    }

    while (ntrynotes) {
        JSTryNote *tn = &script->trynotes[--ntrynotes];
        uint32 start = (uint32) tn->start,
               catchLength = (uint32) tn->length,
               catchStart = (uint32) tn->catchStart;

        if (!JS_XDRUint32(xdr, &start) ||
            !JS_XDRUint32(xdr, &catchLength) ||
            !JS_XDRUint32(xdr, &catchStart)) {
            goto error;
        }
        tn->start = (ptrdiff_t) start;
        tn->length = (ptrdiff_t) catchLength;
        tn->catchStart = (ptrdiff_t) catchStart;
    }
    return JS_TRUE;

  error:
    if (xdr->mode == JSXDR_DECODE) {
        if (script->filename && !filenameWasSaved) {
            JS_free(cx, (void *) script->filename);
            script->filename = NULL;
        }
        if (notes && magic < JSXDR_MAGIC_SCRIPT_4)
            JS_free(cx, (void *) notes);
        js_DestroyScript(cx, script);
        *scriptp = NULL;
    }
    return JS_FALSE;
}

#if JS_HAS_XDR_FREEZE_THAW
/*
 * These cannot be exposed to web content, and chrome does not need them, so
 * we take them out of the Mozilla client altogether.  Fortunately, there is
 * no way to serialize a native function (see fun_xdrObject in jsfun.c).
 */

static JSBool
script_freeze(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
    JSXDRState *xdr;
    JSScript *script;
    JSBool ok, hasMagic;
    uint32 len;
    void *buf;
    JSString *str;

    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;
    script = (JSScript *) JS_GetPrivate(cx, obj);
    if (!script)
        return JS_TRUE;

    /* create new XDR */
    xdr = JS_XDRNewMem(cx, JSXDR_ENCODE);
    if (!xdr)
        return JS_FALSE;

    /* write  */
    ok = js_XDRScript(xdr, &script, &hasMagic);
    if (!ok)
        goto out;
    if (!hasMagic) {
        *rval = JSVAL_VOID;
        goto out;
    }

    buf = JS_XDRMemGetData(xdr, &len);
    if (!buf) {
        ok = JS_FALSE;
        goto out;
    }

    JS_ASSERT((jsword)buf % sizeof(jschar) == 0);
    len /= sizeof(jschar);
    str = JS_NewUCStringCopyN(cx, (jschar *)buf, len);
    if (!str) {
        ok = JS_FALSE;
        goto out;
    }

#if IS_BIG_ENDIAN
  {
    jschar *chars;
    uint32 i;

    /* Swap bytes in Unichars to keep frozen strings machine-independent. */
    chars = JS_GetStringChars(str);
    for (i = 0; i < len; i++)
        chars[i] = JSXDR_SWAB16(chars[i]);
  }
#endif
    *rval = STRING_TO_JSVAL(str);

out:
    JS_XDRDestroy(xdr);
    return ok;
}

static JSBool
script_thaw(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
            jsval *rval)
{
    JSXDRState *xdr;
    JSString *str;
    void *buf;
    uint32 len;
    JSScript *script, *oldscript;
    JSBool ok, hasMagic;

    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;

    if (argc == 0)
        return JS_TRUE;
    str = js_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;

    /* create new XDR */
    xdr = JS_XDRNewMem(cx, JSXDR_DECODE);
    if (!xdr)
        return JS_FALSE;

    buf = JS_GetStringChars(str);
    len = JS_GetStringLength(str);
#if IS_BIG_ENDIAN
  {
    jschar *from, *to;
    uint32 i;

    /* Swap bytes in Unichars to keep frozen strings machine-independent. */
    from = (jschar *)buf;
    to = (jschar *) JS_malloc(cx, len * sizeof(jschar));
    if (!to) {
        JS_XDRDestroy(xdr);
        return JS_FALSE;
    }
    for (i = 0; i < len; i++)
        to[i] = JSXDR_SWAB16(from[i]);
    buf = (char *)to;
  }
#endif
    len *= sizeof(jschar);
    JS_XDRMemSetData(xdr, buf, len);

    /* XXXbe should magic mismatch be error, or false return value? */
    ok = js_XDRScript(xdr, &script, &hasMagic);
    if (!ok)
        goto out;
    if (!hasMagic) {
        *rval = JSVAL_FALSE;
        goto out;
    }

    /* Swap script for obj's old script, if any. */
    oldscript = (JSScript *) JS_GetPrivate(cx, obj);
    ok = JS_SetPrivate(cx, obj, script);
    if (!ok) {
        JS_free(cx, script);
        goto out;
    }
    if (oldscript)
        js_DestroyScript(cx, oldscript);

    script->object = obj;
    js_CallNewScriptHook(cx, script, NULL);

out:
    /*
     * We reset the buffer to be NULL so that it doesn't free the chars
     * memory owned by str (argv[0]).
     */
    JS_XDRMemSetData(xdr, NULL, 0);
    JS_XDRDestroy(xdr);
#if IS_BIG_ENDIAN
    JS_free(cx, buf);
#endif
    *rval = JSVAL_TRUE;
    return ok;
}

static const char js_thaw_str[] = "thaw";

#endif /* JS_HAS_XDR_FREEZE_THAW */
#endif /* JS_HAS_XDR */

static JSFunctionSpec script_methods[] = {
#if JS_HAS_TOSOURCE
    {js_toSource_str,   script_toSource,        0,0,0},
#endif
    {js_toString_str,   script_toString,        0,0,0},
    {"compile",         script_compile,         2,0,0},
    {"exec",            script_exec,            1,0,0},
#if JS_HAS_XDR_FREEZE_THAW
    {"freeze",          script_freeze,          0,0,0},
    {js_thaw_str,       script_thaw,            1,0,0},
#endif /* JS_HAS_XDR_FREEZE_THAW */
    {0,0,0,0,0}
};

#endif /* JS_HAS_SCRIPT_OBJECT */

static void
script_finalize(JSContext *cx, JSObject *obj)
{
    JSScript *script;

    script = (JSScript *) JS_GetPrivate(cx, obj);
    if (script)
        js_DestroyScript(cx, script);
}

static JSBool
script_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
#if JS_HAS_SCRIPT_OBJECT
    return script_exec(cx, JSVAL_TO_OBJECT(argv[-2]), argc, argv, rval);
#else
    return JS_FALSE;
#endif
}

static uint32
script_mark(JSContext *cx, JSObject *obj, void *arg)
{
    JSScript *script;

    script = (JSScript *) JS_GetPrivate(cx, obj);
    if (script)
        js_MarkScript(cx, script, arg);
    return 0;
}

JS_FRIEND_DATA(JSClass) js_ScriptClass = {
    js_Script_str,
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   script_finalize,
    NULL,             NULL,             script_call,      NULL,/*XXXbe xdr*/
    NULL,             NULL,             script_mark,      0
};

#if JS_HAS_SCRIPT_OBJECT

static JSBool
Script(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    /* If not constructing, replace obj with a new Script object. */
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) {
        obj = js_NewObject(cx, &js_ScriptClass, NULL, NULL);
        if (!obj)
            return JS_FALSE;
    }
    return script_compile(cx, obj, argc, argv, rval);
}

#if JS_HAS_XDR_FREEZE_THAW

static JSBool
script_static_thaw(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval)
{
    obj = js_NewObject(cx, &js_ScriptClass, NULL, NULL);
    if (!obj)
        return JS_FALSE;
    if (!script_thaw(cx, obj, argc, argv, rval))
        return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

static JSFunctionSpec script_static_methods[] = {
    {js_thaw_str,       script_static_thaw,     1,0,0},
    {0,0,0,0,0}
};

#else  /* !JS_HAS_XDR_FREEZE_THAW */

#define script_static_methods   NULL

#endif /* !JS_HAS_XDR_FREEZE_THAW */

JSObject *
js_InitScriptClass(JSContext *cx, JSObject *obj)
{
    return JS_InitClass(cx, obj, NULL, &js_ScriptClass, Script, 1,
                        NULL, script_methods, NULL, script_static_methods);
}

#endif /* JS_HAS_SCRIPT_OBJECT */

/*
 * Shared script filename management.
 */
JS_STATIC_DLL_CALLBACK(int)
js_compare_strings(const void *k1, const void *k2)
{
    return strcmp(k1, k2) == 0;
}

/* Shared with jsatom.c to save code space. */
extern void * JS_DLL_CALLBACK
js_alloc_table_space(void *priv, size_t size);

extern void JS_DLL_CALLBACK
js_free_table_space(void *priv, void *item);

/* NB: This struct overlays JSHashEntry -- see jshash.h, do not reorganize. */
typedef struct ScriptFilenameEntry {
    JSHashEntry         *next;          /* hash chain linkage */
    JSHashNumber        keyHash;        /* key hash function result */
    const void          *key;           /* ptr to filename, below */
    JSPackedBool        mark;           /* mark flag, for GC */
    char                filename[3];    /* two or more bytes, NUL-terminated */
} ScriptFilenameEntry;

JS_STATIC_DLL_CALLBACK(JSHashEntry *)
js_alloc_entry(void *priv, const void *key)
{
    size_t nbytes = offsetof(ScriptFilenameEntry, filename) + strlen(key) + 1;

    return (JSHashEntry *) malloc(JS_MAX(nbytes, sizeof(JSHashEntry)));
}

JS_STATIC_DLL_CALLBACK(void)
js_free_entry(void *priv, JSHashEntry *he, uintN flag)
{
    if (flag != HT_FREE_ENTRY)
        return;
    free(he);
}

static JSHashAllocOps table_alloc_ops = {
    js_alloc_table_space,   js_free_table_space,
    js_alloc_entry,         js_free_entry
};

JSBool
js_InitRuntimeScriptState(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

#ifdef JS_THREADSAFE
    /* Must come through here once in primordial thread to init safely! */
    if (!rt->scriptFilenameTableLock) {
        rt->scriptFilenameTableLock = JS_NEW_LOCK();
        if (!rt->scriptFilenameTableLock)
            return JS_FALSE;
    }
#endif
    if (!rt->scriptFilenameTable) {
        JS_ACQUIRE_LOCK(rt->scriptFilenameTableLock);
        if (!rt->scriptFilenameTable) {
            rt->scriptFilenameTable =
                JS_NewHashTable(16, JS_HashString, js_compare_strings, NULL,
                                &table_alloc_ops, NULL);
        }
        JS_RELEASE_LOCK(rt->scriptFilenameTableLock);
        if (!rt->scriptFilenameTable) {
            js_FinishRuntimeScriptState(cx);    /* free lock if threadsafe */
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

void
js_FinishRuntimeScriptState(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (rt->scriptFilenameTable) {
        JS_HashTableDestroy(rt->scriptFilenameTable);
        rt->scriptFilenameTable = NULL;
    }
#ifdef JS_THREADSAFE
    if (rt->scriptFilenameTableLock) {
        JS_DESTROY_LOCK(rt->scriptFilenameTableLock);
        rt->scriptFilenameTableLock = NULL;
    }
#endif
}

#ifdef DEBUG_brendan
size_t sft_savings = 0;
#endif

const char *
js_SaveScriptFilename(JSContext *cx, const char *filename)
{
    JSRuntime *rt = cx->runtime;
    JSHashTable *table;
    JSHashNumber hash;
    JSHashEntry **hep;
    ScriptFilenameEntry *sfe;

    JS_ACQUIRE_LOCK(rt->scriptFilenameTableLock);
    table = rt->scriptFilenameTable;
    hash = JS_HashString(filename);
    hep = JS_HashTableRawLookup(table, hash, filename);
    sfe = (ScriptFilenameEntry *) *hep;
#ifdef DEBUG_brendan
    if (sfe)
        sft_savings += strlen(sfe->filename);
#endif
    if (!sfe) {
        sfe = (ScriptFilenameEntry *)
              JS_HashTableRawAdd(table, hep, hash, filename, NULL);
        if (sfe) {
            sfe->key = strcpy(sfe->filename, filename);
            JS_ASSERT(!sfe->mark);
        }
    }
    JS_RELEASE_LOCK(rt->scriptFilenameTableLock);
    if (!sfe) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }
    return sfe->filename;
}

void
js_MarkScriptFilename(const char *filename)
{
    ScriptFilenameEntry *sfe;

    /*
     * Back up from filename by its offset within its hash table entry.
     * The sfe->key member, redundant given sfe->filename but required by
     * the old jshash.c code, here gives us a useful sanity check.  This
     * assertion will very likely botch if someone tries to mark a string
     * that wasn't allocated as an sfe->filename.
     */
    sfe = (ScriptFilenameEntry *)
          (filename - offsetof(ScriptFilenameEntry, filename));
    JS_ASSERT(sfe->key == sfe->filename);
    sfe->mark = JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(intN)
js_script_filename_sweeper(JSHashEntry *he, intN i, void *arg)
{
    ScriptFilenameEntry *sfe = (ScriptFilenameEntry *) he;

    if (!sfe->mark)
        return HT_ENUMERATE_REMOVE;
    sfe->mark = JS_FALSE;
    return HT_ENUMERATE_NEXT;
}

void
js_SweepScriptFilenames(JSRuntime *rt)
{
    JS_HashTableEnumerateEntries(rt->scriptFilenameTable,
                                 js_script_filename_sweeper,
                                 rt);
#ifdef DEBUG_brendan
    printf("script filename table savings so far: %u\n", sft_savings);
#endif
}

JSScript *
js_NewScript(JSContext *cx, uint32 length, uint32 nsrcnotes, uint32 ntrynotes)
{
    JSScript *script;

    /* Round up source note count to align script->trynotes for its type. */
    if (ntrynotes)
        nsrcnotes += JSTRYNOTE_ALIGNMASK;
    script = (JSScript *) JS_malloc(cx,
                                    sizeof(JSScript) +
                                    length * sizeof(jsbytecode) +
                                    nsrcnotes * sizeof(jssrcnote) +
                                    ntrynotes * sizeof(JSTryNote));
    if (!script)
        return NULL;
    memset(script, 0, sizeof(JSScript));
    script->code = script->main = (jsbytecode *)(script + 1);
    script->length = length;
    script->version = cx->version;
    if (ntrynotes) {
        script->trynotes = (JSTryNote *)
                           ((jsword)(SCRIPT_NOTES(script) + nsrcnotes) &
                            ~(jsword)JSTRYNOTE_ALIGNMASK);
    }
    return script;
}

JS_FRIEND_API(JSScript *)
js_NewScriptFromCG(JSContext *cx, JSCodeGenerator *cg, JSFunction *fun)
{
    uint32 mainLength, prologLength, nsrcnotes, ntrynotes;
    JSScript *script;
    const char *filename;

    mainLength = CG_OFFSET(cg);
    prologLength = CG_PROLOG_OFFSET(cg);
    CG_COUNT_FINAL_SRCNOTES(cg, nsrcnotes);
    CG_COUNT_FINAL_TRYNOTES(cg, ntrynotes);
    script = js_NewScript(cx, prologLength + mainLength, nsrcnotes, ntrynotes);
    if (!script)
        return NULL;

    /* Now that we have script, error control flow must go to label bad. */
    script->main += prologLength;
    memcpy(script->code, CG_PROLOG_BASE(cg), prologLength * sizeof(jsbytecode));
    memcpy(script->main, CG_BASE(cg), mainLength * sizeof(jsbytecode));
    if (!js_InitAtomMap(cx, &script->atomMap, &cg->atomList))
        goto bad;

    filename = cg->filename;
    if (filename) {
        script->filename = js_SaveScriptFilename(cx, filename);
        if (!script->filename)
            goto bad;
    }
    script->lineno = cg->firstLine;
    script->depth = cg->maxStackDepth;
    if (cg->principals) {
        script->principals = cg->principals;
        JSPRINCIPALS_HOLD(cx, script->principals);
    }

    if (!js_FinishTakingSrcNotes(cx, cg, SCRIPT_NOTES(script)))
        goto bad;
    if (script->trynotes)
        js_FinishTakingTryNotes(cx, cg, script->trynotes);

    /* Tell the debugger about this compiled script. */
    js_CallNewScriptHook(cx, script, fun);
    return script;

bad:
    js_DestroyScript(cx, script);
    return NULL;
}

JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun)
{
    JSRuntime *rt;
    JSNewScriptHook hook;

    rt = cx->runtime;
    hook = rt->newScriptHook;
    if (hook) {
        /*
         * We use a dummy stack frame to protect the script from a GC caused
         * by debugger-hook execution.
         *
         * XXX We really need a way to manage local roots and such more
         * XXX automatically, at which point we can remove this one-off hack
         * XXX and others within the engine.  See bug 40757 for discussion.
         */
        JSStackFrame dummy;

        memset(&dummy, 0, sizeof dummy);
        dummy.down = cx->fp;
        dummy.script = script;
        cx->fp = &dummy;

        hook(cx, script->filename, script->lineno, script, fun,
             rt->newScriptHookData);

        cx->fp = dummy.down;
    }
}

void
js_DestroyScript(JSContext *cx, JSScript *script)
{
    JSRuntime *rt;
    JSDestroyScriptHook hook;

    rt = cx->runtime;
    hook = rt->destroyScriptHook;
    if (hook)
        hook(cx, script, rt->destroyScriptHookData);

    JS_ClearScriptTraps(cx, script);
    js_FreeAtomMap(cx, &script->atomMap);
    if (script->principals)
        JSPRINCIPALS_DROP(cx, script->principals);
    JS_free(cx, script);
}

void
js_MarkScript(JSContext *cx, JSScript *script, void *arg)
{
    JSAtomMap *map;
    uintN i, length;
    JSAtom **vector;

    map = &script->atomMap;
    length = map->length;
    vector = map->vector;
    for (i = 0; i < length; i++)
        GC_MARK_ATOM(cx, vector[i], arg);

    if (script->filename)
        js_MarkScriptFilename(script->filename);
}

jssrcnote *
js_GetSrcNote(JSScript *script, jsbytecode *pc)
{
    jssrcnote *sn;
    ptrdiff_t offset, target;

    target = PTRDIFF(pc, script->code, jsbytecode);
    if ((uint32)target >= script->length)
        return NULL;
    offset = 0;
    for (sn = SCRIPT_NOTES(script); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        offset += SN_DELTA(sn);
        if (offset == target && SN_IS_GETTABLE(sn))
            return sn;
    }
    return NULL;
}

uintN
js_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JSAtom *atom;
    JSFunction *fun;
    uintN lineno;
    ptrdiff_t offset, target;
    jssrcnote *sn;
    JSSrcNoteType type;

    /*
     * Special case: function definition needs no line number note because
     * the function's script contains its starting line number.
     */
    if (*pc == JSOP_DEFFUN) {
        atom = GET_ATOM(cx, script, pc);
        fun = (JSFunction *) JS_GetPrivate(cx, ATOM_TO_OBJECT(atom));
        return fun->script->lineno;
    }

    /*
     * General case: walk through source notes accumulating their deltas,
     * keeping track of line-number notes, until we pass the note for pc's
     * offset within script->code.
     */
    lineno = script->lineno;
    offset = 0;
    target = PTRDIFF(pc, script->code, jsbytecode);
    for (sn = SCRIPT_NOTES(script); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        offset += SN_DELTA(sn);
        type = (JSSrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            if (offset <= target)
                lineno = (uintN) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            if (offset <= target)
                lineno++;
        }
        if (offset > target)
            break;
    }
    return lineno;
}

jsbytecode *
js_LineNumberToPC(JSScript *script, uintN target)
{
    ptrdiff_t offset;
    uintN lineno;
    jssrcnote *sn;
    JSSrcNoteType type;

    offset = 0;
    lineno = script->lineno;
    for (sn = SCRIPT_NOTES(script); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        if (lineno >= target)
            break;
        offset += SN_DELTA(sn);
        type = (JSSrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            lineno = (uintN) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            lineno++;
        }
    }
    return script->code + offset;
}

uintN
js_GetScriptLineExtent(JSScript *script)
{
    uintN lineno;
    jssrcnote *sn;
    JSSrcNoteType type;

    lineno = script->lineno;
    for (sn = SCRIPT_NOTES(script); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        type = (JSSrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            lineno = (uintN) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            lineno++;
        }
    }
    return 1 + lineno - script->lineno;
}
