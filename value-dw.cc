#include <iostream>
#include <memory>
#include "make_unique.hh"

#include "value-dw.hh"
#include "dwcst.hh"
#include "dwit.hh"
#include "atval.hh"

value_type const value_die::vtype = value_type::alloc ("T_DIE");

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
      o.flags (f);
      value_attr {m_gr, **it, m_die, 0}.show (o);
    }

  o.flags (f);
}

std::unique_ptr <value>
value_die::clone () const
{
  return std::make_unique <value_die> (*this);
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


value_type const value_attr::vtype = value_type::alloc ("T_ATTR");

void
value_attr::show (std::ostream &o) const
{
  unsigned name = (unsigned) dwarf_whatattr ((Dwarf_Attribute *) &m_attr);
  unsigned form = dwarf_whatform ((Dwarf_Attribute *) &m_attr);
  std::ios::fmtflags f {o.flags ()};
  o << constant (name, &dw_attr_short_dom) << " ("
    << constant (form, &dw_form_short_dom) << ")\t";
  auto vpr = at_value (m_attr, m_die, m_gr);
  while (auto v = vpr->next ())
    {
      if (auto d = value::as <value_die> (v.get ()))
	o << "[" << std::hex
	  << dwarf_dieoffset ((Dwarf_Die *) &d->get_die ()) << "]";
      else
	v->show (o);
      o << ";";
    }
  o.flags (f);
}

std::unique_ptr <value>
value_attr::clone () const
{
  return std::make_unique <value_attr> (*this);
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
