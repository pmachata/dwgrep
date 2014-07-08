#!/bin/sh

expect_count ()
{
    COUNT=$1
    shift
    GOT=$(timeout 10 ../dwgrep -c "$@" 2>/dev/null)
    if [ "$GOT" != "$COUNT" ]; then
	echo "FAIL: dwgrep -c" "$@"
	echo "expected: $COUNT"
	echo "     got: $GOT"
	exit 1
    else
	echo "PASS: dwgrep -c" "$@"
    fi
}

expect_count 1 ./empty -e '1 10 ?lt'
expect_count 1 ./empty -e '10 1 ?gt'

expect_count 1 ./duplicate-const -e '
	winfo dup 2/child ?gt 2/(?const_type, ?volatile_type, ?restrict_type)
	?(2/tag ?eq) ?(2/@type ?eq)'

expect_count 1 ./nontrivial-types.o -e '
	winfo ?subprogram !@declaration dup child ?formal_parameter
	?(@type ((?const_type, ?volatile_type, ?typedef) @type)*
	  (?structure_type, ?class_type))'

# Test that universe annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?(offset 0xb8 ?eq) ?(pos 10 ?eq)'

# Test that child annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?root child ?(offset 0xb8 ?eq) ?(pos 6 ?eq)'

# Test that format annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?root "%( child offset %)" ?("0xb8" ?eq) ?(pos 6 ?eq)'

# Test that attribute annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?root attribute ?@stmt_list ?(pos 6 ?eq)'

# Test that unit annotates position.
expect_count 11 ./nontrivial-types.o -e '
	winfo dup unit ?(2/pos ?eq)'

# Test that elem annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?root drop [10, 11, 12]
	?(elem ?(pos 0 ?eq) ?(10 ?eq))
	?(elem ?(pos 1 ?eq) ?(11 ?eq))
	?(elem ?(pos 2 ?eq) ?(12 ?eq))'

# Tests star closure whose body ends with stack in a different state
# than it starts in (different slots are taken in the valfile).
expect_count 1 ./typedef.o -e '
	winfo
	?([] swap (@type ?typedef [()] 2/swap add swap)* drop length 3 ?eq)
	?(offset 0x45 ?eq)'

# Test decoding signed and unsigned value.
expect_count 1 ./enum.o -e '
	winfo ?(@name "f" ?eq) child ?(@name "V" ?eq)
	?(@const_value "%s" "-1" ?eq)'
expect_count 1 ./enum.o -e '
	winfo ?(@name "e" ?eq) child ?(@name "V" ?eq)
	?(@const_value "%s" "4294967295" ?eq)'

# Test match operator
expect_count 7 ./duplicate-const -e '
	winfo ?(@decl_file "" ?match)'
expect_count 7 ./duplicate-const -e '
	winfo ?(@decl_file ".*petr.*" ?match)'
expect_count 1 ./nontrivial-types.o -e '
	winfo ?(@language "%s" "DW_LANG_C89" ?match)'
expect_count 1 ./nontrivial-types.o -e '
	winfo ?(@encoding "%s" "^DW_ATE_signed$" ?match)'

# Test true/false
expect_count 1 ./typedef.o -e '
	winfo ?(@external true ?eq)'
expect_count 1 ./typedef.o -e '
	winfo ?(@external false !eq)'

# Test that (dup parent) doesn't change the bottom DIE as well.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?structure_type dup parent ?(swap offset 0x2d ?eq)'

# Check that when promoting assertions close to producers of their
# slots, we don't move across alternation or closure.
expect_count 3 ./nontrivial-types.o -e '
	winfo ?subprogram child* ?formal_parameter'
expect_count 3 ./nontrivial-types.o -e '
	winfo ?subprogram child? ?formal_parameter'
expect_count 3 ./nontrivial-types.o -e '
	winfo ?subprogram (child,) ?formal_parameter'

# Check casting.
expect_count 1 ./enum.o -e '
	winfo ?(@name "e" ?eq) child @const_value "%x" "0xffffffff" ?eq'
expect_count 1 ./enum.o -e '
	winfo ?(@name "e" ?eq) child @const_value hex "%s" "0xffffffff" ?eq'
expect_count 1 ./enum.o -e '
	winfo ?(@name "e" ?eq) child @const_value "%o" "037777777777" ?eq'
expect_count 1 ./enum.o -e '
	winfo ?(@name "e" ?eq) child @const_value oct "%s" "037777777777" ?eq'
expect_count 1 ./enum.o -e '
	winfo ?(@name "e" ?eq) child @const_value
	"%b" "0b11111111111111111111111111111111" ?eq'
expect_count 1 ./enum.o -e '
	winfo ?(@name "e" ?eq) child @const_value bin
	"%s" "0b11111111111111111111111111111111" ?eq'
expect_count 1 ./enum.o -e '
	winfo ?(@name "e" ?eq) child tag "%d" "40" ?eq'

# Check decoding of huge literals.
expect_count 1 ./empty -e '
	[0xffffffffffffffff "%s" elem !(pos (0,1) ?eq)]
	?(length 16 ?eq) !(elem "f" !eq)'
expect_count 1 ./empty -e '
	18446744073709551615 0xffffffffffffffff ?eq'
