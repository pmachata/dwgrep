#!/bin/sh

DWGREP=$1
cd $(dirname $0)

failures=0
total=0

if [ -z "$ZW_TEST_TIMEOUT" ]; then
    case "$DWGREP" in
	valgrind*) ZW_TEST_TIMEOUT=100;;
	*) ZW_TEST_TIMEOUT=1;;
    esac
fi

fail ()
{
    echo "FAIL: $@" >&2
    export failures=$((failures + 1))
}

trap 'fail "ERR trap triggered"' ERR

expect_count ()
{
    export total=$((total + 1))
    COUNT=$1
    shift
    GOT=$(timeout $ZW_TEST_TIMEOUT $DWGREP -c "$@" 2>&1)
    if [ "$GOT" != "$COUNT" ]; then
	fail "$DWGREP -c" "$@"
	echo "expected: $COUNT" >&2
	echo "     got: $GOT" >&2
    fi
}

expect_error ()
{
    export total=$((total + 1))
    ERR=$1
    shift
    GOT=$(timeout $ZW_TEST_TIMEOUT $DWGREP "$@" 2>&1 >/dev/null)
    if [ "${GOT/${ERR}//}" == "$GOT" ]; then
	fail "$DWGREP" "$@"
	echo "expected error: $ERR" >&2
	echo "           got: $GOT" >&2
    fi
}

expect_out ()
{
    export total=$((total + 1))
    OUT=$1
    shift
    TMP=$(mktemp)
    GOT=$(timeout $ZW_TEST_TIMEOUT $DWGREP "$@" 2>$TMP)
    if [ -s $TMP ]; then
	fail "$DWGREP" "$@"
	echo "expected no error" >&2
	echo -n "    got: " >&2
	cat $TMP >&2
    fi
    if [ "$OUT" != "$GOT" ]; then
	fail "$DWGREP" "$@"
	echo "expected output: $OUT" >&2
	echo "            got: $GOT" >&2
    fi
    rm -f $TMP
}

expect_count 1 -e '1   10 ?lt'
expect_count 1 -e '10  10 !lt'
expect_count 1 -e '100 10 !lt'
expect_count 1 -e '1   10 ?le'
expect_count 1 -e '10  10 ?le'
expect_count 1 -e '100 10 !le'
expect_count 1 -e '1   10 !eq'
expect_count 1 -e '10  10 ?eq'
expect_count 1 -e '100 10 !eq'
expect_count 1 -e '1   10 ?ne'
expect_count 1 -e '10  10 !ne'
expect_count 1 -e '100 10 ?ne'
expect_count 1 -e '1   10 !ge'
expect_count 1 -e '10  10 ?ge'
expect_count 1 -e '100 10 ?ge'
expect_count 1 -e '1   10 !gt'
expect_count 1 -e '10  10 !gt'
expect_count 1 -e '100 10 ?gt'

expect_count 0 -e '1   10 !lt'
expect_count 0 -e '10  10 ?lt'
expect_count 0 -e '100 10 ?lt'
expect_count 0 -e '1   10 !le'
expect_count 0 -e '10  10 !le'
expect_count 0 -e '100 10 ?le'
expect_count 0 -e '1   10 ?eq'
expect_count 0 -e '10  10 !eq'
expect_count 0 -e '100 10 ?eq'
expect_count 0 -e '1   10 !ne'
expect_count 0 -e '10  10 ?ne'
expect_count 0 -e '100 10 !ne'
expect_count 0 -e '1   10 ?ge'
expect_count 0 -e '10  10 !ge'
expect_count 0 -e '100 10 !ge'
expect_count 0 -e '1   10 ?gt'
expect_count 0 -e '10  10 ?gt'
expect_count 0 -e '100 10 !gt'


expect_count 1 -e '1   != 10'
expect_count 1 -e '10  == 10'
expect_count 1 -e '100 != 10'
expect_count 1 -e '1   <  10'
expect_count 1 -e '10  <= 10'
expect_count 1 -e '10  >= 10'
expect_count 1 -e '100 >  10'

expect_count 0 -e '1   == 10'
expect_count 0 -e '10  != 10'
expect_count 0 -e '100 == 10'
expect_count 0 -e '1   >  10'
expect_count 0 -e '10  < 10'
expect_count 0 -e '10  > 10'
expect_count 0 -e '100 < 10'

# Test comments.
expect_count 1 -e '
	1 #blah blah -45720352304573257230453045302 {}[*[{)+*]&+*
	2 # sub mul div mod
	add == 3'
expect_count 1 -e '
	1 //blah blah -45720352304573257230453045302 {}[*[{)+*]&+*
	2 // sub mul div mod
	add == 3'
expect_count 1 -e '
	1 /*blah blah -45720352304573257230453045302 {}[*[{)+*]&+***/ 2 /*
	sub mul div mod*/ add
	== 3'

# Test let.
expect_count 1 -e 'let A := 1; A == 1'
expect_count 1 -e 'let A := 1; let B := A; B == 1'
expect_count 1 -e 'let A := 1, 2; A == 1'
expect_count 1 -e 'let A := 1, 2; A == 2'
expect_count 0 -e 'let A := 1; A == 2'
expect_count 1 -e 'let A B := 1 2; (A == 1) (B == 2)'
expect_count 1 -e 'let A B := (1, 2) (3, 4); (A == 1) (B == 3)'
expect_count 1 -e 'let A B := (1, 2) (3, 4); (A == 2) (B == 3)'
expect_count 1 -e 'let A B := (1, 2) (3, 4); (A == 1) (B == 4)'
expect_count 1 -e 'let A B := (1, 2) (3, 4); (A == 2) (B == 4)'
expect_count 2 -e 'let A := 1; (A, A) 1 ?eq'

# Test that universe annotates position.
expect_count 1 ./nontrivial-types.o -e '
	entry (offset == 0xb8) (pos == 10)'

# Test that child annotates position.
expect_count 1 ./nontrivial-types.o -e '
	entry ?root child (offset == 0xb8) (pos == 6)'

