#include <stdlib.h>
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsapi.h"
#include "jsscript.h"
#include "xsstaint.h"
#include "xssdbg.h"

/* Checks if the jsval has a xss-taintstructure
 * v: the value to check
 * returns an int that is 0 if it hasn't a xss-taintstructure
 * otherwise 1
 */
JS_PUBLIC_API(int) xssJSValIsXssType(jsval v) {

	int result = 0;
	XSS_taint* xsstaint;

	/* check if it is a JSGCThing */
	if (JSVAL_IS_GCTHING(v)) {

		/* get the taintstructure */
		XSS_JSVAL_GET_TAINT(v, xsstaint);
		
		if (xsstaint != 0) {
			result = 1;
		}
	}

	return result;
}

/**
 * Checks if the given jsval is tainted.
 * v: the jsval to test
 * returns true if the jsval has a taintstructure and is tainted
 */
JS_PUBLIC_API(int) xssJSValIsTainted(jsval v) {
	
	int result = XSS_NOT_TAINTED;
	XSS_taint* xsstaint;

	/* check if it is a JSGCThing */
	if (JSVAL_IS_GCTHING(v)) {

		/* get the taintstructure */
		XSS_JSVAL_GET_TAINT(v, xsstaint);
		
		/* check if there is a taintstructure and 
		   if the value is tainted */
		if (xsstaint != 0) {
			result = xsstaint->istainted;
		}
	}

	return result;
}

/**
 * Gets the Type of the jsval
 * v: the jsval to get the type
 * returns the type of the jsval
 */
int xssJSValGetType(jsval v) {

	int result = XSS_NOTYPE;
	XSS_taint* xss_taint;
	
	/* check type of tainted values */
	if (JSVAL_IS_GCTHING(v)) {
		XSS_JSVAL_GET_TAINT((jsval) v,xss_taint);
		if ((xss_taint != 0) &&
			(xss_taint->type != XSS_NOTYPE)) {
			result = xss_taint->type;
		}
	}

	/* no tainted value, so get the original type */
	if (result == XSS_NOTYPE) {

		if (JSVAL_IS_OBJECT(v)) {
			result = JSVAL_OBJECT;
		}
		if (JSVAL_IS_INT(v)) {
			result = JSVAL_INT;
		}
		if (JSVAL_IS_DOUBLE(v)) {
			result = JSVAL_DOUBLE;
		}
		if (JSVAL_IS_STRING(v)) {
			result = JSVAL_STRING;
		}
		if (JSVAL_IS_BOOLEAN(v)) {
			result = JSVAL_BOOLEAN;
		}
		if (JSVAL_IS_NULL(v)) {
			result = JSVAL_NULL;
		}
		if (JSVAL_IS_VOID(v)) {
			result = JSVAL_VOID;
		}
	}

	return result;
}

/**
 * Gets the last node of the scope-structure
 * node: the node to get the last node from
 * returns the last node or NULL if the parameter was NULL
 */
XSS_scope *xssGetLastScopeNode(XSS_scope *node) {
	
	XSS_scope *result = NULL;

	if (node != NULL) {
		if (node->next == NULL) {
			result = node;
		} else {
			result = xssGetLastScopeNode(node->next);
		}
	}

	return result;
}

/**
 * Frees the memory allocated by the node
 * node: the node to free
 */
void xssFreeNode(XSS_scope *node) {
	if (node != NULL) {
		free(node);
		node = NULL;
	}
}

/**
 * Frees the node and all subnodes
 * node: the node to start freeing the memory
 */
int xssFreeScope(XSS_scope *node) {
	int result = 0;
	if (node != NULL) {
		if (node->next != NULL) 
			result = xssFreeScope(node->next);
#ifdef XSS_DEBUG /* XSS_DEBUG */
		if (xssGetDoLog() == XSS_DO_LOG) {
			fprintf(stderr, "xssFreeScope(): freeing node: ");
			XSS_PRINT_SCOPE(stderr, ((JSScript*)0), node);
		}
#endif /* XSS_DEBUG */
		xssFreeNode(node);
		node = NULL;
		++result;
	}

	return result;
}

/**
 * Gets the node before the given node
 * node: the node to return the parent
 * returns the node before "node" or NULL if a parameter was NULL or
 * there is no node before
 */
XSS_scope *xssScopeNodeBefore(XSS_scope *node) {
	XSS_scope *result = NULL;

    /* check input parameter */
	if (node != NULL) {
		result = node->prev;
	}
	return result;
}

