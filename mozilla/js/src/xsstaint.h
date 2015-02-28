#ifndef taint_h___
#define taint_h___

#include "jstypes.h"
#include "jsprvtd.h"
#include "jsopcode.h"
#include <time.h>

JS_BEGIN_EXTERN_C

/* the taint-structure that contains the information about the taint-status */
typedef struct {
	/* main value that says, if the value is tainted or not */
	int istainted;
	/* original type of the value */
	int type;
} XSS_taint;

/* structure if a scope is tainted.
   this is implemented as a double-linked-list */
typedef struct XSS_scope {

	/* flag if this scope is tainted. */
	int istainted;
	/* flag if one of the previous scopes is tainted */
	int prev_istainted;

	/* the opcode that created the scope */
	JSOp opcode;

	/* pc-marks to set boundries of the scope */
	jsbytecode *from;
	jsbytecode *to;

	struct XSS_scope *next;
	struct XSS_scope *prev;

} XSS_scope;

/*
 * Constants
 */

/* xss-structure has no type */
#define XSS_NOTYPE -1

/* mainconstants if value is tainted or not */
#define XSS_NOT_TAINTED 0
#define XSS_TAINTED 1

/* dont't taint the result in any way. */
#define XSS_DONT_TAINT -1

/* defines an empty pc-mark */
#define XSS_PC_EMPTY NULL

/* define true/false */
#define XSS_TRUE 1
#define XSS_FALSE 0

/* environment string for user interaction on permission
   decisions
 */
#define XSS_ENV_USERINTERACTION_STR "XSS_USERINTERACTION"
#define XSS_ENV_USERINTERACTION_TRUE 1
#define XSS_ENV_USERINTERACTION_FALSE 0

/* environment string for user interaction on permission
   decisions
 */
#define XSS_ENV_DONTCHECKTAINT "XSS_DONTCHECKTAINT"
#define XSS_ENV_DONTCHECKTAINT_TRUE 1
#define XSS_ENV_DONTCHECKTAINT_FALSE 0

/* Checks if the jsval has a xss-taintstructure */
extern JS_PUBLIC_API(int) xssJSValIsXssType(jsval);

/* Checks if the given jsval is tainted. */
extern JS_PUBLIC_API(int) xssJSValIsTainted(jsval);

/* Gets the type of the jsval */
extern JS_PUBLIC_API(int) xssJSValGetType(jsval);

/* Gets the last node of the scope-structure */
extern XSS_scope *xssGetLastScopeNode(XSS_scope *node);

/* Frees the memory allocated by the node */
extern void xssFreeNode(XSS_scope *node);

/* Frees the node and all subnodes */
extern int xssFreeScope(XSS_scope *node);

/* Gets the node before the given node */
extern XSS_scope *xssScopeNodeBefore(XSS_scope *node);

/* Removes the node from the list */
extern XSS_scope *xssRemoveNodeFromScope(XSS_scope *node);

/* Checks if the pc is in the current scope */
extern int xssScopePcInCurrent(jsbytecode *pc, XSS_scope *current);

/* Checks if the two scopes overlap */
extern int xssScopeOverlap(XSS_scope *scope1, XSS_scope *scope2);

/* Merges the two scopes into one scope if they overlap */
extern XSS_scope *xssScopeMerge(XSS_scope *scope1, XSS_scope *scope2);

/* Checks if the scope is unlimited (i.e. boundries XSS_PC_EMPTY) */
extern int xssScopeUnlimited(XSS_scope *scope);

/* Checks if the current stack element is tainted */
extern int xssScopeStackIsTainted(jsval *current_sp, jsval *taintedsp);

/* gets the scope with a given op (searches backward) */
extern XSS_scope *xssScopeWithOp(XSS_scope *current, JSOp op);

/* constant defines that logging is enabled */
#define XSS_DO_LOG 1

/* set logging 1 => log */
extern void xssSetDoLog(int v);

/* get logging status 1 => log */
extern int xssGetDoLog();

#include "xssdbg.h"

/*
 * Macros to access/modify the taintstructure
 */

/* Set the origtype of the tainted type to the given type.
 * if the taint-value is 0 nothing is done
 */
#define XSS_TAINTSTRUCTURE_SET_ORIGTYPE(xss_taint, origtype)	\
	if (xss_taint) {											\
		xss_taint->type = origtype;								\
	}

