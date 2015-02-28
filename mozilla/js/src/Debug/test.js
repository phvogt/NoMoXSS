/* JSOP_SETVAR 
var evil = 42;
function test() {
	var z = 17;
	var x = "juhu" + evil;
	print(x);
	print(z);
}
test();
*/
/* Array-/Object-tests */
/*
var evil = 42;
var notevil = 3;
var x = [1];
if (evil) {
	x[1] = 3;
}
print("length = " + x.length);
print("x[1] = " + x[1]);
*/
/* enterwith, setprop 
var evil = 42;
var notevil = 3;
var x = {};
if (evil) {
	x.y = 3;
}
var mylength = 0;
for (var i in x) {
//	print("x." + i + "= " + x[i]);
	++mylength;
}
with(x) {
	print("y = " + y);
	z = 3;
}
print("mylength = " + mylength);
print("x.z = " + x.z);
*/
/* initelem, initprop, endinit
var evil = 42;
var notevil = 3;
var x;
if (evil) {
//	x = [ 1 ];
	x = { a : 1 };
}
print("x= " + x);
print("typeof(x)= " + typeof(x));
if (typeof(x) == "object") {
//	print("x[1]= " + x[1]);
//	print("x[b]= " + x['b']);
	print("x[b]= " + x.b);
}
*/
/* delname
var evil = 42;
var notevil = 3;
x = 10;
if (evil) {
	notevil = delete x;
}
print("notevil = " + notevil);
print("x = " + x);
*/
/* delprop
var evil = 42;
var x = { y : 2, z : 3};
var notevil = 0;
if (evil) {
	notevil = delete x.z;
}
print("notevil = " + notevil);
var mylength = 0;
for (var prop in x) {
	print("x."+prop+" = " + x[prop]);
	++mylength;
}
//print("x.y = " + x.y);
//print("x.z = " + x.z);
print("x.length = " + x.length);
print("mylength = " + mylength);
*/
/* inc-/dec von name/prop/elem
var evil = 42;
var x = [ 1];
if (evil) {
	x[1]++;
}
print(x[1]);
*/
/*
var evil = 42;
var x = [ 1, 2];
var notevil = 0;
if (evil) {
	notevil = delete x[1];
}
print("notevil = " + notevil);
for (i=0;i<x.length;i++) {
	print("x["+i+"] = " + x[i]);
}
print("x.length = " + x.length);
*/
/* JSOP_VOID 
var evil = 42;
var x;
if (evil) {
	x = new Array(void 0);
}
print(x[0]);
*/
/* JSOP_UINT16 
var evil = 42;
var x;
if (evil) {
	x = 3;
}
print(x);
*/
/* JSOP_TYPEOF 
var evil = 42;
var notevil = "1.5";
if (evil) {
	notevil = typeof(notevil);
}
print(notevil);
*/
/* JSOP_TRAP 
var evil = 42;
var x = 5;
function test() { 
	return x;
};
if (evil) {
	trap(test, 0, 'print("vorher: " + x);x = 15;print("nachher: " + x);x;');
}
print("ergebnis: " + test());
*/
/* JSOP_TOOBJECT 
var evil = 42;
var x = "3";
if (evil) {
	for (p in x) {
		print("p: " + p);
		print("x[p]: " + x[p]);
	}
}
*/
/* JSOP_THIS 
var evil = 42;
var o = { 
  x : 12, 
  test : function () { 
	if (evil) {
		return this.x; 
	}
  }
}
print(o.test());
*/
/* JSOP_TABLESWITCH 
var evil = 1;
var notevil = 2;
switch (evil) {
	default:
		notevil = 5;
	break;
	case 1 : notevil = 3;
		break;
	case 2 : notevil = 4;
		break;		
}
print(notevil);
*/
/* JSOP_SWAP 
// kein bsp, da nur noch legacy! 
*/
/* JSOP_SETSP 
var TO_STRING = Boolean.prototype.toString;

try {
    var s = new String("Not a Boolean");
    s.toString = TO_STRING;
    s.toString();
} catch ( e ) {
}
*/
/* JSOP_SETPROP 
var evil = 42;
var dut = { x : 0};
if (evil) {
	dut.x = 13;
}
print(dut.x);
print(dut.length);
*/
/* JSOP_SETNAME 
var evil = 42;
var notevil = 3;
if (evil) {
	notevil = { x : 3};
}
print(notevil);
*/
/* JSOP_SETELEM 
var evil = 42;
var x = [ 1 ];
if (evil) {
	x[0] = 17;
}
print(x[0]);
*/
/* JSOP_SETCONST 
var evil = 42;
if (evil) {
	const x = 3;// = evil;
}
if (typeof(x) == "undefined") {
 print("not evil x = " + x);
} else {
 print("evil x = " + x);
}
*/
/* JSOP_SETCALL 
// special variable "it" is defined in js.c for this testcase ;)
var evil = 42;
var notevil;
//it.funny = 41;
print(it.funny);
if (evil) {
	notevil = it.item('funny') = evil;
}
print(it.funny);
print(notevil);
*/
/* JSOP_SETARG 
var evil = 42;
function test(x) {
	if (evil) {
		x = 3;
	}
	print(x);
}
test("test");
*/
/* JSOP_RETRVAL, JSOP_SETRVAL, JSOP_RETURN 
var evil = 42;
function x() {
	try {
		if (evil) {
			return "JUHU"+3;
		}
		throw("fehler");
	} catch (e) {
		if (evil) {
			return 1;
		}
	} finally {
		if (evil) {
//			return 2;
		}
	}
}
var y = x();
print(y);
*/
/* JSOP_PUSHOBJ 
var evil = 42;
var x = 3;
if (evil) {
	print(x);
}
*/
/* JSOP_PUSH 
var evil = 42;
if (evil) {
	for (x in {}) {
		print(x);
	}
}
*/
/* JSOP_POS 
var evil = 42;
var notevil = -1;
if (evil) {
	notevil = +notevil;
}
print(notevil);
*/
/* JSOP_POP2, JSOP_POP, JSOP_POPV 
var evil = 42;
var notevil;
if (evil) {
	notevil = 2;
	5;
}
*/
/* JSOP_OBJECT 
var evil = 42;
var notevil;
if (evil) {
	notevil = /x+/;
}
print(notevil);
*/
/* JSOP_NUMBER, JSOP_STRING 
var evil = 42;
var notevil;
if (evil) {
	notevil = 123.23;
	print(notevil);
	notevil = "juhu";
	print(notevil);
}
*/
/* JSOP_NULL, JSOP_ZERO, JSOP_ONE 
var evil = 42;
var notevil;
var notevil2;
if (evil) {
	notevil = null;
	notevil2 = 0;
	notevil2 = 1;
}
print(notevil);
*/
/* JSOP_NOT 
var evil = 42;
var notevil = 3;
if (evil) {
	print(!notevil);
}
print(!evil);
*/
/* JSOP_NOP 
var evil = 42;
if (evil) {
	do {} while (false);
}
*/
/* JSOP_NEW_EQ, JSOP_NEW_NE 
var notevil = 1;
var notevil2 = "1";
var result;
var result2;
var evil = 42;
if (evil) {
	result = notevil === notevil2;
	result2 = notevil !== notevil2;
}
print(result);
print(result2);
*/
/* JSOP_NEW 
var evil = 42;
var notevil = {};
function Test(x) {
	this.y = x;
}
//if (evil) {
	notevil.x = new Test(evil);
//	notevil = new Object();
//}
//print(notevil);
print(notevil.x.y);
*/
/* JSOP_NEG 
var evil = 42;
var notevil = 3;
var result;
if (evil) {
	result = -notevil;
}
print(result);
*/
/* JSOP_NAMEDFUNOBJ 
var x;
var evil = 42;
if (evil) {
	x = function a() {};
}
x();
*/
/* JSOP_NAME 
var evil = 1;
var notevil = 2;
if (evil) {
	notevil;
}
print(notevil);
*/
/* JSOP_LSH, JSOP_RSH, JSOP_URSH 
var evil = 42;
var notevil = 3;
var notevil2;
if (evil) {
	notevil2 = 1 << notevil;
	print(notevil2);
	notevil2 = 100 >> notevil;
	print(notevil2);
	notevil2 = -100 >>> notevil;
	print(notevil2);
}
*/
/* JSOP_LOOKUPSWITCH, JSOP_LOOKUPSWITCHX 
var evil = "z";
var notevil = 1;
switch ( evil ) {
    case "z": notevil = 2;
		break;
    default:
		 notevil = 3;
	break;
}
print(notevil);
*/
/* JSOP_ENTERWITH, JSOP_LEAVEWITH 
var evil = 42;
var notevil = 4;
var notevil2 = { 'x' : 3 };
if (evil) {
	with ( notevil2 ) {
		notevil = 3;
	}
}
print(notevil);
*/
/* JSOP_INSTANCEOF 
var evil = 42;
var notevil = {};
var result;
var result2;
if (evil) {
	notevil.x = {};
}
//if (evil) {
	result = notevil.x instanceof Object;
//	result = "x" instanceof notevil;
//	result2 = notevil instanceof Object;
//}
print(result);
///print(result2);
*/
/* JSOP_INITPROP 
var evil = 42;
if (evil) {
	var obj = { 'x' : 5 };
}
print(obj.x);
*/
/* JSOP_INITELEM, JSOP_NEWINIT, JSOP_ENDINIT 
var evil = 42;
var notevil = 3;
if (evil) {
	var x = [1, notevil];
}
print("0: "+ x[0]);
print("1: "+ x[1]);
*/
/* JSOP_INITCATCHVAR 
var evil = 42;
var notevil = 1;
var dut = {};
try {
	if (evil) {
		dut.x = 3;
	}
	throw(dut.x);
} catch (e) {
	print("catch exception: " + e);
} finally {
	print("finally notevil: " + notevil);
}
*/
/* JSOP_IN 
var evil = 42;
var dut = {};
var notevil = 1;
if (evil) {
	dut.x = 3;
}
notevil = "x" in dut;
print(notevil);
*/
/* JSOP_IFNE 
var evil = 0;
var notevil = 10;
do { 
  ++notevil;
  print(notevil); 
  ++evil;
} while(evil < 2);
print(notevil);
*/
/* JSOP_IFEQ 
var evil = 42;
var notevil = 2;
var a;
//if (evil) {
//	print("juhu");
//}
a = evil && notevil;
print(a);
a = !evil && notevil;
print(a);
*/
/* JSOP_IFEQ 2
var evil = true;
var evil2 = false;
var x = 5;
if (!(1 && evil && evil && evil2)) {
	x = 6;
	print(x);
} else {
	x = 8;
}
print(evil2);
print(x);
*/
/* JSOP_GROUP 
var evil = 1;
var x = (2 + evil);
*/
/* JSOP_GOSUB, JSOP_GOSUBX, JSOP_RETSUB, JSOP_GOTO, JSOP_GOTOX 
try {
	throw("test");
} catch ( e ) {
	print(e);
} finally {
	print("finally");
}
print("juhu");
*/
/* JSOP_GETVAR 
var evil = 42;
function test() {
	var notevil = 3;
	if (evil) {
		notevil = 5 + notevil;
	}
	print (notevil);
}
test();
*/
/* JSOP_GETTER, JSOP_SETTER */
// op2 = JSOP_INITELEM
/* ??????? */
/*
// op2 = JSOP_INITPROP 
var evil = 42;
//if (evil) {
	var obj = { 
		x : 0,
		set a(newValue) { this.x = newValue;},
		get a() { return this.x; }
	}
//}
print(obj.a);
obj.a = evil;
print(obj.a);
print(obj.x);
*/
// op2 = JSOP_SETELEM 
/*
var evil = 1;
var geheim;
var geheim2 = [];
var x = [];
//x[0] setter = function(newValue) { geheim = newValue;};
//x[0] getter = function() { return geheim;} 
print("length: " + x.length);
if (evil) {
	x[0] setter = function(newValue) { geheim2[0] = newValue;};
	x[0] getter = function() { return geheim2[0];} 
}
print("length: " + x.length);
//print("length: " + x.length);
//print("geheim2.length: " + geheim2.length);
//x[0] = evil;
//print("geheim2.length: " + geheim2.length);
//print("geheim2: " + geheim2);
//print("x[0]: " + x[0]);
//print("length: " + x.length);
*/
/*
// op2 obj2 = JSOP_SETNAME 
var evil = 1;
var notevil = 2;
var geheim;
if (evil) {
	name setter = function(newValue) { geheim = newValue;};
	name getter = function() { return geheim;} 
}
name = notevil;
print(name);
/*
// op2 obj2 = JSOP_SETPROP
obj = new Object();
obj.nameSETS = 0;
obj.nameGETS = 0;
obj.name setter = function(newValue) {this._name=newValue; this.nameSETS++;}
obj.name getter = function() {this.nameGETS++; return this._name;}
obj.name = evil;
print(obj.name);
print(obj._name);
*/
/* JSOP_GETPROP 
var evil = 42;
var notevil = 1;
var obj = { 'x' : 0 };
if (evil) {
	notevil = obj.x;
}
print(notevil);
*/
/* JSOP_GETELEM 
var evil = 42;
var notevil = 1;
var x = [0];
if (evil) {
	notevil = x[0];
}
print(notevil);
*/
/* JSOP_GETARG 
var notevil = 3;
var evil = 42;
function test(a) {
	var notevil2 = 0;
	if (evil) {
		notevil2 = a; //print(a);
	}	
	print(notevil2);
}
test(notevil);
*/
/* JSOP_GE, JSOP_LE, JSOP_GT, JSOP_LT 
var evil = 42;
var test = 1;
var notevil = 2;
if (evil) {
	notevil = test >= 3;
}
print(notevil);
*/
/* JSOP_FORVAR 
var x = 1;
var notevil = 0;
var evil = 42;
function test (a) {
	var dut = { };
	dut.x = evil;
	var test;
	if (evil) {
		for (test in dut)
			notevil = dut[test];
	}
}
test(x);
print(x);
*/
/* JSOP_FORPROP 
var evil = 42;
var notevil = 0;
var evil = { };
evil.x = 2;
var ob = { 'x' : 0 };
for (ob.x in evil)
	notevil = evil[ob.x];
print(ob.x);
print(notevil);
*/
/* JSOP_FORNAME 
var evil = 42;
var dut = { };
dut.x = evil;
var notevil = 0;
var notevil2 = 1;
if (evil) {
	for (notevil in dut)
		notevil2 = dut[notevil];
}
print(notevil);
print(notevil2);
*/
/* JSOP_FORELEM 
var evil = { };
evil.x = 3;
var notevil = 0;
var ob = [];
for (ob[0] in evil) 
	notevil = evil[ob[0]];
print("ob = " + ob);
print("ob[0] = " +ob[0]);
print("notevil = " + notevil);
*/
/* JSOP_FORARG 
var evil = 42;
var x = 1;
function test (a) {
	var notevil = 0;
	var dut = { };
	dut.x = evil;
//	dut.x = notevil;
	if (evil) {
		for (a in dut)
			notevil = dut[a];
	}
	print(notevil);
}
test(x);
print(x);
*/
/* JSOP_TRY, JSOP_FINALLY 
try {
	var a = 1;
	throw("test");
} catch (e) {
	print(e);
} finally {
	a = 2;
}
print(a);
*/
/* JSOP_TRUE, JSOP_FALSE 
var evil = 42;
var notevil = 2;
if (evil) {
	notevil = true;
}
print(notevil);
*/
/* JSOP_EXPORTALL, JSOP_EXPORTNAME, JSOP_IMPORTALL, JSOP_IMPORTNAME, JSOP_IMPORTPROP
  check test.html in Debug\mytests!  
var evil = 42;
//var notevil = 2;
var notevil2 = 2;
var w = {notevil : 4};
if (evil) {
//	export *;
//	export notevil;
	export w;
//	export testit;
}
print(evil);
//print(notevil);
//print(notevil2);
//print(testit);
if (evil) {
	import w.*;
}
*/
/* JSOP_EXCEPTION, JSOP_THROW 
var evil = 42;
var dut = { 'x' : evil };
try {
	if (evil) {
		throw(dut.x);
	}
} catch ( e ) {
	var x = "aber: " + e;
    print(x);
}
*/
/* JSOP_EVAL 
var evil = 42;
var notevil = 41;
var dut = {};
var test;
function mytest() {
	return notevil;
}
if (evil) {
	test = eval("dut.x = evil");
//	print(eval("print("+evil+")"));
//	test = eval("print(evil);evil;");
//	test = eval("print(notevil);notevil;");
//	test = mytest();
//	eval("notevil = notevil + 1;");
	print(test);
}
//print(notevil);
print(dut.x);
*/
/* JSOP_EQ, JSOP_NE 
var evil = 42;
var notevil = {};
if (evil) {
	var a = notevil == null;
}
print(a);
*/
/* JSOP_ENUMELEM 
var evil = 42;
var ob = [];
var dut = { 'x' : evil };
var notevil = 2;
if (evil) {
	for (ob[0] in dut) {
		notevil = dut[ob[0]];
	}
}
print(notevil);
print(ob[0]);
print(ob.length);
*/
/* JSOP_DUP2 
var evil = 42;
var notevil = 3;
var t = [notevil];
if (evil) {
	t[notevil] += 1;
}
print(t[0]);
*/
/* JSOP_MOD, JSOP_MUL, JSOP_SUB, JSOP_DIV  
var evil = 3;
var notevil = 4;
var a = 0;
if (evil) {
//	a = 10 % notevil;
//	a = 10 * notevil;
	a = 10 - notevil;
//	a = 10 / notevil;
}
print(notevil);
print(a);
*/
/* JSOP_DELPROP 
var evil = 42;
var x = { notevil : 1 };
x.notevil = "notevil";
if (evil) {
	var a = delete(x.notevil);
}
print(a);
print(x.notevil);
print(x);
*/
/* JSOP_DELNAME 
var evil = 42;
var notevil = 1;
if (evil) {
	var a = delete(notevil);
}
print(a);
print(notevil);
*/
/* JSOP_DELELEM 
var evil = 42;
var notevil = [ 1, 2, 3];
var test = "";
if (evil) {
	test = delete notevil[1];
}
print(notevil[1]);
print(test);
*/
/* JSOP_DEFVAR 
var evil = 42;
if (evil) {
	var notevil = 2;
}
print(notevil);
*/
/* JSOP_DEFSHARP, JSOP_USESHARP 
var evil = 42;
var x;
if (evil) {
	x = #3= [ 1, evil, #3#, 3];
}
var a = x[2][0];
print(a);
*/
/* JSOP_DEFLOCALFUN 
var evil = 42;
function x() {
	function y() {
		print("y");
	} 
	print("x");
	y();
}
x();
*/
/* JSOP_DEFFUN 
var evil = 42;
function notevil() {};
notevil();
*/
/* JSOP_DEFCONST 
var evil = 42;
if (evil) {
	const notevil = 1;
}
print(notevil);
*/
/* JSOP_DEFAULT, JSOP_DEFAULTX 
var evil = 3;
var notevil = 1;
switch(notevil) {
	case (evil == 3):
		notevil = 2;
		break;
	default:
		notevil = 3;
}
print(notevil);
*/
/* JSOP_PROPINC, JSOP_PROPDEC, JSOP_DECPROP, JSOP_INCPROP 
var evil = 42;
var y = { test : evil};
y.test++;
print(y.test);
var y = { notevil : 1};
var evil = true;
if (evil) {
	y.notevil++;
}
print(y.notevil);
*/
/* JSOP_NAMEINC, JSOP_NAMEDEC, JSOP_DECNAME, JSOP_INCNAME 
var evil = true;
var notevil = 2;
if (evil) {
	notevil = --notevil;
}
print(notevil);
print(evil);
*/
/* JSOP_ELEMINC, JSOP_DECELEM, JSOP_INCELEM, JSOP_ELEMDEC 
var evil = 42;
var x = [ 1,2,3];
var notevil = 0;
if (evil) {
	notevil = --x[2]; // ++x[2]; x[2]--; x[2]++;
}
//print(notevil);
print(x[2]);
//print(x[0]);
*/
/* JSOP_DEBUGGER 
var evil = true;
var x = 5;
if (evil) {
	debugger;
}
print(evil);
*/
/* JSOP_CLOSURE 
var evil = true;
var notevil = 2 ;
function f() { notevil = 1;}
if (evil) 
	function f() {
		notevil = 3;
	}

f();
print(notevil);
*/
/* JSOP_CASE, JSOP_CASEX, JSOP_CONDSWITCH 
var notevil = true;
var evil = 42;
// the value in switch says, to what the case-expr has to evaluate to be executed.
//switch(notevil) {
switch(evil) {
	case evil == false :
		notevil = 4;
	case evil == 3 :
		notevil = 3;
		break;
	default:
		notevil = 2;
		break;
	break;
}
print(notevil);
*/
/* JSOP_CALL 
var evil = 1;
var notevil = 2;
function test() {
	notevil = 3 ;
}
if (evil) {
	test();
}
print(notevil);
*/
/* JSOP_BITNOT 
var evil = 42;
var notevil = 3;
if (evil) {
	notevil = ~notevil;
}
print(notevil);
*/
/* JSOP_BITAND, JSOP_BITOR, JSOP_BITXOR
var evil = 42;
var notevil = 15;
if (evil) {
	notevil = notevil & 2;// ^ 3;//| 3; // ~notevil; // 
}
print(notevil);
 */
