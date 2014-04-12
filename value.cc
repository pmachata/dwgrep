#include <iostream>

#include "value.hh"
#include "make_unique.hh"
#include "dwcst.hh"
#include "dwit.hh"
#include "vfcst.hh"
#include "atval.hh"

namespace
{
  template <class T>
  cmp_result
  compare (T const &a, T const &b)
  {
    if (a < b)
      return cmp_result::less;
    if (b < a)
      return cmp_result::greater;
    return cmp_result::equal;
  }

  value_type
  alloc_vtype ()
  {
    static uint8_t last = 0;
    value_type vt {last++};
    if (last == 0)
      {
	std::cerr << "Ran out of value type identifiers." << std::endl;
	std::terminate ();
      }
    return vt;
  }
}

value_type const value_type::none = alloc_vtype ();

value_type const value_cst::vtype = alloc_vtype ();

void
value_cst::show (std::ostream &o) const
{
  o << m_cst;
}

std::unique_ptr <value>
value_cst::clone () const
{
  return std::make_unique <value_cst> (m_cst);
}

constant
value_cst::get_type_const () const
{
  return {(int) slot_type_id::T_CONST, &slot_type_dom};
}

cmp_result
value_cst::cmp (value const &that) const
{
  if (auto v = value::as <value_cst> (&that))
    return compare (m_cst, v->m_cst);
  else
    return cmp_result::fail;
}


value_type const value_str::vtype = alloc_vtype ();

void
value_str::show (std::ostream &o) const
{
  o << m_str;
}

std::unique_ptr <value>
value_str::clone () const
{
  return std::make_unique <value_str> (std::string (m_str));
}

constant
value_str::get_type_const () const
{
  return {(int) slot_type_id::T_STR, &slot_type_dom};
}

cmp_result
value_str::cmp (value const &that) const
{
  if (auto v = value::as <value_str> (&that))
    return compare (m_str, v->m_str);
  else
    return cmp_result::fail;
}


value_type const value_seq::vtype = alloc_vtype ();

void
value_seq::show (std::ostream &o) const
{
  o << "[";
  bool seen = false;
  for (auto const &v: m_seq)
    {
      if (seen)
	o << ", ";
      seen = true;
      v->show (o);
    }
  o << "]";
}

std::unique_ptr <value>
value_seq::clone () const
{
  seq_t vv2;
  for (auto const &v: m_seq)
    vv2.emplace_back (std::move (v->clone ()));
  return std::make_unique <value_seq> (std::move (vv2));
}

constant
value_seq::get_type_const () const
{
  return {(int) slot_type_id::T_SEQ, &slot_type_dom};
}

cmp_result
value_seq::cmp (value const &that) const
{
  assert (! "value_seq::cmp NIY");
  abort ();
}


value_type const value_die::vtype = alloc_vtype ();

void
value_die::show (std::ostream &o) const
{
  std::ios::fmtflags f {o.flags ()};

  Dwarf_Die *die = const_cast <Dwarf_Die *> (&m_die);
  o << "[" << std::hex << dwarf_dieoffset (die) << "]\t"
    << constant (dwarf_tag (die), &dw_tag_short_dom);

  for (auto it = attr_iterator {die}; it != attr_iterator::end (); ++it)
    {
      o << "\n\t";
      value_attr {**it, m_die}.show (o);
    }

  o.flags (f);
}

std::unique_ptr <value>
value_die::clone () const
{
  return std::make_unique <value_die> (m_die);
}

constant
value_die::get_type_const () const
{
  return {(int) slot_type_id::T_NODE, &slot_type_dom};
}

cmp_result
value_die::cmp (value const &that) const
{
  if (auto v = value::as <value_die> (&that))
    return compare (dwarf_dieoffset ((Dwarf_Die *) &m_die),
		    dwarf_dieoffset ((Dwarf_Die *) &v->m_die));
  else
    return cmp_result::fail;
}


value_type const value_attr::vtype = alloc_vtype ();

void
value_attr::show (std::ostream &o) const
{
  unsigned name = (unsigned) dwarf_whatattr ((Dwarf_Attribute *) &m_attr);
  unsigned form = dwarf_whatform ((Dwarf_Attribute *) &m_attr);
  o << constant (name, &dw_attr_short_dom) << " ("
    << constant (form, &dw_form_short_dom) << ")\t";
  auto v = at_value (m_attr, m_die);
  if (auto d = value::as <value_die> (v.get ()))
    o << "[" << dwarf_dieoffset ((Dwarf_Die *) &d->get_die ()) << "]";
  else
    v->show (o);
}

std::unique_ptr <value>
value_attr::clone () const
{
  return std::make_unique <value_attr> (m_attr, m_die);
}

constant
value_attr::get_type_const () const
{
  return {(int) slot_type_id::T_ATTR, &slot_type_dom};
}

cmp_result
value_attr::cmp (value const &that) const
{
  if (auto v = value::as <value_attr> (&that))
    {
      Dwarf_Off a = dwarf_dieoffset ((Dwarf_Die *) &m_die);
      Dwarf_Off b = dwarf_dieoffset ((Dwarf_Die *) &v->m_die);
      if (a != b)
	return compare (a, b);
      else
	return compare (dwarf_whatattr ((Dwarf_Attribute *) &m_attr),
			dwarf_whatattr ((Dwarf_Attribute *) &v->m_attr));
    }
  else
    return cmp_result::fail;
}
