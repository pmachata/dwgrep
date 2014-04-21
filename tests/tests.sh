#!/bin/sh

expect_count ()
{
    COUNT=$1
    shift
    GOT=$(../dwgrep -c "$@" 2>/dev/null)
    if [ "$GOT" != "$COUNT" ]; then
	echo "FAIL: dwgrep -c" "$@"
	echo "expected: $COUNT"
	echo "     got: $GOT"
	exit 1
    else
	echo "PASS: dwgrep -c" "$@"
    fi
}

expect_count 1 ./duplicate-const -e '
	dup 2/child ?gt 2/(?const_type, ?volatile_type, ?restrict_type)
	?{2/tag ?eq} ?{2/@type ?eq}'

expect_count 1 ./nontrivial-types.o -e '
	?subprogram !@declaration +child ?formal_parameter
	?{@type ((?const_type, ?volatile_type, ?typedef) @type)*
	  (?structure_type, ?class_type)}'

# Test that universe annotates position.
expect_count 1 ./nontrivial-types.o -e '
	?{offset 0xb8 ?eq} ?{pos 10 ?eq}'

# Test that child annotates position.
expect_count 1 ./nontrivial-types.o -e '
	?root child ?{offset 0xb8 ?eq} ?{pos 6 ?eq}'

# Test that format annotates position.
expect_count 1 ./nontrivial-types.o -e '
	?root "%( child offset %)" ?{"0xb8" ?eq} ?{pos 6 ?eq}'

# Test that attribute annotates position.
expect_count 1 ./nontrivial-types.o -e '
	?root attribute ?@stmt_list ?{pos 6 ?eq}'

# Test that unit annotates position.
expect_count 11 ./nontrivial-types.o -e '
	+unit ?{2/pos ?eq}'

# Test that each annotates position.
expect_count 1 ./nontrivial-types.o -e '
	?root drop [10, 11, 12]
	?{each ?{pos 0 ?eq} ?{10 ?eq}}
	?{each ?{pos 1 ?eq} ?{11 ?eq}}
	?{each ?{pos 2 ?eq} ?{12 ?eq}}'

# Tests star closure whose body ends with stack in a different state
# than it starts in (different slots are taken in the valfile).
expect_count 1 ./typedef.o -e '
	?{[] swap (@type ?typedef [()] 2/swap add swap)* drop length 3 ?eq}
	?{offset 0x45 ?eq}'

# Test count of universe.
expect_count 1 ./typedef.o -e '
	?root ?{count 6 ?eq}'

# Test count of child.
expect_count 1 ./typedef.o -e '
	?root child ?base_type ?{count 5 ?eq}'

# Test count of count.
expect_count 1 ./typedef.o -e '
	?root count count ?{1 ?eq}'

# Test closure.  We expect 6 nodes (for count unapplied), 6x count of
# 6 (for count applied to each node) and 6x count of 1 (for count
# applied to count).
expect_count 18 ./typedef.o -e '
	count*'
expect_count 18 ./typedef.o -e '
	count* (?{(1, 6) ?eq}, ?{type T_NODE ?eq})'

# Test count in format strings.
expect_count 6 ./typedef.o -e '
	"%(count%)" ?{"6" ?eq}'
expect_count 6 ./typedef.o -e '
	"%(count%)" count 1 ?eq'
expect_count 5 ./typedef.o -e '
	"%( +child count %)--%( count %)" ?{"5--6" ?eq}'
expect_count 5 ./typedef.o -e '
	"%( +child count %)--%( count %)" count ?{5 ?eq}'

# Test decoding signed and unsigned value.
expect_count 1 ./enum.o -e '
	?{@name "f" ?eq} child ?{@name "V" ?eq}
	?{@const_value "%s" "-1" ?eq}'
expect_count 1 ./enum.o -e '
	?{@name "e" ?eq} child ?{@name "V" ?eq}
	?{@const_value "%s" "4294967295" ?eq}'

# Test match operator
expect_count 7 ./duplicate-const -e '
	?{@decl_file "" ?match}'
expect_count 7 ./duplicate-const -e '
	?{@decl_file ".*petr.*" ?match}'
expect_count 1 ./nontrivial-types.o -e '
	?{@language "%s" "DW_LANG_C89" ?match}'
expect_count 1 ./nontrivial-types.o -e '
	?{@encoding "%s" "^DW_ATE_signed$" ?match}'

# Test true/false
expect_count 1 ./typedef.o -e '
	?{@external true ?eq}'
expect_count 1 ./typedef.o -e '
	?{@external false !eq}'

# Test that +@at expands to (+attribute ?@at value), not to
# (+attribute ?@at +value).
expect_count 1 ./typedef.o -e '
	+@external drop type T_NODE ?eq'

# Test that (dup parent) doesn't change the bottom DIE as well.
expect_count 1 ./nontrivial-types.o -e '
	?structure_type dup parent ?{swap offset 0x2d ?eq}'
