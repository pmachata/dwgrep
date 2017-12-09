Assorted examples
=================

Type mismatch in ``DW_AT_specification``
----------------------------------------

Find type DIE's that are inconsistent between instance and
specification.  This is `GCC bug 43053`__::

	let A := entry ?TAG_subprogram;
	let B := A child ?TAG_formal_parameter;
	let C := A @AT_specification child ?TAG_formal_parameter;
	(B pos == C pos) (B @AT_type != C @AT_type)

*A* holds the offending subprogram, and *B* and *C* are the two
mismatched types.

.. __: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43053


Duplicate ``DW_AT_const_type``
------------------------------

Find two distinct cv-modifier DIE's that both qualify the same
underlying type the same way.  This is `GCC bug 56740`__::

   let A := entry (?TAG_const_type || ?TAG_volatile_type || ?TAG_restrict_type);
   let B := A root child* (> A) (label == A label) (@AT_type == A @AT_type);

*A* and *B* are the two duplicate DIE's.

.. __: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=56740
