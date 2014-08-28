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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <iostream>
#include <memory>
#include <system_error>
#include <cerrno>

#include "atval.hh"
#include "dwcst.hh"
#include "dwit.hh"
#include "dwpp.hh"
#include "flag_saver.hh"
#include "op.hh"
#include "value-dw.hh"

value_type const value_dwarf::vtype = value_type::alloc ("T_DWARF");

namespace
{
  std::shared_ptr <Dwfl>
  open_dwfl (std::string const &fn)
  {
    int fd = open (fn.c_str (), O_RDONLY);
    if (fd == -1)
      throw std::runtime_error
	(std::error_code (errno, std::system_category ()).message ());

    const static Dwfl_Callbacks callbacks =
      {
	.find_elf = dwfl_build_id_find_elf,
	.find_debuginfo = dwfl_standard_find_debuginfo,
	.section_address = dwfl_offline_section_address,
      };

    auto dwfl = std::shared_ptr <Dwfl> (dwfl_begin (&callbacks), dwfl_end);
    if (dwfl == nullptr)
      throw_libdwfl ();

    dwfl_report_begin (&*dwfl);
    if (dwfl_report_offline (&*dwfl, fn.c_str (), fn.c_str (), fd) == nullptr)
      throw_libdwfl ();
    if (dwfl_report_end (&*dwfl, nullptr, nullptr) != 0)
      throw_libdwfl ();

    return dwfl;
  }
}

value_dwarf::value_dwarf (std::string const &fn, size_t pos)
  : value {vtype, pos}
  , m_fn {fn}
  , m_dwctx {std::make_shared <dwfl_context> (open_dwfl (fn))}
{}

void
value_dwarf::show (std::ostream &o, brevity brv) const
{
  o << "<Dwarf \"" << m_fn << "\">";
}

std::unique_ptr <value>
value_dwarf::clone () const
{
  return std::make_unique <value_dwarf> (*this);
}

cmp_result
value_dwarf::cmp (value const &that) const
{
  if (auto v = value::as <value_dwarf> (&that))
    return compare (m_dwctx->get_dwfl (), v->m_dwctx->get_dwfl ());
  else
    return cmp_result::fail;
}


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
	value_attr {m_dwctx, **it, m_die, 0}.show (o, brevity::full);
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
  auto vpr = at_value (m_dwctx, m_die, m_attr);
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


namespace
{
  void
  show_loclist_op (std::ostream &o, brevity brv,
		   std::shared_ptr <dwfl_context> dwctx,
		   Dwarf_Attribute const &attr, Dwarf_Op *dwop)
  {
    o << dwop->offset << ':'
      << constant {dwop->atom, &dw_locexpr_opcode_dom, brevity::brief};
    {
      auto prod = dwop_number (dwctx, attr, dwop);
      while (auto v = prod->next ())
	{
	  o << "<";
	  v->show (o, brevity::brief);
	  o << ">";
	}
    }

    {
      bool sep = false;
      auto prod = dwop_number2 (dwctx, attr, dwop);
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
}

value_type const value_loclist_elem::vtype
	= value_type::alloc ("T_LOCLIST_ELEM");

void
value_loclist_elem::show (std::ostream &o, brevity brv) const
{
  {
    ios_flag_saver s {o};
    o << std::hex << std::showbase << m_low << ".." << m_high;
  }

  o << ":[";
  for (size_t i = 0; i < m_exprlen; ++i)
    {
      if (i > 0)
	o << ", ";
      show_loclist_op (o, brv, m_dwctx, m_attr, m_expr + i);
    }
  o << "]";
}

std::unique_ptr <value>
value_loclist_elem::clone () const
{
  return std::make_unique <value_loclist_elem> (*this);
}

cmp_result
value_loclist_elem::cmp (value const &that) const
{
  if (auto v = value::as <value_loclist_elem> (&that))
    {
      auto ret = compare (m_attr.valp, v->m_attr.valp);
      if (ret != cmp_result::equal)
	return ret;

      auto r = compare (std::make_tuple (m_low, m_high, m_exprlen),
			std::make_tuple (v->m_low, v->m_high, v->m_exprlen));
      if (r != cmp_result::equal)
	return r;

      for (size_t i = 0; i < m_exprlen; ++i)
	{
	  Dwarf_Op *a = m_expr + i;
	  Dwarf_Op *b = v->m_expr + i;
	  r = compare (std::make_tuple (a->atom, a->number,
					a->number2, a->offset),
		       std::make_tuple (b->atom, b->number,
					b->number2, b->offset));
	  if (r != cmp_result::equal)
	    return r;
	}

      return cmp_result::equal;
    }
  else
    return cmp_result::fail;
}


value_type const value_addr_range::vtype = value_type::alloc ("T_ADDR_RANGE");

void
value_addr_range::show (std::ostream &o, brevity brv) const
{
  ios_flag_saver s {o};
  o << m_low << ".." << m_high;
}

std::unique_ptr <value>
value_addr_range::clone () const
{
  return std::make_unique <value_addr_range> (*this);
}

cmp_result
value_addr_range::cmp (value const &that) const
{
  if (auto v = value::as <value_addr_range> (&that))
    return compare (std::make_tuple (m_low, m_high),
		    std::make_tuple (v->m_low, v->m_high));
  else
    return cmp_result::fail;
}


value_type const value_loclist_op::vtype = value_type::alloc ("T_LOCLIST_OP");

void
value_loclist_op::show (std::ostream &o, brevity brv) const
{
  show_loclist_op (o, brv, m_dwctx, m_attr, m_dwop);
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