/* JSOP_BINDNAME 
var evil = 42;
if (evil) {
	var notevil = 0;	
}
print(notevil);
*/
/* JSOP_ARGUMENTS 
var evil = 42;
function a(i,j,k) {
	if (evil) {
		x = !arguments  || arguments;
		x = x[1];
		print(x);
	}
};
a(1,evil,3);
*/
/* JSOP_ARGSUB 
var evil = 1;
var notevil = 0;
function x(i,j,k) { 
	if (evil) {
		notevil = arguments[1]; 
	}
};
x(evil,1,1);
print(notevil);
*/
/* JSOP_ARGDEC, JSOP_ARGINC, JSOP_DECARG, JSOP_INCARG 
var evil = 1;
var notevil = 0;
function test(a) { 
	print("before = " + a); 
	if (evil) {
		a--;
	}
	print("after = " + a); 
}
test(10);
print(notevil);
*/
/* JSOP_ARGCNT 
var evil = 42;
function mytest(test) {
	if (evil) {
		return arguments.length;
	}
}
print(mytest(evil));
*/
/* JSOP_ANONFUNOBJ 
var evil = 42;
var a = function (b) {
	var c = b;
	print(b);
	if (evil) {
		x = function() { print(c);}
	}
	x();
}
a(evil);
*/
/* JSOP_OR, JSOP_ORX, JSOP_AND, JSOP_ANDX 
var evil = 0;//42;
var notevil = 2;
//var a = 0 || evil || 4;
//var evil = 2;
//var a = 1 && evil;
//if (evil) {
//	notevil = 4 && evil && 3;
	notevil = 0 || evil || 3;
//	notevil = 4 && notevil && 3;
//}
print(notevil);
*/
/* JSOP_ADD 
var evil = 42;
var notevil = 2;
if (evil) {
	notevil = notevil + 2;
}
print(notevil);
*/



