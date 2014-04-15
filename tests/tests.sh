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

expect_count 1 ./nontrivial-types.o -e '
	?{offset 0xb8 ?eq} ?{pos 10 ?eq}'

expect_count 1 ./nontrivial-types.o -e '
	?root child ?{offset 0xb8 ?eq} ?{pos 6 ?eq}'

expect_count 1 ./nontrivial-types.o -e '
	?root "%( child offset %)" ?{"0xb8" ?eq} ?{pos 6 ?eq}'

expect_count 1 ./nontrivial-types.o -e '
	?root attribute ?@stmt_list ?{pos 6 ?eq}'

expect_count 11 ./nontrivial-types.o -e '
	+unit ?{2/pos ?eq}'

expect_count 1 ./nontrivial-types.o -e '
	?root drop [10, 11, 12]
	?{each ?{pos 0 ?eq} ?{10 ?eq}}
	?{each ?{pos 1 ?eq} ?{11 ?eq}}
	?{each ?{pos 2 ?eq} ?{12 ?eq}}'
