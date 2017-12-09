.. _syntax:

Syntax
======

This section describes all syntactic constructs in Zwerg language.
You may want to check out :ref:`tutorial`, which introduces many (but
not all) of these concepts on an example, and
:ref:`zw_vocabulary_core` or :ref:`zw_vocabulary_dwarf` to learn about
the actual function words that you can use.


Comments
--------

Form::

	// Ignored until the end of the line.
	# This one as well.
	/* C-like comments work as well.  */

Comments are ignored.  Nesting comments is not allowed.


Empty expression
----------------

Empty expression yields exactly the stacks that get to its input.

For example, the following is an empty expression within negative
sub-expression assertion.  This expression never yields anything::

	!()	# An expression that never yields.

As another example, it may be useful to bind the initial Dwarf value
to a name, typically in cases when one needs to refer to it several
times.  One way to do this is this::

	let Dw := ;	# There's an empty expression between := and ;.


Words
-----

Form::

	foo
	?foo
	!foo
	@foo	# and more.

Most functional parts of Zwerg expressions are expressed by words.
Words consume input stacks and may yield possibly several stacks of
arbitrary composition.  A typical word would inspect or pop one or
more words on each incoming stack, carry out some computation, and
produce zero or more stacks with the results of computation pushed to
them.

For example, the word ``entry`` expects a stack with a Dwarf value on
top.  It then examines the Dwarf data, and for each debug info entry
(DIE), it produces a stack with that DIE pushed on top (instead of the
consumed Dwarf value).  E.g.::

	$ dwgrep ./tests/nontrivial-types.o -e ''
	<Dwarf "./tests/nontrivial-types.o">

	$ dwgrep ./tests/nontrivial-types.o -e 'entry'
	[b]	compile_unit
		producer (strp)	GNU C 4.6.3 20120306 (Red Hat 4.6.3-2);
		[... more attributes ...]
	[2d]	structure_type
		name (string)	foo;
		[... more attributes ...]
	[35]	subprogram
		external (flag)	true;
		name (strp)	blah;
		[... more attributes ...]
	[... more output ...]

Some words (actually quite a few of them) will change their behavior
depending on what values are on the stack.  E.g. ``entry`` is besides
Dwarfs applicable to, as of this writing, compilation units and
abbreviation units.

The words that start with ``?`` and ``!`` are assertions.  While what
was written about words in general applies to assertions as well,
their mode of operation is more restricted.  Assertions either yield
the incoming stack unchanged, or yield nothing at all.

The assertions come in pairs, where the ``?``-flavored one tests the
positive condition (e.g. ``?root`` tests whether a DIE on top of stack
is a root DIE), and the ``!``-flavored one tests the opposite
condition (e.g. ``!root`` tests whether a DIE on top of stack is not a
root DIE).

Concatenation
-------------

Form::

	EXPR₁ EXPR₂ …

The resulting expression passes all input stacks to *EXPR₁*, takes the
output of that, passes it to *EXPR₂*, and so on.  The result of
concatenation is then whatever the last constituent expression yields.

The constituent expressions are evaluated in plain context.


Parenthesizing
--------------

Form::

	(EXPR₁)

Concatenation has a high precedence, so parentheses can be used
liberally to adjust how an expression should be chopped into
sub-expression.  The parenthesized expression is evaluated in plain
context.

For example::

	foo bar, baz	# these two ...
	(foo bar), baz	# ... mean the same thing
	foo (bar, baz)	# this one is different


Integer literals
----------------

Form::

	“-”?(“0x”|“0o”|“0b”|“0”|“”){digits}

Integer literals are words that start with a decimal digit, or by a minus sign
followed by a decimal digit. For integer literal to be valid, the word needs to
be composed of a prefix that determines radix of the literal, followed by one or
more digits from a set specified as follows:

- ``0x`` and ``0X`` are hexadecimal prefixes.  Valid digits are
  ``[0-9a-fA-F]``.

- ``0o``, ``0O`` and ``0`` are octal prefixes.  Valid digits are
  ``[0-7]``.

- ``0b`` and ``0B`` are binary prefixes.  Valid digits are ``[0-1]``.

