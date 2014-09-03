#!/bin/sh

failures=0
total=0

expect_count ()
{
    export total=$((total + 1))
    COUNT=$1
    shift
    GOT=$(timeout 10 ../dwgrep -c "$@" 2>/dev/null)
    if [ "$GOT" != "$COUNT" ]; then
	echo "FAIL: dwgrep -c" "$@"
	echo "expected: $COUNT"
	echo "     got: $GOT"
	export failures=$((failures + 1))
    fi
}

expect_count 1 ./empty -e '1   10 ?lt'
expect_count 1 ./empty -e '10  10 !lt'
expect_count 1 ./empty -e '100 10 !lt'
expect_count 1 ./empty -e '1   10 ?le'
expect_count 1 ./empty -e '10  10 ?le'
expect_count 1 ./empty -e '100 10 !le'
expect_count 1 ./empty -e '1   10 !eq'
expect_count 1 ./empty -e '10  10 ?eq'
expect_count 1 ./empty -e '100 10 !eq'
expect_count 1 ./empty -e '1   10 ?ne'
expect_count 1 ./empty -e '10  10 !ne'
expect_count 1 ./empty -e '100 10 ?ne'
expect_count 1 ./empty -e '1   10 !ge'
expect_count 1 ./empty -e '10  10 ?ge'
expect_count 1 ./empty -e '100 10 ?ge'
expect_count 1 ./empty -e '1   10 !gt'
expect_count 1 ./empty -e '10  10 !gt'
expect_count 1 ./empty -e '100 10 ?gt'

expect_count 0 ./empty -e '1   10 !lt'
expect_count 0 ./empty -e '10  10 ?lt'
expect_count 0 ./empty -e '100 10 ?lt'
expect_count 0 ./empty -e '1   10 !le'
expect_count 0 ./empty -e '10  10 !le'
expect_count 0 ./empty -e '100 10 ?le'
expect_count 0 ./empty -e '1   10 ?eq'
expect_count 0 ./empty -e '10  10 !eq'
expect_count 0 ./empty -e '100 10 ?eq'
expect_count 0 ./empty -e '1   10 !ne'
expect_count 0 ./empty -e '10  10 ?ne'
expect_count 0 ./empty -e '100 10 !ne'
expect_count 0 ./empty -e '1   10 ?ge'
expect_count 0 ./empty -e '10  10 !ge'
expect_count 0 ./empty -e '100 10 !ge'
expect_count 0 ./empty -e '1   10 ?gt'
expect_count 0 ./empty -e '10  10 ?gt'
expect_count 0 ./empty -e '100 10 !gt'


expect_count 1 ./empty -e '1   != 10'
expect_count 1 ./empty -e '10  == 10'
expect_count 1 ./empty -e '100 != 10'
expect_count 1 ./empty -e '1   <  10'
expect_count 1 ./empty -e '10  <= 10'
expect_count 1 ./empty -e '10  >= 10'
expect_count 1 ./empty -e '100 >  10'

expect_count 0 ./empty -e '1   == 10'
expect_count 0 ./empty -e '10  != 10'
expect_count 0 ./empty -e '100 == 10'
expect_count 0 ./empty -e '1   >  10'
expect_count 0 ./empty -e '10  < 10'
expect_count 0 ./empty -e '10  > 10'
expect_count 0 ./empty -e '100 < 10'

# Test comments.
expect_count 1 ./empty -e '
	1 #blah blah -45720352304573257230453045302 {}[*[{)+*]&+*
	2 # sub mul div mod
	add == 3'
expect_count 1 ./empty -e '
	1 //blah blah -45720352304573257230453045302 {}[*[{)+*]&+*
	2 // sub mul div mod
	add == 3'
expect_count 1 ./empty -e '
	1 /*blah blah -45720352304573257230453045302 {}[*[{)+*]&+***/ 2 /*
	sub mul div mod*/ add
	== 3'

