Syntax Reference
================

This section describes all syntactic constructs in Zwerg language.
You may want to check out a XXX tutorial, which introduces many (but
not all) of these in a gradual manner, and XXX vocabulary reference,
to learn about the actual function words that you can use inside this
syntax.

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


Lexical scopes
--------------

Form::

	(|ID₁ ID₂ …| EXPR₁)

A presence of binding block in a parenthesized construct converts the
whole form into a function that pops as many arguments as there are
identifiers in the binding block, and binds them to these identifiers
such that the rightmost one gets the value on TOS and then
progressively lefter ID's get progressively deeper stack values.  The
expression is then evaluated as usual, except one can use the bound
names.

Sometimes it's useful to enclose the whole expression into a scope and
pop and bound the initial Dwarf value::

	(|Dw| Dw entry (name == Dw symtab name))

Another example shows how to implement set union even without
first-class Zwerg support::

	[foo] [bar] (|A B| A [B elem !(== A elem)] add)


Alternation
-----------

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

You would also use alternation to create a sequence value::

	[1, 2, 3]


An OR-list
----------

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


Comparisons
-----------

Form::

	EXPR₁ “==” EXPR₂
	EXPR₁ “!=” EXPR₂
	EXPR₁ “<” EXPR₂
	EXPR₁ “<=” EXPR₂
	EXPR₁ “>” EXPR₂
	EXPR₁ “>=” EXPR₂

An expression such as ``A OP B``, where *OP* refers to one of the
comparison operators mentioned in the above listing, behaves as
follows::

	?(let .tmp1 := A; let .tmp2 := B; .tmp1 .tmp2 OP2)

Depending on operator in question, *OP2* refers to, respectively, ?eq,
?ne, ?lt, ?le, ?gt, and ?ge.

In plain English, the two constituent expressions are both evaluated
in separate sub-expression contexts.  Their TOS's are gathered and
compared using a comparison assertion word.

The comparison assertions are words that hold if the top two stack
values fulfill the relation implied by their name::

	1 2 ?lt        # holds
	1 2 ?gt        # doesn't hold
	2 2 ?eq        # holds
	2 2 ?ne        # doesn't hold
	2 2 !eq        # also doesn't hold


Regular expression matching
---------------------------

Form::

	EXPR₁ “=~” EXPR₂
	EXPR₁ “!~” EXPR₂

These are comparison operators (see above) associated with words,
respectively, ``?match`` and ``!match``.  XXX link


Iteration
---------

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

Zwerg string literals are enclosed in quotation marks.

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

	$ dwgrep '1 2 3 "%s %s %s"'
	3 2 1
	XXX update when this is implemented correctly

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


Sequence capture
----------------

Form::

	“[” EXPR₁ “]”

Sequences offer a way to gather values yielded by another expression.
If you think of alternation as a fork, then capture is a join.

All input stacks are passed to 

*EXPR₁* is evaluated in sub-expression context, gathers TOS's of what
it yields, and pushes a sequence with those elements.

     : [1, 2, 3]        # produces a list with elements 1, 2 and 3
     : [child]          # list of immediate children
     : [child*]         # list of all descendants

   - [] produces an empty list.  It is exactly equivalent to (among
     others):
     : [ 0 == 1 ]

   - [()] is a capture of nop.  It wraps TOS in a list and pushes it.
     : 1 [()]	# gives 1 [1]

Form::

	“[” (“|” ID₁ ID₂ … “|”)? EXPR₁ “]”

   - Sub-expression capture allows a binding block.  A presence of
     such block converts the whole form into a function that pops as
     many arguments as there are identifiers in the binding block, and
     binds them to these identifiers such that the rightmost one get
     the value on TOS and then progressively lefter ID's get
     progressively deeper stack values.  The expression is then
     evaluated as usual, except one can use the bound names:

     : [|A| A child]		# convert DIE on TOS into a list of its children
     : [|A B| A foo B bar]
     : [|A| A]			# wrap TOS in a list

** •“let” ID₁ ID₂ … “:=” EXPR₁ “;” — binding
    - EXPR₁ is evaluated in sub-expression context.  Then values near
      TOS are bound to given identifiers and exported into surrounding
      context.  From that point on, mentioning ID is the same as
      pushing the value to TOS.

    - Ordering of ID's is such that the rightmost is bound to TOS, the
      next one to the left to one below TOS, etc.  E.g.:

      : let A B := 1 2 ;	# A is 1, B is 2

      The mnemonic for this is that the list of variables describes
      stack layout, with TOS being on the right.

    - When a binding is mentioned that references a block, that counts
      as invocation of the referenced block.  In other words, there's
      an implicit "apply" for bindings that reference blocks.
      : let inc := { 1 add };
      : 2 inc	# 3
