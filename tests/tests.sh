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

expect_count 1 ./duplicate-const -e '
	{?TAG_const_type, ?TAG_volatile_type, ?TAG_restrict_type} ->?cvr_type;
	winfo ->P;
	P child ?cvr_type ->A;
	P child ?cvr_type ?(?lt: A) ->B;
	?((A label) ?eq: (B label))
	?((A @AT_type) ?eq: (B @AT_type))'

expect_count 1 ./nontrivial-types.o -e '
	winfo ?TAG_subprogram !AT_declaration dup child ?TAG_formal_parameter
	?(@AT_type ((?TAG_const_type,?TAG_volatile_type,?TAG_typedef) @AT_type)*
	  (?TAG_structure_type, ?TAG_class_type))'

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
	winfo ->A; A unit ->B; (A pos == B pos)'

# Test a bug with scope promotion.
expect_count 1 ./empty -e '
	1->A; ?(A->X;)'

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
	?([1, 3, 5] elem ?(pos ?0) (== 1))
	?([1, 3, 5] elem ?(pos ?1) (== 3))
	?([1, 3, 5] elem ?(pos ?2) (== 5))'
expect_count 1 ./empty -e '
	?([[1, 3, 5] elem ?(pos !0) ((3,5) ==)] length ?2)
	?([[1, 3, 5] elem ?(pos !1) ((1,5) ==)] length ?2)
	?([[1, 3, 5] elem ?(pos !2) ((1,3) ==)] length ?2)'

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

# Test match operator
expect_count 7 ./duplicate-const -e '
	winfo ?(@AT_decl_file "" ?match)'
expect_count 7 ./duplicate-const -e '
	winfo ?(@AT_decl_file ".*petr.*" ?match)'
expect_count 1 ./nontrivial-types.o -e '
	winfo ?(@AT_language "%s" "DW_LANG_C89" ?match)'
expect_count 1 ./nontrivial-types.o -e '
	winfo ?(@AT_encoding "%s" "^DW_ATE_signed$" ?match)'

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
	(0, 1, 20) (?10 || ?20)'

# Check closures.
expect_count 1 ./empty -e '
	{->A; {->B; A}} 2 swap apply 9 swap apply (== 1 1 add)'
expect_count 1 ./empty -e '
	{->A; {A}} 5 swap apply apply (== 2 3 add)'
expect_count 1 ./empty -e '
	{dup add} -> double; 1 double (== 2)'
expect_count 1 ./empty -e '
	{->x; {->y; x y add}} ->adder;
	3 adder 2 swap apply (== 5)'
expect_count 1 ./empty -e '
	{->L f; [ L elem f ] } ->map;
	[1, 2, 3] {1 add} map ?([2, 3, 4] ?eq)'
expect_count 1 ./empty -e '
	{->L B E;
	  {->V;
	    if (V 0 ?ge) then (V) else (L length V add) ->X;
	    if (X 0 ?lt) then (0) else X
	  } ->wrap;
	  B wrap ->begin;    E wrap ->end;
	  [L elem ?(pos ?(begin ?ge) ?(end ?lt))]
	} -> slice;
	[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
	?(1 5 slice [1, 2, 3, 4] ?eq)
	?(5 -1 slice [5, 6, 7, 8] ?eq)
	?(-2 -1 slice [8] ?eq)'

# Check that bindings remember position.
expect_count 3 ./empty -e '
	[0, 1, 2] elem ->E; E dup pos ?eq'

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

expect_count 1 ./duplicate-const -e '
	"%([winfo label]%)"
	"[DW_TAG_compile_unit, DW_TAG_subprogram, DW_TAG_variable, "\
	"DW_TAG_variable, DW_TAG_variable, DW_TAG_variable, "\
	"DW_TAG_base_type, DW_TAG_pointer_type, DW_TAG_const_type, "\
	"DW_TAG_base_type, DW_TAG_array_type, DW_TAG_subrange_type, "\
	"DW_TAG_base_type, DW_TAG_const_type, DW_TAG_variable, "\
	"DW_TAG_variable, DW_TAG_const_type]" ?eq
	"%([winfo label]%)" ?eq'

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
	winfo ?(offset ?0x91) @AT_location ?(pos ?1)'

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