// -----------------------------------------------------------------------------------------
// other tests
// -----------------------------------------------------------------------------------------

/* try-catch-finally 
var evil = 42;
function dutf(x) {
	if (x > 0) {
		throw x;
	}
}
try{
	try {
		if (evil) {
			throw "juhu";
		}
	} catch (e) {
		if (e) {
			print("EXCEPTION = " + e);
			throw "oje ";
		}
	} finally {
		eval("var y = print('wird auf jeden fall ausgefuehrt')");
		print("y = " + y);
		if (evil) {
			throw "oje2";
		}
	}
} catch(e2) {
	print("OUTER EXCEPTION = " + e2);
}
*/
/* just for-var-iterator 
var evil = 7;
var o = { x: evil};
var x;
var notevil;
for (x in o) {
	notevil = o[x];
}
print(notevil);
*/
/* just for-iterator 
var i = 3;
var evil = 1;
var notevil;
for (i = 0; i<evil; i++ ) {
	notevil = i;
}
print(notevil);
print(i);
*/
/* script_freeze 
version(150);
function testi() {
	print("juhu");
}
var s = new Script(""+testi.toString()); 
var frozen = s.freeze();
print("len: "+frozen.length); 
print("frozen: "+frozen);
*/
/* obj-tainting 
var evil = true;
var dut = {};
if (evil) {
	dut.x = 3;
}
var anz = 0;
for (var x in dut) {
  ++anz;
}
print(anz);
*/
//for (x in {}) {
//	print(x);
//}