# Test that universe annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo (offset == 0xb8) (pos == 10)'

# Test that child annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?root child (offset == 0xb8) (pos == 6)'

# Test that format annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?root "%( child offset %)" (== "0xb8") (pos == 6)'

# Test that attribute annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?root attribute ?AT_stmt_list (pos == 6)'

# Test that unit annotates position.
expect_count 11 ./nontrivial-types.o -e '
	let A := winfo; let B := A unit; (A pos == B pos)'

# Test a bug with scope promotion.
expect_count 1 ./empty -e '
	let A:=1; ?(let X:=A;)'
expect_count 1 ./empty -e '
	0 [|B| 1 [|V| B]] ?([[0]] ?eq)'
expect_count 1 ./empty -e '
	1 {|A| 3 [|B| A]} apply ?([1] ?eq)'

# Test that elem annotates position.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?root drop [10, 11, 12]
	?(elem (pos == 0) (== 10))
	?(elem (pos == 1) (== 11))
	?(elem (pos == 2) (== 12))'
expect_count 3 ./empty -e '
	[0, 1, 2] elem dup (== pos)'
expect_count 3 ./empty -e '
	[2, 1, 0] relem dup (== pos)'
expect_count 1 ./empty -e '
	["210" relem] == ["012" elem]'

# Check literal assertions.
expect_count 1 ./empty -e '
	?([1, 3, 5] elem (pos == 0) (== 1))
	?([1, 3, 5] elem (pos == 1) (== 3))
	?([1, 3, 5] elem (pos == 2) (== 5))'
expect_count 1 ./empty -e '
	?([[1, 3, 5] elem (pos != 0) ((3,5) ==)] (length == 2))
	?([[1, 3, 5] elem (pos != 1) ((1,5) ==)] (length == 2))
	?([[1, 3, 5] elem (pos != 2) ((1,3) ==)] (length == 2))'

# Tests star closure whose body ends with stack in a different state
# than it starts in (different slots are taken in the valfile).
expect_count 1 ./typedef.o -e '
	winfo
	?([] swap (@AT_type ?TAG_typedef [()] rot add swap)* drop length 3 ?eq)
	?(offset 0x45 ?eq)'

# Test decoding signed and unsigned value.
expect_count 1 ./enum.o -e '
	winfo (@AT_name == "f") child (@AT_name == "V")
	(@AT_const_value "%s" == "-1")'
expect_count 1 ./enum.o -e '
	winfo (@AT_name == "e") child (@AT_name == "V")
	(@AT_const_value "%s" == "4294967295")'
expect_count 2 ./char_16_32.o -e '
	winfo (@AT_name == "bar") (@AT_const_value == 0xe1)'
expect_count 1 ./nullptr.o -e '
	winfo ?(integrate @AT_type ?TAG_unspecified_type) (@AT_const_value == 0)'
expect_count 1 ./testfile_const_type -e '
	winfo @AT_location elem ?OP_GNU_const_type value
	((pos == 0) (type == T_DIE) || (pos == 1) (type == T_SEQ))'

# Test match operator
expect_count 7 ./duplicate-const -e '
	winfo ?(@AT_decl_file "" ?match)'
expect_count 7 ./duplicate-const -e '
	winfo ?(@AT_decl_file ".*petr.*" ?match)'
expect_count 1 ./nontrivial-types.o -e '
	winfo ?(@AT_language "%s" "DW_LANG_C89" ?match)'
expect_count 1 ./nontrivial-types.o -e '
	winfo ?(@AT_encoding "%s" "^DW_ATE_signed$" ?match)'
expect_count 7 ./duplicate-const -e '
	winfo (@AT_decl_file =~ "")'
expect_count 7 ./duplicate-const -e '
	winfo (@AT_decl_file =~ ".*petr.*")'
expect_count 7 ./duplicate-const -e '
	winfo (@AT_decl_file !~ ".*pavel.*")'

