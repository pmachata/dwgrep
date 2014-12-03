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

#include "builtin-dw-abbrev.hh"
#include "dwpp.hh"
#include "dwcst.hh"
#include "dwit.hh"
#include "dwmods.hh"


// entry :: T_ABBREV_UNIT ->* T_ABBREV

namespace
{
  struct producer_entry_abbrev_unit
    : public value_producer <value_abbrev>
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    std::vector <Dwarf_Abbrev *> m_abbrevs;
    Dwarf_Die m_cudie;
    Dwarf_Off m_offset;
    size_t m_i;

    producer_entry_abbrev_unit (std::unique_ptr <value_abbrev_unit> a)
      : m_dwctx {a->get_dwctx ()}
      , m_offset {0}
      , m_i {0}
    {
      if (dwarf_cu_die (&a->get_cu (), &m_cudie, nullptr, nullptr,
			nullptr, nullptr, nullptr, nullptr) == nullptr)
	throw_libdw ();
    }

    std::unique_ptr <value_abbrev>
    next () override
    {
      size_t length;
      Dwarf_Abbrev *abbrev = dwarf_getabbrev (&m_cudie, m_offset, &length);
      if (abbrev == nullptr)
	throw_libdw ();
      if (abbrev == DWARF_END_ABBREV)
	return nullptr;

      m_offset += length;
      return std::make_unique <value_abbrev> (m_dwctx, *abbrev, m_i++);
    }
  };
}

std::unique_ptr <value_producer <value_abbrev>>
op_entry_abbrev_unit::operate (std::unique_ptr <value_abbrev_unit> a)
{
  return std::make_unique <producer_entry_abbrev_unit> (std::move (a));
}

std::string
op_entry_abbrev_unit::docstring ()
{
  return
R"docstring(

Take an abbreviation unit on TOS and yield all abbreviations that it
contains::

	$ dwgrep ./tests/twocus -e 'abbrev (offset == 0x34) entry "%s"'
	[1] offset:0x34, children:yes, tag:compile_unit
	[2] offset:0x47, children:yes, tag:subprogram
	[3] offset:0x60, children:yes, tag:subprogram
	[4] offset:0x71, children:no, tag:unspecified_parameters
	[5] offset:0x76, children:no, tag:base_type

)docstring";
}


// attribute::T_ABBREV ->* T_ABBREV_ATTR

namespace
{
  struct producer_attribute_abbrev
    : public value_producer <value_abbrev_attr>
  {
    std::unique_ptr <value_abbrev> m_value;
    size_t m_n;
    size_t m_i;

    producer_attribute_abbrev (std::unique_ptr <value_abbrev> value)
      : m_value {std::move (value)}
      , m_n {dwpp_abbrev_attrcnt (m_value->get_abbrev ())}
      , m_i {0}
    {}

    std::unique_ptr <value_abbrev_attr>
    next () override
    {
      if (m_i < m_n)
	{
	  unsigned int name;
	  unsigned int form;
	  Dwarf_Off offset;
	  if (dwarf_getabbrevattr (&m_value->get_abbrev (), m_i,
				   &name, &form, &offset) != 0)
	    throw_libdw ();

	  return std::make_unique <value_abbrev_attr>
	    (name, form, offset, m_i++);
	}
      else
	return nullptr;
    }
  };
}

std::unique_ptr <value_producer <value_abbrev_attr>>
op_attribute_abbrev::operate (std::unique_ptr <value_abbrev> a)
{
  return std::make_unique <producer_attribute_abbrev> (std::move (a));
}

std::string
op_attribute_abbrev::docstring ()
{
  return
R"docstring(

Takes an abbreviation on TOS and yields all its attributes::

	$ dwgrep ./tests/nullptr.o -e 'entry (offset == 0x3f) abbrev attribute'
	0x31 external (flag_present)
	0x33 name (string)
	0x35 decl_file (data1)
	0x37 decl_line (data1)
	0x39 declaration (flag_present)
	0x3b object_pointer (ref4)

)docstring";
}


// offset :: T_ABBREV_UNIT -> T_CONST

value_cst
op_offset_abbrev_unit::operate (std::unique_ptr <value_abbrev_unit> a)
{
  Dwarf_Die cudie;
  Dwarf_Off abbrev_off;
  if (dwarf_cu_die (&a->get_cu (), &cudie, nullptr, &abbrev_off,
		    nullptr, nullptr, nullptr, nullptr) == nullptr)
    throw_libdw ();

  return value_cst {constant {abbrev_off, &dw_offset_dom ()}, 0};
}