- Without prefix, decimal base is assumed.  Valid digits are
  ``[0-9]``.

- An initial ``-`` means the number is negative.

Zwerg integers can hold any 64-bit signed or unsigned number::

	$ dwgrep '0xffffffffffffffff -0x7fffffffffffffff add'
	0x8000000000000000

Invalid integer literals are diagnosed at query compilation time::

	$ dwgrep '123foo'
	dwgrep: Invalid integer literal: `123foo'


Named constants
---------------

Form::

	DW_AT_name
	DW_TAG_base_type
	DW_FORM_strp
	DW_LANG_C

Zwerg has support for named constants.  They aren't merely aliases for
numbers--Zwerg remembers their domain and uses it when the value needs
to be displayed or converted to a string.  It is still possible to
extract the underlying numerical value::

	$ dwgrep 'DW_TAG_base_type value'
	36
	$ dwgrep 'DW_TAG_base_type hex'
	0x24

Numbers in non-decimal bases use the same trick, so if you use a
hexadecimal number, it will keep its hexadecimal-ness throughout the
script::

	$ dwgrep '10 0x10 0o10 0b10'
	---
	0b10
	010
	0x10
	10

Similarly values decoded from attribute will have the
appropriate "skin"--they will be named constants, hexadecimal or
decimal numbers, depending on what is deemed the best fit::

	$ dwgrep ... -e 'entry @AT_language'
	DW_LANG_C89

	$ dwgrep ... -e 'entry @AT_decl_line'
	4
	6
	11

	$ dwgrep ... -e 'entry @AT_low_pc'
	0x80483f0
	0x80482f0


Lexical scopes (``(|A|…)``)
------------------------------

Form::

	(|ID₁ ID₂ …| EXPR₁)

A presence of binding block in a parenthesized construct converts the
whole form into a function that pops as many arguments as there are
identifiers in the binding block, and binds them to these identifiers
such that the rightmost one gets the value on TOS and then
progressively lefter *ID*'s get progressively deeper stack values.
The expression is then evaluated as usual, except one can use the
bound names.

Sometimes it's useful to enclose the whole expression into a scope and
pop and bound the initial Dwarf value::

	(|Dw| Dw entry (name == Dw symtab name))

Another example shows how to implement set union even without
first-class Zwerg support::

	[foo] [bar] (|A B| A [B elem !(== A elem)] add)


ALT-lists (``,``)
-----------------

Form::

	EXPR₁ “,” EXPR₂ …

The resulting expression passes all input stacks to all of *EXPR₁*,
*EXPR₂*, etc.  It then yields any and all stacks that any of the
constituent expressions yields.

All constituent expressions shall have the same overall stack effect
(the number of slots pushed minus number of slots popped will be the
same for each branch).

The constituent expressions are evaluated in plain context.

E.g. to compare a TOS value to one of the fixed list of values, you
would do::

	(== (1, 2, 3))

You would also use ALT-lists to create a sequence value::

	[1, 2, 3]


OR-lists (``||``)
-----------------

Form::

	EXPR₁ “||” EXPR₂ …

Each input stack is passed first to *EXPR₁*.  If that yields anything,
that's what the overall expression yields.  Otherwise the same
original input stack is passed to *EXPR₂*, and so in this fashion.  If
neither constituent expression yields anything, the overall expression
doesn't yield anything either.  The mode of operation is very similar
to shell-like OR lists, hence the name.

All constituent expressions shall have the same overall stack effect
(the number of slots pushed minus number of slots popped will be the
same for each branch).

The constituent expressions are evaluated in plain context.

An important use of an OR list is to implement case-like conditions
and fallback cases::

	let name := (@DW_AT_linkage_name || @DW_AT_MIPS_linkage_name
	             || @DW_AT_name || "???");

	let has_loc := (?(@DW_AT_location) true || false);


Infix assertions (``==``)
-------------------------

Form::

	EXPR₁ OP₁ EXPR₂

Zwerg has support for infix binary assertions.  These forms are
evaluated as follows::

	?(let .tmp1 := EXPR₁; let .tmp2 := EXPR₂; .tmp1 .tmp2 OP₂)

In plain English, *EXPR₁* and *EXPR₂* are each evaluated in
sub-expression context.  The TOS's of the stacks that this produces
are then put to stack and a certain operator is evaluated.  This shows
that the two expressions are independent of each other.

What actual word *OP₂* refers to depends on what operator OP₁ is, but
it will be an assertion that looks at top two stack slots.  There
might be no actual word per se, but conceptually this is how the form
behaves.

Another thing worth noticing is that the whole form behaves as an
assertion.  No side effects leak from the constituent *EXPR*'s or from
the operator itself.

Individual operators are essentially just words, just with a bit of
syntactical support on Zwerg side.  So which operators are available
depends on which vocabularies any particular wrapper brings in.  Zwerg
core defines the following *OP₁*'s with the following associated
*OP₂*'s::

	EXPR₁ “==” EXPR₂	# ?eq
	EXPR₁ “!=” EXPR₂	# ?ne
	EXPR₁ “<” EXPR₂		# ?lt
	EXPR₁ “<=” EXPR₂	# ?le
	EXPR₁ “>” EXPR₂		# ?gt
	EXPR₁ “>=” EXPR₂	# ?ge
	EXPR₁ “=~” EXPR₂	# ?match
	EXPR₁ “!~” EXPR₂	# !match

See :ref:`?eq <zw_vocabulary_core ?eq>` to learn about comparison
operators.  See :ref:`?match <zw_vocabulary_core ?match>` to learn
about regular expression matching.

For example::

	$ dwgrep ./tests/typedef.o -e 'entry (offset == 0x1d)'
	[1d]	typedef
		name (strp)	int_t;
		decl_file (data1)	/home/petr/proj/dwgrep/typedef.c;
		decl_line (data1)	1;
		type (ref4)	[28];


Iteration (``*``)
-----------------

Form::

	EXPR₁ “*”
	EXPR₁ “+”
	EXPR₁ “?”

The effect of expressions ``EXPR₁*`` is that the input stack is
yielded unchanged, and also passed to *EXPR₁*.  The output is
collected and yielded, and then passed back to *EXPR₁*.  This process
is repeated until *EXPR₁* stops yielding more stacks.

This is typically used for forming closures--peeling uninteresting
types, forming sets of nodes, etc.  E.g.::

	child*	# nodes of tree rooted in node that's on TOS

One can also (ab)use this to create an explicit for loop::

	0 (1 add ?(10 ?lt))*

*EXPR₁* shall push exactly the same number of stack slots as it pops.

The effect of ``EXPR₁+`` is the same as that of ``EXPR₁ EXPR₁*``.

The effect of ``EXPR₁?`` is the same as that of ``(, EXPR₁)``.

For example::

	A B swap? ?gt drop    # "max" -- keep the greater number on stack
	A B swap? ?lt drop    # "min"


Formatting strings
------------------

Form::

	“r”? “"” (formatting string) “"” “\”?

Zwerg string literals are functions that push themselves to an
incoming stack, which they then yield.  The literal is enclosed in
quotes and if a quote itself should be part of the string, it should
be escaped with a backslash (``\``).  Should a backslash be part of
the string and not considered an escape character, it should be
escaped by another backslash::

	"a simple string"
	"a string \"with\" quotes"
	"a string with backslash: \\"