# Test true/false
expect_count 1 ./typedef.o -e '
	winfo ?(@AT_external == true)'
expect_count 1 ./typedef.o -e '
	winfo ?(@AT_external != false)'

# Test that (dup parent) doesn't change the bottom DIE as well.
expect_count 1 ./nontrivial-types.o -e '
	winfo ?TAG_structure_type dup parent ?(swap offset == 0x2d)'

# Check that when promoting assertions close to producers of their
# slots, we don't move across alternation or closure.
expect_count 3 ./nontrivial-types.o -e '
	winfo ?TAG_subprogram child* ?TAG_formal_parameter'
expect_count 3 ./nontrivial-types.o -e '
	winfo ?TAG_subprogram child? ?TAG_formal_parameter'
expect_count 3 ./nontrivial-types.o -e '
	winfo ?TAG_subprogram (child,) ?TAG_formal_parameter'

# Check casting.
expect_count 1 ./enum.o -e '
	winfo (@AT_name == "e") child
	@AT_const_value "%x" "0xffffffff" ?eq'
expect_count 1 ./enum.o -e '
	winfo (@AT_name == "e") child
	@AT_const_value hex "%s" "0xffffffff" ?eq'
expect_count 1 ./enum.o -e '
	winfo (@AT_name == "e") child
	@AT_const_value "%o" "037777777777" ?eq'
expect_count 1 ./enum.o -e '
	winfo (@AT_name == "e") child
	@AT_const_value oct "%s" "037777777777" ?eq'
expect_count 1 ./enum.o -e '
	winfo (@AT_name == "e") child @AT_const_value
	"%b" "0b11111111111111111111111111111111" ?eq'
expect_count 1 ./enum.o -e '
	winfo (@AT_name == "e") child @AT_const_value bin
	"%s" "0b11111111111111111111111111111111" ?eq'
expect_count 1 ./enum.o -e '
	winfo (@AT_name == "e") child label "%d" "40" ?eq'

# Check decoding of huge literals.
expect_count 1 ./empty -e '
	[0xffffffffffffffff "%s" elem !(pos == (0,1))]
	(length == 16) !(elem != "f")'
expect_count 1 ./empty -e '
	18446744073709551615 == 0xffffffffffffffff'
expect_count 1 ./empty -e '
	01777777777777777777777 == 0xffffffffffffffff'
expect_count 1 ./empty -e '
	0b1111111111111111111111111111111111111111111111111111111111111111
	== 0xffffffffffffffff'
expect_count 1 ./empty -e'
	-0xff dup dup dup "%s %d %b %o" == "-0xff -255 -0b11111111 -0377"'
expect_count 1 ./empty -e'
	-0377 dup dup dup "%x %d %b %s" == "-0xff -255 -0b11111111 -0377"'
expect_count 1 ./empty -e'
	-255 dup dup dup "%x %s %b %o" == "-0xff -255 -0b11111111 -0377"'
expect_count 1 ./empty -e'
	-0b11111111 dup dup dup "%x %d %s %o"
	== "-0xff -255 -0b11111111 -0377"'

# Check arithmetic.
expect_count 1 ./empty -e '-1 1 add (== 0)'
expect_count 1 ./empty -e '-1 10 add (== 9)'
expect_count 1 ./empty -e '1 -10 add (== -9)'
expect_count 1 ./empty -e '-10 1 add (== -9)'
expect_count 1 ./empty -e '10 -1 add (== 9)'

expect_count 1 ./empty -e '
	-1 0xffffffffffffffff add "%s" == "0xfffffffffffffffe"'
expect_count 1 ./empty -e '
	0xffffffffffffffff -1 add "%s" == "0xfffffffffffffffe"'