/* JSOP_IFEQ, JSOP_IFEQX, JSOP_IFNE, JSOP_IFNEX
var evil = true;
// JSOP_IFEQ*
if (evil) {
 print("evil");
} else {
 print("not evil");
}
// JSOP_IFNE*
do { 
  print(evil); 
  evil = false; 
} while(evil);
*/
/* php-faktor ;) */
/*
var x = new Object();
x[eval('"y"')] = "juhu";
print(x.eval('y'));
*/
/* JSOP_DUP 
var evil = 1;
var t = { test : evil };
t.test += 1;
print(t.test);
*/
/* JSOP_VARDEC, JSOP_VARINC, JSOP_DECVAR, JSOP_INCVAR 
var evil = 3;
function a() {
 var b = 2;
 if (evil == 3) {
	--b;
 }
 print("b= " +b);
}
a();
*/
/*
var x = new Array(1);
x[0] = "1";
print(typeof(x[0]));
var y = x[0]++;
print(typeof(x[0]));
print(typeof(y));
*/
/*
switch(1) {
	case (x == 1) :
}
*/
// if (true) function f() {return 1;}
/*
function test() {
	print(f());
}
test();
*/
/*
var a = 1;
var b = 3;
if (a == 1) {
	print("juhu");
	b = 2;
} else {
	b = 2;
}
print(b);
*/
/*
var now = (new Date()).valueOf();
function xxx(t) {
print("y");
  t++;
  print("z");
}
xxx(now);
*/
/*
var b;
var a = function () {
var evil = 2;
evil = evil + 3;
b = --evil;
}
a();
print(b);
*/
/*
var evil = 3;
var newone = ~evil;
print (newone);
*/
/*
function ToHexString( n ) {

    var hex = new Array();

    hex[hex.length] = n % 16;
	print("!!!!!!!!!!!!!! h = " + hex[0]);

    var string ="";

    for ( var index = 0 ; index < hex.length ; index++ ) {
        switch ( hex[index] ) {
            case 10:
                string += "A";
                break;
            case 11:
                string += "B";
                break;
            case 12:
                string += "C";
                break;
            case 13:
                string += "D";
                break;
            case 14:
                string += "E";
                break;
            case 15:
                string += "F";
                break;
            default:
                string += hex[index];
        }
    }

    if ( string.length == 1 ) {
        string = "0" + string;
    }
    return string;
}

print(ToHexString(10));
*/
/*
var a = 1;
mylabel: do {
  a ++;  
  if (a <3) continue mylabel;
} while (0);
print(a);
*/
/*
for (var a = 1; a<3;a++) {
	print(a);
}
*/
//function Test() {
//}
//var x = document.cookie;
//print("juhu");
//var a = 1.1;
//var j = (2.2 + a) * 2.0 / 3 ;

//var a = new Date();
//x = "asdf" + a;

//print(true.valueOf());
//print(true.toString());

//print((new Array(4294967295)).length);

/*
var a = 1;
if (a == 1) {
	print("a");
} else {
	print("b");
}
print("c");
*/

/*
var a = 1;
if (a == 1) {
	print("a");
}
print("c");
*/
/*
var a = 1;

while (a < 3) {
  a++;
}
*/
/*
var a = 0;
var result = 0;
switch (a) {
    case 0:
        result += 22;
        break;
    case 11:
        result += 22;
        break;
    default:
        result += 232;
}
print(result);
*/

/*
var juhu = function() {
	var a = "hallo";
	x = function () { print(a) };
}
juhu();
*/
/*
var a = 2;
if ((a == 2) && (a != 1)) {
print("then");
} else {
print("else");
}
*/
//print(typeof(x));

//print("length = "+(new Array(0)).length);

//var TEST_ARRAY = new Array(true); 
//print(TEST_ARRAY.join('\v'));

