#include <sstream>
#include "constant.hh"

void
untyped_constant_dom_t::show (constant c, std::ostream &o) const
{
  o << c.value ();
}

untyped_constant_dom_t const untyped_constant_dom;

std::ostream &
operator<< (std::ostream &o, constant cst)
{
  cst.dom ()->show (cst, o);
  return o;
}