Other recognized escapes include:

+-----------+---------------------------------------------------------+
| escape    | meaning                                                 |
+===========+=========================================================+
| ``\\``    | Escaped backslash stands for a backslash::              |
|           |                                                         |
|           |	$ dwgrep '"\\a\\"'                                    |
|           |	\a\                                                   |
+-----------+---------------------------------------------------------+
| ``\123``  | *123* stands for octal number.  The escape is replaced  |
|           | with an ASCII character the code of which is the octal  |
|           | number::                                                |
|           |                                                         |
|           |	$ dwgrep '"\110\145\154\154\157"'                     |
|           |	Hello                                                 |
+-----------+---------------------------------------------------------+
| ``\xab``  | *ab* stands for hexadecimal number.  The meaning is     |
|           | analogous to that of \\123::                            |
|           |                                                         |
|           |	$ dwgrep '"\x77\x6f\x72\x6c\x64\x21"'                 |
|           |	world!                                                |
+-----------+---------------------------------------------------------+
| ``\t``    | Stands for the tab character.                           |
+-----------+---------------------------------------------------------+
| ``\n``    | Both a literal end-of-line character, as well as the    |
|           | ``\n`` escape stand for new line::                      |
|           |                                                         |
| ``<EOL>`` |	$ dwgrep '"foo\nbar"'                                 |
|           |	foo                                                   |
|           |	bar                                                   |
|           |                                                         |
|           |	$ dwgrep '"foo                                        |
|           |	bar"'                                                 |
|           |	foo                                                   |
|           |	bar                                                   |
+-----------+---------------------------------------------------------+
| ``\<EOL>``| Escaped newline is ignored::                            |
|           |                                                         |
|           |	$ dwgrep '"foo\                                       |
|           |	bar"'                                                 |
|           |	foobar                                                |
+-----------+---------------------------------------------------------+

