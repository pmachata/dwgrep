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

  bool
  safe_arith () const override
  {
    return true;
  }

  bool plain () const override
  {
    return true;
  }
} signed_constant_dom_obj;

constant_dom const &signed_constant_dom = signed_constant_dom_obj;


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

bool
unsigned_constant_dom_t::safe_arith () const
{
  return true;
}

bool
unsigned_constant_dom_t::plain () const
{
  return true;
}

unsigned_constant_dom_t unsigned_constant_dom_obj;
constant_dom const &unsigned_constant_dom = unsigned_constant_dom_obj;

unsigned_constant_dom_t column_number_dom_obj;
constant_dom const &column_number_dom = column_number_dom_obj;

unsigned_constant_dom_t line_number_dom_obj;
constant_dom const &line_number_dom = line_number_dom_obj;


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

  bool
  safe_arith () const override
  {
    return true;
  }
} hex_constant_dom_obj;

constant_dom const &hex_constant_dom = hex_constant_dom_obj;


static struct
  : public constant_dom
{
  void
  show (uint64_t v, std::ostream &o) const override
  {
    std::ios::fmtflags f {o.flags ()};
    o << (v > 0 ? "0" : "") << std::oct << v;
    o.flags (f);
  }

  bool
  sign () const override
  {
    return false;
  }

  bool
  safe_arith () const override
  {
    return true;
  }
} oct_constant_dom_obj;

constant_dom const &oct_constant_dom = oct_constant_dom_obj;


static struct
  : public constant_dom
{
  void
  show (uint64_t v, std::ostream &o) const override
  {
    std::ios::fmtflags f {o.flags ()};
    o << std::boolalpha << static_cast <bool> (v);
    o.flags (f);
  }

  bool
  sign () const override
  {
    return false;
  }
} bool_constant_dom_obj;

constant_dom const &bool_constant_dom = bool_constant_dom_obj;


std::ostream &
operator<< (std::ostream &o, constant cst)
{
  cst.dom ()->show (cst.value (), o);
  return o;
}

bool
constant::operator< (constant that) const
{
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