# Test that format annotates position.
expect_count 1 ./nontrivial-types.o -e '
	entry ?root "%( child offset %)" (== "0xb8") (pos == 6)'

# Test that attribute annotates position.
expect_count 1 ./nontrivial-types.o -e '
	entry ?root attribute ?AT_stmt_list (pos == 6)'

# Test that entry::T_CU annotates position.
expect_count 11 ./nontrivial-types.o -e '
       	let A := entry; let B := A unit entry; (A pos == B pos)'

# Test a bug with scope promotion.
expect_count 1 -e '
	let A:=1; ?(let X:=A;)'
expect_count 1 -e '
	0 [|B| 1 [|V| B]] ?([[0]] ?eq)'
expect_count 1 -e '
	1 {|A| 3 [|B| A]} apply ?([1] ?eq)'

# Test that elem annotates position.
expect_count 1 ./nontrivial-types.o -e '
	entry ?root drop [10, 11, 12]
	?(elem (pos == 0) (== 10))
	?(elem (pos == 1) (== 11))
	?(elem (pos == 2) (== 12))'
expect_count 3 -e '
	[0, 1, 2] elem dup (== pos)'
expect_count 3 -e '
	[2, 1, 0] relem dup (== pos)'
expect_count 1 -e '
	["210" relem] == ["012" elem]'

# Check literal assertions.
expect_count 1 -e '
	?([1, 3, 5] elem (pos == 0) (== 1))
	?([1, 3, 5] elem (pos == 1) (== 3))
	?([1, 3, 5] elem (pos == 2) (== 5))'
expect_count 1 -e '
	?([[1, 3, 5] elem (pos != 0) ((3,5) ==)] (length == 2))
	?([[1, 3, 5] elem (pos != 1) ((1,5) ==)] (length == 2))
	?([[1, 3, 5] elem (pos != 2) ((1,3) ==)] (length == 2))'

# Tests star closure whose body ends with stack in a different state
# than it starts in (different slots are taken in the valfile).
expect_count 1 ./typedef.o -e '
	entry
	?([] swap (@AT_type ?TAG_typedef [()] rot add swap)* drop length 3 ?eq)
	?(offset 0x45 ?eq)'

# Test decoding signed and unsigned value.
expect_count 1 ./enum.o -e '
	entry (@AT_name == "f") child (@AT_name == "V")
	(@AT_const_value "%s" == "-1")'
expect_count 1 ./enum.o -e '
	entry (@AT_name == "e") child (@AT_name == "V")
	(@AT_const_value "%s" == "4294967295")'
expect_count 2 ./char_16_32.o -e '
	entry (@AT_name == "bar") (@AT_const_value == 0xe1)'
expect_count 1 ./nullptr.o -e '
	entry ?(@AT_type ?TAG_unspecified_type) (@AT_const_value == 0)'
expect_count 1 ./testfile_const_type -e '
	entry @AT_location elem ?OP_GNU_const_type value
	((pos == 0) (type == T_DIE) || (pos == 1) (type == T_SEQ))'

# Test match operator
expect_count 7 ./duplicate-const -e '
	entry ?(@AT_decl_file "" ?match)'
expect_count 7 ./duplicate-const -e '
	entry ?(@AT_decl_file ".*petr.*" ?match)'
expect_count 1 ./nontrivial-types.o -e '
	entry ?(@AT_language "%s" "DW_LANG_C89" ?match)'
expect_count 1 ./nontrivial-types.o -e '
	entry ?(@AT_encoding "%s" "^DW_ATE_signed$" ?match)'
expect_count 7 ./duplicate-const -e '
	entry (@AT_decl_file =~ "")'
expect_count 7 ./duplicate-const -e '
	entry (@AT_decl_file =~ ".*petr.*")'
expect_count 7 ./duplicate-const -e '
	entry (@AT_decl_file !~ ".*pavel.*")'

# Test true/false
expect_count 1 ./typedef.o -e '
	entry ?(@AT_external == true)'
expect_count 1 ./typedef.o -e '
	entry ?(@AT_external != false)'

# Test that (dup parent) doesn't change the bottom DIE as well.
expect_count 1 ./nontrivial-types.o -e '
	entry ?TAG_structure_type dup parent ?(swap offset == 0x2d)'

# Check that when promoting assertions close to producers of their
# slots, we don't move across alternation or closure.
expect_count 3 ./nontrivial-types.o -e '
	entry ?TAG_subprogram child* ?TAG_formal_parameter'
expect_count 3 ./nontrivial-types.o -e '
	entry ?TAG_subprogram child? ?TAG_formal_parameter'
expect_count 3 ./nontrivial-types.o -e '
	entry ?TAG_subprogram (child,) ?TAG_formal_parameter'

# Check casting.
expect_count 1 ./enum.o -e '
	entry (@AT_name == "e") child
	@AT_const_value "%x" "0xffffffff" ?eq'
expect_count 1 ./enum.o -e '
	entry (@AT_name == "e") child
	@AT_const_value hex "%s" "0xffffffff" ?eq'
expect_count 1 ./enum.o -e '
	entry (@AT_name == "e") child
	@AT_const_value "%o" "037777777777" ?eq'
expect_count 1 ./enum.o -e '
	entry (@AT_name == "e") child
	@AT_const_value oct "%s" "037777777777" ?eq'
expect_count 1 ./enum.o -e '
	entry (@AT_name == "e") child @AT_const_value
	"%b" "0b11111111111111111111111111111111" ?eq'
expect_count 1 ./enum.o -e '
	entry (@AT_name == "e") child @AT_const_value bin
	"%s" "0b11111111111111111111111111111111" ?eq'
expect_count 1 ./enum.o -e '
	entry (@AT_name == "e") child label "%d" "40" ?eq'

# Check decoding of huge literals.
expect_count 1 -e '
	[0xffffffffffffffff "%s" elem !(pos == (0,1))]
	(length == 16) !(elem != "f")'
