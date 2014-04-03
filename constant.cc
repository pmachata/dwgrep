#include <sstream>
#include "constant.hh"

void
signed_constant_dom_t::show (constant c, std::ostream &o) const
{
  o << ((int64_t) c.value ());
}

void
unsigned_constant_dom_t::show (constant c, std::ostream &o) const
{
  o << c.value ();
}

signed_constant_dom_t const signed_constant_dom;
unsigned_constant_dom_t const unsigned_constant_dom;

std::ostream &
operator<< (std::ostream &o, constant cst)
{
  cst.dom ()->show (cst, o);
  return o;
}
