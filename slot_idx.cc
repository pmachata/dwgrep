#include <iostream>
#include "slot_idx.hh"

std::ostream &
operator<< (std::ostream &o, slot_idx const &idx)
{
  return o << static_cast <unsigned> (idx);
}