std::string
op_offset_abbrev_unit::docstring ()
{
  return
R"docstring(

XXX

)docstring";
}


// offset :: T_ABBREV -> T_CONST

value_cst
op_offset_abbrev::operate (std::unique_ptr <value_abbrev> a)
{
  constant c {dwpp_abbrev_offset (a->get_abbrev ()), &dw_offset_dom ()};
  return value_cst {c, 0};
}

std::string
op_offset_abbrev::docstring ()
{
  return
R"docstring(

XXX

)docstring";
}


// offset :: T_ABBREV_ATTR -> T_CONST

value_cst
op_offset_abbrev_attr::operate (std::unique_ptr <value_abbrev_attr> a)
{
  return value_cst {constant {a->offset, &dw_offset_dom ()}, 0};
}

std::string
op_offset_abbrev_attr::docstring ()
{
  return
R"docstring(

XXX

)docstring";
}


// label :: T_ABBREV -> T_CONST

value_cst
op_label_abbrev::operate (std::unique_ptr <value_abbrev> a)
{
  unsigned int tag = dwarf_getabbrevtag (&a->get_abbrev ());
  return value_cst {constant {tag, &dw_tag_dom ()}, 0};
}

std::string
op_label_abbrev::docstring ()
{
  return
R"docstring(

Takes an abbreviation on TOS and yields its tag::

	$ dwgrep ./tests/a1.out -e 'raw unit root abbrev label'
	DW_TAG_compile_unit
	DW_TAG_partial_unit

)docstring";
}


// label :: T_ABBREV_ATTR -> T_CONST

value_cst
op_label_abbrev_attr::operate (std::unique_ptr <value_abbrev_attr> a)
{
  return value_cst {constant {a->name, &dw_attr_dom ()}, 0};
}

std::string
op_label_abbrev_attr::docstring ()
{
  return
R"docstring(

Takes an abbreviation attribute on TOS and yields its name::

	$ dwgrep ./tests/a1.out -e 'unit root abbrev attribute label'
	DW_AT_producer
	DW_AT_language
	DW_AT_name
	DW_AT_comp_dir
	DW_AT_low_pc
	DW_AT_high_pc
	DW_AT_stmt_list

)docstring";
}


// form :: T_ABBREV_ATTR -> T_CONST

value_cst
op_form_abbrev_attr::operate (std::unique_ptr <value_abbrev_attr> a)
{
  return value_cst (constant {a->form, &dw_form_dom ()}, 0);
}

std::string
op_form_abbrev_attr::docstring ()
{
  return
R"docstring(

Takes an abbreviation attribute on TOS and yields its form.

)docstring";
}


// ?haschildren :: T_ABBREV

pred_result
pred_haschildrenp_abbrev::result (value_abbrev &a)
{
  return pred_result (dwarf_abbrevhaschildren (&a.get_abbrev ()));
}

std::string
pred_haschildrenp_abbrev::docstring ()
{
  return
R"docstring(

Inspects an abbreviation on TOS and holds if DIE's described by this
abbreviation may have children.

)docstring";
}


// abbrev :: T_DWARF ->* T_ABBREV_UNIT
namespace
{
  struct producer_abbrev_dwarf
    : public value_producer <value_abbrev_unit>
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    std::vector <Dwarf *> m_dwarfs;
    std::vector <Dwarf *>::iterator m_it;
    cu_iterator m_cuit;
    size_t m_i;

    producer_abbrev_dwarf (std::shared_ptr <dwfl_context> dwctx)
      : m_dwctx {(assert (dwctx != nullptr), dwctx)}
      , m_dwarfs {all_dwarfs (*dwctx)}
      , m_it {m_dwarfs.begin ()}
      , m_cuit {cu_iterator::end ()}
      , m_i {0}
    {}

    std::unique_ptr <value_abbrev_unit>
    next () override
    {
      if (! maybe_next_dwarf (m_cuit, m_it, m_dwarfs.end ()))
	return nullptr;

      return std::make_unique <value_abbrev_unit>
	(m_dwctx, *(*m_cuit++)->cu, m_i++);
    }
  };
}

