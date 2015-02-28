/* EXIT_FAILURE and EXIT_SUCCESS */
#include <stdlib.h>  
/* strlen */
#include <string.h>

/* get SpiderMonkey API declarations */
#include "jsapi.h"

int
main(int argc, char** argv)
{
  /* pointer to our runtime object */
  JSRuntime *runtime=NULL;
  /* pointer to our context */
  JSContext *context=NULL;
  /* pointer to our global JavaScript object */
  JSObject  *global=NULL;

  /* script to run (should return 100) */
  char *script="var x=10;x*x;";
  /* JavaScript value to store the result of the script */
  jsval rval;

  JSString *str;
  JSBool ok;

  /* create new runtime, new context, global object */
  if (    (!(runtime = JS_NewRuntime (1024L*1024L)))
       || (!(context = JS_NewContext (runtime, 8192)))
       || (!(global  = JS_NewObject  (context, NULL, NULL, NULL)))
     ) return EXIT_FAILURE;
  /* set global object of context and initialize standard ECMAScript
     objects (Math, Date, ...) within this global object scope */
  if (!JS_InitStandardClasses(context, global)) return EXIT_FAILURE;

  /* now we are ready to run our script */
  ok = JS_EvaluateScript(context, global,script, strlen(script),
			 "script", 1, &rval);

  str = JS_ValueToString(context, rval);
    printf("script result: %s\n", JS_GetStringBytes(str));

  /* clean up */
  JS_DestroyContext(context);
  JS_DestroyRuntime(runtime);
  JS_ShutDown();
  return EXIT_SUCCESS;
}