expect_count 1 ./empty -e '
	01777777777777777777777 0xffffffffffffffff ?eq'
expect_count 1 ./empty -e '
	0b1111111111111111111111111111111111111111111111111111111111111111
	0xffffffffffffffff ?eq'
expect_count 1 ./empty -e'
	-0xff dup dup dup "%s %d %b %o" "-0xff -255 -0b11111111 -0377" ?eq'
expect_count 1 ./empty -e'
	-0377 dup dup dup "%x %d %b %s" "-0xff -255 -0b11111111 -0377" ?eq'
expect_count 1 ./empty -e'
	-255 dup dup dup "%x %s %b %o" "-0xff -255 -0b11111111 -0377" ?eq'
expect_count 1 ./empty -e'
	-0b11111111 dup dup dup "%x %d %s %o"
	"-0xff -255 -0b11111111 -0377" ?eq'

# Check arithmetic.
expect_count 1 ./empty -e '-1 1 add ?(0 ?eq)'
expect_count 1 ./empty -e '-1 10 add ?(9 ?eq)'
expect_count 1 ./empty -e '1 -10 add ?(-9 ?eq)'
expect_count 1 ./empty -e '-10 1 add ?(-9 ?eq)'
expect_count 1 ./empty -e '10 -1 add ?(9 ?eq)'

expect_count 1 ./empty -e '
	-1 0xffffffffffffffff add "%s" "0xfffffffffffffffe" ?eq'
expect_count 1 ./empty -e '
	0xffffffffffffffff -1 add "%s" "0xfffffffffffffffe" ?eq'

expect_count 1 ./empty -e '-1 1 sub ?(-2 ?eq)'
expect_count 1 ./empty -e '-1 10 sub ?(-11 ?eq)'
expect_count 1 ./empty -e '1 -10 sub ?(11 ?eq)'
expect_count 1 ./empty -e '-10 1 sub ?(-11 ?eq)'
expect_count 1 ./empty -e '10 -1 sub ?(11 ?eq)'

expect_count 1 ./empty -e '-2 2 mul ?(-4 ?eq)'
expect_count 1 ./empty -e '-2 10 mul ?(-20 ?eq)'
expect_count 1 ./empty -e '2 -10 mul ?(-20 ?eq)'
expect_count 1 ./empty -e '-10 2 mul ?(-20 ?eq)'
expect_count 1 ./empty -e '10 -2 mul ?(-20 ?eq)'
expect_count 1 ./empty -e '-10 -2 mul ?(20 ?eq)'
expect_count 1 ./empty -e '-2 -10 mul ?(20 ?eq)'

# Check iterating over empty compile unit.
expect_count 1 ./empty -e '
	winfo offset'

# Check N/(dup X)
expect_count 5 ./typedef.o -e '
	winfo dup child 2/(dup @name) 4/type 2/swap ?eq'

# Check ||.
expect_count 6 ./typedef.o -e '
	winfo (@decl_line || drop 42)'
expect_count 2 ./typedef.o -e '
	winfo (@decl_line || drop 42) ?(42 ?eq)'
expect_count 6 ./typedef.o -e '
	winfo (@decl_line || @byte_size || drop 42)'
expect_count 1 ./typedef.o -e '
	winfo (@decl_line || @byte_size || drop 42) ?(42 ?eq)'

# Check closures.
expect_count 1 ./empty -e '
	{->A; {->B; A}} 2 swap apply 9 swap apply ?(1 1 add ?eq)'
expect_count 1 ./empty -e '
	{->A; {A}} 5 swap apply apply ?(2 3 add ?eq)'
expect_count 1 ./empty -e '
	{dup add} -> double; 1 double ?(2 ?eq)'
expect_count 1 ./empty -e '
	{->x; {->y; x y add}} ->adder;
	3 adder 2 swap apply ?(5 ?eq)'
expect_count 1 ./empty -e '
	{->L f; [ L elem f ] } ->map;
	[1, 2, 3] {1 add} map ?([2, 3, 4] ?eq)'

# Check recursion.
expect_count 1 ./empty -e '
	{->A; (?(A 10 ?ge) 0 || A 1 add F 1 add)} ->F;
	0 F
	?(10 ?eq)'

expect_count 1 ./empty -e '
	{->F T; (?(F T ?le) F, ?(F T ?lt) F 1 add T seq) } -> seq;
	[1 10 seq] ?([1, 2, 3, 4, 5, 6, 7, 8, 9, 10] ?eq)'

expect_count 1 ./empty -e '
	{->N; (?(N 2 ?lt) 1 || N 1 sub fact N mul)} -> fact;
	?(5 fact 120 ?eq)
	?(6 fact 720 ?eq)
	?(7 fact 5040 ?eq)
	?(8 fact 40320 ?eq)'

# Check ifelse.
expect_count 1 ./empty -e '
	[if ?(1) then (2,3) else (4,5)] ?([2,3] ?eq)'
expect_count 1 ./empty -e '
	[if !(1) then (2,3) else (4,5)] ?([4,5] ?eq)'
expect_count 6 ./typedef.o -e '
	[winfo] ?(length 6 ?eq)
	elem if child then 1 else 0 "%s %(offset%)"
	?(("1 0xb","0 0x1d","0 0x28","0 0x2f","0 0x3a","0 0x45") ?eq)'
