.. _tutorial:

Tutorial
========

To get our feet wet, we will develop a query to find functions the
arguments of which are directly of structure or class types.  Passing
such values in parameters may incur a potentially expensive copy, and
it might be desirable to audit these uses.  If you have a ``dwgrep``
distribution tarball handy, there's a file
``tests/nontrivial-types.o``, which you can use to play along with the
tutorial.  Or your can compile one yourself, this is the source that
we are using::

	struct foo {};

	int blah (struct foo f) {
	  return 0;
	}

	int main(int argc, char *argv[])
	{
	  return blah ((struct foo) {});
	}

First and foremost, a quick word about the command line interface.
(XXX link to full reference) ``dwgrep`` is to some extent modeled
after ``grep`` utility.  The following invokes ``dwgrep`` with an
empty query on aforementioned file::

	$ dwgrep '' ./tests/nontrivial-types.o
	<Dwarf "./tests/nontrivial-types.o">

Much like with ``grep``, you can change the order of arguments if you
denote which of them is query::

	$ dwgrep ./tests/nontrivial-types.o -e ''
	<Dwarf "./tests/nontrivial-types.o">

You can also use ``-c`` if you only care about the number of results,
not the results themselves::

	$ dwgrep -c ./tests/nontrivial-types.o -e ''
	1

The empty query is not very interesting, but it shows us that the
input stack actually contains one value: the Dwarf file that we gave
on the command line.  So let's look inside!

``entry``, ``unit`` --- DIE iteration
-------------------------------------

