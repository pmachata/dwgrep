#!/bin/sh

expect_count ()
{
    COUNT=$1
    shift
    GOT=$(../dwgrep -c "$@" 2>/dev/null)
    if [ "$GOT" != "$COUNT" ]; then
	echo "test: dwgrep -c" "$@"
	echo "EXPECTED: $COUNT"
	echo "     GOT: $GOT"
	exit 1
    fi
}

expect_count 1 ./duplicate-parameters -e '
	dup 2/child ?gt 2/(?const_type, ?volatile_type, ?restrict_type)
	?{2/tag ?eq} ?{2/@type ?eq}'