expect_count 1 ./empty -e '-1 1 sub (== -2)'
expect_count 1 ./empty -e '-1 10 sub (== -11)'
expect_count 1 ./empty -e '1 -10 sub (== 11)'
expect_count 1 ./empty -e '-10 1 sub (== -11)'
expect_count 1 ./empty -e '10 -1 sub (== 11)'

expect_count 1 ./empty -e '-2 2 mul (== -4)'
expect_count 1 ./empty -e '-2 10 mul (== -20)'
expect_count 1 ./empty -e '2 -10 mul (== -20)'
expect_count 1 ./empty -e '-10 2 mul (== -20)'
expect_count 1 ./empty -e '10 -2 mul (== -20)'
expect_count 1 ./empty -e '-10 -2 mul (== 20)'
expect_count 1 ./empty -e '-2 -10 mul (== 20)'

# Check iterating over empty compile unit.
expect_count 1 ./empty -e '
	winfo offset'

# Check ||.
expect_count 6 ./typedef.o -e '
	winfo (@AT_decl_line || drop 42)'
expect_count 2 ./typedef.o -e '
	winfo (@AT_decl_line || drop 42) (== 42)'
expect_count 6 ./typedef.o -e '
	winfo (@AT_decl_line || @AT_byte_size || drop 42)'
expect_count 1 ./typedef.o -e '
	winfo (@AT_decl_line || @AT_byte_size || drop 42) (== 42)'
expect_count 1 ./empty -e '
	(0, 1, 20) (== 10 || == 20)'

# Check closures.
expect_count 1 ./empty -e '
	{|A| {|B| A}} 2 swap apply 9 swap apply (== 1 1 add)'
expect_count 1 ./empty -e '
	{|A| {A}} 5 swap apply apply (== 2 3 add)'
expect_count 1 ./empty -e '
	let double := {dup add}; (1 double == 2)'
expect_count 1 ./empty -e '
	let adder := {|x| {|y| x y add}};
	3 adder 2 swap apply (== 5)'
expect_count 1 ./empty -e '
	let map := {|f| [|L| L elem f]};
	[1, 2, 3] {1 add} map ?([2, 3, 4] ?eq)'
expect_count 1 ./empty -e '
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

# Check that bindings remember position.
expect_count 3 ./empty -e '
	let E := [0, 1, 2] elem; E (== pos)'

# Check recursion.
expect_count 1 ./empty -e '
	{|A| (?(A 10 ?ge) 0 || A 1 add F 1 add)} ->F;
	0 F
	?(10 ?eq)'

expect_count 1 ./empty -e '
	{|F T| ?(F T ?le) F, ?(F T ?lt) F 1 add T seq} -> seq;
	[1 10 seq] ?([1, 2, 3, 4, 5, 6, 7, 8, 9, 10] ?eq)'

expect_count 1 ./empty -e '
	{|N| (?(N 2 ?lt) 1 || N 1 sub fact N mul)} -> fact;
	?(5 fact 120 ?eq)
	?(6 fact 720 ?eq)
	?(7 fact 5040 ?eq)
	?(8 fact 40320 ?eq)'

# Examples.
expect_count 1 ./duplicate-const -e '
	let ?cvr_type := {?TAG_const_type,?TAG_volatile_type,?TAG_restrict_type};
	let P := winfo ;
	let A := P child ?cvr_type ;
	let B := P child ?cvr_type ?(?lt: A) ;
	?((A label) ?eq: (B label))
	?((A @AT_type) ?eq: (B @AT_type))'

expect_count 1 ./nontrivial-types.o -e '
	winfo ?TAG_subprogram !AT_declaration dup child ?TAG_formal_parameter
	?(@AT_type ((?TAG_const_type,?TAG_volatile_type,?TAG_typedef) @AT_type)*
	  (?TAG_structure_type, ?TAG_class_type))'

# Check ifelse.
expect_count 1 ./empty -e '
	[if ?(1) then (2,3) else (4,5)] ?([2,3] ?eq)'