expect_count 1 -e '
	18446744073709551615 == 0xffffffffffffffff'
expect_count 1 -e '
	01777777777777777777777 == 0xffffffffffffffff'
expect_count 1 -e '
	0b1111111111111111111111111111111111111111111111111111111111111111
	== 0xffffffffffffffff'
expect_count 1 -e'
	-0xff dup dup dup "%s %d %b %o" == "-0xff -255 -0b11111111 -0377"'
expect_count 1 -e'
	-0377 dup dup dup "%x %d %b %s" == "-0xff -255 -0b11111111 -0377"'
expect_count 1 -e'
	-255 dup dup dup "%x %s %b %o" == "-0xff -255 -0b11111111 -0377"'
expect_count 1 -e'
	-0b11111111 dup dup dup "%x %d %s %o"
	== "-0xff -255 -0b11111111 -0377"'

# Check arithmetic.
expect_count 1 -e '-1 1 add (== 0)'
expect_count 1 -e '-1 10 add (== 9)'
expect_count 1 -e '1 -10 add (== -9)'
expect_count 1 -e '-10 1 add (== -9)'
expect_count 1 -e '10 -1 add (== 9)'

expect_count 1 -e '
	-1 0xffffffffffffffff add "%s" == "0xfffffffffffffffe"'
expect_count 1 -e '
	0xffffffffffffffff -1 add "%s" == "0xfffffffffffffffe"'

expect_count 1 -e '-1 1 sub (== -2)'
expect_count 1 -e '-1 10 sub (== -11)'
expect_count 1 -e '1 -10 sub (== 11)'
expect_count 1 -e '-10 1 sub (== -11)'
expect_count 1 -e '10 -1 sub (== 11)'

expect_count 1 -e '-2 2 mul (== -4)'
expect_count 1 -e '-2 10 mul (== -20)'
expect_count 1 -e '2 -10 mul (== -20)'
expect_count 1 -e '-10 2 mul (== -20)'
expect_count 1 -e '10 -2 mul (== -20)'
expect_count 1 -e '-10 -2 mul (== 20)'
expect_count 1 -e '-2 -10 mul (== 20)'

# Check iterating over empty compile unit.
expect_count 1 ./empty -e '
	entry offset'

# Check ||.
expect_count 6 ./typedef.o -e '
	entry (@AT_decl_line || drop 42)'
expect_count 2 ./typedef.o -e '
	entry (@AT_decl_line || drop 42) (== 42)'
expect_count 6 ./typedef.o -e '
	entry (@AT_decl_line || @AT_byte_size || drop 42)'
expect_count 1 ./typedef.o -e '
	entry (@AT_decl_line || @AT_byte_size || drop 42) (== 42)'
expect_count 1 -e '
	(0, 1, 20) (== 10 || == 20)'

# Check closures.
expect_count 2 -e '
	{let A := 1, 2; A} apply'
expect_count 1 -e '
	[[let A := 1, 2; {A}]
         (|B| B elem ?0 B elem ?1) swap? drop apply] == [2, 1]'
expect_count 1 -e '
	{|A| {|B| A}} 2 swap apply 9 swap apply (== 1 1 add)'
expect_count 1 -e '
	{|A| {A}} 5 swap apply apply (== 2 3 add)'
expect_count 1 -e '
	let double := {dup add}; (1 double == 2)'
expect_count 1 -e '
	let adder := {|x| {|y| x y add}};
	3 adder 2 swap apply (== 5)'
expect_count 1 -e '
	let map := {|f| [|L| L elem f]};
	[1, 2, 3] {1 add} map ?([2, 3, 4] ?eq)'
