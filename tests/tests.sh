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