expect_count 1 ./empty -e '
	[if !(1) then (2,3) else (4,5)] ?([4,5] ?eq)'
expect_count 6 ./typedef.o -e '
	[winfo] ?(length 6 ?eq)
	elem if child then 1 else 0 "%s %(offset%)"
	?(("1 0xb","0 0x1d","0 0x28","0 0x2f","0 0x3a","0 0x45") ?eq)'

# Check various Dwarf operators.
expect_count 1 ./empty -e '
	[[DW_AT_name, DW_TAG_const_type, DW_FORM_ref_sig8, DW_LANG_Go,
	  DW_INL_inlined, DW_ATE_UTF, DW_ACCESS_private, DW_VIS_exported,
	  DW_ID_case_insensitive, DW_VIRTUALITY_virtual, DW_CC_nocall,
	  DW_ORD_col_major, DW_DSC_range, DW_OP_bra, DW_DS_trailing_separate,
	  DW_ADDR_none, DW_END_little, DW_MACINFO_start_file,
	  DW_MACRO_GNU_undef_indirect] elem hex]
	[0x3, 0x26, 0x20, 0x16, 0x1, 0x10, 0x3, 0x2, 0x3, 0x1, 0x3, 0x1, 0x1,
	 0x28, 0x5, 0x0, 0x2, 0x3, 0x6] ?eq'

expect_count 1 ./empty -e '
	let Dw := "duplicate-const" dwopen;

	"%([Dw winfo label]%)"
	"[DW_TAG_compile_unit, DW_TAG_subprogram, DW_TAG_variable,"\
	" DW_TAG_variable, DW_TAG_variable, DW_TAG_variable,"\
	" DW_TAG_base_type, DW_TAG_pointer_type, DW_TAG_const_type,"\
	" DW_TAG_base_type, DW_TAG_array_type, DW_TAG_subrange_type,"\
	" DW_TAG_base_type, DW_TAG_const_type, DW_TAG_variable,"\
	" DW_TAG_variable, DW_TAG_const_type]" ?eq
	"%([Dw winfo label]%)" ?eq'

expect_count 1 ./duplicate-const -e '
	"%([winfo ?root attribute label]%)"
	"[DW_AT_producer, DW_AT_language, DW_AT_name, DW_AT_comp_dir, "\
	"DW_AT_low_pc, DW_AT_high_pc, DW_AT_stmt_list]" ?eq'

expect_count 1 ./duplicate-const -e '
	"%([winfo ?root attribute label]%)"
	"[DW_AT_producer, DW_AT_language, DW_AT_name, DW_AT_comp_dir, "\
	"DW_AT_low_pc, DW_AT_high_pc, DW_AT_stmt_list]" ?eq'

expect_count 1 ./duplicate-const -e '
	"%([winfo ?root attribute form]%)"
	"[DW_FORM_strp, DW_FORM_data1, DW_FORM_strp, DW_FORM_strp, "\
	"DW_FORM_addr, DW_FORM_data8, DW_FORM_sec_offset]" ?eq'

expect_count 1 ./empty -e '
	winfo ?root ?(label ?TAG_compile_unit) !(label !TAG_compile_unit)
	?AT_name !(!AT_name)
	attribute ?(?AT_name label ?AT_name) !(!AT_name || label !AT_name)
	?(?FORM_strp form ?FORM_strp) !(!FORM_strp || form !FORM_strp)'

expect_count 1 ./empty -e '
	winfo ?root ?(label ?DW_TAG_compile_unit) !(label !DW_TAG_compile_unit)
	?DW_AT_name !(!DW_AT_name)
	attribute ?(?DW_AT_name label ?DW_AT_name)
	!(!DW_AT_name || label !DW_AT_name)
	?(?DW_FORM_strp form ?DW_FORM_strp)
	!(!DW_FORM_strp || form !DW_FORM_strp)'

