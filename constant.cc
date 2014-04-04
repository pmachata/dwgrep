#include <sstream>
#include <iostream>
#include "constant.hh"

void
signed_constant_dom_t::show (uint64_t v, std::ostream &o) const
{
  o << static_cast <int64_t> (v);
}

bool
signed_constant_dom_t::sign () const
{
  return true;
}


void
unsigned_constant_dom_t::show (uint64_t v, std::ostream &o) const
{
  o << v;
}

bool
unsigned_constant_dom_t::sign () const
{
  return false;
}

signed_constant_dom_t const signed_constant_dom;
unsigned_constant_dom_t const unsigned_constant_dom;

std::ostream &
operator<< (std::ostream &o, constant cst)
{
  cst.dom ()->show (cst.value (), o);
  return o;
}

bool
constant::operator< (constant that) const
{
  // For two different domains, complain about comparisons that don't
  // have at least one comparand signed_constant_dom or
  // unsigned_constant_dom.
  if (dom () != that.dom ()
      && dom () != &signed_constant_dom
      && dom () != &unsigned_constant_dom
      && that.dom () != &signed_constant_dom
      && that.dom () != &unsigned_constant_dom)
    std::cerr << "Warning: comparing " << *this << " to " << that
	      << " is probably not meaningful (different domains).\n";

  if (dom ()->sign () == that.dom ()->sign ())
    return m_value < that.m_value;

  if (dom ()->sign ())
    {
      if (static_cast <int64_t> (m_value) < 0)
	return true;
      else
	return m_value < that.m_value;
    }
  else
    {
      if (static_cast <int64_t> (that.m_value) < 0)
	return false;
      else
	return m_value < that.m_value;
    }
}

bool
constant::operator> (constant that) const
{
  return that < *this;
}

bool
constant::operator== (constant that) const
{
  return ! (*this != that);
}

bool
constant::operator!= (constant that) const
{
  return *this < that || that < *this;
}