/* Sets istainted of the taintstructure to the given value
 * if the taintstructure-value is 0 nothing is done
 */
#define XSS_TAINTSTRUCTURE_SET_ISTAINTED(xss_taint, b_istainted)	\
	if (xss_taint) {											\
		xss_taint->istainted = b_istainted;						\
	}

/* copies the xss-taintstructure from "from" to "to" */
#define XSS_TAINTSTRUCTURE_COPY_PTR(from, to)					\
	if ((from != 0) && (to != 0)) {								\
		to->istainted = from->istainted;						\
		to->type = from->type;									\
	}

/* copies the tainted-information in the xss-taintstructure 
 * from "from" to "to" 
 */
#define XSS_TAINTSTRUCTURE_COPY_TAINTEDINFO_PTR(from, to)		\
	if ((from != 0) && (to != 0)) {								\
		to->istainted = from->istainted;						\
	}

/* Check if the taintvalue is a number */
#define XSS_TAINTSTRUCTURE_IS_NUMBER(xss_taint)						\
	(xss_taint && ((xss_taint->type == JSVAL_INT) || (xss_taint->type == JSVAL_DOUBLE)))

/* Check if the taintvalue is a int
 */
#define XSS_TAINTSTRUCTURE_IS_INT(xss_taint)							\
	(xss_taint && (xss_taint->type == JSVAL_INT))

/* Check if the taintvalue is a int
 */
#define XSS_TAINTSTRUCTURE_IS_DOUBLE(xss_taint)						\
	(xss_taint && (xss_taint->type == JSVAL_DOUBLE))


/* 
 * Macros to access/manipulate jsvals
 */

/* checks if the value has a taintstructure */
#define XSS_JSVAL_HAS_TAINTSTRUCTURE(val)						\
	xssJSValIsXssType(val)

#ifdef XSS /* XSS */

/* get the taint-structure of a JSGCThing-object */
#define XSS_JSVAL_GET_TAINT(val, rval)							\
		rval = js_XSSGettaint(JSVAL_TO_GCTHING(val));

#else

#define XSS_JSVAL_GET_TAINT(val, rval)

#endif /* XSS */

/* Sets the xss_origtype to the type of v, if v is a tainted value */
#define XSS_JSVAL_GET_ORIGTYPE(v, xss_origtype)					\
	JS_BEGIN_MACRO												\
	XSS_taint* xss_taint;										\
	xss_origtype = XSS_NOTYPE;								    \
	if (JSVAL_IS_GCTHING(v)) {									\
		XSS_JSVAL_GET_TAINT((jsval) v,xss_taint);				\
		if ((xss_taint != 0) &&									\
			(xss_taint->type != XSS_NOTYPE)) {					\
			xss_origtype = xss_taint->type;						\
		}														\
	}															\
	JS_END_MACRO

/* gets the type of the jsval */
#define XSS_JSVAL_GET_TYPE(v)									\
	xssJSValGetType(v)

