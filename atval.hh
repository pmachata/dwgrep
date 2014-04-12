#ifndef _ATVAL_H_
#define _ATVAL_H_

#include <libdw.h>

class value;

// Obtain a value of ATTR at DIE.
std::unique_ptr <value> at_value (Dwarf_Attribute attr, Dwarf_Die die);

#endif /* _ATVAL_H_ */