/**
 * Removes the node from the list
 * node: the node to remove
 * returns the node before the removed node
 */
XSS_scope *xssRemoveNodeFromScope(XSS_scope *node) {
	XSS_scope *before_node = NULL;

    /* check input parameter */
	if (node != NULL) {
		/* get the node before */
		before_node = xssScopeNodeBefore(node);
		if (before_node) {			
			/* remove node from scope and free node */
			before_node->next = node->next;
			if (before_node->next) 
				before_node->next->prev = before_node;
		}
		xssFreeNode(node);
		node = NULL;
	}

	return before_node;
}

/**
 * Checks if the pc is in the current scope
 * pc: the programcounter
 * current: the scope to check
 * returns XSS_TRUE if the pc is in the scope and the scope isn't NULL
 * otherwise XSS_FALSE
 */
int xssScopePcInCurrent(jsbytecode *pc, XSS_scope *current) {
	
	int result = XSS_FALSE;

	/* check scope for NULL */
	if (current != NULL) {
		if ((pc >= current->from) && (pc < current->to)) {
			result = XSS_TRUE;
		}
	}
	return result;
}

/**
 * Checks if the two scopes overlap
 * scope1, scope2: the two scopes to check
 * returns XSS_TRUE if the scopes overlap and aren't null. Otherwise XSS_FALSE
 */
int xssScopeOverlap(XSS_scope *scope1, XSS_scope *scope2) {
	int result = XSS_FALSE;
	if ((scope1 != NULL) && (scope2  != NULL)) {
		jsbytecode *f1, *f2, *t1, *t2;
		f1 = scope1->from;
		t1 = scope1->to;
		f2 = scope2->from;
		t2 = scope2->to;
		if ((f1<=t2) && (f2<=t1)) result = XSS_TRUE;
		if ((f2<=t1) && (f1<=t2)) result = XSS_TRUE;
	}
	return result;
}

/**
 * Merges the two scopes into one scope if they overlap
 * scope1, scope2: the two scopes
 * returns a scope if the two scopes overlap or NULL if the don't
 */
XSS_scope *xssScopeMerge(XSS_scope *scope1, XSS_scope *scope2) {
	
	/* create the new scope */
	XSS_scope *result = NULL;
	
	if(xssScopeOverlap(scope1, scope2)) {
		result = XSS_SCOPE_CREATE;
		XSS_SCOPE_INIT(result, XSS_NOT_TAINTED);

		if ((scope1 != NULL) && (scope2 != NULL)) {
			/* calculate min- and max-borders */
			jsbytecode *min, *max;
			min = scope1->from;
			max = scope1->to;
			if (scope2->from < min) min = scope2->from;
			if (scope2->to > max) max = scope2->from;

			/* set the borders of the new scope */
			result->from = min;
			result->to = max;
		}
	}
	return result;
}

/**
 * Checks if the scope is unlimited (i.e. boundries XSS_PC_EMPTY)
 * scope: the scope to check
 * returns XSS_TRUE if both boundries are NULL, otherwise XSS_FALSE
 */
int xssScopeUnlimited(XSS_scope *scope) {
	int result = XSS_FALSE;

	if (scope != NULL) {
		if ((scope->from == XSS_PC_EMPTY) && (scope->to == XSS_PC_EMPTY)) {
			result = XSS_TRUE;
		}
	}

	return result;
}

/**
 * Checks if the current stackelement is tainted
 * current_sp: the current stack pointer
 * taintedsp: the stack pointer to the last untainted element
 */
int xssScopeStackIsTainted(jsval *current_sp, jsval *taintedsp) {
	int result = XSS_NOT_TAINTED;

	if (taintedsp != NULL) {
		if (current_sp > taintedsp)
			result = XSS_TAINTED;
	}

	return result;
}

/**
 * searches backward for a scope with the given op
 * if no op is found, NULL is returned
 * current: the current scope
 * op: the op to search for
 * returns the found scope or NULL
 */
XSS_scope *xssScopeWithOp(XSS_scope *current, JSOp op) {
	XSS_scope *result = NULL;

	if (current != NULL) {
		if (current->opcode != op) {
			result = xssScopeWithOp(current->prev, op);
		} else {
			result = current;
		}
	}

	return result;
}

static int XSS_DO_LOG_FLAG = 0;

/* set logging 1 => log */
void xssSetDoLog(int v) {
	XSS_DO_LOG_FLAG = v;
}

/* get logging status 1 => log */
int xssGetDoLog() {
	return XSS_DO_LOG_FLAG;
}