/* sets the original type in the taintstructure of a jsval */
#define XSS_JSVAL_SET_ORIGTYPE(jsvalue, type)					\
	if (XSS_JSVAL_HAS_TAINTSTRUCTURE(jsvalue)) {				\
																\
		JS_BEGIN_MACRO											\
			XSS_taint* taint;									\
																\
			XSS_JSVAL_GET_TAINT((jsval) jsvalue, taint);		\
			XSS_TAINTSTRUCTURE_SET_ORIGTYPE(taint, type);		\
		JS_END_MACRO;											\
	} else {													\
		XSS_PRINTDEBUG_STR("XSS_JSVAL_SET_ORIGTYPE: jsvalue ( " # jsvalue " ) has no taintstructure\n");		\
	}

/* Converts the xssval back to its original value. If it isn't a 
 * xss-value then it is returned unchanged.
 */
#define XSS_TO_ORIG_JSVAL(xssval, result)						\
		JS_BEGIN_MACRO											\
			int xss_origtype = XSS_NOTYPE;						\
			int mydouble;										\
			jsdouble *tempdp;									\
			result = xssval;									\
			if (JSVAL_IS_DOUBLE(xssval)) {						\
																\
					XSS_JSVAL_GET_ORIGTYPE(xssval, xss_origtype);	\
																\
					if (xss_origtype == JSVAL_BOOLEAN) {		\
						tempdp = JSVAL_TO_DOUBLE(xssval);		\
						mydouble = (int) *tempdp;				\
						if (mydouble == 0) {					\
							result = BOOLEAN_TO_JSVAL(JS_FALSE);\
						} else {								\
							result = BOOLEAN_TO_JSVAL(JS_TRUE);	\
						}										\
																\
					} else if (xss_origtype == JSVAL_INT) {		\
						tempdp = JSVAL_TO_DOUBLE(xssval);		\
						mydouble = (int) *tempdp;				\
						result = INT_TO_JSVAL(mydouble);		\
					} else if (xss_origtype == JSVAL_VOID) {	\
						result = JSVAL_VOID;					\
					} else if (xss_origtype == JSVAL_NULL){		\
						tempdp = JSVAL_TO_DOUBLE(xssval);		\
						mydouble = (int) *tempdp;				\
						result = JSVAL_OBJECT;					\
						if (mydouble == JSVAL_NULL) {			\
							result = JSVAL_NULL;				\
						}										\
					} else {									\
						result = xssval;						\
					}											\
																\
			} else {											\
				if (JSVAL_IS_OBJECT(xssval)) {					\
					result = xssval;							\
					XSS_JSVAL_GET_ORIGTYPE(xssval, xss_origtype);	\
					if ((xss_origtype == JSVAL_NULL)			\
								&& (xssval == JSVAL_NULL)) {	\
						result = JSVAL_NULL;					\
					}											\
				}												\
			}													\
		JS_END_MACRO											

/* copies the taintstructure from the jsvals "from" to "to" */
#define XSS_JSVAL_COPY_TAINTSTRUCTURE(from, to)					\
	if (XSS_JSVAL_HAS_TAINTSTRUCTURE(from) &&					\
			XSS_JSVAL_HAS_TAINTSTRUCTURE(to)) {					\
																\
		JS_BEGIN_MACRO											\
			XSS_taint* from_taint;								\
			XSS_taint* to_taint;								\
																\
			XSS_JSVAL_GET_TAINT((jsval) from, from_taint);		\
			XSS_JSVAL_GET_TAINT((jsval) to, to_taint);			\
			XSS_TAINTSTRUCTURE_COPY_PTR(from_taint, to_taint);	\
		JS_END_MACRO;											\
																\
	} else {													\
		if (!XSS_JSVAL_HAS_TAINTSTRUCTURE(from)) {				\
			XSS_PRINTDEBUG_STR("XSS_JSVAL_COPY_TAINTSTRUCTURE: from ( " #from " ) has no taintstructure\n");	\
		}														\
		if (!XSS_JSVAL_HAS_TAINTSTRUCTURE(to)) {				\
			XSS_PRINTDEBUG_STR("XSS_JSVAL_COPY_TAINTSTRUCTURE: to ( " #to " )  has no taintstructure\n");		\
		}														\
	}

/* copies the istainted-information from the jsvals "from" to "to" */
#define XSS_JSVAL_COPY_TAINTEDINFO(from, to)					\
	if (XSS_JSVAL_HAS_TAINTSTRUCTURE(from) &&					\
			XSS_JSVAL_HAS_TAINTSTRUCTURE(to)) {					\
																\
		JS_BEGIN_MACRO											\
			XSS_taint* from_taint;								\
			XSS_taint* to_taint;								\
																\
			XSS_JSVAL_GET_TAINT((jsval) from, from_taint);		\
			XSS_JSVAL_GET_TAINT((jsval) to, to_taint);			\
			XSS_TAINTSTRUCTURE_COPY_TAINTEDINFO_PTR(from_taint, to_taint);	\
		JS_END_MACRO;											\
																\
	} else {													\
		if (!XSS_JSVAL_HAS_TAINTSTRUCTURE(from)) {				\
			XSS_PRINTDEBUG_STR("XSS_JSVAL_COPY_TAINTEDINFO: from ( " #from " ) has no taintstructure\n");	\
		}														\
		if (!XSS_JSVAL_HAS_TAINTSTRUCTURE(to)) {				\
			XSS_PRINTDEBUG_STR("XSS_JSVAL_COPY_TAINTEDINFO: to ( " #to " )  has no taintstructure\n");		\
	}

/* checks if the jsval is tainted */
#define XSS_JSVAL_IS_TAINTED(value)								\
	xssJSValIsTainted(value)

/* sets istainted according (if the jsvalue has a taintstructure and
   istainted is not set to "don't taint" */
#define XSS_JSVAL_SET_ISTAINTED(istainted, jsvalue)					\
	if (istainted != XSS_DONT_TAINT) {								\
																	\
		if (!XSS_JSVAL_HAS_TAINTSTRUCTURE(jsvalue)) {				\
			XSS_PRINTDEBUG_STR("XSS_JSVAL_SET_ISTAINTED: jsvalue ( " #jsvalue " ) has no taintstructure\n");		\
		} else {													\
			JS_BEGIN_MACRO											\
				XSS_taint* taint;									\
																	\
				XSS_JSVAL_GET_TAINT(jsvalue, taint);				\
				XSS_TAINTSTRUCTURE_SET_ISTAINTED(taint, istainted);	\
				if ((taint != NULL) && (taint->type == XSS_NOTYPE)) {	\
					if (JSVAL_IS_OBJECT(jsvalue)) {					\
						XSS_JSVAL_SET_ORIGTYPE(jsvalue, JSVAL_OBJECT);	\
					} else if (JSVAL_IS_STRING(jsvalue)) {			\
						XSS_JSVAL_SET_ORIGTYPE(jsvalue, JSVAL_STRING);	\
					}												\
				}													\
			JS_END_MACRO;											\
		}															\
	}


/* set taint-value on jsvalue on istainted (and also if there is no taintstructure) */
#define XSS_JSVAL_TAINT_ON_FLAG(istainted, jsvalue)					\
	if ((istainted == XSS_TAINTED) && !XSS_JSVAL_HAS_TAINTSTRUCTURE(jsvalue)) {	\
		XSS_ADD_TAINTSTRUCTURE(rval);								\
	}																\
	if (istainted == XSS_TAINTED) {									\
		XSS_JSVAL_SET_ISTAINTED(XSS_TAINTED, jsvalue);				\
	}

/*
 * Other macros
 */

/* resets the boolean if the output should be tainted */
#define XSS_RESET_TAINTOUTPUT(value)							\
	value = XSS_NOT_TAINTED;

/* converts a boolean to a double-jsval */
#define XSS_BOOLEAN_TO_DOUBLE_JSVAL(val,boolval)				\
		JS_BEGIN_MACRO											\
			jsdouble* dp;										\
			XSS_taint* xss_taint;								\
			dp = JS_NewDouble(cx, val);							\
			XSS_JSVAL_GET_TAINT(DOUBLE_TO_JSVAL(dp),xss_taint);	\
			xss_taint->type = JSVAL_BOOLEAN;					\
			boolval = DOUBLE_TO_JSVAL(dp);						\
		JS_END_MACRO

/* converts a double to a double-jsval */
#define XSS_INT_TO_DOUBLE_JSVAL(inint,outjsval)				    \
		JS_BEGIN_MACRO											\
			jsdouble* dp;										\
			XSS_taint* xss_taint;								\
			dp = JS_NewDouble(cx, inint);					    \
			XSS_JSVAL_GET_TAINT(DOUBLE_TO_JSVAL(dp),xss_taint);	\
			xss_taint->type = JSVAL_INT;						\
			outjsval = DOUBLE_TO_JSVAL(dp);						\
		JS_END_MACRO

/* converts a void to a double-jsval */
#define XSS_VOID_TO_DOUBLE_JSVAL(inint,outjsval)				\
		JS_BEGIN_MACRO											\
			jsdouble* dp;										\
			XSS_taint* xss_taint;								\
			dp = JS_NewDouble(cx, inint);					    \
			XSS_JSVAL_GET_TAINT(DOUBLE_TO_JSVAL(dp),xss_taint);	\
			xss_taint->type = JSVAL_VOID;						\
			outjsval = DOUBLE_TO_JSVAL(dp);						\
		JS_END_MACRO

/* converts a null to a double-jsval */
#define XSS_NULL_TO_DOUBLE_JSVAL(inint,outjsval)				\
		JS_BEGIN_MACRO											\
			jsdouble* dp;										\
			XSS_taint* xss_taint;								\
			dp = JS_NewDouble(cx, inint);					    \
			XSS_JSVAL_GET_TAINT(DOUBLE_TO_JSVAL(dp),xss_taint);	\
			xss_taint->type = JSVAL_NULL;						\
			outjsval = DOUBLE_TO_JSVAL(dp);						\
		JS_END_MACRO

/* adds a taintstructure to the jsval */
#define XSS_ADD_TAINTSTRUCTURE(injsval)							\
		JS_BEGIN_MACRO											\
			jsdouble tempval;									\
			if (!XSS_JSVAL_HAS_TAINTSTRUCTURE(injsval)) {		\
				if (JSVAL_IS_BOOLEAN(injsval)) {				\
					XSS_BOOLEAN_TO_DOUBLE_JSVAL(JSVAL_TO_BOOLEAN(injsval), tempval);	\
					injsval = DOUBLE_TO_JSVAL(tempval);			\
					XSS_JSVAL_SET_ORIGTYPE(injsval, JSVAL_BOOLEAN);	\
				} else if (JSVAL_IS_INT(injsval)) {				\
					XSS_INT_TO_DOUBLE_JSVAL(JSVAL_TO_INT(injsval), tempval);	\
					injsval = DOUBLE_TO_JSVAL(tempval);			\
					XSS_JSVAL_SET_ORIGTYPE(injsval, JSVAL_INT);	\
				} else if (JSVAL_IS_VOID(injsval)) {			\
					XSS_VOID_TO_DOUBLE_JSVAL(JSVAL_TO_INT(injsval), tempval);	\
					injsval = DOUBLE_TO_JSVAL(tempval);			\
					XSS_JSVAL_SET_ORIGTYPE(injsval, JSVAL_VOID);	\
				} else if (JSVAL_IS_NULL(injsval)) {			\
					XSS_NULL_TO_DOUBLE_JSVAL(JSVAL_TO_INT(injsval), tempval);	\
					injsval = DOUBLE_TO_JSVAL(tempval);			\
					XSS_JSVAL_SET_ORIGTYPE(injsval, JSVAL_NULL);	\
				}												\
			} else {											\
				if (JSVAL_IS_OBJECT(injsval)) {					\
					XSS_JSVAL_SET_ORIGTYPE(injsval, JSVAL_OBJECT);	\
				} else if (JSVAL_IS_STRING(injsval)) {			\
					XSS_JSVAL_SET_ORIGTYPE(injsval, JSVAL_STRING);	\
				}												\
			}													\
		JS_END_MACRO

/* initializes a scope-node */
#define XSS_SCOPE_INIT(scope, taintit)							\
	scope->istainted = taintit;									\
	scope->prev_istainted = XSS_NOT_TAINTED;					\
	scope->opcode = JSOP_NOP;									\
	scope->from = XSS_PC_EMPTY;									\
	scope->to = XSS_PC_EMPTY;									\
	scope->next = NULL;											\
	scope->prev = NULL

/* checks if the scope is tainted */
#define XSS_SCOPE_ISTAINTED(scope)								\
	(scope != NULL) && ((scope->istainted) || (scope->prev_istainted))	

/* creates a new scope-node */
#define XSS_SCOPE_CREATE										\
	(XSS_scope *)  malloc (sizeof (XSS_scope))

/* frees the node and all subnodes */
#define XSS_SCOPE_FREE(node, current_node)						\
	xssFreeScope(node);											\
	node = NULL;												\
	current_node = NULL

/* frees the node */
#define XSS_SCOPE_FREE_NODE(node)								\
	xssFreeNode(node)

/* sets the values of the scope-structure */
#define XSS_SCOPE_SET(prev_node, node, i_istainted, i_from, i_to, i_op)	\
	if (node != NULL) {											\
		node->istainted = i_istainted;							\
		node->opcode = i_op;									\
		/* set bounds, from must be smaller than to */			\
		if (i_from <= i_to) {									\
			node->from = i_from;								\
			node->to = i_to;									\
		} else {												\
			node->from = i_to;									\
			node->to = i_from;									\
		}														\
		/* set links */											\
		node->prev = prev_node;									\
		if (node->prev != NULL) {								\
			node->prev->next = node;							\
		}														\
		/* set prev_tainted */									\
		if (node->prev != NULL) {								\
			node->prev_istainted = node->prev->prev_istainted	\
									|| node->prev->istainted;	\
		} else {												\
			node->prev_istainted = XSS_NOT_TAINTED;				\
		}														\
	}

/* advances the node to the next node, if it isn't NULL */
#define XSS_SCOPE_ADVANCE_NEXT(current)							\
	if (current != NULL) {										\
		if (current->next) current = current->next;				\
	}

/* removes the current node */
#define XSS_SCOPE_REMOVE(current)								\
	xssRemoveNodeFromScope(current)

/* checks if the pc is in the current scope */
#define XSS_SCOPE_PC_IN_CURRENT(pc, current)					\
	xssScopePcInCurrent(pc, current)

/* checks if the two scopes overlap */
#define XSS_SCOPE_OVERLAP(scope1, scope2)						\
	xssScopeOverlap(scope1, scope2)

/* merges the two scopes, if they overloap  */
#define XSS_SCOPE_MERGE(scope1, scope2)							\
	xssScopeMerge(scope1, scope2)

/* checks if the scope has unlimited boundries */
#define XSS_SCOPE_UNLIMITED(scope)								\
	xssScopeUnlimited(scope)

/* get the node before this node */
#define XSS_SCOPE_GETNODEBEFORE(root, node)						\
	xssScopeNodeBefore(root, node)

/* checks if the current stackelement is tainted */
#define XSS_SCOPE_STACK_IS_TAINTED(current_sp, taintedsp)		\
	xssScopeStackIsTainted(current_sp, taintedsp)	

/* sets the current stack element as tainted */
#define XSS_STACK_SET_TAINTED(current_sp, taintedsp)			\
	if (taintedsp != NULL) {									\
		if ((current_sp == NULL) || (current_sp > taintedsp)) {	\
			current_sp = taintedsp;								\
		} 														\
	}

/* set the pointer of the tainted stackelement if the output
   should be tainted */
#define XSS_SET_STACK_TAINTED_ON_OUTPUT(current_sp, taintedsp, taintoutput) \
	if (taintoutput == XSS_TAINTED) {							\
		XSS_STACK_SET_TAINTED(current_sp, taintedsp);			\
	}

/* resets the pointer to the last untainted stackelement if the
   current stack pointer is below or equal the untainted stackelement 
   pointer */
#define XSS_SCOPE_STACK_REMOVE(current_sp, taintedsp)			\
	if ((taintedsp != NULL) && (current_sp <= taintedsp)) {		\
		taintedsp = NULL;										\
	}

/* get the scope with the given op (searches backward) */
#define XSS_SCOPE_WITH_OP_BACK(current, op)						\
	xssScopeWithOp(current, op)

#ifndef XSS_DEBUG /* XSS_DEBUG */

/* init the root-scope */
#define XSS_SCOPE_INIT_ROOT(framefp, taintit, taintretval)			\
	framefp.scope_root = XSS_SCOPE_CREATE;							\
	XSS_SCOPE_INIT(framefp.scope_root, taintit);					\
	framefp.scope_current = framefp.scope_root;						\
	framefp.scope_sp = NULL;										\
	framefp.taint_retval = taintretval
#else 

/* init the root-scope */
#define XSS_SCOPE_INIT_ROOT(framefp, taintit, taintretval)			\
	framefp.scope_root = XSS_SCOPE_CREATE;							\
	XSS_SCOPE_INIT(framefp.scope_root, taintit);					\
	framefp.scope_current = framefp.scope_root;						\
	if (xssGetDoLog() == XSS_DO_LOG)								\
		fprintf(stderr, "init root: %d\n", taintit);				\
	framefp.scope_count = 1;										\
	framefp.scope_sp = NULL;										\
	framefp.taint_retval = taintretval


#endif /* XSS_DEBUG */

#define XSS_LOG(output1,output2)									\
	if (PR_XSS_DEBUG_LOG() == 1) {									\
	do {															\
		FILE* xss_logfile;											\
		time_t xss_timer;											\
		char xss_buf[128];											\
		struct tm xss_now;											\
		xss_logfile = fopen ("xss_connections.log", "a");			\
		xss_timer = time(NULL);										\
		xss_now = *localtime(&xss_timer);							\
		strftime(xss_buf,128,"%Y%m%d %H%M%S ", &xss_now);			\
		fprintf (xss_logfile, "%s \"%s\": " output1, xss_buf,		\
			PR_XSS_GET_DEBUG_FILENAME(), output2);					\
		fclose (xss_logfile);										\
	} while (0);}

JS_END_EXTERN_C

#endif
