# This test case uses dwarf.exp from GDB.
# GDB_SRCPATH=/path/to/gdb/source tclsh attribute-die-cooked-no-dup.tcl
# gcc -c attribute-die-cooked-no-dup.s

source "dwarf.exp"

Dwarf::assemble "attribute-die-cooked-no-dup.s" {
    build_id 0102030405060708

    cu {is_64 0 version 4 addr_size 8} {
	declare_labels a b c d

	compile_unit {} {
	    a: subprogram {}
	    b: subprogram {
		{specification :$a}
	    }
	    c: subprogram {
		{specification :$b}
	    }
	    d: subprogram {
		{abstract_origin :$c}
	    }
	    GNU_call_site {
		{abstract_origin :$d}
	    }
	}
    }
}