There are more escape sequences (essentially those mentioned in ``man
ascii`` are supported as well).

If a string literal is prefixed by ``r``, it's actually a raw string
literal.  These work the same as normal formatting strings, but escape
sequences are left intact in the string::

	$ dwgrep '"foo \"bar\\\""'
	foo "bar\"

	$ dwgrep 'r"foo \"bar\\\""'
	foo \"bar\\\"

String literals can be split, provided that all but the last segment
end not with a mere quote, but ``"\``.  The following two examples
produce equivalent programs::

	"a long string "\ "that continues here"
	"a long string that continues here"

Any whitespace (but only whitespace) is allowed between ``"\`` and the
following ``"``.

String literals can contain formatting directives.  Formatting
directive are two-character sequences whose first character is a
``%``.  Such strings are not merely literals anymore, they behave more
as template strings.  They are functions that take values from the
stack and splice their string representation together with the
non-formatting-directive parts of the string.

A pseudo-directive ``%%`` is not really a directive at all, it's an
escape that stands for a single percent character.

The simplest formatting directive is ``%s``::

	$ dwgrep ... -e 'entry ?AT_decl_column !AT_decl_line
	                 "%s has DW_AT_decl_column, but NOT DW_AT_decl_line"'
	[58] formal_parameter has DW_AT_decl_column, but NOT DW_AT_decl_line

	$ dwgrep ... -e 'entry attribute (form "%s" =~ "DW_FORM_ref.*")'
	type (ref4)	[65];
	sibling (ref4)	[65];
	type (ref4)	[2d];
	[... etc ...]

The most general formatting directive is a pair of ``%(`` and ``%)``,
which encloses an expression.  The formatting string passes input
stacks to that expression, which is evaluated in plain context.  TOS
of yielded stacks is then popped, converted to a string, spliced, and
the resulting string is pushed to stack.  For example::

	$ dwgrep ... -f /dev/stdin <<"EOF"
	(|Dw|
	 let T := Dw entry ?TAG_typedef ;
	 let U := T @AT_type (?TAG_typedef @AT_type)* !TAG_typedef ;

	 "[%( T offset %)] typedef %( T @AT_name %) %( U @AT_name || "???" %) "\
	 "(%( U @AT_encoding || "???" %), %( U @AT_byte_size || "???" %) bytes)"
	)
	EOF
	[0x1d] typedef int_t int (DW_ATE_signed, 4 bytes)
	[0x2f] typedef int_t_t int (DW_ATE_signed, 4 bytes)
	[0x3a] typedef int_t_t_t int (DW_ATE_signed, 4 bytes)

If there's more than one formatting directive in a given string, they
are resolved in the order from right to left (sic!): rightmost
formatting directive gets TOS value::

	$ dwgrep 'dwgrep '1 2 3 "{%s [%s (%s)]}"'
	{1 [2 (3)]}

This principle is applied throughout Zwerg: when a binding block
contains several names the rightmost name is bound to value on TOS.
The reason should be apparent from the above example: because the
strings are parsed from left to right, rightmost word produces the
topmost stack value.