There are several functions for entering the graph, the most useful
one is likely ``entry``.  This takes [#takes]_ a Dwarf, and yields
[#yields]_ all DIE's in that Dwarf::

	$ dwgrep ./tests/nontrivial-types.o -e 'entry'
	[b]	compile_unit
		producer (strp)	GNU C 4.6.3 20120306 (Red Hat 4.6.3-2);
		[... more attributes ...]
	[2d]	structure_type
		name (string)	foo;
	[... more output ...]


.. [#takes] The word "takes" is generally used to mean that the
   function expects a stack, whose top value(s) fits the given
   description.  E.g. ``length`` takes a string or a sequence, ``add``
   takes two integers, strings or integers, etc.
   
   Taken values are discarded from the stack, though typically the
   function in question pushes some other value(s) in their stead.

.. [#yields] This word is used to mean that the function produces
   stack(s) with the outlined property.  E.g. if ``length`` takes a
   string or a sequence, and yields their length, that means that the
   top value is popped, its length is determined, and a value
   corresponding to that length is pushed back.  Such stack is then
   sent to output.

Other words include ``unit``, which selects compilation and type
units, and ``abbrev`` which selects abbreviation units.

It is impractical to have to repeat the whole ``dwgrep`` command line
every time a code snippet needs to be shown.  In the following we will
use the following notation::

	$ 'unit'
	CU 0

The dollar at the beginning represents the command line prompt, and
then the query itself is given in single quotes.

If we don't care about query output (e.g. if it is trivial or
uninteresting), we will just list the query itself::

	entry


``?DW_TAG_foo``, ``?TAG_foo`` --- DIE tag assertion
---------------------------------------------------

The assertion ``?DW_TAG_foo`` holds [#holds]_ for DIE's the tag of
which is ``DW_TAG_foo``.

.. [#holds] The functions which either yield an unmodified input or
   nothing at all are called assertions, and are very common in Zwerg
   expressions.  We say that assertion holds if the values on top of
   stack (TOS) satisfy some property.  If the assertion holds, it
   yields an unchanged incoming stack.  If it doesn't hold, it yields
   nothing at all.  Assertions never modify the stack.

You can write ``?DW_TAG_foo`` also as ``?TAG_foo``, they do the same
thing.  In the following, that's how we will refer to this assertion.

In Zwerg, function pipelines are formed by simply placing functions
next to each other.  The stacks that the left function produces
becomes an input of the right function.

For example, to list all subprograms, one would use the following
expression::

	$ 'entry ?TAG_subprogram'
	[35]	subprogram
  		external (flag)	true;
  		name (strp)	blah;
  		[... more attributes ...]
	[6c]	subprogram
		external (flag)	true;
		name (strp)	main;
  		[... more attributes ...]


``?DW_AT_foo``, ``!DW_AT_foo`` --- attribute presence assertion
---------------------------------------------------------------

``?DW_AT_foo`` holds on DIE's that have an attribute ``DW_AT_foo``,
and on attributes that are ``DW_AT_foo``.  As with tags (and any
Dwarf-related assertions at all), you can shorten this to ``?AT_foo``.

``!DW_AT_foo`` similarly holds on DIE's that do NOT have an attribute
``DW_AT_foo``, or on attributes that are NOT ``DW_AT_foo``.

For assertions, concatenation happens to work as a logical and, so
for example, this is how we can get a list of all subprograms that
have a declaration attribute::

	entry ?TAG_subprogram ?AT_declaration

Similarly, to get subprograms that do not have declaration attribute,
we say::

  	entry ?TAG_subprogram !AT_declaration

``child`` --- child traversal
-----------------------------

``child`` takes a DIE and yields each of its children.  (Which is to
say: it yields nothing for child-less DIE's, or it can actually yield
a number of stacks if there are many DIE's.  Each of them will have
one of the children on TOS.)

Applied to DIE's in our running example, this will get us to formal
arguments of the selected subprograms::

	$ 'entry ?TAG_subprogram !AT_declaration child'
	[58]	formal_parameter
		name (string)	f;
  		[... more attributes ...]
	[8f]	formal_parameter
		name (strp)	argc;
  		[... more attributes ...]
	[9d]	formal_parameter
		name (strp)	argv;
  		[... more attributes ...]


If we want to make sure these children are actually formal
parameters::

	entry ?TAG_subprogram !AT_declaration child ?TAG_formal_parameter

``@DW_AT_foo`` --- value of attribute ``DW_AT_foo``
---------------------------------------------------

This word is used for accessing values of attributes.  It always takes
a DIE, but what it yields varies by the attribute type.  It could be
another DIE, a string or a number, a sequence of other values, or
whatever contrived value type is deemed best for representing a given
piece of Dwarf.

We could for example obtain names of the formal parameters selected
above::

	$ 'entry ?TAG_subprogram !AT_declaration child ?TAG_formal_parameter @AT_name'
	f
	argc
	argv

This could be used e.g. to select a particular attribute--we'll see
later how to do this.

In is not an error to request value of attribute that a DIE doesn't
have.  In such case, ``@AT_*`` would simply not yield at all::

	$ dwgrep ./tests/aranges.o -c -e 'entry ?AT_data_member_location'
	0

``@AT_*`` forms could actually also yield more than once.  For example
attributes of locating expression types yield once for every covered
address range::

	$ dwgrep ./tests/aranges.o -c -e 'entry ?AT_location'
	1

	$ dwgrep ./tests/aranges.o -e 'entry @AT_location'
	0x10000..0x10010:[0:reg5]
	0x10010..0x1001a:[0:fbreg<-24>]

There's another use of this same feature: for attributes with
reference form, we get the effect of traversing over the edge rooted
at given attribute.  For example, we could get types of formal
parameters::

	$ 'entry ?TAG_subprogram !AT_declaration child ?TAG_formal_parameter @AT_type'
	[2d]	structure_type
		name (string)	foo;
  		[... more attributes ...]
	[65]	base_type
		byte_size (data1)	4;
		encoding (data1)	DW_ATE_signed;
		name (string)	int;
	[ac]	pointer_type
		byte_size (data1)	8;
		type (ref4)	[b2];

That's quite a bit more useful--we could find out whether the formal
parameters have a structure types::

	$ 'entry ?TAG_subprogram !AT_declaration child ?TAG_formal_parameter
	   @AT_type ?TAG_structure_type'
	[2d]	structure_type
		name (string)	foo;
  		[... more attributes ...]

So that will let us know whether there are any offenders like that.
That's closer to being interesting, but not quite what we need either.
We would like to know about the subprograms themselves, that have this
property!

``?(EXPR)``, ``!(EXPR)`` --- Sub-expressions assertions
-------------------------------------------------------

Some Zwerg expressions are evaluated in what we call a sub-expression
context.  What happens in sub-expression context, stays there--the
stack effects of sub-expression computation never leak back to the
surrounding expression.

``?(EXPR)`` expression is one such case.  It asserts that *EXPR*
produces at least one element.  We can use it to get to DIE's that
have arguments that are structures::

	$ 'entry ?TAG_subprogram !AT_declaration
	   ?(child ?TAG_formal_parameter @AT_type ?TAG_structure_type)'
	[35]	subprogram
		external (flag)	true;
		name (strp)	blah;
		decl_file (data1)	/home/petr/proj/dwgrep/x.c;
		decl_line (data1)	3;
		prototyped (flag)	true;
		type (ref4)	[65];
		low_pc (addr)	0x10000;
		high_pc (addr)	0x1000b;
		frame_base (block1)	0..0xffffffffffffffff:[0:call_frame_cfa];
		sibling (ref4)	[65];

This asks whether, after going to types of children that are formal
parameters, we get a structure.  Because the initial two assertions
have no stack effects anyway, we might say the same thing thus::

	entry ?(?TAG_subprogram !AT_declaration
	        child ?TAG_formal_parameter @AT_type ?TAG_structure_type)'

The other sub-expression assertion, ``!(EXPR)``, holds if *EXPR*
produces no values at all.  E.g. to select child-less DIE's in some
query, we would say::

	some other query !(child)

``EXPR == EXPR``, ``EXPR != EXPR`` --- comparisons
--------------------------------------------------

As you might well know, mere presence of ``DW_AT_declaration``
attribute doesn't tell use whether a DIE is a pure declaration.  We
can probably safely assume that when a compiler produces that
attribute, it will have a value of true (and a form of
``DW_FORM_flag_present``), so most of the time ``?AT_declaration``
(and ``!AT_declaration``) is all you need to write.  But if there are
grounds for suspicion that this is not so, or if we simply want to
shield ourselves from the possibility, we need to actually look at
``DW_AT_declaration``'s value.  So instead of ?AT_declaration, we
should be writing this::

	(@AT_declaration == true)

This intuitively-looking construct actually deserves a closer
attention.  Comparison operators are always evaluated in
sub-expression context.  The mode of operation is that each side is
evaluated separately with the same incoming stack.  Then if the
comparison holds for any pair of produced values, the overall
assertion holds.  Zwerg has a full suite of these operators--``!=``,
``<``, ``<=``, etc.  There's also ``=~`` and ``!~`` for matching
regular expressions.

Importantly, comparisons are assertions.  If they hold, they produce
unchanged incoming stack, otherwise they produce nothing at all.  Thus
expressions such as ``((A > B) == (C > D))`` don't mean what they seem
to.  This one for example is just ``((A > B) (C > D))``--i.e. two
independent conditions.  But consider for example this snippet::

  	((A > B) != (C > D))	# WRONG!

If the two ``>``'s hold, the expression reduces to ``!=``, or
inequality of two nops.  Such assertion thus simply never holds
[#alwaysfail]_.

.. [#alwaysfail] If for whatever reason you actually do need an
   assertion that never holds, a simple one is ``!()``.

Precedence of comparison operators is lower than that of
concatenation, so you can write a couple words on each side of the
operator.  For example, to look for DIE's where one of the location
expression opcodes is ``DW_OP_addr``, you could say::

	entry (@AT_location child label == DW_OP_addr)

Due to this precedence setting, comparisons are typically enclosed in
parens (as in the example), so that they don't force too much of your
computation into sub-expression context.  The precedence is however
above ``,`` and ``||`` that are introduced further, so those need to
be parenthesized further if they should be a comparison operand.

For completeness sake, to check that a flag is false, you would use
the following form::

	!(@AT_declaration == true)

If there's no ``DW_AT_declaration`` at a given DIE, the left hand side
of the inner expression doesn't yield anything, and the outer ``!()``
succeeds--which is what we want, because flag absence is an implicit
false value.  If the attribute is present, then the ``!()``
effectively works as a logical negation.  Contrast this with the
following::

	(@AT_declaration == false)	# WRONG!

You would be probably hard pressed to even find a Dwarf file that
actually encodes false flags like this, so the above is useless.

Back to the problem at hand--besides DW_TAG_structure_type, we care
about ``DW_TAG_class_type`` as well!  We can express "and" easily
simply by juxtaposing the assertions, but we would like a way of
expressing "or" as well.

``EXPR, EXPR`` --- alternation
------------------------------

An expression like ``EXPR₁, EXPR₂, ...`` evaluates all constituent
*EXPRₙ*'s with the same input, and then yields all values that each
*EXPRₙ* yields.  If the expressions are assertions, this happens to
behave exactly like a logical or.  So::

	entry ?TAG_subprogram !AT_declaration
	?(child ?TAG_formal_parameter @AT_type (?TAG_structure_type, ?TAG_class_type))

But the applicability is wider.  Since the semantics are
essentially those of a fork, one can for example ask whether an
attribute has one of a number of values::

	$ 'entry (@AT_name == ("argc", "argv"))'
	[8f]	formal_parameter
		name (strp)	argc;
  		[... more attributes ...]
	[9d]	formal_parameter
		name (strp)	argv;
  		[... more attributes ...]

``EXPR || EXPR`` --- C-style fallback
-------------------------------------

An expression like ``EXPR₁ || EXPR₂ || ...`` works differently.  The
input stack is passed to *EXPR₁* first, and anything that this yields,
is sent to output.  But if nothing is yielded, the same input stack is
passed to *EXPR₂*.  And so on.  It thus yields whatever is yielded by
the first expression that actually yields anything.  It therefore
operates in a manner similar to the operator ``||`` in C language.
The typical use would be in fallbacks.  For example if we prefer
``DW_AT_MIPS_linkage_name`` to ``DW_AT_name``, but can make do with
the latter, that would be encoded as follows::

	entry (@AT_MIPS_linkage_name || @AT_name)

For selecting structures and classes, we can use either of these two
tools interchangeably.

So this is fine, but it still shows only functions that take
structure (or class) arguments directly.  But what if they take a
const argument?  Or if they take a typedef that evaluates to a
structure?  For these cases we need to keep peeling the fluff until
we get to the interesting DIE's.  Enter iterators:

``EXPR*``, ``EXPR+``, ``EXPR?`` --- expression iteration
--------------------------------------------------------

- ``EXPR*`` leaves the working set unchanged, then adds to that the
  result of one application of *EXPR*, then of another, etc.  It works
  similarly to ``*`` in regular expressions.
- ``EXPR+`` is exactly like ``EXPR EXPR*``.
- ``EXPR?`` is exactly like ``(, EXPR)`` --- it /may/ apply once

We can use this tool to remove ``DW_TAG_const_type``,
``DW_TAG_volatile_type`` and ``DW_TAG_typedef`` layers from our
potential structure::

	entry ?TAG_subprogram !AT_declaration
	?(child ?TAG_formal_parameter
	  @AT_type ((?TAG_const_type, ?TAG_volatile_type, ?TAG_typedef) @AT_type)*
	  (?TAG_structure_type, ?TAG_class_type))

Next on, we would like to write a message:

Literals, Strings, Formatting
-----------------------------

Zwerg has roughly C-like string literals, using \ as an escape
character.  Hello world program looks like this in Zwerg::

	"Hello, world!"

This is an example of a string literal.  Literals in Zwerg add
themselves to the stack.  There are many types of literals--apart from
strings and usual numeric literals, dwrgep knows about all the named
Dwarf constants--e.g. ``DW_AT_name``, ``DW_TAG_array_type``,
``DW_FORM_flag``, etc. are all valid forms [#dwgrepzwerg]_.

.. [#dwgrepzwerg] That dwarf constants are recognized is actually not
	a feature of Zwerg per se.  It's the way that ``dwgrep``
	wrapper sets up Zwerg query engine.

Like C printf, string literals in Zwerg allow formatting directives.
To write a nice error message for our running example, we could do for
example this::

	$ 'entry ?TAG_subprogram !AT_declaration
	   ?(child ?TAG_formal_parameter
	     @AT_type ((?TAG_const_type, ?TAG_volatile_type, ?TAG_typedef) @AT_type)*
	     (?TAG_structure_type, ?TAG_class_type))
	   "%s: one of the parameters has non-trivial type."'
	[35] subprogram: one of the parameters has non-trivial type.

It's clear that we'd like to improve on this a bit.  We'd like to
mention which parameter it is, and we'd like to tell the user the name
of the function, not just a DIE offset.  We'll address both--later.
But first, a bit of background.

When dwgrep sees a string with formatting directives, it converts it
into a function.  That function takes one value for each ``%s``,
substitutes the ``%s`` with values of corresponding parameters, and
then pushes the result to stack.  Consequently, to convert anything to
a string in dwgrep, you would just say::

	"%s"

E.g.::

	$ 'entry ?TAG_subprogram "%s"'
	[35] subprogram
	[6c] subprogram

When there are more formatting directives, each of them takes one
value from the stack, in order from left to right::

	$ dwgrep '1 2 "%s %s"'
	2 1

We could already get the desired format string improvements with these
tools in our hands already.  But there's a bit of syntax that will
make our job easier still.

``let X := EXPR;`` --- name binding
-----------------------------------

Often you need to refer back to a value that was computed earlier.
Since this is a stack machine, one way to do this is to use stack
shuffling words--``dup``, ``swap``, ``rot``, ``over`` and ``drop``--to
move stuff around the way you need it.  But keeping track of what is
where when gets old quickly.  For this reason, Zwerg allows that you
give value a name.  Later on, when that name is mentioned, it acts as
a function that pushes the bound value to stack.

In an expression such as ``let X Y := EXPR;``, *EXPR* is evaluated in
a sub-expression context.  Top of stack is bound to name *Y*, and the
value below that to *X*, and so on in this fashion if there are more
names.  E.g.::

	$ dwgrep 'let A := 1;
	          A A add'
	2

	$ dwgrep 'let A B := 10 2;
	          A B div'
	5

Let's use this tool to first remember the two values that we care
about: the subprogram (S) and its naked structure parameter (P)::

	$ dwgrep ./tests/nontrivial-types.o -f /dev/stdin <<EOF
	let S := entry ?TAG_subprogram !AT_declaration;
	let P := S child ?TAG_formal_parameter
	         ?(@AT_type ((?TAG_const_type,
	                      ?TAG_volatile_type, ?TAG_typedef) @AT_type)*
	           (?TAG_structure_type, ?TAG_class_type));
	P S "%s: %s has non-trivial type."
	EOF

	---
	[35] subprogram: [58] formal_parameter has non-trivial type.
	<Dwarf "./tests/nontrivial-types.o">

Which is not too shabby, but having to keep track of which ``%s``
takes which value is perhaps not too comfortable.  For that reason,
Zwerg allows splicing of expressions in strings.

``%( EXPR %)`` --- format string splicing
-----------------------------------------

In format strings, code between ``%(`` and the matching ``%)`` is
evaluated in plain context, after which TOS of the result is popped
and inserted in place of the ``%(...%)``.  ``%s`` is then exactly
equivalent to ``%(%)``.

With this tool, we can make the formatting string clearer::

	$ dwgrep ./tests/nontrivial-types.o -f /dev/stdin <<EOF
	let S := entry ?TAG_subprogram !AT_declaration;
	let P := S child ?TAG_formal_parameter
	         ?(@AT_type ((?TAG_const_type,
	                      ?TAG_volatile_type, ?TAG_typedef) @AT_type)*
	           (?TAG_structure_type, ?TAG_class_type));
	"%( S %): %( P %) has non-trivial type."
	EOF

	---
	[35] subprogram: [58] formal_parameter has non-trivial type.
	<Dwarf "./tests/nontrivial-types.o">

But the actual output is still not very nice.  Ideally we'd mention
names and source code corrdinates instead of Dwarf offsets and tag
names.  But with splicing, that's actually quite easy to achieve::

	$ dwgrep ./tests/nontrivial-types.o -f /dev/stdin <<"EOF"
	let S := entry ?TAG_subprogram !AT_declaration;
	let P := S child ?TAG_formal_parameter
	         ?(@AT_type ((?TAG_const_type,
	                      ?TAG_volatile_type, ?TAG_typedef) @AT_type)*
	           (?TAG_structure_type, ?TAG_class_type));

	"%( S @AT_decl_file %): %( S @AT_decl_line %): note: in function "\
	"`%( S @AT_name %)', parameter `%( P @AT_name %)' type is not trivial."
	EOF

	---
	/home/petr/proj/dwgrep/x.c: 3: note: in function `blah', parameter `f' type is not trivial.
	<Dwarf "./tests/nontrivial-types.o">

The message here is already fairly decent, the only thing making it
ugly is that we actually yield a two-value stack.  We'll deal with
this next.

One thing to note here though is the string continuation syntax.  Note
how the formatting string is split into two fragments.  The former one
then ends with ``"\`` instead of the customary ``"``, which is a
signal to the lexer that it should concatenate the two fragments
together before handing them further.  For all intents and purposes,
these two fragments form a single string literal.

If we are paranoid, we can guard against missing ``@AT_decl_file`` and
``@AT_decl_line``.  This is actually fairly important, because
requesting a missing attribute is not an error, but merely causes the
computation to stop.  If, say, ``@AT_decl_line`` weren't available,
the computation would be silently dropped--right at the point where we
had an offender and were ready to report them.  So let's change the
formatting string thus::

	"%( S @AT_decl_file || "???" %): %( S @AT_decl_line || "???" %): "\
	"note: in function `%( S @AT_name %)', "\
	"parameter `%( P @AT_name %)' type is not trivial."

Note how you can use string literals inside ``%( %)`` inside
formatting strings.  Not that it would be a good idea to nest layers
and layers of strings, but in principle it is possible, and for a
quick default like this, there's no harm.

Now to get rid of the Dwarf value that's occupying our bottom stack
slot.  The simplest approach is to drop the value at the point where
we don't need it anymore::

	$ dwgrep ./tests/nontrivial-types.o -f /dev/stdin <<"EOF"
	let S := entry ?TAG_subprogram !AT_declaration;
	let P := S child ?TAG_formal_parameter
	         ?(@AT_type ((?TAG_const_type,
	                      ?TAG_volatile_type, ?TAG_typedef) @AT_type)*
	           (?TAG_structure_type, ?TAG_class_type));
	drop

	"%( S @AT_decl_file || "???" %): %( S @AT_decl_line || "???" %): "\
	"note: in function `%( S @AT_name %)', "\
	"parameter `%( P @AT_name %)' type is not trivial."
	EOF
	/home/petr/proj/dwgrep/x.c: 3: note: in function `blah', parameter `f' type is not trivial.

But there's one more way to make this work, and it would allow us to
introduce another Zwerg feature.

``(|X Y| EXPR)`` --- scoped bindings
------------------------------------

This expression introduces a function that takes one parameter for
each name mentioned between the pipes, then passes the remaining stack
to *EXPR*, which is evaluated in plain context.  When the bound names
are mentioned within *EXPR*, they recall the bound values.  E.g.::

	$ dwgrep '1 (|A| A A add)'
	2

	$ dwgrep '10 2 (|A B| A B div)'
	5

	$ dwgrep '1 (|A| A A add (|A| A A add))'
	4

If we enclose the whole expression into a scope, we can drop the Dwarf
from the stack where we don't need it, but still keep it around as a
name::

	$ dwgrep ./tests/nontrivial-types.o -f /dev/stdin <<"EOF"
	(|D|
	  let S := D entry ?TAG_subprogram !AT_declaration;
	  let P := S child ?TAG_formal_parameter
	           ?(@AT_type ((?TAG_const_type,
	                        ?TAG_volatile_type, ?TAG_typedef) @AT_type)*
	             (?TAG_structure_type, ?TAG_class_type));

	  "%( S @AT_decl_file || "???" %): %( S @AT_decl_line || "???" %): "\
	  "note: in function `%( S @AT_name %)', "\
	  "parameter `%( P @AT_name %)' type is not trivial."
	)
	EOF
	/home/petr/proj/dwgrep/x.c: 3: note: in function `blah', parameter `f' type is not trivial.

So, that's it.  This was a quick tour through the interesting parts of
``dwgrep``.  You may now want to check out :ref:`syntax`, and
:ref:`zw_vocabulary_core` or :ref:`zw_vocabulary_dwarf` to learn about
the actual function words that you can use.


