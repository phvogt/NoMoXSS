#ifndef xssdbg_h___
#define xssdbg_h___

/*
 * Contains macros to help debug the engine (with the xss-extension)
 */

#include "jsapi.h"
#include "jsgc.h"
#include "jsstddef.h"

/* prints the atom to stdout  */
#define XSS_PRINT_ATOM(atom) \
    if (ATOM_IS_STRING(atom)) \
        printf("  atom = \"%s\"\n", js_AtomToPrintableString(cx, atom)); \
    else if (ATOM_IS_INT(atom))												\
        printf("  atom = %ld\n", (long)ATOM_TO_INT(atom));						 \
    else																	  \
        printf("  atom = %.16g\n", *ATOM_TO_DOUBLE(atom));						   \
/*			printf("  atom = %s \n", js_AtomToPrintableString(cx,atom)); */

/* prints the jsval */
#define XSS_PRINT_VAL(x) printf("  fetch val = %s \n", js_AtomToPrintableString(cx,js_ValueToStringAtom(cx, x)))

/* prints the type of the given value */
#define XSS_PRINT_TYPE(val, strname)							\
	if (JSVAL_IS_OBJECT(val)) {									\
		printf(strname " is object\n");							\
	}															\
	if (JSVAL_IS_INT(val)) {									\
		printf(strname " is int\n");							\
	} else														\
	if (JSVAL_IS_DOUBLE(val)) {									\
		printf(strname " is double\n");							\
	} else														\
	if (JSVAL_IS_NUMBER(val)) {									\
		printf(strname " is number\n");							\
	}															\
	if (JSVAL_IS_STRING(val)) {									\
		printf(strname "  is string\n");						\
	}															\
	if (JSVAL_IS_BOOLEAN(val)) {								\
		printf(strname "  is boolean \n");						\
	}															\
	if (JSVAL_IS_NULL(val)) {									\
		printf(strname "  is null \n");							\
	}															\
	if (JSVAL_IS_VOID(val)) {									\
		printf(strname "  is void \n");							\
	} 


/* returns the type of the given value in str */
#define XSS_GET_TYPE_STR(val, str)								\
	if (JSVAL_IS_OBJECT(val)) {									\
		str = "object";											\
	}															\
	if (JSVAL_IS_INT(val)) {									\
		str = "int";											\
	} else														\
	if (JSVAL_IS_DOUBLE(val)) {									\
		str = "double";											\
	} else														\
	if (JSVAL_IS_NUMBER(val)) {									\
		str = "number";											\
	}															\
	if (JSVAL_IS_STRING(val)) {									\
		str = "string";											\
	}															\
	if (JSVAL_IS_BOOLEAN(val)) {								\
		str = "boolean";										\
	}															\
	if (JSVAL_IS_NULL(val)) {									\
		str = "null";											\
	}															\
	if (JSVAL_IS_VOID(val)) {									\
		str = "void";											\
	} 

/* just prints the content of a taint-structure */
#define XSS_PRINT_TAINTVALUE(taint)									\
	if (taint != 0) {												\
		fprintf(tracefp,"t: %d %d", taint->istainted, taint->type);	\
	} else {														\
		fprintf(tracefp,"t: null!");								\
	}

