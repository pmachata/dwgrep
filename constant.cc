#include <sstream>
#include "constant.hh"

void
untyped_constant_dom_t::format (constant c, std::ostream &o) const
{
  o << c.value ();
}

untyped_constant_dom_t const untyped_constant_dom;
