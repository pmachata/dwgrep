/*
   Copyright (C) 2018 Petr Machata
   Copyright (C) 2014, 2015 Red Hat, Inc.
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

#include <iostream>
#include <memory>

#include "atval.hh"
#include "dwcst.hh"
#include "dwit.hh"
#include "dwpp.hh"
#include "flag_saver.hh"
#include "op.hh"
#include "value-dw.hh"
#include "cache.hh"

value_type const value_dwarf::vtype = value_type::alloc ("T_DWARF",
R"docstring(

Values of this type represent opened Dwarf files::

	$ dwgrep ./tests/a1.out -e ''
	<Dwarf "./tests/a1.out">

	$ dwgrep '"tests/a1.out" dwopen'
	<Dwarf "tests/a1.out">

If a given file contains .gnu_debugaltlink, it is subsumed by the
Dwarf handle as well.

Values of type Dwarf (as well as many other Dwarf-related Zwerg
values) come in two flavors: cooked and raw.  Raw values generally
present the underlying bits faithfully, cooked ones do some amount of
interpretation.  For example, cooked DIE's merge
``DW_TAG_imported_unit`` nodes, and thus a given node may be presented
as having more children than the underlying bits suggest.  The actual
ways in which the interpretation differs between raw and cooked are
described at each word that makes the distinction.

Two words are used for switching Dwarf back and forth: ``raw`` and
``cooked``.

)docstring");

namespace
{
  const Dwfl_Callbacks callbacks =
    {
      .find_elf = dwfl_build_id_find_elf,
      .find_debuginfo = dwfl_standard_find_debuginfo,
      .section_address = dwfl_offline_section_address,
    };
}

value_dwarf::value_dwarf (std::string const &fn, size_t pos, doneness d)
  : value {vtype, pos}
  , doneness_aspect {d}
  , m_fn {fn}
  , m_dwctx {std::make_shared <dwfl_context> (open_dwfl (fn, callbacks))}
{}

value_dwarf::value_dwarf (std::string const &fn,
			  std::shared_ptr <dwfl_context> dwctx,
			  size_t pos, doneness d)
  : value {vtype, pos}
  , doneness_aspect {d}
  , m_fn {fn}
  , m_dwctx {dwctx}
{}

void
value_dwarf::show (std::ostream &o) const
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


value_type const value_cu::vtype = value_type::alloc ("T_CU",
R"docstring(

Values of this type represent compile units, partial units and type
units found in Dwarf files::

	$ dwgrep tests/dwz-partial2-1 -e 'unit'
	CU 0x1f
	CU 0x8a
	CU 0xdc

)docstring");

void
value_cu::show (std::ostream &o) const
{
  ios_flag_saver s {o};
  o << "CU " << std::hex << std::showbase << m_offset;
}

std::unique_ptr <value>
value_cu::clone () const
{
  return std::make_unique <value_cu> (*this);
}

cmp_result
value_cu::cmp (value const &that) const
{
  if (auto v = value::as <value_cu> (&that))
    return compare (&m_cu, &v->m_cu);
  else
    return cmp_result::fail;
}


namespace
{
  template <class T>
  T &
  unconst (T const &t)
  {
    return const_cast <T &> (t);
  }
}

value_type const value_die::vtype = value_type::alloc ("T_DIE",
R"docstring(

Values of this type represent debug info entries, or DIE's, found in
CU's::

	$ dwgrep ./tests/a1.out -e 'unit root'
	[b]	compile_unit
		producer (GNU_strp_alt)	GNU C 4.6.3 20120306 (Red Hat 4.6.3-2);
		language (data1)	DW_LANG_C89;
		name (GNU_strp_alt)	foo.c;
		comp_dir (GNU_strp_alt)	/home/petr/proj/dwgrep;
		low_pc (addr)	0x4004b2;
		high_pc (addr)	0x4004b8;
		stmt_list (data4)	0;

)docstring");

void
value_die::show (std::ostream &o) const
{
  Dwarf_Die *die = &unconst (m_die);
  ios_flag_saver fs {o};
  o << '[' << std::hex << dwarf_dieoffset (die) << "] "
    << constant (dwarf_tag (die), &dw_tag_dom (), brevity::brief);
}

cmp_result
value_die::cmp (value const &that) const
{
  if (auto v = value::as <value_die> (&that))
    {
      {
	auto ret = compare (dwarf_cu_getdwarf (m_die.cu),
			    dwarf_cu_getdwarf (v->m_die.cu));
	if (ret != cmp_result::equal)
	  return ret;
      }

      {
	auto ret = compare (dwarf_dieoffset ((Dwarf_Die *) &m_die),
			    dwarf_dieoffset ((Dwarf_Die *) &v->m_die));
	if (ret != cmp_result::equal)
	  return ret;

	// If import paths are different, then each DIE comes from a
	// different part of the tree and they are logically
	// different.  But if one of DIE's has an import path and the
	// other does not, the other is in a sense a template that
	// describes potentially several DIEs.  If one of the DIE's is
	// raw, its import path (if any) is ignored.
	if (is_raw () || m_import == nullptr
	    || v->is_raw () || v->m_import == nullptr)
	  return ret;
      }

      assert (m_import != nullptr);
      assert (v->m_import != nullptr);

      // Explore recursively.
      return m_import->cmp (*v->m_import);
    }
  else
    return cmp_result::fail;
}

namespace
{
  bool
  get_parent (value_die const &value, Dwarf_Die &ret)
  {
    Dwarf_Off par_off = value.get_dwctx ()->find_parent (value.get_die ());
    if (par_off == parent_cache::no_off)
      return false;

    if (dwarf_offdie (dwarf_cu_getdwarf (value.get_die ().cu),
		      par_off, &ret) == nullptr)
      throw_libdw ();

    return true;
  }

  std::unique_ptr <value_die>
  fetch_parent_die (value_die const *a)
  {
    assert (a != nullptr);
    doneness d = a->get_doneness ();

    // Both cooked and raw DIE's have parents (unless they don't, in
    // which case we are already at root).  But for cooked DIE's,
    // when the parent is partial unit root, we need to traverse
    // further along the import chain.
    Dwarf_Die par_die;
    do
      if (! get_parent (*a, par_die))
	return nullptr;
    while (d == doneness::cooked
	   // Import another partial unit if possible, and keep
	   // looking for the actual parent.
	   && dwarf_tag (&par_die) == DW_TAG_partial_unit
	   && a->get_import () != nullptr
	   && (a = a->get_import ().get ()));

    return std::make_unique <value_die> (a->get_dwctx (), par_die, 0, d);
  }
}

std::unique_ptr <value_die>
value_die::get_parent () const
{
  return fetch_parent_die (this);
}


value_type const value_attr::vtype = value_type::alloc ("T_ATTR",
R"docstring(

Values of this type represent attributes attached to DIE's::

	$ dwgrep ./tests/a1.out -e 'unit root attribute'
	producer (GNU_strp_alt)	GNU C 4.6.3 20120306 (Red Hat 4.6.3-2);
	language (data1)	DW_LANG_C89;
	name (GNU_strp_alt)	foo.c;
	comp_dir (GNU_strp_alt)	/home/petr/proj/dwgrep;
	low_pc (addr)	0x4004b2;
	high_pc (addr)	0x4004b8;
	stmt_list (data4)	0;


)docstring");

void
value_attr::show (std::ostream &o) const
{
  {
    unsigned name = (unsigned) dwarf_whatattr ((Dwarf_Attribute *) &m_attr);
    unsigned form = dwarf_whatform ((Dwarf_Attribute *) &m_attr);
    ios_flag_saver s {o};
    o << constant (name, &dw_attr_dom (), brevity::full) << " ("
      << constant (form, &dw_form_dom (), brevity::brief) << ") at ";
  }

  o << get_value_die () << " ";
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
      Dwarf_Off a = dwarf_dieoffset (const_cast <Dwarf_Die *> (&get_die ()));
      Dwarf_Off b = dwarf_dieoffset (const_cast <Dwarf_Die *> (&v->get_die ()));
      if (a != b)
	return compare (a, b);
      else
	return compare (dwarf_whatattr ((Dwarf_Attribute *) &m_attr),
			dwarf_whatattr ((Dwarf_Attribute *) &v->m_attr));
    }
  else
    return cmp_result::fail;
}


value_type const value_abbrev_unit::vtype = value_type::alloc ("T_ABBREV_UNIT",
R"docstring(

Values of this type represent abbreviation units found in Dwarf files::

	$ dwgrep tests/dwz-partial2-1 -e 'abbrev'
	abbrev unit 0
	abbrev unit 0

)docstring");

void
value_abbrev_unit::show (std::ostream &o) const
{
  ios_flag_saver s {o};
  o << "abbrev unit " << std::hex << std::showbase
    << dwpp_cu_abbrev_unit_offset (m_cu);
}

std::unique_ptr <value>
value_abbrev_unit::clone () const
{
  return std::make_unique <value_abbrev_unit> (*this);
}

cmp_result
value_abbrev_unit::cmp (value const &that) const
{
  if (auto v = value::as <value_abbrev_unit> (&that))
    return compare (&m_cu, &v->m_cu);
  else
    return cmp_result::fail;
}


value_type const value_abbrev::vtype = value_type::alloc ("T_ABBREV",
R"docstring(

Values of this type represent individual abbreviations found in
abbreviation units::

	$ dwgrep ./tests/a1.out -e 'unit root abbrev'
	[1] offset:0, children:yes, tag:compile_unit
		0 producer (GNU_strp_alt)
		0x3 language (data1)
		0x5 name (GNU_strp_alt)
		0x8 comp_dir (GNU_strp_alt)
		0xb low_pc (addr)
		0xd high_pc (addr)
		0xf stmt_list (data4)

)docstring");

void
value_abbrev::show (std::ostream &o) const
{
  o << '[' << dwarf_getabbrevcode (&m_abbrev) << "] "
    << "offset:" << constant (dwpp_abbrev_offset (m_abbrev),
			      &dw_offset_dom (), brevity::full)
    << ", children:" << (dwarf_abbrevhaschildren (&m_abbrev) ? "yes" : "no")
    << ", tag:" << constant (dwarf_getabbrevtag (&m_abbrev),
			     &dw_tag_dom (), brevity::brief);
}

std::unique_ptr <value>
value_abbrev::clone () const
{
  return std::make_unique <value_abbrev> (*this);
}

cmp_result
value_abbrev::cmp (value const &that) const
{
  if (auto v = value::as <value_abbrev> (&that))
    // That Dwarf_Abbrev& ultimately comes from libdw, which keeps one
    // of each.  Thus the pointer actually serves as identity.
    return compare (&m_abbrev, &v->m_abbrev);
  else
    return cmp_result::fail;
}


value_type const value_abbrev_attr::vtype = value_type::alloc ("T_ABBREV_ATTR",
R"docstring(

Values of this type represent attributes attached to abbreviations::

	$ dwgrep ./tests/a1.out -e 'unit root abbrev attribute'
	0 producer (GNU_strp_alt)
	0x3 language (data1)
	0x5 name (GNU_strp_alt)
	0x8 comp_dir (GNU_strp_alt)
	0xb low_pc (addr)
	0xd high_pc (addr)
	0xf stmt_list (data4)

)docstring");

void
value_abbrev_attr::show (std::ostream &o) const
{
  o << constant (offset, &dw_offset_dom (), brevity::full)
    << ' ' << constant (name, &dw_attr_dom (), brevity::brief)
    << " (" << constant (form, &dw_form_dom (), brevity::brief) << ")";
}

std::unique_ptr <value>
value_abbrev_attr::clone () const
{
  return std::make_unique <value_abbrev_attr> (*this);
}

cmp_result
value_abbrev_attr::cmp (value const &that) const
{
  if (auto v = value::as <value_abbrev_attr> (&that))
    return compare (offset, v->offset);
  else
    return cmp_result::fail;
}


namespace
{
  void
  show_loclist_op (std::ostream &o,
		   std::shared_ptr <dwfl_context> dwctx,
		   Dwarf_Attribute const &attr, Dwarf_Op *dwop)
  {
    o << dwop->offset << ':'
      << constant {dwop->atom, &dw_locexpr_opcode_dom (), brevity::brief};

    bool sep = false;

    for (auto prod = dwop_number (dwctx, attr, dwop);
	 auto v = prod->next (); )
      {
	if (sep)
	  o << ' ';
	sep = true;
	o << *v;
      }

    for (auto prod = dwop_number2 (dwctx, attr, dwop);
	 auto v = prod->next (); )
      {
	if (sep)
	  o << ' ';
	sep = true;
	o << *v;
      }
  }
}

value_type const value_loclist_elem::vtype
	= value_type::alloc ("T_LOCLIST_ELEM",
R"docstring(

Values of this type represent location expressions.  A location
expression behaves a bit like a sequence with address range attached
to it.  It contains location expression instructions, values of type
``T_LOCLIST_OP``::

	$ dwgrep ./tests/bitcount.o -e 'entry @AT_location'
	0x10000..0x10017:[0:reg5]
	0x10017..0x1001a:[0:breg5<0>, 2:breg1<0>, 4:and, 5:stack_value]
	0x1001a..0x10020:[0:reg5]
	0x10000..0x10007:[0:lit0, 1:stack_value]
	0x10007..0x1001e:[0:reg0]
	0x1001e..0x10020:[0:lit0, 1:stack_value]

)docstring");

void
value_loclist_elem::show (std::ostream &o) const
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
      show_loclist_op (o, m_dwctx, m_attr, m_expr + i);
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


value_type const value_loclist_op::vtype = value_type::alloc ("T_LOCLIST_OP",
R"docstring(

Values of this type hold location expression instructions::

	$ dwgrep ./tests/testfile_const_type -e 'entry @AT_location elem'
	0:fbreg<0>
	0:fbreg<0>
	2:GNU_deref_type<8>/<37>
	5:GNU_const_type<[25] base_type>/<[0, 0, 0x80, 0x67, 0x45, 0x23, 0x1, 0]>
	16:div
	17:GNU_convert<44>
	19:stack_value

)docstring");

void
value_loclist_op::show (std::ostream &o) const
{
  show_loclist_op (o, m_dwctx, m_attr, m_dwop);
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
