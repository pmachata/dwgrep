# This test case uses dwarf.exp from GDB.
# GDB_SRCPATH=/path/to/gdb/source tclsh entry_dwarf_counts_every_unit_anew.tcl
# gcc -c entry_dwarf_counts_every_unit_anew.s

source "dwarf.exp"

Dwarf::assemble "entry_dwarf_counts_every_unit_anew.s" {
    build_id 0102030405060708

    cu {is_64 0 version 4 addr_size 8} {
	compile_unit {} {}
    }
    cu {is_64 0 version 4 addr_size 8} {
	compile_unit {} {}
    }
}
