# This test case uses dwarf.exp from GDB.
#   - But with the following patch applied:
#     https://sourceware.org/ml/gdb-patches/2015-02/msg00456.html
#
# GDB_SRCPATH=/path/to/gdb/source tclsh imported-AT_decl_file.tcl
# gcc -c imported-AT_decl_file.s

source "dwarf.exp"

Dwarf::assemble "imported-AT_decl_file.s" {
    build_id 0102030405060708

    cu {is_64 0 version 4 addr_size 8} {
	declare_labels a
	DW_TAG_compile_unit {
	    {MACRO_AT_stmt_list {
		{include "foo"}
		{file_name "foo.c" 1}
	    }}
	} {
	    a: DW_TAG_subprogram {
		{DW_AT_decl_file 1 DW_FORM_data1}
	    }
	}
    }

    cu {is_64 0 version 4 addr_size 8} {
	DW_TAG_compile_unit {} {
	    DW_TAG_subprogram {
		{DW_AT_specification $a DW_FORM_ref_addr}
	    }
	}
    }
}
