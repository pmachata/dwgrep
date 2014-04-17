#ifndef _ATVAL_H_
#define _ATVAL_H_

#include <libdw.h>
#include "dwgrep.hh"

class value;

// Obtain a value of ATTR at DIE.
std::unique_ptr <value> at_value (Dwarf_Attribute attr, Dwarf_Die die,
				  dwgrep_graph::sptr gr);

#endif /* _ATVAL_H_ */
