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