Now that we have introduced ``%( %)``, it is easy to use it to define
the others:

- ``%s`` stands for ``%( %)``
- ``%d`` stands for ``%( value %)``
- ``%x`` stands for ``%( value hex %)``
- ``%o`` stands for ``%( value oct %)``
- ``%b`` stands for ``%( value bin %)``


Sequence capture (``[]``)
-------------------------

Form::

	“[” EXPR₁ “]”

Sequences offer a way to gather values yielded by another expression.
If you think of ALT-list as a fork, then capture is a join.

The resulting expression passes any input stacks to *EXPR₁*.  For each
input stack, it gathers the stacks produced by *EXPR₁*, takes the top
value off each of them, and collects these values in a sequence.  This
sequence it then pushes to TOS.

*EXPR₁* is evaluated in sub-expression context.  That means that the
sequence is pushed to TOS *in addition* to whatever was already
there--nothing is removed.

Of particular interest is interplay between ALT-list and capture,
which allows easy and syntactically familiar construction of sequence
literals::

	$ dwgrep '[1, 2, 3]'
	[1, 2, 3]

Or you can use this construct to e.g. collect children of a node on
TOS::

	$ dwgrep ./tests/typedef.o -e 'entry ?root [child]'
	---
	[[1d] typedef, [28] base_type, [2f] typedef, [3a] typedef, [45] variable]
	[b]	compile_unit
		producer (strp)	GNU C 4.6.3 20120306 (Red Hat 4.6.3-2);
		language (data1)	DW_LANG_C89;
		name (strp)	typedef.c;
		comp_dir (strp)	/home/petr/proj/dwgrep;
		stmt_list (data4)	0;

Or the whole tree rooted at node on TOS::

	dwgrep ./tests/typedef.o -e 'entry ?root [child*]'
	---
	[[b] compile_unit, [45] variable, [3a] typedef, [2f] typedef, [28] base_type, [1d] typedef]
	[b]	compile_unit
		producer (strp)	GNU C 4.6.3 20120306 (Red Hat 4.6.3-2);
		language (data1)	DW_LANG_C89;
		name (strp)	typedef.c;
		comp_dir (strp)	/home/petr/proj/dwgrep;
		stmt_list (data4)	0;

Form::

	“[” “]”

This produces an empty list.  Alternatively, one could also do::

	[!()]

But that's somewhat awkward.

To capture an empty expression, one would need to explicitly
parenthesize it::

	$ dwgrep '1 [()]'
	---
	[1]
	1


Capture with binding (``[|A|…]``)
---------------------------------

Form::

	“[” “|” ID₁ ID₂ … “|” EXPR₁ “]”

Sub-expression capture allows a binding block.  A presence of such
block converts the whole form into a function that pops as many
arguments as there are identifiers in the binding block, and binds
them to these identifiers such that the rightmost one get the value on
TOS and then progressively lefter *ID*'s get progressively deeper
stack values.  The expression is then evaluated as usual, except one
can use the bound names::

	$ dwgrep ./tests/typedef.o -e 'entry ?root [|A| A child]'
	[[1d] typedef, [28] base_type, [2f] typedef, [3a] typedef, [45] variable]

	$ dwgrep '1 [|A| A]'	# or you could just write '[1]' ;)
	[1]


Name binding (``let``)
----------------------

Form::

	“let” ID₁ ID₂ … “:=” EXPR₁ “;”

This form introduces a new name into the current scope.  It passes the
input stack to *EXPR₁*, which is evaluated in sub-expression context.
Then values near TOS are bound to given identifiers and exported into
surrounding context.  Then the original stack is yielded as many times
as *EXPR₁* yields, each time with a possibly different set of
bindings.  Bound names may be mentioned later, and they push the bound
value to the stack.

Consider the following examples::

	$ dwgrep 'let X := 1; X'
	1

	$ dwgrep 'let X := 1, 2; X'
	1
	2

