#include <sstream>
#include <iostream>
#include "constant.hh"

static struct
  : public constant_dom
{
  void
  show (uint64_t v, std::ostream &o) const override
  {
    o << static_cast <int64_t> (v);
  }

  bool
  sign () const override
  {
    return true;
  }
} signed_constant_dom_obj;

constant_dom const &signed_constant_dom = signed_constant_dom_obj;


static struct
  : public constant_dom
{
  void
  show (uint64_t v, std::ostream &o) const override
  {
    o << v;
  }

  bool
  sign () const override
  {
    return false;
  }
} unsigned_constant_dom_obj;

constant_dom const &unsigned_constant_dom = unsigned_constant_dom_obj;


static struct
  : public constant_dom
{
  void
  show (uint64_t v, std::ostream &o) const override
  {
    std::ios::fmtflags f {o.flags ()};
    o << (v > 0 ? "0x" : "") << std::hex << v;
    o.flags (f);
  }

  bool
  sign () const override
  {
    return false;
  }
} hex_constant_dom_obj;

constant_dom const &hex_constant_dom = hex_constant_dom_obj;


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
