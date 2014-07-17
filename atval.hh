#ifndef _ATVAL_H_
#define _ATVAL_H_

#include <libdw.h>
#include "dwgrep.hh"

class value;

struct value_producer
{
  virtual ~value_producer () {}

  // Produce next value.
  virtual std::unique_ptr <value> next () = 0;
};

// Obtain a value of ATTR at DIE.
std::unique_ptr <value_producer> at_value (Dwarf_Attribute attr, Dwarf_Die die,
					   dwgrep_graph::sptr gr);

#endif /* _ATVAL_H_ */