# check type constants
expect_count 1 ./empty -e '
	?(1 type T_CONST ?eq "%s" "T_CONST" ?eq)
	?("" type T_STR ?eq "%s" "T_STR" ?eq)
	?([] type T_SEQ ?eq "%s" "T_SEQ" ?eq)
	?({} type T_CLOSURE ?eq "%s" "T_CLOSURE" ?eq)'
expect_count 1 ./duplicate-const -e '
	?(winfo ?root type T_DIE ?eq "%s" "T_DIE" ?eq)
	?(winfo ?root attribute ?AT_name type T_ATTR ?eq "%s" "T_ATTR" ?eq)'

# Check some list and seq words.
expect_count 1 ./empty -e '
	[1] !empty !(?empty) ?(length 1 ?eq)
	[] ?empty !(!empty) ?(length 0 ?eq)
	"1" !empty !(?empty) ?(length 1 ?eq)
	"" ?empty !(!empty) ?(length 0 ?eq)'
expect_count 1 ./empty -e '
	?([1, 2, 3] [4, 5, 6] add [1, 2, 3, 4, 5, 6] ?eq)
	?("123" "456" add "123456" ?eq)'
expect_count 1 ./empty -e '
	?("123456" ?("234" ?find) ?("123" ?find) ?("456" ?find) ?(dup ?find)
		   !("234" !find) !("123" !find) !("456" !find) !(dup !find))
	?([1,2,3,4,5,6] ?([2,3,4] ?find) ?([1,2,3] ?find)
			?([4,5,6] ?find) ?(dup ?find)
			!([2,3,4] !find) !([1,2,3] !find)
			!([4,5,6] !find) !(dup !find))'

# Check location expression support.

#   Support of DW_AT_location with DW_FORM_block*.
expect_count 1 ./typedef.o -e '
	winfo ?(@AT_location elem label DW_OP_addr ?eq)'
#   Support of DW_AT_location with DW_FORM_exprloc
expect_count 2 ./enum.o -e '
	winfo ?(@AT_location elem label DW_OP_addr ?eq)'

#   Numbering of elements of location list.
expect_count 1 ./bitcount.o -e '
	winfo (offset == 0x91) @AT_location (pos == 1)'

# Test multi-yielding value.
expect_count 3 ./bitcount.o -e '
	[winfo ?AT_location] elem (pos == 0) attribute ?AT_location value'

# ?OP_*
expect_count 1 ./bitcount.o -e '
	[winfo @AT_location] elem (pos == 1)
	([?OP_and] length == 1)
	([?OP_or] length == 0)
	([elem ?OP_and] length == 1)
	([elem ?OP_or] length == 0)'

# high, low, address
expect_count 1 ./bitcount.o -e '
	[winfo @AT_location] elem (pos == 1) address
	(high == 0x1001a) (low == 0x10017) (== 65559 65562 arange)'

expect_count 2 ./duplicate-const -e '
	winfo attribute ?AT_high_pc
	(form == DW_FORM_data8)
	(value == 46)
	address (== 0x4004fb) ("%s" == "0x4004fb")'

expect_count 2 ./duplicate-const -e '
	winfo attribute ?AT_low_pc
	(form == DW_FORM_addr)
	address (== value) ("%s" == "0x4004cd")'

expect_count 2 ./duplicate-const -e '
	winfo (low == 0x4004cd) (high == 0x4004fb)'

expect_count 1 ./empty -e '
	10 20 arange
	?(9 !contains) ?(10 ?contains) ?(11 ?contains) ?(12 ?contains)
	?(13 ?contains) ?(14 ?contains) ?(15 ?contains) ?(16 ?contains)
	?(17 ?contains) ?(18 ?contains) ?(19 ?contains) ?(20 !contains)'

