GUIDELINES
==========

Contribution
------------

The best way to contribute is to fork this project and to request pulls.


Coding guidelines
-----------------

**Indentation**

Please use tabulations to indent your code, and white spaces to draw ascii art.

It means that if you want to align something to the left you must use tabulation, and if you want to aligne something to the right you must use white space.


_Good:_

```
void function (void) {
⇨gint integer;␣␣␣␣/* some comment
⇨gchar character;␣␣* here */

⇨while (test) {
⇨⇨do_something();
⇨}
}
```

_Wrong:_

```
void function (void) {
␣␣gint integer;⇨⇨⇨⇨/* some comment
␣␣gchar character;␣␣* here */

␣␣while (test) {
⇨⇨⇨⇨do_something();
␣␣}
}
```

**Comments**

CPlusPlus-like comments beginning with ``//`` are tolerated.


_Good:_

```c
/* This is a comment,
 * you see?
 */

// This is another comment.
```

**Braces**

Please put the opening left brace at the end of the previous line.  
Never write a ``for``, ``while``, ``if``, and ``else`` block without braces.  
Never write an ``for``, ``while``, ``if``, ``else`` block in only one line.  
If a for statement has an empty block, please write a block with a comment in the empty block.

_Good:_

```c
void function (void) {
	gint it;

	if (test) {
		do_something();
	}
	else if (another_test) {
		do_somethingelse();
	}
	else {
		return;
	}
	for (it = 0; do(&it); it++) {
		// 'for' does it
	}
}
```

_Wrong:_

```c
void function (void)
{
	gint it;

	if (test)
	{
		do_something();
	}
	else if (another_test)
		do_somethingelse();
	else return;

	for (it = 0; do(&it); it++);
}	
  
```

**Parenthesis and spacing**

Leave a white space before ``if``, ``for`` and ``while`` and the opening left parenthesis.  
Please do not write white space between a function name and the opening left parenthesis, unless this is a declaration.  
Please do not put extra white space inside parenthesis.

Please write a white space to the left and the right of an operator.

_Good:_

```c
void function (gint it) {
	gpointer n = NULL;
	gint m = 0;

	if (this && that) {
		do(it);
	}
	else if (i < j) {
		let(it);
	}

	n = f(it)->well;
	m = 5 + 2
}
```

_Wrong:_

```c
void function (gint it) {
	gpointer n=NULL;
	gint m=0;

	if(this && thaht) {
		do (it);
	}
	else if (i<j) {
		let( it );
	}

	n=f (it)->well;
	m = 5+2;
}
```

**Splitted lines**

Please do not split lines if they are not too long to be splitted two times or more (three lines or more).

_Good:_

```c
long_function_name(something very long, and this,
	and that, and this is very very longer than long,
	and this is another very long line);

this_long_variable = "has a not a very very long value";
```

_Wrong:_

```c
function_name(
	something not very long);

this_long_variable =
	"has not a very very long value";
```

**Good to know**

All the code does not currently follow these guidelines, if you change something in the code and if this part does not follow these guidelines, do not hesitate to fix the appearance too.

Rules are not absolute.
