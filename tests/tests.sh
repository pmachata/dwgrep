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

expect_count 1 ./duplicate-const -e '1 10 ?lt'
expect_count 1 ./duplicate-const -e '10 1 ?gt'

expect_count 1 ./duplicate-const -e '
	winfo dup 2/child ?gt 2/(?const_type, ?volatile_type, ?restrict_type)
	?(2/tag ?eq) ?(2/@type ?eq)'

expect_count 1 ./nontrivial-types.o -e '
	winfo ?subprogram !@declaration -child ?formal_parameter
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
	winfo -unit ?(2/pos ?eq)'

# Test that each annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?root drop [10, 11, 12]
	?(each ?(pos 0 ?eq) ?(10 ?eq))
	?(each ?(pos 1 ?eq) ?(11 ?eq))
	?(each ?(pos 2 ?eq) ?(12 ?eq))'

# Tests star closure whose body ends with stack in a different state
# than it starts in (different slots are taken in the valfile).
expect_count 1 ./typedef.o -e '
	winfo
	?([] swap (@type ?typedef -[()] 2/swap add swap)* drop length 3 ?eq)
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

# Test that -@at expands to (-attribute ?@at value), not to
# (-attribute ?@at -value).
expect_count 1 ./typedef.o -e '
	winfo -@external drop type T_NODE ?eq'

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
	winfo ?(@name "e" ?eq) child tag "%d" "40" ?eq'

# Check iterating over empty compile unit.
expect_count 1 ./empty -e '
	winfo offset'

# Check that -[child offset] doesn't get optimized to something silly.
expect_count 0 ./enum.o -e '
	winfo -[child offset] swap type T_CONST ?eq'

# Check N/-X
expect_count 5 ./typedef.o -e '
	winfo -child 2/-@name 4/type 2/swap ?eq'