std::unique_ptr <value_producer <value_abbrev_unit>>
op_abbrev_dwarf::operate (std::unique_ptr <value_dwarf> a)
{
  return std::make_unique <producer_abbrev_dwarf> (a->get_dwctx ());
}

std::string
op_abbrev_dwarf::docstring ()
{
  return
R"docstring(

Takes a Dwarf on TOS and yields each abbreviation unit present
therein.

)docstring";
}


// abbrev :: T_CU -> T_ABBREV_UNIT

value_abbrev_unit
op_abbrev_cu::operate (std::unique_ptr <value_cu> a)
{
  return value_abbrev_unit {a->get_dwctx (), a->get_cu (), 0};
}

std::string
op_abbrev_cu::docstring ()
{
  return
R"docstring(

Takes a CU on TOS and yields the abbreviation unit that contains this
unit's abbreviations.

)docstring";
}


// abbrev :: T_DIE -> T_ABBREV

value_abbrev
op_abbrev_die::operate (std::unique_ptr <value_die> a)
{
  // If the DIE doesn't have an abbreviation yet, force its
  // look-up.
  if (a->get_die ().abbrev == nullptr)
    dwarf_haschildren (&a->get_die ());
  assert (a->get_die ().abbrev != nullptr);

  return value_abbrev {a->get_dwctx (), *a->get_die ().abbrev, 0};
}

std::string
op_abbrev_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields the abbreviation that describes this
DIE.

)docstring";
}


// code :: T_ABBREV -> T_CONST

value_cst
op_code_abbrev::operate (std::unique_ptr <value_abbrev> a)
{
  return value_cst {constant {dwarf_getabbrevcode (&a->get_abbrev ()),
	&dw_abbrevcode_dom ()}, 0};
}

std::string
op_code_abbrev::docstring ()
{
  return
R"docstring(

Takes abbreviation on TOS and yields its code.

)docstring";
}


// ?DW_AT_* :: T_ABBREV

pred_result
pred_atname_abbrev::result (value_abbrev &a)
{
  size_t cnt = dwpp_abbrev_attrcnt (a.get_abbrev ());
  for (size_t i = 0; i < cnt; ++i)
    {
      unsigned int name;
      if (dwarf_getabbrevattr (&a.get_abbrev (), i,
			       &name, nullptr, nullptr) != 0)
	throw_libdw ();
      if (name == m_atname)
	return pred_result::yes;
    }
  return pred_result::no;
}

std::string
pred_atname_abbrev::docstring ()
{
  return
R"docstring(

Inspects an abbreviation on TOS and holds if that abbreviation has
this attribute.  Note that unlike with DIE's, no integration takes
place with abbreviations::

	$ dwgrep -c ./tests/nullptr.o -e 'entry (offset == 0x6d) abbrev ?AT_name'
	0

)docstring";
}


// ?DW_AT_A* :: T_ABBREV_ATTR

pred_result
pred_atname_abbrev_attr::result (value_abbrev_attr &a)
{
  return pred_result (a.name == m_atname);
}

std::string
pred_atname_abbrev_attr::docstring ()
{
  return
R"docstring(

Inspects an abbreviation attribute on TOS and holds if its name is the
same as indicated by this word::

	$ dwgrep ./tests/nullptr.o -e 'entry (offset == 0x3f) abbrev attribute ?AT_name'
	0x33 name (string)

)docstring";
}


// ?DW_TAG_* :: T_ABBREV

pred_result
pred_tag_abbrev::result (value_abbrev &a)
{
  return pred_result (dwarf_getabbrevtag (&a.get_abbrev ()) == m_tag);
}

std::string
pred_tag_abbrev::docstring ()
{
  return
R"docstring(

Inspects an abbreviation on TOS and holds if its tag is the same as
indicated by this word.

)docstring";
}


// ?DW_FORM_* :: T_ABBREV_ATTR

pred_result
pred_form_abbrev_attr::result (value_abbrev_attr &a)
{
  return pred_result (a.form == m_form);
}

std::string
pred_form_abbrev_attr::docstring ()
{
  return
R"docstring(

Inspects an abbreviation attribute on TOS and holds if its form is the
same as indicated by this word::

	$ dwgrep ./tests/nullptr.o -e 'entry abbrev ?(attribute ?FORM_data2)'
	[17] offset:0xc9, children:no, tag:formal_parameter
		0xc9 abstract_origin (ref4)
		0xcb const_value (data2)

)docstring";
}
