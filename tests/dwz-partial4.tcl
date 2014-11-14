# This test case uses dwarf.exp from GDB.

set srcdir {..}
set hex "0\[xX\]\[0-9a-fA-F\]+"
set decimal "\[0-9\]+"

source {dwarf.exp}

Dwarf::assemble "dwz-partial4-C.s" {
    build_id 0102030405060708

    cu {is_64 0 version 4 addr_size 8} {
	partial_unit {} {
	    subprogram {} {
		formal_parameter {} {
		}
	    }
	}
    }
}

Dwarf::assemble "dwz-partial4-1.s" {
    gnu_debugaltlink "dwz-partial4-C" 0102030405060708

    cu {is_64 0 version 4 addr_size 8} {
	compile_unit {} {
	    structure_type {} {}
	    structure_type {} {}
	}
    }
}

# tclsh dwz-partial4.tcl
# gcc -c dwz-partial4-1.s
# gcc -c dwz-partial4-C.s -o dwz-partial4-C