Ordering of ID's is such that the rightmost is bound to TOS, the next
one to the left to one below TOS, etc.  The mnemonic for this is that
the list of variables describes stack layout, with TOS being on the
right.  E.g.::

	$ dwgrep 'let A B := 1 2, 3 4; A B'
	---
	2
	1
	---
	4
	3

The new bindings are introduced into the most enclosing scope.  The
following constructs comprise a scope:

- Any sub-expression context has a scope of its own::

	$ dwgrep '?(let A := 1;) A'
	Error: Unknown identifier `A'.

- Each branch of an OR-list or ALT-list has a scope of its own::

	$ dwgrep '(let A := 1;, let A := 2;) A'
	Error: Unknown identifier `A'.

- Parenthesized expressions with name binding blocks are a scope::

	$ dwgrep '"" (|X| let A := 1;) A'
	Error: Unknown identifier `A'.

- ``then`` and ``else`` branches of an ``if-then-else`` form
  each introduce a scope::

	$ dwgrep 'if ?() then let A := 1; else let A := 2; A'
	Error: Unknown identifier `A'.

  Typically these constructs can be rewritten as follows::

	$ dwgrep 'let A := if ?() then 1 else 2; A'
	1

On the other hand, simple parenthesizing does not introduce a scope::

	$ dwgrep '(let A := 1;) A'
	1

It is not allowed to rebind an once-bound name within the same scope::

	$ dwgrep 'let A := 1; let A := 1;'
	Error: Name `A' rebound.

It is also not possible to access names from outer scopes if they are
shadowed by the same name in an inner scope.  In the following, the
inner reference to ``A`` will always resolve to ``2``, and there is no
way to access the outer ``A`` of ``1``::

	$ dwgrep 'let A := 1; 2 [|A| A]'
	[2]

Finally, it's not allowed to rebind existing words::

	$ dwgrep 'let child := 1;'
	Error: Can't rebind a builtin: `child'


Sub-expression assertions (``?()``, ``!()``)
--------------------------------------------

Form::

	“?(” EXPR₁ “)”
	“!(” EXPR₁ “)”

The form ``?(...)`` sends an input stack to *EXPR₁*, which is then
evaluated in sub-expression context.  If it yields anything, the
overall assertion succeeds and yields the original stack.  The form
``!(...)`` has the inverse semantics: it succeeds when *EXPR₁* yields
nothing.

The behavior of these two forms is equivalent to the following::

	([ EXPR₁ ] != [])	# ?( EXPR₁ )
	([ EXPR₁ ] == [])	# !( EXPR₁ )

For example, to select leaf DIE's of a Dwarf graph::

	!(child)

To test whether one of the DIE's contains an empty location
expression::

	entry ?(@AT_location !(elem))


Conditionals (``if-then-else``)
-------------------------------

Form::

	“if” EXPR₀ “then” EXPR₁ “else” EXPR₂

Input stack is passed to *EXPR₀*, which is evaluated in sub-expression
context.  Next a body expression is evaluated, which is either *EXPR₁*
if *EXPR₀* yielded anything.  Or, if *EXPR₀* yielded nothing, *EXPR₂*
is the body expression.

The input stack is then passed to the body expression, which is
evaluated in plain context.  The overall form then yields anything
that the body expression yields.

Both *EXPR₁* and *EXPR₂* shall have the same overall stack effect (the
number of slots pushed minus number of slots popped will be the same
for each branch).

This form could be circumscribed by the following snippet::

	(?(EXPR₀) (EXPR₁), !(EXPR₀) (EXPR₂))

As an example, consider the following snippet from a script::

	if ?DW_TAG_formal_parameter then (
	  // Of formal parameters we ignore those that are children of
	  // subprograms that are themselves declarations.
	  ?(parent !DW_TAG_subroutine_type !(@DW_AT_declaration == true))
	) else (
	  ?DW_TAG_variable
	)

In particular, note the following surprising example::

	$ dwgrep 'if false then "yes" else "no"'	# WARNING!
	yes

The reason is that ``false`` is a named constant, which means the
tested expression always yields a stack: namely a stack with ``false``
on top.  This is how Booleans should be tested in Zwerg::

	if (@AT_declaration == true) then … else …