expect_count 1 -e '
	let slice := {|L B E|
	  let wrap := {|V|
	    let X := if (V 0 ?ge) then (V) else (L length V add);
	    if (X 0 ?lt) then 0 else X
	  };
	  let begin end := B wrap E wrap;
	  [L elem ?(pos ?(begin ?ge) ?(end ?lt))]
	};
	[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
	?(1 5 slice [1, 2, 3, 4] ?eq)
	?(5 -1 slice [5, 6, 7, 8] ?eq)
	?(-2 -1 slice [8] ?eq)'
expect_count 1 -e '
	1 2 3 [|A B C| A, B, C] == [1, 2, 3]'
expect_count 1 -e '
	1 2 3 {|A B C| [A, B, C]} apply == [1, 2, 3]'
expect_count 1 -e '
	1 2 3 {|A B C| {[A, B, C]}} apply apply == [1, 2, 3]'
expect_count 1 -e '
	1 2 3 {|A B C| {{[A, B, C]}}} apply apply apply == [1, 2, 3]'
expect_count 1 -e '
	1 2 3 {|A B C| {{[C, B, A]}}} apply apply apply == [3, 2, 1]'

# Check that bindings remember position.
expect_count 3 -e '
	let E := [0, 1, 2] elem; E (== pos)'
expect_count 3 -e '
	{{let E := [0, 1, 2] elem; {E}}} apply apply apply (== pos)'

# Check recursion.
expect_count 1 -e '
	let .F := {|A F| (?(A 10 ?ge) 0 || A 1 add {F} F 1 add)};
	let F := {{.F} .F};
	0 F
	?(10 ?eq)'

expect_count 1 -e '
	let .seq := {|F T seq| ?(F T ?le) F, ?(F T ?lt) F 1 add T {seq} seq};
	let seq := {{.seq} .seq};
	[1 10 seq] ?([1, 2, 3, 4, 5, 6, 7, 8, 9, 10] ?eq)'

expect_count 1 -e '
	let .fact := {|N fact| (?(N 2 ?lt) 1 || N 1 sub {fact} fact N mul)};
	let fact := {{.fact} .fact};
	?(5 fact 120 ?eq)
	?(6 fact 720 ?eq)
	?(7 fact 5040 ?eq)
	?(8 fact 40320 ?eq)'

# Examples.
expect_count 1 ./duplicate-const -e '
	let ?cvr_type := {?TAG_const_type,?TAG_volatile_type,?TAG_restrict_type};
	let P := entry ;
	let A := P child ?cvr_type ;
	let B := P child ?cvr_type ?(?lt: A) ;
	?((A label) ?eq: (B label))
	?((A @AT_type) ?eq: (B @AT_type))'

expect_count 1 ./duplicate-const -e '
	let A:=entry (?TAG_const_type||?TAG_volatile_type||?TAG_restrict_type);
	A root child* (> A) (label == A label) (@AT_type == A @AT_type) A'
expect_count 1 ./duplicate-const -e '
	let A:=entry (?TAG_const_type||?TAG_volatile_type||?TAG_restrict_type);
	A root child+ (> A) (label == A label) (@AT_type == A @AT_type) A'

expect_count 1 ./nontrivial-types.o -e '
	entry ?TAG_subprogram !AT_declaration dup child ?TAG_formal_parameter
	?(@AT_type ((?TAG_const_type,?TAG_volatile_type,?TAG_typedef) @AT_type)*
	  (?TAG_structure_type, ?TAG_class_type))'

# Check ifelse.
expect_count 1 -e '
	[if ?(1) then (2,3) else (4,5)] ?([2,3] ?eq)'
expect_count 1 -e '
	[if !(1) then (2,3) else (4,5)] ?([4,5] ?eq)'
expect_count 6 ./typedef.o -e '
	[entry] ?(length 6 ?eq)
	elem if child then 1 else 0 "%s %(swap offset%)"
	?(("1 0xb","0 0x1d","0 0x28","0 0x2f","0 0x3a","0 0x45") ?eq)'

# Check various Dwarf operators.
expect_count 1 -e '
	[[DW_AT_name, DW_TAG_const_type, DW_FORM_ref_sig8, DW_LANG_Go,
	  DW_INL_inlined, DW_ATE_UTF, DW_ACCESS_private, DW_VIS_exported,
	  DW_ID_case_insensitive, DW_VIRTUALITY_virtual, DW_CC_nocall,
	  DW_ORD_col_major, DW_DSC_range, DW_OP_bra, DW_DS_trailing_separate,
	  DW_ADDR_none, DW_END_little, DW_MACINFO_start_file,
	  DW_MACRO_GNU_undef_indirect] elem hex]
	[0x3, 0x26, 0x20, 0x16, 0x1, 0x10, 0x3, 0x2, 0x3, 0x1, 0x3, 0x1, 0x1,
	 0x28, 0x5, 0x0, 0x2, 0x3, 0x6] ?eq'

expect_count 1 -e '
	let Dw := "duplicate-const" dwopen;

	"%([Dw entry label]%)"
	"[DW_TAG_compile_unit, DW_TAG_subprogram, DW_TAG_variable,"\
	" DW_TAG_variable, DW_TAG_variable, DW_TAG_variable,"\
	" DW_TAG_base_type, DW_TAG_pointer_type, DW_TAG_const_type,"\
	" DW_TAG_base_type, DW_TAG_array_type, DW_TAG_subrange_type,"\
	" DW_TAG_base_type, DW_TAG_const_type, DW_TAG_variable,"\
	" DW_TAG_variable, DW_TAG_const_type]" ?eq
	"%([Dw entry label]%)" ?eq'

expect_count 1 ./duplicate-const -e '
	"%([entry ?root attribute label]%)"
	"[DW_AT_producer, DW_AT_language, DW_AT_name, DW_AT_comp_dir, "\
	"DW_AT_low_pc, DW_AT_high_pc, DW_AT_stmt_list]" ?eq'

expect_count 1 ./duplicate-const -e '
	"%([entry ?root attribute label]%)"
	"[DW_AT_producer, DW_AT_language, DW_AT_name, DW_AT_comp_dir, "\
	"DW_AT_low_pc, DW_AT_high_pc, DW_AT_stmt_list]" ?eq'

expect_count 1 ./duplicate-const -e '
	"%([entry ?root attribute form]%)"
	"[DW_FORM_strp, DW_FORM_data1, DW_FORM_strp, DW_FORM_strp, "\
	"DW_FORM_addr, DW_FORM_data8, DW_FORM_sec_offset]" ?eq'

expect_count 1 ./empty -e '
	entry ?root ?(label ?TAG_compile_unit) !(label !TAG_compile_unit)
	?AT_name !(!AT_name)
	attribute ?(?AT_name label ?AT_name) !(!AT_name || label !AT_name)
	?(?FORM_strp form ?FORM_strp) !(!FORM_strp || form !FORM_strp)'

expect_count 1 ./empty -e '
	entry ?root ?(label ?DW_TAG_compile_unit) !(label !DW_TAG_compile_unit)
	?DW_AT_name !(!DW_AT_name)
	attribute ?(?DW_AT_name label ?DW_AT_name)
	!(!DW_AT_name || label !DW_AT_name)
	?(?DW_FORM_strp form ?DW_FORM_strp)
	!(!DW_FORM_strp || form !DW_FORM_strp)'

# check type constants
expect_count 1 -e '
	?(1 type T_CONST ?eq "%s" "T_CONST" ?eq)
	?("" type T_STR ?eq "%s" "T_STR" ?eq)
	?([] type T_SEQ ?eq "%s" "T_SEQ" ?eq)
	?({} type T_CLOSURE ?eq "%s" "T_CLOSURE" ?eq)'
expect_count 1 ./duplicate-const -e '
	?(entry ?root type T_DIE ?eq "%s" "T_DIE" ?eq)
	?(entry ?root attribute ?AT_name type T_ATTR ?eq "%s" "T_ATTR" ?eq)'

# Check some list and seq words.
expect_count 1 -e '
	[1] !empty !(?empty) ?(length 1 ?eq)
	[] ?empty !(!empty) ?(length 0 ?eq)
	"1" !empty !(?empty) ?(length 1 ?eq)
	"" ?empty !(!empty) ?(length 0 ?eq)'
expect_count 1 -e '
	?([1, 2, 3] [4, 5, 6] add [1, 2, 3, 4, 5, 6] ?eq)
	?("123" "456" add "123456" ?eq)'
expect_count 1 -e '
	?("123456" ?("234" ?find) ?("123" ?find) ?("456" ?find) ?(dup ?find)
		   !("234" !find) !("123" !find) !("456" !find) !(dup !find)
		   ?("1234567" !find) !("1234567" ?find))
	?([1,2,3,4,5,6] ?([2,3,4] ?find) ?([1,2,3] ?find)
			?([4,5,6] ?find) ?(dup ?find)
			!([2,3,4] !find) !([1,2,3] !find)
			!([4,5,6] !find) !(dup !find)
			?([1,2,3,4,5,6,7] !find) !([1,2,3,4,5,6,7] ?find))'
expect_count 1 -e '
	?("123456" ?("234" !starts) ?("123" ?starts) ?("456" !starts) ?(dup ?starts)
		   !("234" ?starts) !("123" !starts) !("456" ?starts) !(dup !starts)
		   ?("1234567" !starts) !("1234567" ?starts))
	?([1,2,3,4,5,6] ?([2,3,4] !starts) ?([1,2,3] ?starts)
			?([4,5,6] !starts) ?(dup ?starts)
			!([2,3,4] ?starts) !([1,2,3] !starts)
			!([4,5,6] ?starts) !(dup !starts)
			?([1,2,3,4,5,6,7] !starts) !([1,2,3,4,5,6,7] ?starts))'
expect_count 1 -e '
	?("123456" ?("234" !ends) ?("123" !ends) ?("456" ?ends) ?(dup ?ends)
		   !("234" ?ends) !("123" ?ends) !("456" !ends) !(dup !ends)
		   ?("1234567" !ends) !("1234567" ?ends))
	?([1,2,3,4,5,6] ?([2,3,4] !ends) ?([1,2,3] !ends)
			?([4,5,6] ?ends) ?(dup ?ends)
			!([2,3,4] ?ends) !([1,2,3] ?ends)
			!([4,5,6] !ends) !(dup !ends)
			?([1,2,3,4,5,6,7] !ends) !([1,2,3,4,5,6,7] ?ends))'

# Check location expression support.

#   Support of DW_AT_location with DW_FORM_block*.
expect_count 1 ./typedef.o -e '
	entry ?(@AT_location elem label DW_OP_addr ?eq)'
#   Support of DW_AT_location with DW_FORM_exprloc
expect_count 2 ./enum.o -e '
	entry ?(@AT_location elem label DW_OP_addr ?eq)'

#   Numbering of elements of location list.
expect_count 1 ./bitcount.o -e '
	entry (offset == 0x91) @AT_location (pos == 1)'

# Test multi-yielding value.
expect_count 3 ./bitcount.o -e '
	[entry ?AT_location] elem (pos == 0) attribute ?AT_location value'

# ?OP_*
expect_count 1 ./bitcount.o -e '
	[entry @AT_location] elem (pos == 1)
	([?OP_and] length == 1)
	([?OP_or] length == 0)
	([elem ?OP_and] length == 1)
	([elem ?OP_or] length == 0)'

# high, low, address
expect_count 1 ./bitcount.o -e '
	[entry @AT_location] elem (pos == 1) address
	(high == 0x1001a) (low == 0x10017) (== 65559 65562 aset)'

expect_count 2 ./duplicate-const -e '
	entry attribute ?AT_high_pc
	(form == DW_FORM_data8)
	(value == 46)
	address (== 0x4004fb) ("%s" == "0x4004fb")'

expect_count 2 ./duplicate-const -e '
	entry attribute ?AT_low_pc
	(form == DW_FORM_addr)
	address (== value) ("%s" == "0x4004cd")'

expect_count 2 ./duplicate-const -e '
	entry (low == 0x4004cd) (high == 0x4004fb)'

expect_count 1 -e '
	10 20 aset
	?(9 !contains) ?(10 ?contains) ?(11 ?contains) ?(12 ?contains)
	?(13 ?contains) ?(14 ?contains) ?(15 ?contains) ?(16 ?contains)
	?(17 ?contains) ?(18 ?contains) ?(19 ?contains) ?(20 !contains)'

expect_count 1 -e '
	10 20 aset
	?(9 20 aset !contains) ?(10 20 aset ?contains)
	?(11 20 aset ?contains) ?(12 20 aset ?contains)
	?(13 20 aset ?contains) ?(14 20 aset ?contains)
	?(15 20 aset ?contains) ?(16 20 aset ?contains)
	?(17 20 aset ?contains) ?(18 20 aset ?contains)
	?(19 20 aset ?contains)'

expect_count 1 -e '
	10 20 aset
	?(9 10 aset !contains)
	?(9 11 aset !contains) ?(9 12 aset !contains)
	?(9 13 aset !contains) ?(9 14 aset !contains)
	?(9 15 aset !contains) ?(9 16 aset !contains)
	?(9 17 aset !contains) ?(9 18 aset !contains)
	?(9 19 aset !contains) ?(9 20 aset !contains)
	?(9 21 aset !contains)'

expect_count 1 -e '
	10 20 aset
	?(9 21 aset !contains) ?(10 21 aset !contains)
	?(11 21 aset !contains) ?(12 21 aset !contains)
	?(13 21 aset !contains) ?(14 21 aset !contains)
	?(15 21 aset !contains) ?(16 21 aset !contains)
	?(17 21 aset !contains) ?(18 21 aset !contains)
	?(19 21 aset !contains) ?(20 21 aset !contains)'

expect_count 1 -e '10 20 aset 10 10 aset ?contains'
expect_count 1 -e '10 20 aset 15 15 aset ?contains'
expect_count 1 -e '10 10 aset dup ?contains'

expect_count 1 -e '
	5 10 aset
	?(4 5 aset !overlaps) ?(4 6 aset ?overlaps)
	?(4 7 aset ?overlaps) ?(4 8 aset ?overlaps)
	?(4 9 aset ?overlaps) ?(4 10 aset ?overlaps)
	?(4 11 aset ?overlaps) ?(5 11 aset ?overlaps)
	?(6 11 aset ?overlaps) ?(7 11 aset ?overlaps)
	?(8 11 aset ?overlaps) ?(9 11 aset ?overlaps)
	?(10 11 aset !overlaps)'

expect_count 1 -e '
	5 10 aset !empty
	5 5 aset ?empty'

expect_count 1 -e '
	1 10 aset 2 20 aset overlap == 2 10 aset'
expect_count 1 -e '
	0x10 0x20 aset add: (0x30 0x40 aset) add: (0x50 0x60 aset)
	overlap: (0x15 0x55 aset)
	== 0x15 0x20 aset add: (0x30 0x40 aset) add: (0x50 0x55 aset)'

expect_count 1 -e '
	(10 20 aset length == 10)
	(10 10 aset length == 0)'

expect_count 1 ./aranges.o -e '
	entry ?TAG_lexical_block
	([address range] length == 2)
	address range (pos == 1) (== 0x1000e 0x10015 aset)'

expect_count 1 ./aranges.o -e '
	entry ?TAG_lexical_block
	([@AT_ranges range] length == 2)
	@AT_ranges range (pos == 1) (== 0x1000e 0x10015 aset)'

expect_count 1 ./aranges.o -e '
	entry ?TAG_lexical_block address [|A| A elem]
	== [0x10004, 0x10005, 0x10006, 0x10007, 0x10008,
	    0x1000e, 0x1000f, 0x10010, 0x10011, 0x10012, 0x10013, 0x10014]'

expect_count 1 ./aranges.o -e '
	entry ?TAG_lexical_block address [|A| A relem]
	== [[0x10004, 0x10005, 0x10006, 0x10007, 0x10008,
	     0x1000e, 0x1000f, 0x10010, 0x10011, 0x10012, 0x10013, 0x10014]
	    relem]'

expect_count 1 ./pointer_const_value.o -e '
	entry @AT_const_value == 0'

expect_count 1 ./ptrmember_const_value.o -e '
	entry @AT_const_value == 0'

expect_count 4 ./float_const_value.o -e '
	entry
	(?((raw attribute ?AT_name cooked value == "fv")
	   (@AT_const_value == [0, 0, 0xc0, 0x3f]))
	 || ?((raw attribute ?AT_name cooked value == "dv")
	      (@AT_const_value == [0, 0, 0, 0, 0, 0, 0xf8, 0x3f]))
	 || ?((raw attribute ?AT_name cooked value == "ldv")
	      (@AT_const_value == [0, 0, 0, 0, 0, 0, 0, 0xc0,
				   0xff, 0x3f, 0, 0, 0, 0, 0, 0])
	 || ?((raw attribute ?AT_name cooked value == "f128v")
	      (@AT_const_value == [0, 0, 0, 0, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0x80, 0xff, 0x3f]))))'

expect_count 1 -e '
	[1, 2, 3, 4] [3, 4, 5, 6]
	(|A B| A [B elem !(== A elem)] add) == [1, 2, 3, 4, 5, 6]'

# Abbreviations.

expect_count 1 ./bitcount.o -e '
	[entry abbrev offset] ==
	[0, 19, 19, 30, 19, 19, 19, 19, 41, 19, 54, 80, 95, 104]'
expect_count 1 ./bitcount.o -e '
	[entry abbrev code] ==
	[1, 2, 2, 3, 2, 2, 2, 2, 4, 2, 5, 6, 7, 8]'

expect_count 0 ./duplicate-const -e 'entry ?haschildren !(child)'
expect_count 0 ./duplicate-const -e 'entry !haschildren ?(child)'

expect_count 1 ./duplicate-const -e '
	[entry ?haschildren] length == [entry abbrev ?haschildren] length'

expect_count 1 ./duplicate-const -e '
	[entry ([abbrev attribute] length == [attribute] length)] length
	== [entry] length'
expect_count 1 ./duplicate-const -e '
	[entry ([abbrev attribute form] == [attribute form])] length
	== [entry] length'
expect_count 1 ./duplicate-const -e '
	[entry ([abbrev attribute label] == [attribute label])] length
	== [entry] length'
expect_count 1 ./duplicate-const -e '
	[entry ?(abbrev ?AT_name)] == [entry ?AT_name]'
expect_count 1 ./duplicate-const -e '
	[entry ?(abbrev !AT_name)] == [entry !AT_name]'
expect_count 1 ./duplicate-const -e '
	[entry ?(abbrev attribute ?AT_name)] == [entry ?AT_name]'
expect_count 1 ./bitcount.o -e '
	[entry (pos == 0) abbrev attribute ?FORM_strp label]
	== [DW_AT_producer, DW_AT_name, DW_AT_comp_dir]'

expect_count 1 ./duplicate-const -e '[entry label] == [entry abbrev label]'

expect_count 1 ./twocus -e '[abbrev offset] == [0, 0x34]'
expect_count 1 ./twocus -e '?(abbrev entry (|A| A pos 1 add == A code))'

# Test that dwgrep doesn't crash on a DIE whose abbrev claims to have
# children, but that ends up having none.
expect_count 3 ./haschildren_childless -e 'entry'

# Test that dwgrep handles well "dwz -m" files with common debuginfo.
expect_count 1 ./dwz-dupfile -e 'raw entry (@AT_name == "W")'

# Test raw/cooked Dwarf interpretation.
expect_count 4 ./dwz-partial -e 'unit'
expect_count 5 ./dwz-partial -e 'raw unit'

expect_count 1 ./duplicate-const -e 'raw (==)'
expect_count 1 ./duplicate-const -e 'raw unit (==)'

expect_count 1 ./twocus -e '
	unit root type == T_DIE'
expect_count 1 ./dwz-partial -e '
	(|A| [A raw abbrev] == [A abbrev])'
expect_count 1 ./twocus -e '
	(|A| [A unit offset] == [A raw unit offset])'
expect_count 1 ./twocus -e '
	(|A| [A unit root offset] == [A raw unit root offset])'
expect_count 1 ./dwz-partial -e '
	[unit (pos == 0) entry offset] ==
	[0x34, 0x14, 0x17, 0x1a, 0x1d, 0x1f, 0x21, 0x5b, 0x64, 0x80, 0x87]'
expect_count 1 ./dwz-partial -e '
	[unit (pos == 0) raw entry offset] ==
	[0x34, 0x56, 0x5b, 0x64, 0x80, 0x87]'
expect_count 1 ./dwz-partial -e '
	[unit (pos == 0) root raw child offset] ==
	[0x56, 0x5b, 0x64, 0x80, 0x87]'
expect_count 1 ./dwz-partial -e '
	[unit (pos == 0) root child offset] ==
	[0x14, 0x17, 0x1a, 0x1d, 0x1f, 0x21, 0x5b, 0x64, 0x80, 0x87]'
expect_count 1 ./dwz-partial -e '
	[entry (offset == 0x14) parent offset] ==
	[0x34, 0xa4, 0xe1, 0x11e]'
expect_count 1 ./dwz-partial -e '
	[unit root child (offset == 0x14) parent offset] ==
	[0x34, 0xa4, 0xe1, 0x11e]'

expect_count 4 ./dwz-partial -e '
	(|A| A entry (offset == 0x14)
	     A entry (offset == 0x14)) ?eq'
# Also check that an entry with no or incomplete import history ends
# up comparing equal to an entry with full history.
expect_count 4 ./dwz-partial -e '
	(|A| A entry (offset == 0x14)
	     A raw entry (offset == 0x14) cooked) ?eq'

expect_count 4 ./dwz-partial -e '
	entry (offset == 0x14)
	?(root ?TAG_compile_unit)
	?(raw root ?TAG_partial_unit)'

expect_count 1 ./nullptr.o -e '
	[|A| A raw entry (offset == 0x6e) attribute label]
	== [DW_AT_specification, DW_AT_inline, DW_AT_object_pointer,
	    DW_AT_sibling]'
expect_count 1 ./nullptr.o -e '
	[|A| A entry (offset == 0x6e) attribute label]
	== [DW_AT_specification, DW_AT_inline, DW_AT_object_pointer,
	    DW_AT_sibling, DW_AT_external, DW_AT_name,
	    DW_AT_decl_file, DW_AT_decl_line]'

# Test version.
expect_count 4 ./dwz-partial -e 'unit (version == 3)'
expect_count 5 ./dwz-partial -e 'raw unit (version == 3)'

# Test name.
expect_count 1 ./empty -e 'name == "./empty"'
expect_count 1 ./empty -e 'raw name == "./empty"'
expect_count 1 ./empty -e 'entry name == "empty.c"'
expect_count 1 ./empty -e 'raw entry name == "empty.c"'

# Test for inconsistent types.
expect_count 1 ./inconsistent-types -e '
	let A := entry ?TAG_subprogram;
	let B := A child ?TAG_formal_parameter;
	let C := A @AT_specification child ?TAG_formal_parameter;
	(B pos == C pos) (B @AT_type != C @AT_type)'

# Tests for ticket #14
expect_out '0' ../tests/const_value_signedness_from_enumerator.o -e '
	entry ?TAG_template_value_parameter @AT_const_value'
expect_out '0' ../tests/const_value_signedness_small_enough.o -e '
	entry ?TAG_template_value_parameter @AT_const_value'

# Test for ticket #7
expect_count 1 -e '()******************'
expect_count 1 -e '()++++++++++++++++++'

# Test merging of -e's.
expect_count 2 -e 1 -e , -e 2

# Test unreadable file.
expect_error "can't open script file" -f soauehoeutaoeutaoe

# Test that zero bytes don't terminate the query too soon.
TMP=$(mktemp)
echo -e '7 == "foo\x00bar" length' > $TMP
expect_count 1 -f $TMP
rm $TMP

# Test position assertion.
expect_count 1 -e '[1, 2, 3] elem ?0 == 1'
expect_count 1 -e '[1, 2, 3] elem ?1 == 2'
expect_count 1 -e '[1, 2, 3] elem ?2 == 3'
expect_count 0 -e '[1, 2, 3] elem ?3'

expect_count 1 -e '[[1, 2, 3] elem !0] == [2, 3]'
expect_count 1 -e '[[1, 2, 3] elem !1] == [1, 3]'
expect_count 1 -e '[[1, 2, 3] elem !2] == [1, 2]'
expect_count 1 -e '[[1, 2, 3] elem !3] == [1, 2, 3]'

expect_count 1 -e '[1, 2, 3] relem ?0 == 3'
expect_count 1 -e '[[1, 2, 3] relem !0] == [2, 1]'

expect_error "unbound" -e foo
expect_error "valid ELF" ./empty.s -e entry

# Could be misparsed as '123 drop'
expect_error "Invalid integer literal" -e '123drop'
expect_error "Invalid integer literal" -e '?123drop'

expect_error "out of range" -e '1234567890987654321234567890987654321'
expect_error "out of range" -e '?1234567890987654321234567890987654321'

### Test actual output of individual types by dwgrep driver.  ###

# T_CONST
expect_out '1' -e '1'

# T_STR
expect_out 'blah' -e '"blah"'

# T_SEQ and inner form of T_STR
expect_out '[1, "blah", []]' -e '[1, "blah", []]'

# T_DWARF
expect_out '<Dwarf "a1.out">' a1.out -e ''

# T_CU
expect_out '<CU 0>' a1.out -e 'unit'
expect_out '<CU 0>
<CU 0x53>' twocus -e unit

# T_DIE (both naked form and inner form)
expect_out '[23]	const_type
	type	[25] volatile_type' a1.out -e 'entry (offset == 0x23)'

# T_ATTR
expect_out 'byte_size	8
type	[23] const_type' a1.out -e 'entry (offset == 0x20) attribute'

# T_LOCLIST_ELEM
expect_out '0..0xffffffffffffffff:0 call_frame_cfa' \
	a1.out -e 'entry (offset == 0x32) @AT_frame_base'

# T_ASET
expect_out '0x10004..0x10009, 0x1000e..0x10015' \
	aranges.o -e 'entry @AT_ranges'

# T_ELFSYM
expect_out "1:	0000000000000000      0 FILE	LOCAL	DEFAULT	enum.cc
12:	0000000000000000      4 OBJECT	GLOBAL	DEFAULT	ae
13:	0x00000000000004      4 OBJECT	GLOBAL	DEFAULT	af" \
	enum.o -e 'symbol (name != "")'

expect_out "9:	0000000000000000     24 ARM_TFUNC	GLOBAL	DEFAULT	main" \
	   -e '"y.o" dwopen symbol (pos == 9)'

expect_out "\
9:	0000000000000000     24 ARM_TFUNC	GLOBAL	DEFAULT	main
9:	0000000000000000     24 FUNC	MIPS_SPLIT_COMMON	DEFAULT	main" \
	 -e '("y.o", "y-mips.o") dwopen symbol (pos == 9)'

expect_out "\
9:	0000000000000000     24 FUNC	MIPS_SPLIT_COMMON	DEFAULT	main
9:	0000000000000000     24 ARM_TFUNC	GLOBAL	DEFAULT	main" \
	 -e '("y-mips.o", "y.o") dwopen symbol (pos == 9)'

expect_out 'STT_FILE stdin@0
STT_NOTYPE $a@0
STT_ARM_TFUNC main@0' \
	 y.o -e 'symbol (name != "") "%s"'

# T_LOCLIST_OP

# Test both T_LOCLIST_ELEM and the corresponding T_LOCLIST_OP output.
expect_out '0x10017..0x1001a:0 breg5 <0>, 0x2 breg1 <0>, 0x4 and, 0x5 stack_value' \
	bitcount.o -e 'entry (offset == 0x91) @AT_location (pos == 1)'

expect_out '0 breg5 <0>
0x2 breg1 <0>
0x4 and
0x5 stack_value' \
	 bitcount.o -e 'entry (offset == 0x91) @AT_location (pos == 1) elem'


### Test script arguments.  ###

# Test command line arguments and file names.
expect_out \
'["y.o", 1]
["y.o", 2]
["y.o", 3]
["a1.out", 1]
["a1.out", 2]
["a1.out", 3]
["bitcount.o", 1]
["bitcount.o", 2]
["bitcount.o", 3]' \
	   y.o a1.out bitcount.o \
	   --a 1,2,3 -he '[|Dw N| Dw name, N]'

# Test that reordering file names after --a keeps the Dwarf argument at the
# bottom of the stack.
expect_out \
'["y.o", 1]
["y.o", 2]
["y.o", 3]
["a1.out", 1]
["a1.out", 2]
["a1.out", 3]
["bitcount.o", 1]
["bitcount.o", 2]
["bitcount.o", 3]' \
	   --a 1,2,3 -he '[|Dw N| Dw name, N]' \
	   y.o a1.out bitcount.o

# Test interplay between file names and argument values, and that file names are
# deduced correctly for each yielded stack.
expect_out \
'y.o,1:
["y.o", 1]
y.o,2:
["y.o", 2]
y.o,3:
["y.o", 3]
a1.out,1:
["a1.out", 1]
a1.out,2:
["a1.out", 2]
a1.out,3:
["a1.out", 3]
bitcount.o,1:
["bitcount.o", 1]
bitcount.o,2:
["bitcount.o", 2]
bitcount.o,3:
["bitcount.o", 3]' \
	   y.o a1.out bitcount.o \
	   --a 1,2,3 -e '[|Dw N| Dw name, N]'

# Thorough test for ordering arguments.
expect_out "$(seq 0 63)" \
	   --a 0,16,32,48 --a 0,4,8,12 --a 0,1,2,3 \
	   -he '(|A B C| A B C add add)'

# Test passing one argument with --a, and string arguments.
expect_count 1 --a 1  --a 2 --a 3 -e '[|A B C| A, B, C] == [1, 2, 3]'
expect_count 1  -a 1   -a 2  -a 3 -e '[|A B C| A, B, C] == ["1", "2", "3"]'
expect_count 1  -a 1,2 -a 2  -a 3 -e '[|A B C| A, B, C] == ["1,2", "2", "3"]'

# Test that <no-file> is shown when there are no files to process.
expect_out \
'<no-file>:
y.o' \
    --a '"y.o" dwopen' \
    -He 'name'

# Test that script arguments that are Dwarf are treated as if they were files
# given on command line.
expect_out \
'y.o:
y.o
a1.out:
a1.out
bitcount.o:
bitcount.o' \
    --a '("y.o","a1.out","bitcount.o") dwopen' \
    -e 'name'

# Test errors when evaluating arguments.
expect_error "argument*foo*unbound name" --a foo -e ''
expect_error "argument*drop*stack overflow" --a drop -e ''
expect_error 'argument*1\?*empty stack' --a '1?' -e ''

# Test position.
expect_out "$(yes 1 | head -n 27)" \
	   --a '[0,1,2] elem' --a '[0,1,2] elem' --a '[0,1,2] elem' \
	   -che '(|M N O| M, N, O) == dup pos'

# Test arguments that pass more than one stack slot.
expect_count 1 --a '1 2 3' -e '== 3'

# Test position of passed-in Dwarfs.
expect_out '1
0' \
	   y.o a1.out \
	   -che '(pos == 0) (name == "y.o")'

expect_out '0
1' \
	   y.o a1.out \
	   -che '(pos == 1) (name == "a1.out")'

expect_out '0
0' \
	   y.o a1.out \
	   -che 'pos > 1'

# =============================================================================

echo "$total tests total, $failures failures."

trap - ERR
[ $failures -eq 0 ]
