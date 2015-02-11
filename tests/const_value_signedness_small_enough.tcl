# This test case uses dwarf.exp from GDB.
# GDB_SRCPATH=/path/to/gdb/source tclsh const_value_signedness_small_enough.tcl
# gcc -c const_value_signedness_small_enough.s

source "dwarf.exp"

Dwarf::assemble "const_value_signedness_small_enough.s" {
    build_id 0102030405060708

    cu {is_64 0 version 4 addr_size 8} {
	declare_labels a

	DW_TAG_compile_unit {} {
	    DW_TAG_class_type {} {
		DW_TAG_template_value_param {
		    {DW_AT_type :$a}
		    {DW_AT_const_value 0 DW_FORM_data1}
		}
	    }
	    a: DW_TAG_enumeration_type {} {
		DW_TAG_enumerator {
		    {DW_AT_const_value 0 DW_FORM_data1}
		}
	    }
	}
    }
}
