#include <sstream>
#include "cst.hh"

void
untyped_cst_dom_t::format (cst c, std::ostream &o) const
{
  o << c.value ();
}

untyped_cst_dom_t const untyped_cst_dom;
