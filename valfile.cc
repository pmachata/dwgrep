#include <iostream>

#include "valfile.hh"
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
}

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
  if (auto v = dynamic_cast <value_cst const *> (&that))
    return compare (m_cst, v->m_cst);
  else
    return cmp_result::fail;
}


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
  if (auto v = dynamic_cast <value_str const *> (&that))
    return compare (m_str, v->m_str);
  else
    return cmp_result::fail;
}


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
  if (auto v = dynamic_cast <value_die const *> (&that))
    return compare (dwarf_dieoffset ((Dwarf_Die *) &m_die),
		    dwarf_dieoffset ((Dwarf_Die *) &v->m_die));
  else
    return cmp_result::fail;
}


void
value_attr::show (std::ostream &o) const
{
  unsigned name = (unsigned) dwarf_whatattr ((Dwarf_Attribute *) &m_attr);
  unsigned form = dwarf_whatform ((Dwarf_Attribute *) &m_attr);
  o << constant (name, &dw_attr_short_dom) << " ("
    << constant (form, &dw_form_short_dom) << ")\t";
  auto v = at_value (m_attr, m_die);
  if (auto d = dynamic_cast <value_die const *> (v.get ()))
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
  if (auto v = dynamic_cast <value_attr const *> (&that))
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


valfile::valfile (valfile const &that)
{
  for (auto const &vf: that.m_values)
    if (vf != nullptr)
      m_values.push_back (vf->clone ());
    else
      m_values.push_back (nullptr);
}

namespace
{
  int
  compare_vf (std::vector <std::unique_ptr <value> > const &a,
	      std::vector <std::unique_ptr <value> > const &b)
  {
    // There shouldn't really be stacks of different sizes in one
    // program.
    assert (a.size () == b.size ());

    // The stack that has nullptr where the other has non-nullptr is
    // smaller.
    {
      auto it = a.begin ();
      auto jt = b.begin ();
      for (; it != a.end (); ++it, ++jt)
	if (*it == nullptr && *jt != nullptr)
	  return -1;
	else if (*it != nullptr && *jt == nullptr)
	  return 1;
    }

    // The stack with smaller type_info addresses is smaller.
    {
      auto it = a.begin ();
      auto jt = b.begin ();
      for (; it != a.end (); ++it, ++jt)
	if (*it != nullptr && *jt != nullptr)
	  {
	    if (&typeid (**it) < &typeid (**jt))
	      return -1;
	    else if (&typeid (**it) > &typeid (**jt))
	      return 1;
	  }
    }

    // We have the same number of slots with values of the same type.
    // Now compare the values directly.
    {
      auto it = a.begin ();
      auto jt = b.begin ();
      for (; it != a.end (); ++it, ++jt)
	if (*it != nullptr && *jt != nullptr)
	  switch ((*it)->cmp (**jt))
	    {
	    case cmp_result::fail:
	      assert (! "Comparison of same-typed slots shouldn't fail!");
	      abort ();
	    case cmp_result::less:
	      return -1;
	    case cmp_result::greater:
	      return 1;
	    case cmp_result::equal:
	      break;
	    }
    }

    // The stacks are the same!
    return 0;
  }
}

bool
valfile::operator< (valfile const &that) const
{
  return compare_vf (m_values, that.m_values) < 0;
}

bool
valfile::operator== (valfile const &that) const
{
  return compare_vf (m_values, that.m_values) == 0;
}