expect_count 1 ./empty -e '
	10 20 arange
	?(9 20 arange !contains) ?(10 20 arange ?contains)
	?(11 20 arange ?contains) ?(12 20 arange ?contains)
	?(13 20 arange ?contains) ?(14 20 arange ?contains)
	?(15 20 arange ?contains) ?(16 20 arange ?contains)
	?(17 20 arange ?contains) ?(18 20 arange ?contains)
	?(19 20 arange ?contains) ?(20 20 arange !contains)'

expect_count 1 ./empty -e '
	10 20 arange
	?(9 9 arange !contains) ?(9 10 arange !contains)
	?(9 11 arange !contains) ?(9 12 arange !contains)
	?(9 13 arange !contains) ?(9 14 arange !contains)
	?(9 15 arange !contains) ?(9 16 arange !contains)
	?(9 17 arange !contains) ?(9 18 arange !contains)
	?(9 19 arange !contains) ?(9 20 arange !contains)
	?(9 21 arange !contains)'

expect_count 1 ./empty -e '
	10 20 arange
	?(9 21 arange !contains) ?(10 21 arange !contains)
	?(11 21 arange !contains) ?(12 21 arange !contains)
	?(13 21 arange !contains) ?(14 21 arange !contains)
	?(15 21 arange !contains) ?(16 21 arange !contains)
	?(17 21 arange !contains) ?(18 21 arange !contains)
	?(19 21 arange !contains) ?(20 21 arange !contains)
	?(21 21 arange !contains)'

expect_count 1 ./empty -e '10 20 arange 10 10 arange ?contains'
expect_count 1 ./empty -e '10 20 arange 15 15 arange ?contains'
expect_count 1 ./empty -e '10 20 arange 20 20 arange !contains'
expect_count 1 ./empty -e '10 10 arange dup ?contains'

expect_count 1 ./empty -e '
	5 10 arange
	?(4 5 arange !overlaps) ?(4 6 arange ?overlaps)
	?(4 7 arange ?overlaps) ?(4 8 arange ?overlaps)
	?(4 9 arange ?overlaps) ?(4 10 arange ?overlaps)
	?(4 11 arange ?overlaps) ?(5 11 arange ?overlaps)
	?(6 11 arange ?overlaps) ?(7 11 arange ?overlaps)
	?(8 11 arange ?overlaps) ?(9 11 arange ?overlaps)
	?(10 11 arange !overlaps)'

expect_count 1 ./empty -e '
	5 10 arange !empty
	5 5 arange ?empty'

expect_count 1 ./empty -e '
	(10 20 arange length == 10)
	(10 10 arange length == 0)'

expect_count 1 ./aranges.o -e '
	winfo ?TAG_lexical_block
	([address] length == 2)
	address (pos == 1) (== 0x1000e 0x10015 arange)'

expect_count 1 ./aranges.o -e '
	winfo ?TAG_lexical_block
	([@AT_ranges] length == 2)
	@AT_ranges (pos == 1) (== 0x1000e 0x10015 arange)'

expect_count 1 ./pointer_const_value.o -e '
	winfo @AT_const_value == 0'

expect_count 4 ./float_const_value.o -e '
	winfo
	(?((@AT_name == "fv")
	   (@AT_const_value == [0, 0, 0xc0, 0x3f]))
	 || ?((@AT_name == "dv")
	      (@AT_const_value == [0, 0, 0, 0, 0, 0, 0xf8, 0x3f]))
	 || ?((@AT_name == "ldv")
	      (@AT_const_value == [0, 0, 0, 0, 0, 0, 0, 0xc0,
				   0xff, 0x3f, 0, 0, 0, 0, 0, 0])
	 || ?((@AT_name == "f128v")
	      (@AT_const_value == [0, 0, 0, 0, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0x80, 0xff, 0x3f]))))'

expect_count 1 ./empty -e '
	[1, 2, 3, 4] [3, 4, 5, 6]
	(|A B| A [B elem !(== A elem)] add) == [1, 2, 3, 4, 5, 6]'

echo "$total tests total, $failures failures."
[ $failures -eq 0 ]
