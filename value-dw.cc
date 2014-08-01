/*
   Copyright (C) 2014 Red Hat, Inc.
   This file is part of dwgrep.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   dwgrep is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include "flag_saver.hh"
#include <iostream>
#include <memory>
#include "make_unique.hh"

#include "value-dw.hh"
#include "dwcst.hh"
#include "dwit.hh"
#include "atval.hh"

value_type const value_die::vtype = value_type::alloc ("T_DIE");

void
value_die::show (std::ostream &o, brevity brv) const
{
  ios_flag_saver fs {o};

  Dwarf_Die *die = const_cast <Dwarf_Die *> (&m_die);
  o << '[' << std::hex << dwarf_dieoffset (die) << ']'
    << (brv == brevity::full ? '\t' : ' ')
    << constant (dwarf_tag (die), &dw_tag_dom, brevity::brief);

  if (brv == brevity::full)
    for (auto it = attr_iterator {die}; it != attr_iterator::end (); ++it)
      {
	o << "\n\t";
	value_attr {m_gr, **it, m_die, 0}.show (o, brevity::full);
      }
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
value_attr::show (std::ostream &o, brevity brv) const
{
  unsigned name = (unsigned) dwarf_whatattr ((Dwarf_Attribute *) &m_attr);
  unsigned form = dwarf_whatform ((Dwarf_Attribute *) &m_attr);

  ios_flag_saver s {o};
  o << constant (name, &dw_attr_dom, brevity::brief) << " ("
    << constant (form, &dw_form_dom, brevity::brief) << ")\t";
  auto vpr = at_value (m_attr, m_die, m_gr);
  while (auto v = vpr->next ())
    {
      if (auto d = value::as <value_die> (v.get ()))
	o << "[" << std::hex
	  << dwarf_dieoffset ((Dwarf_Die *) &d->get_die ()) << "]";
      else
	v->show (o, brv);
      o << ";";
    }
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

value_type const value_loclist_op::vtype = value_type::alloc ("T_LOCLIST_OP");

void
value_loclist_op::show (std::ostream &o, brevity brv) const
{
  o << m_dwop->offset << ':'
    << constant {m_dwop->atom, &dw_locexpr_opcode_dom, brevity::brief};
  {
    auto prod = dwop_number (m_dwop, m_attr, m_gr);
    while (auto v = prod->next ())
      {
	o << "<";
	v->show (o, brevity::brief);
	o << ">";
      }
  }

  {
    bool sep = false;
    auto prod = dwop_number2 (m_dwop, m_attr, m_gr);
    while (auto v = prod->next ())
      {
	if (! sep)
	  {
	    o << "/";
	    sep = true;
	  }

	o << "<";
	v->show (o, brevity::brief);
	o << ">";
      }
  }
}

std::unique_ptr <value>
value_loclist_op::clone () const
{
  return std::make_unique <value_loclist_op> (*this);
}

cmp_result
value_loclist_op::cmp (value const &that) const
{
  if (auto v = value::as <value_loclist_op> (&that))
    {
      auto ret = compare (m_attr.valp, v->m_attr.valp);
      if (ret != cmp_result::equal)
	return ret;

      return compare (m_dwop->offset, v->m_dwop->offset);
    }
  else
    return cmp_result::fail;
}