/* prints taintvalue of a JSGCThing */
#define XSS_PRINT_GC_TAINT(gcthing)								\
	if (JSVAL_IS_GCTHING(gcthing)) {							\
		XSS_PRINTDEBUG_STR("(");								\
		XSS_JSVAL_GET_TAINT(gcthing,xss_taint);					\
		XSS_PRINT_TAINTVALUE(xss_taint);						\
		XSS_PRINTDEBUG_STR(")");								\
	} else {													\
		XSS_PRINTDEBUG_STR("(" #gcthing " no gcthing!)\n");		\
	}

/* taints a string if it contains "evil" */
#define XSS_TAINT_ISEVILSTR(cmpstr, isevil)						\
		JS_BEGIN_MACRO											\
			jschar *mychars;									\
			JSString *mystr;									\
			isevil = 0;											\
																\
			mychars = (jschar *) JS_malloc(cx, (4 + 1) *		\
						sizeof(jschar));						\
			mychars[0] = 'e';									\
			mychars[1] = 'v';									\
			mychars[2] = 'i';									\
			mychars[3] = 'l';									\
			mychars[4] = 0;										\
			mystr = js_NewString(cx, mychars, 4, 0);			\
			if (mystr && cmpstr &&								\
				(js_CompareStrings(mystr,cmpstr) == 0)) {		\
																\
				isevil = 1;										\
			}													\
			mychars = (jschar *) JS_malloc(cx, (6 + 1) *		\
						sizeof(jschar));						\
			mychars[0] = '"';									\
			mychars[1] = 'e';									\
			mychars[2] = 'v';									\
			mychars[3] = 'i';									\
			mychars[4] = 'l';									\
			mychars[5] = '"';									\
			mychars[6] = 0;										\
			mystr = js_NewString(cx, mychars, 6, 0);			\
			if (mystr && cmpstr &&								\
				(js_CompareStrings(mystr,cmpstr) == 0)) {		\
																\
				isevil = 1;										\
			}													\
		JS_END_MACRO

/* taints the evilval if the cmpstr is evil or "evil" */
#define XSS_TAINT_EVILSTR(cmpstr, evilval)						\
		JS_BEGIN_MACRO											\
			int isevil = 0;										\
			XSS_TAINT_ISEVILSTR(cmpstr, isevil);				\
			if (isevil == 1) {									\
				XSS_JSVAL_SET_ISTAINTED(XSS_TAINTED,evilval);	\
			}													\
		JS_END_MACRO

/* prints a debugstring */
#ifdef XSS_DEBUG

#define XSS_PRINTDEBUG_STR(str)									\
	if (xssGetDoLog() == XSS_DO_LOG)							\
		fprintf(tracefp, str)

#define XSS_PRINTDEBUG_INT(str)									\
	if (xssGetDoLog() == XSS_DO_LOG)							\
		fprintf(tracefp, "%i", str)

#else

#define XSS_PRINTDEBUG_STR(str) 

#define XSS_PRINTDEBUG_INT(str)	

#endif /* XSS_DEBUG */

#define XSS_PRINT_INPUTS(filep)									\
		tracefp = (FILE *) filep;								\
        if (tracefp) {											\
            intN nuses, n;										\
																\
            fprintf(tracefp, "%4u: ", js_PCToLineNumber(cx, script, pc));	\
            js_Disassemble1(cx, script, pc,									\
                            PTRDIFF(pc, script->code, jsbytecode), JS_FALSE,\
                            tracefp);										\
            nuses = cs->nuses;												\
            if (nuses) {													\
                SAVE_SP(fp);												\
                for (n = -nuses; n < 0; n++) {								\
                    str = js_DecompileValueGenerator(cx, n, sp[n], NULL);	\
                    if (str != NULL) {										\
                        fprintf(tracefp, "%s %s(@ %d)",						\
                                (n == -nuses) ? "  inputs:" : ",",			\
                                JS_GetStringBytes(str), sp[n]);				\
						XSS_PRINT_GC_TAINT(sp[n]);							\
                    }											\
                }												\
                fprintf(tracefp, " @ %d\n", sp - fp->spbase);	\
            }													\
		}

#define XSS_PRINT_OUPUT(filep)									\
		tracefp = (FILE *) filep;								\
        if (tracefp) {											\
            intN ndefs, n;										\
            jsval *siter;										\
																\
            ndefs = cs->ndefs;									\
            if (ndefs) {										\
                SAVE_SP(fp);									\
                for (n = -ndefs; n < 0; n++) {					\
                    str = js_DecompileValueGenerator(cx, n, sp[n], NULL);	\
                    if (str) {												\
                        fprintf(tracefp, "%s %s(@ %d)",						\
                                (n == -ndefs) ? "  output:" : ",",			\
                                JS_GetStringBytes(str), sp[n]);				\
						XSS_PRINT_GC_TAINT(sp[n]);							\
                    }											\
                }												\
                fprintf(tracefp, " @ %d\n", sp - fp->spbase);	\
            }													\
            fprintf(tracefp, "  stack: ");						\
            for (siter = fp->spbase; siter < sp; siter++) {		\
                str = js_ValueToSource(cx, *siter);				\
                fprintf(tracefp, "%s ",							\
                        str ? JS_GetStringBytes(str) : "<null>");	\
            }													\
            fputc('\n', tracefp);								\
        }

/* prints the node */
#define XSS_PRINT_SCOPE(tracefp, script, node)					\
	if (node) {													\
		if (xssGetDoLog() == XSS_DO_LOG) {						\
			fprintf(tracefp, "scope: n: %u ", node);			\
			fprintf(tracefp, "t: %d ", node->istainted);		\
			fprintf(tracefp, "p: %d ", node->prev_istainted);	\
			fprintf(tracefp, "op: %s ", js_CodeSpec[node->opcode].name);			\
			if (script) {										\
				fprintf(tracefp, "f: %05u ", PTRDIFF(node->from, script->code, jsbytecode));	\
				fprintf(tracefp, "t: %05u ", PTRDIFF(node->to, script->code, jsbytecode));		\
			} else {											\
				fprintf(tracefp, "f: %u ", node->from);			\
				fprintf(tracefp, "t: %u ", node->to);			\
			}													\
			fprintf(tracefp, "n: %u ", node->next);				\
			fprintf(tracefp, "\n");								\
		}														\
	}

#endif

