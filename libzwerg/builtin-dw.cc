/*
   Copyright (C) 2017, 2018 Petr Machata
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

#include <memory>
#include <sstream>

#include "atval.hh"
#include "builtin-dw.hh"
#include "dwcst.hh"
#include "dwit.hh"
#include "dwmods.hh"
#include "dwpp.hh"
#include "op.hh"
#include "overload.hh"
#include "value-cst.hh"
#include "value-str.hh"
#include "value-dw.hh"

// dwopen
value_dwarf
op_dwopen_str::operate (std::unique_ptr <value_str> a) const
{
  return value_dwarf (a->get_string (), 0, doneness::cooked);
}

std::string
op_dwopen_str::docstring ()
{
  return
R"docstring(

Takes a string, which is interpreted as a file name, that file is
opened, and a Dwarf value representing that file is yielded::

	$ dwgrep '"dwgrep/dwgrep" dwopen unit'
	CU 0

)docstring";
}


// unit
namespace
{
  bool
  next_acceptable_unit (doneness d, cu_iterator &it)
  {
    if (d == doneness::raw)
      return true;

    for (; it != cu_iterator::end (); ++it)
      // In cooked mode, we reject partial units.
      if (dwarf_tag (*it) != DW_TAG_partial_unit)
	return true;

    return false;
  }

  struct dwarf_unit_producer
    : public value_producer <value_cu>
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    std::vector <Dwarf *> m_dwarfs;
    std::vector <Dwarf *>::iterator m_it;
    cu_iterator m_cuit;
    size_t m_i;
    doneness m_doneness;

    dwarf_unit_producer (std::shared_ptr <dwfl_context> dwctx, doneness d)
      : m_dwctx {dwctx}
      , m_dwarfs {all_dwarfs (*dwctx)}
      , m_it {m_dwarfs.begin ()}
      , m_cuit {cu_iterator::end ()}
      , m_i {0}
      , m_doneness {d}
    {}

    std::unique_ptr <value_cu>
    next () override
    {
      do
	if (! maybe_next_dwarf (m_cuit, m_it, m_dwarfs.end ()))
	  return nullptr;
      while (! next_acceptable_unit (m_doneness, m_cuit));

      Dwarf_CU &cu = *(*m_cuit)->cu;
      Dwarf_Off off = m_cuit.offset ();
      ++m_cuit;

      return std::make_unique <value_cu> (m_dwctx, cu, off, m_i++, m_doneness);
    }
  };
}

std::unique_ptr <value_producer <value_cu>>
op_unit_dwarf::operate (std::unique_ptr <value_dwarf> a) const
{
  return std::make_unique <dwarf_unit_producer> (a->get_dwctx (),
						 a->get_doneness ());
}

std::string
op_unit_dwarf::docstring ()
{
  return
R"docstring(

Take a Dwarf on TOS and yield units defined therein.  In raw mode,
yields all units without exception, in cooked mode it only yields
actual compile units (i.e. those units whose CU DIE's tag is
``DW_TAG_compile_unit``.

For example::

	$ dwgrep ./tests/dwz-partial -e 'raw unit root "%s"'
	[b] partial_unit
	[34] compile_unit
	[a4] compile_unit
	[e1] compile_unit
	[11e] compile_unit

	$ dwgrep ./tests/dwz-partial -e 'cooked unit root "%s"'
	[34] compile_unit
	[a4] compile_unit
	[e1] compile_unit
	[11e] compile_unit

This operator is identical in operation to the following expression::

	entry ?root unit

)docstring";
}


namespace
{
  value_cu
  cu_for_die (std::shared_ptr <dwfl_context> dwctx, Dwarf_Die die, doneness d)
  {
    Dwarf_Die cudie;
    if (dwarf_diecu (&die, &cudie, nullptr, nullptr) == nullptr)
      throw_libdw ();

    Dwarf_Off offset = dwarf_dieoffset (&cudie) - dwarf_cuoffset (&cudie);
    return value_cu (dwctx, *cudie.cu, offset, 0, d);
  }
}


value_cu
op_unit_die::operate (std::unique_ptr <value_die> a) const
{
  return cu_for_die (a->get_dwctx (), a->get_die (), a->get_doneness ());
}

std::string
op_unit_die::docstring ()
{
  return
R"docstring(

Take a DIE on TOS and yield a unit that this DIE belongs to::

	$ dwgrep ./tests/twocus -e 'unit'
	CU 0
	CU 0x53

	$ dwgrep ./tests/twocus -e 'entry (offset == 0x4b) unit'
	CU 0

)docstring";
}

value_cu
op_unit_attr::operate (std::unique_ptr <value_attr> a) const
{
  return cu_for_die (a->get_dwctx (), a->get_die (), doneness::cooked);
}

std::string
op_unit_attr::docstring ()
{
  return
R"docstring(

Take an attribute on TOS and yield a unit where the attribute
originated::

	$ dwgrep ./tests/twocus -e 'unit'
	CU 0
	CU 0x53

	$ dwgrep ./tests/twocus -e 'entry (offset == 0xb3) attribute unit'
	CU 0x53
	CU 0x53
	CU 0x53

)docstring";
}

// entry
namespace
{
  template <class It>
  std::pair <It, It> get_it_range (Dwarf_Die cudie, bool skip);

  template <>
  std::pair <all_dies_iterator, all_dies_iterator>
  get_it_range (Dwarf_Die cudie, bool skip)
  {
    Dwarf *dw = dwarf_cu_getdwarf (cudie.cu);
    cu_iterator cuit {dw, cudie};
    all_dies_iterator a (cuit);
    all_dies_iterator e (++cuit);
    if (skip)
      ++a;
    return std::make_pair (a, e);
  }

  template <>
  std::pair <child_iterator, child_iterator>
  get_it_range (Dwarf_Die cudie, bool skip)
  {
    // N.B. this always skips the passed-in DIE.
    child_iterator a {cudie};
    return std::make_pair (a, child_iterator::end ());
  }

  template <class It>
  bool
  import_partial_units (std::vector <std::pair <It, It>> &stack,
			std::shared_ptr <dwfl_context> dwctx,
			std::shared_ptr <value_die> &import)
  {
    Dwarf_Die *die = *stack.back ().first;
    Dwarf_Attribute at_import;
    Dwarf_Die cudie;
    if (dwarf_tag (die) == DW_TAG_imported_unit
	&& dwarf_hasattr (die, DW_AT_import)
	&& dwarf_attr (die, DW_AT_import, &at_import) != nullptr
	&& dwarf_formref_die (&at_import, &cudie) != nullptr)
      {
	// Do this first, before we bump the iterator and DIE gets
	// invalidated.
	import = std::make_shared <value_die> (dwctx, import, *die, 0,
					       doneness::cooked);

	// Skip DW_TAG_imported_unit.
	stack.back ().first++;

	// `true` to skip root DIE of DW_TAG_partial_unit.
	stack.push_back (get_it_range <It> (cudie, true));
	return true;
      }

    return false;
  }

  template <class It>
  bool
  drop_finished_imports (std::vector <std::pair <It, It>> &stack,
			 std::shared_ptr <value_die> &import)
  {
    assert (! stack.empty ());
    if (stack.back ().first != stack.back ().second)
      return false;

    stack.pop_back ();

    // We have one more item in STACK than values in IMPORT chain, so
    // this can actually be empty at this point.
    if (import != nullptr)
      import = import->get_import ();

    return true;
  }

  // This producer encapsulates the logic for iteration through a
  // range of DIE's, with optional inlining of partial units along the
  // way.  Cooked producers do inline, raw ones don't.
  template <class It>
  struct die_it_producer
    : public value_producer <value_die>
  {
    std::shared_ptr <dwfl_context> m_dwctx;

    // Stack of iterator ranges.
    std::vector <std::pair <It, It>> m_stack;

    // Chain of DIE's where partial units were imported.
    std::shared_ptr <value_die> m_import;

    size_t m_i;
    doneness m_doneness;

    die_it_producer (std::shared_ptr <dwfl_context> dwctx, Dwarf_Die die,
		     doneness d)
      : m_dwctx {dwctx}
      , m_i {0}
      , m_doneness {d}
    {
      m_stack.push_back (get_it_range <It> (die, false));
    }

    std::unique_ptr <value_die>
    next () override
    {
      do
	if (m_stack.empty ())
	  return nullptr;
      while (drop_finished_imports (m_stack, m_import)
	     || (m_doneness == doneness::cooked
		 && import_partial_units (m_stack, m_dwctx, m_import)));

      return std::make_unique <value_die>
	(m_dwctx, m_import, **m_stack.back ().first++, m_i++, m_doneness);
    }
  };

  std::unique_ptr <value_producer <value_die>>
  make_cu_entry_producer (std::shared_ptr <dwfl_context> dwctx, Dwarf_CU &cu,
			  doneness d)
  {
    return std::make_unique <die_it_producer <all_dies_iterator>>
      (dwctx, dwpp_cudie (cu), d);
  }
}


std::unique_ptr <value_producer <value_die>>
op_entry_cu::operate (std::unique_ptr <value_cu> a) const
{
  return make_cu_entry_producer (a->get_dwctx (), a->get_cu (),
				 a->get_doneness ());
}

std::string
op_entry_cu::docstring ()
{
  return
R"docstring(

Takes a CU on TOS and yield DIE's that it defines::

	$ dwgrep ./tests/twocus -e 'unit (offset == 0x53) entry "%s"'
	[5e] compile_unit
	[80] subprogram
	[a3] subprogram
	[b0] unspecified_parameters
	[b3] base_type

This operator is similar in operation to the following::

	root child*

The difference may lie (and as of this writing lies) in the order in
which the entries are yielded.

In cooked mode, it resolves any ``DW_TAG_imported_unit``'s that it
finds during exploring the DIE tree.  One entry can thus be explored
several times, each time in a different context::

	$ dwgrep ./tests/dwz-partial -e 'entry (offset == 0x14) (|A| A A parent* ?root) "%s inside %s"'
	[14] pointer_type inside [34] compile_unit
	[14] pointer_type inside [a4] compile_unit
	[14] pointer_type inside [e1] compile_unit
	[14] pointer_type inside [11e] compile_unit

	$ dwgrep ./tests/dwz-partial -e 'raw entry (offset == 0x14) (|A| A A parent* ?root) "%s inside %s"'
	[14] pointer_type inside [b] partial_unit

)docstring";
}

namespace
{
  struct dwarf_entry_producer
    : public value_producer <value_die>
  {
    dwarf_unit_producer m_unitprod;
    std::unique_ptr <die_it_producer <all_dies_iterator>> m_dieprod;
    size_t m_i;

    dwarf_entry_producer (std::shared_ptr <dwfl_context> dwctx, doneness d)
      : m_unitprod {dwctx, d}
      , m_i {0}
    {}

    std::unique_ptr <value_die>
    next () override
    {
      while (true)
	{
	  while (m_dieprod == nullptr)
	    if (auto cu = m_unitprod.next ())
	      m_dieprod = std::make_unique <die_it_producer <all_dies_iterator>>
				(m_unitprod.m_dwctx, dwpp_cudie (cu->get_cu ()),
				 m_unitprod.m_doneness);
	    else
	      return nullptr;

	  if (auto ret = m_dieprod->next ())
	    {
	      ret->set_pos (m_i++);
	      return ret;
	    }

	  m_dieprod = nullptr;
	}
    }
  };
}

std::unique_ptr <value_producer <value_die>>
op_entry_dwarf::operate (std::unique_ptr <value_dwarf> a) const
{
  return std::make_unique <dwarf_entry_producer> (a->get_dwctx (),
						  a->get_doneness ());
}

std::string
op_entry_dwarf::docstring ()
{
  return
R"docstring(

Takes a Dwarf on TOS and yields DIE's that it contains.  This operator
behaves equivalently to the following::

	unit entry

)docstring";
}


// child
namespace
{
  std::unique_ptr <value_producer <value_die>>
  make_die_child_producer (std::shared_ptr <dwfl_context> dwctx,
			   Dwarf_Die parent, doneness d)
  {
    return std::make_unique <die_it_producer <child_iterator>>
      (dwctx, parent, d);
  }
}


std::unique_ptr <value_producer <value_die>>
op_child_die::operate (std::unique_ptr <value_die> a) const
{
  return make_die_child_producer (a->get_dwctx (), a->get_die (),
				  a->get_doneness ());
}

std::string
op_child_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields all its children.  If the taken DIE is
cooked, and one of the children is a ``DW_TAG_imported_unit``, the
operator resolves that reference and recursively yields also all its
children::

	$ dwgrep ./tests/dwz-partial2-1 -e 'unit root (offset == 0xe7) raw child "%s"'
	[f9] imported_unit
	[fe] imported_unit
	[103] variable

	$ dwgrep ./tests/dwz-partial2-1 -e 'unit root (offset == 0xe7) child "%s"'
	[76] base_type
	[7d] base_type
	[4b] typedef
	[51] pointer_type
	[54] pointer_type
	[57] pointer_type
	[5a] const_type
	[5c] volatile_type
	[103] variable

)docstring";
}


// elem, relem
namespace
{
  struct elem_loclist_producer
    : public value_producer <value_loclist_op>
  {
    std::unique_ptr <value_loclist_elem> m_value;
    size_t m_i;
    size_t m_n;
    bool m_forward;

    elem_loclist_producer (std::unique_ptr <value_loclist_elem> value,
			   bool forward)
      : m_value {std::move (value)}
      , m_i {0}
      , m_n {m_value->get_exprlen ()}
      , m_forward {forward}
    {}

    std::unique_ptr <value_loclist_op>
    next () override
    {
      size_t idx = m_i++;
      if (idx < m_n)
	{
	  if (! m_forward)
	    idx = m_n - 1 - idx;
	  return std::make_unique <value_loclist_op>
	    (m_value->get_dwctx (), m_value->get_attr (),
	     m_value->get_expr () + idx, idx);
	}
      else
	return nullptr;
    }
  };

  char const elem_loclist_docstring[] =
R"docstring(

Takes a location expression on TOS and yields individual operators::

	$ dwgrep ./tests/testfile_const_type -e 'entry (offset == 0x57)'
	[57]	variable
		name (string)	w;
		decl_file (data1)	/home/mark/src/elfutils/tests/const_type.c;
		decl_line (data1)	6;
		type (ref4)	[25];
		location (exprloc)	0..0xffffffffffffffff:[0:fbreg<0>, 2:GNU_deref_type<8>/<37>, [... snip ...]];

	$ dwgrep ./tests/testfile_const_type -e 'entry (offset == 0x57) @AT_location elem'
	0:fbreg<0>
	2:GNU_deref_type<8>/<37>
	5:GNU_const_type<[25] base_type>/<[0, 0, 0x80, 0x67, 0x45, 0x23, 0x1, 0]>
	16:div
	17:GNU_convert<44>
	19:stack_value

``relem`` yields elements backwards.

)docstring";
}

std::unique_ptr <value_producer <value_loclist_op>>
op_elem_loclist_elem::operate (std::unique_ptr <value_loclist_elem> a) const
{
  return std::make_unique <elem_loclist_producer> (std::move (a), true);
}

std::string
op_elem_loclist_elem::docstring ()
{
  return elem_loclist_docstring;
}

std::unique_ptr <value_producer <value_loclist_op>>
op_relem_loclist_elem::operate (std::unique_ptr <value_loclist_elem> a) const
{
  return std::make_unique <elem_loclist_producer> (std::move (a), false);
}

std::string
op_relem_loclist_elem::docstring ()
{
  return elem_loclist_docstring;
}


// attribute
namespace
{
  bool
  attr_should_be_integrated (int code)
  {
    // Some attributes only make sense at the non-defining DIE and
    // shouldn't be brought down through DW_AT_specification or
    // DW_AT_abstract_origin.

    // Note: DW_AT_decl_* suite should normally be integrated.  GCC
    // will only emit the unique attributes at concrete instance,
    // leading to DIE's that e.g. only have DW_AT_decl_line, because
    // DW_AT_decl_file is inherited.

    switch (code)
      {
      case DW_AT_sibling:
      case DW_AT_declaration:
	return false;

      default:
	return true;
      }
  }

  struct attribute_producer
    : public value_producer <value_attr>
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    std::unique_ptr <value_die> m_die;
    attr_iterator m_it;
    size_t m_i;
    doneness m_doneness;
    bool m_secondary;

    // Already seen attributes.
    std::vector <int> m_seen;

    std::vector <std::unique_ptr <value_die>> m_next;

    void
    schedule (Dwarf_Attribute &at)
    {
      assert (at.code == DW_AT_abstract_origin
	      || at.code == DW_AT_specification);

      Dwarf_Die die_mem;
      if (dwarf_formref_die (&at, &die_mem) == nullptr)
	throw_libdw ();

      m_next.push_back
	(std::make_unique <value_die> (m_dwctx, die_mem, 0, m_doneness));
    }

    bool
    next_die ()
    {
      if (m_next.empty ())
	return false;

      m_die = std::move (m_next.back ());
      m_next.pop_back ();
      m_it = attr_iterator {&m_die->get_die ()};
      return true;
    }

    bool
    seen (int atname) const
    {
      return std::find (std::begin (m_seen), std::end (m_seen),
			atname) != std::end (m_seen);
    }

    attribute_producer (std::unique_ptr <value_die> value)
      : m_dwctx {value->get_dwctx ()}
      , m_it {attr_iterator::end ()}
      , m_i {0}
      , m_doneness {value->get_doneness ()}
      , m_secondary {false}
    {
      m_next.push_back (std::move (value));
      next_die ();
    }

    std::unique_ptr <value_attr>
    next () override
    {
      Dwarf_Attribute at;
      bool const integrate = m_doneness == doneness::cooked;
      do
	{
	again:
	  while (m_it == attr_iterator::end ())
	    if (! integrate || ! next_die ())
	      return nullptr;
	    else
	      m_secondary = true;

	  at = **m_it++;
	  if (integrate
	      && (at.code == DW_AT_specification
		  || at.code == DW_AT_abstract_origin))
	    schedule (at);

	  if (m_secondary && ! attr_should_be_integrated (at.code))
	    goto again;
	}
      while (integrate && seen (at.code));

      m_seen.push_back (at.code);
      return std::make_unique <value_attr> (*m_die, at, m_i++, m_doneness);
    }
  };
}

std::unique_ptr <value_producer <value_attr>>
op_attribute_die::operate (std::unique_ptr <value_die> a) const
{
  return std::make_unique <attribute_producer> (std::move (a));
}

std::string
op_attribute_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields its individual attributes.  If it is a
raw DIE, it yields only those attributes that the DIE contains.  For
cooked DIE's it also integrates attributes at DIE's referenced by
``DW_AT_abstract_origin`` and ``DW_AT_specification``, except those
that are already present at the original DIE, or those that it makes
no sense to import (as of this writing, ``DW_AT_sibling``,
``DW_AT_declaration``).

Attributes yielded from cooked DIEs are cooked, and attributes from
raw DIEs are raw. That has impact on how the attribute value is
decoded. See elaboration at ``value`` for further information.

Example::

	$ dwgrep ./tests/nullptr.o -e 'raw entry (offset == 0x6e) attribute'
	specification (ref4)	[3f];
	inline (data1)	DW_INL_declared_inlined;
	object_pointer (ref4)	[7c];
	sibling (ref4)	[8b];

	$ dwgrep ./tests/nullptr.o -e 'raw entry (offset == 0x3f)'
	[3f]	subprogram
		external (flag_present)	true;			# Will be integrate.
		name (string)	foo;				# Likewise.
		decl_file (data1)	tests/nullptr.cc;	# Likewise.
		decl_line (data1)	3;			# Likewise.
		declaration (flag_present)	true;		# Won't--meaningless.
		object_pointer (ref4)	[4a];			# Won't--duplicate.

	$ dwgrep ./tests/nullptr.o -e 'entry (offset == 0x6e) attribute'
	specification (ref4)	[3f];
	inline (data1)	DW_INL_declared_inlined;
	object_pointer (ref4)	[7c];
	sibling (ref4)	[8b];
	external (flag_present)	true;
	name (string)	foo;
	decl_file (data1)	tests/nullptr.cc;
	decl_line (data1)	3;

)docstring";
}


// offset
value_cst
op_offset_cu::operate (std::unique_ptr <value_cu> a) const
    {
      return value_cst {constant {a->get_offset (), &dw_offset_dom ()}, 0};
    }

std::string
op_offset_cu::docstring ()
{
  return
R"docstring(

Takes a CU on TOS and yields its offset inside the Dwarf section that
it comes from::

	$ dwgrep tests/twocus -e 'unit offset'
	0
	0x53

Note that CU offset is different from the offset of its root element::

	$ dwgrep tests/twocus -e 'unit root offset'
	0xb
	0x5e

|OffsetsNotUnique|

.. |OffsetsNotUnique| replace::
    Note that offset does not uniquely identify a Dwarf object, as
    offsets of objects from .gnu_debugaltlink files are again numbered
    from 0, and may conflict with those from the main file.  Direct
    comparison of individual objects will not be fooled though.

Example::

	$ dwgrep tests/a1.out -e '
		let A := raw unit (pos == 0);
		let B := raw unit (pos == 1);
		A B if (A == B) then "eq" else "ne"'
	---
	ne
	CU 0
	CU 0
	<Dwarf "tests/a1.out">

)docstring";
}


value_cst
op_offset_die::operate (std::unique_ptr <value_die> val) const
{
  constant c {dwarf_dieoffset (&val->get_die ()), &dw_offset_dom ()};
  return value_cst {c, 0};
}

std::string
op_offset_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields its offset inside the Dwarf section that
it comes from::

	$ dwgrep tests/twocus -e 'unit root offset'
	0xb
	0x5e

|OffsetsNotUnique|

Also, DIE's brought in through imports will report the same offset
even though they came in through different paths.  E.g. if a single
DIE is selected in the same context in two different expressions, the
two DIE's compare equal::

	$ dwgrep tests/dwz-partial2-1 -e '
		let A := [entry (offset == 0x14)] elem (pos == 0);
		let B := [entry (offset == 0x14)] elem (pos == 0);
		A B if ?eq then "==" else "!=" swap "%s: %s %s %s"'
	<Dwarf "tests/dwz-partial2-1">: [14] typedef == [14] typedef

However if the two DIE's are taken from different contexts, they will
compare unequal despite them being physically the same DIE::

	$ dwgrep tests/dwz-partial2-1 -e '
		let A := [entry (offset == 0x14)] elem (pos == 0);
		let B := [entry (offset == 0x14)] elem (pos == 1);
		A B if ?eq then "==" else "!=" swap "%s: %s %s %s"'
	<Dwarf "tests/dwz-partial2-1">: [14] typedef != [14] typedef

)docstring";
}


value_cst
op_offset_loclist_op::operate (std::unique_ptr <value_loclist_op> val) const
{
  Dwarf_Op const *dwop = val->get_dwop ();
  return value_cst {constant {dwop->offset, &dw_offset_dom ()}, 0};
}

std::string
op_offset_loclist_op::docstring ()
{
  return
R"docstring(

Takes a location list operation on TOS and yields its offset within
the containing location expression::

	$ dwgrep ./tests/testfile_const_type -e 'entry (offset == 0x57)'
	[57]	variable
		name (string)	w;
		decl_file (data1)	/home/mark/src/elfutils/tests/const_type.c;
		decl_line (data1)	6;
		type (ref4)	[25];
		location (exprloc)	0..0xffffffffffffffff:[0:fbreg<0>, 2:GNU_deref_type<8>/<37>, [... snip ...]];

	$ dwgrep ./tests/testfile_const_type -e 'entry (offset == 0x57) @AT_location elem offset'
	0
	0x2
	0x5
	0x10
	0x11
	0x13

)docstring";
}


// address

value_aset
op_address_die::operate (std::unique_ptr <value_die> a) const
{
  return die_ranges (a->get_die ());
}

std::string
op_address_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields an address set describing address ranges
covered by this DIE.  (This calls dwarf_ranges under the covers.)::

	$ dwgrep ./tests/testfile_const_type -e 'entry (name == "main") address'
	[0x80482f0, 0x80482f3)

)docstring";
}

namespace
{
  std::unique_ptr <value_cst>
  get_die_addr (Dwarf_Die &die, int (cb) (Dwarf_Die *, Dwarf_Addr *))
  {
    Dwarf_Addr addr;
    if (cb (&die, &addr) < 0)
      throw_libdw ();
    return std::make_unique <value_cst>
      (constant {addr, &dw_address_dom ()}, 0);
  }

  std::unique_ptr <value_cst>
  maybe_get_die_addr (Dwarf_Die &die, int atname,
		      int (cb) (Dwarf_Die *, Dwarf_Addr *))
  {
    // Attributes that hold addresses shouldn't generally be found
    // in abstract origins and declarations, so we don't need to
    // concern ourselves with integrating here.
    if (! dwarf_hasattr (&die, atname))
      return nullptr;
    return get_die_addr (die, cb);
  }
}


std::unique_ptr <value_cst>
op_address_attr::operate (std::unique_ptr <value_attr> a) const
{
  if (dwarf_whatattr (&a->get_attr ()) == DW_AT_high_pc)
    return get_die_addr (a->get_die (), &dwarf_highpc);

  if (dwarf_whatattr (&a->get_attr ()) == DW_AT_entry_pc)
    return get_die_addr (a->get_die (), &dwarf_entrypc);

  if (dwarf_whatform (&a->get_attr ()) == DW_FORM_addr)
    {
      Dwarf_Addr addr;
      if (dwarf_formaddr (&a->get_attr (), &addr) < 0)
	throw_libdw ();
      return std::make_unique <value_cst>
	(constant {addr, &dw_address_dom ()}, 0);
    }

  std::cerr << "`address' applied to non-address attribute:\n    ";
  a->show (std::cerr);
  std::cerr << std::endl;

  return nullptr;
}

std::string
op_address_attr::docstring ()
{
  return
R"docstring(

Takes a DIE attribute on TOS and yields an address that it references.
For attributes with an address form, this simply yields the value of
the attribute.  For ``DW_AT_high_pc`` and ``DW_AT_entry_pc``
attributes with a constant form, this converts the relative constant
value to absolute address::

	$ dwgrep ./tests/bitcount.o -e 'unit root'
	[b]	compile_unit
		[... snip ...]
		low_pc (addr)	0x10000;
		high_pc (data8)	32;
		stmt_list (sec_offset)	0;

	$ dwgrep ./tests/bitcount.o -e 'unit root attribute ?AT_high_pc address'
	0x10020

)docstring";
}


value_aset
op_address_loclist_elem::operate (std::unique_ptr <value_loclist_elem> val)
  const
{
  uint64_t low = val->get_low ();
  uint64_t len = val->get_high () - low;

  coverage cov;
  cov.add (low, len);
  return value_aset {cov, 0};
}

std::string
op_address_loclist_elem::docstring ()
{
  return
R"docstring(

Takes a location list element on TOS and yields an address set
describing where it is valid::

	$ dwgrep ./tests/bitcount.o -e 'entry (offset == 0x91)'
	[91]	formal_parameter
		name (string)	u;
		decl_file (data1)	tests/bitcount.c;
		decl_line (data1)	3;
		type (ref4)	[5e];
		location (sec_offset)	0x10000..0x10017:[0:reg5];0x10017..0x1001a:[0:breg5<0>, 2:breg1<0>, 4:and, 5:stack_value];0x1001a..0x10020:[0:reg5];

	$ dwgrep ./tests/bitcount.o -e 'entry (offset == 0x91) @AT_location address'
	[0x10000, 0x10017)
	[0x10017, 0x1001a)
	[0x1001a, 0x10020)

)docstring";
}


// label

value_cst
op_label_die::operate (std::unique_ptr <value_die> val) const
{
  int tag = dwarf_tag (&val->get_die ());
  return value_cst {constant {tag, &dw_tag_dom ()}, 0};
}

std::string
op_label_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields its tag::

	$ dwgrep ./tests/a1.out -e 'raw unit root label'
	DW_TAG_compile_unit
	DW_TAG_partial_unit

)docstring";
}


value_cst
op_label_attr::operate (std::unique_ptr <value_attr> val) const
{
  constant cst {dwarf_whatattr (&val->get_attr ()), &dw_attr_dom ()};
  return value_cst {cst, 0};
}

std::string
op_label_attr::docstring ()
{
  return
R"docstring(

Takes an attribute on TOS and yields its name::

	$ dwgrep ./tests/a1.out -e 'unit root attribute label'
	DW_AT_producer
	DW_AT_language
	DW_AT_name
	DW_AT_comp_dir
	DW_AT_low_pc
	DW_AT_high_pc
	DW_AT_stmt_list

)docstring";
}


value_cst
op_label_loclist_op::operate (std::unique_ptr <value_loclist_op> val) const
{
  constant cst {val->get_dwop ()->atom, &dw_locexpr_opcode_dom (),
      brevity::brief};
  return value_cst {cst, 0};
}

std::string
op_label_loclist_op::docstring ()
{
  return
R"docstring(

Takes a location expression instruction on TOS and yields the
operator::

	$ dwgrep ./tests/bitcount.o -e 'entry (offset == 0x91) @AT_location elem label'
	reg5
	breg5
	breg1
	and
	stack_value
	reg5

)docstring";
}


// form

value_cst
op_form_attr::operate (std::unique_ptr <value_attr> val) const
{
  constant cst {dwarf_whatform (&val->get_attr ()), &dw_form_dom ()};
  return value_cst {cst, 0};
}

std::string
op_form_attr::docstring ()
{
  return
R"docstring(

Takes an attribute on TOS and yields its form.

)docstring";
}


// parent

std::unique_ptr <value_die>
op_parent_die::operate (std::unique_ptr <value_die> a) const
{
  return a->get_parent ();
}

std::string
op_parent_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields its parental DIE.  For cooked DIE's, in
traverses parents across import points back to from which context the
particular DIE was explored::

	$ dwgrep ./tests/a1.out -e 'unit raw entry "%s"'
	[b] compile_unit
	[2d] imported_unit
	[32] subprogram
	[51] variable

	$ dwgrep ./tests/a1.out -e 'entry (offset == 0x14) "%s"'
	[14] typedef

	$ dwgrep ./tests/a1.out -e 'entry (offset == 0x14) parent "%s"'
	[b] compile_unit

)docstring";
}


// ?root

pred_result
pred_rootp_die::result (value_die &a) const
{
  // N.B. the following works the same for raw as well as cooked
  // DIE's.  The difference in behavior is in 'parent', which for
  // cooked DIE's should never get to DW_TAG_partial_unit DIE
  // unless we've actually started traversal in that partial unit.
  // In that case it's fully legitimate that ?root holds on such
  // node.
  return pred_result (a.get_dwctx ()->is_root (a.get_die ()));
}

std::string
pred_rootp_die::docstring ()
{
  return
R"docstring(

Holds for root DIE's, i.e. DIE's that don't have a parental DIE.

)docstring";
}


// root

value_die
op_root_cu::operate (std::unique_ptr <value_cu> a) const
{
  Dwarf_CU &cu = a->get_cu ();
  Dwarf_Die cudie;
  if (dwarf_cu_die (&cu, &cudie, nullptr, nullptr,
		    nullptr, nullptr, nullptr, nullptr) == nullptr)
    throw_libdw ();

  return value_die {a->get_dwctx (), cudie, 0,
      a->get_doneness ()};
}

std::string
op_root_cu::docstring ()
{
  return
R"docstring(

Takes a CU on TOS and yields its root DIE.

)docstring";
}


namespace
{
  value_die
  op_root_die_operate (std::shared_ptr <value_die> a)
  {
    auto d = a->get_doneness ();
    if (d == doneness::cooked)
      while (a->get_import () != nullptr)
	a = a->get_import ();

    return value_die {a->get_dwctx (),
		      dwpp_cudie (a->get_die ()), 0, d};
  }
}

value_die
op_root_die::operate (std::unique_ptr <value_die> a) const
{
  return op_root_die_operate (std::move (a));
}

std::string
op_root_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields CU DIE of the unit that this DIE comes
from.  For cooked DIE's follows back through import points and yields
the CU DIE of this DIE's context.

)docstring";
}


// value

namespace
{
  std::unique_ptr <value_producer <value>>
  at_value (doneness_aspect const &d,
	    std::shared_ptr <dwfl_context> dwctx,
	    value_die const &die, Dwarf_Attribute attr)
  {
    switch (d.get_doneness ())
      {
      case doneness::cooked:
	return at_value_cooked (dwctx, die, attr);
      case doneness::raw:
	return at_value_raw (attr);
      }

    __builtin_unreachable ();
  }
}

std::unique_ptr <value_producer <value>>
op_value_attr::operate (std::unique_ptr <value_attr> a) const
{
  return ::at_value (*a, a->get_dwctx (), a->get_value_die (), a->get_attr ());
}

std::string
op_value_attr::docstring ()
{
  return
R"docstring(

Takes an attribute on TOS and yields its value.  The value can be of
any Zwerg type.  There can even be more than one value.  In particular
location expression attributes yield all the constituent location
expressions::

	$ dwgrep ./tests/aranges.o -e 'entry attribute ?AT_location value'
	0x10000..0x10010:[0:reg5]
	0x10010..0x1001a:[0:fbreg<-24>]

When determining value of a cooked attribute, dwgrep attempts to get
the best semantic representation of that value. Thus reference
attributes yield actual DIEs, values of attributes such as
``DW_AT_language`` or ``DW_AT_encoding`` are yielded with the correct
domain, instead of as a mere number, values of ``DW_AT_const_value``
take into consideration type at the DIE that contains this attribute,
etc.::

	$  dwgrep ./tests/const_value_block.o -e '
		entry (offset == 0x30) attribute ?AT_const_value
		[|At| At label, At form, At value]'
	[const_value, block1, false]

In this example, dwgrep decides boolean decoding is appropriate after
inspecting the ``DW_AT_type`` attribute::

	$ dwgrep ./tests/const_value_block.o -e '
		entry (offset == 0x30) ?AT_const_value @AT_type @AT_encoding'
	DW_ATE_boolean

Values of raw attributes, on the other hand, are kept simple::

	$ dwgrep ./tests/const_value_block.o -e '
		entry (offset == 0x30) raw attribute ?AT_const_value
		[|At| At label, At form, At value]'
	[const_value, block1, [0]]

)docstring";
}


std::unique_ptr <value_producer <value>>
op_value_loclist_op::operate (std::unique_ptr <value_loclist_op> a) const
{
  return std::make_unique <value_producer_cat <value>>
    (dwop_number (a->get_dwctx (), a->get_attr (), a->get_dwop ()),
     dwop_number2 (a->get_dwctx (), a->get_attr (), a->get_dwop ()));
}

std::string
op_value_loclist_op::docstring ()
{
  return
R"docstring(

Takes a location expression instruction on TOS and yields its
operands::

	$ dwgrep ./tests/aranges.o -e 'entry @AT_location elem'
	0:reg5
	0:fbreg<-24>

	$ dwgrep ./tests/aranges.o -e 'entry @AT_location elem value'
	-24

Operands could be of any Zwerg type, some will be e.g. DIE's.

)docstring";
}


// low

std::unique_ptr <value_cst>
op_low_die::operate (std::unique_ptr <value_die> a) const
{
  return maybe_get_die_addr (a->get_die (), DW_AT_low_pc, &dwarf_lowpc);
}

std::string
op_low_die::docstring ()
{
  return
R"docstring(

Equivalent to ``@AT_low_pc``.

)docstring";
}


// high

std::unique_ptr <value_cst>
op_high_die::operate (std::unique_ptr <value_die> a) const
{
  return maybe_get_die_addr (a->get_die (), DW_AT_high_pc, &dwarf_highpc);
}

std::string
op_high_die::docstring ()
{
  return
R"docstring(

Equivalent to ``@AT_high_pc``.

)docstring";
}


// length

value_cst
op_length_loclist_elem::operate (std::unique_ptr <value_loclist_elem> a) const
{
  return value_cst {constant {a->get_exprlen (), &dec_constant_dom}, 0};
}

std::string
op_length_loclist_elem::docstring ()
{
  return
R"docstring(

Takes a location expression on TOS and yields number of operators in
expression::

	$ dwgrep ./tests/bitcount.o -e 'entry @AT_location (length == 4)'
	0x10017..0x1001a:[0:breg5<0>, 2:breg1<0>, 4:and, 5:stack_value]

)docstring";
}


// ?haschildren

pred_result
pred_haschildrenp_die::result (value_die &a) const
{
  return pred_result (dwarf_haschildren (&a.get_die ()));
}

std::string
pred_haschildrenp_die::docstring ()
{
  return
R"docstring(

Inspects a DIE on TOS and holds if that DIE might have children,
i.e. it is the same as ``abbrev ?haschildren``.  Note that even DIE's
for which ?haschildren holds may not actually have children, because
the actual children chain in the Dwarf file is immediately terminated.
To determine whether there are actually any children, use
``?(child)``::

	$ dwgrep ./tests/haschildren_childless -e 'entry (offset == 0x2d) abbrev "%s"'
	[2] offset:0x13, children:yes, tag:subprogram

	$ dwgrep ./tests/haschildren_childless -e '
		entry (offset == 0x2d)
		if child then "yes" else "no" "%s: %s"'
	[2d] subprogram: no

)docstring";
}


// version

value_cst
op_version_cu::operate (std::unique_ptr <value_cu> a) const
{
  Dwarf_CU &cu = a->get_cu ();
  Dwarf_Die cudie;
  Dwarf_Half version;
  if (dwarf_cu_die (&cu, &cudie, &version, nullptr,
		    nullptr, nullptr, nullptr, nullptr) == nullptr)
    throw_libdw ();

  return value_cst {constant {version, &dec_constant_dom}, 0};
}

std::string
op_version_cu::docstring ()
{
  return
R"docstring(

Takes a CU on TOS and yields a constant corresponding to version of
Dwarf standard according to which this CU has been written.

)docstring";
}


// name

value_str
op_name_dwarf::operate (std::unique_ptr <value_dwarf> a) const
{
  return value_str {std::string {a->get_fn ()}, 0};
}

std::string
op_name_dwarf::docstring ()
{
  return
R"docstring(

Take a Dwarf on TOS and yield its filename.  Note that it may not be
possible to use the reported name to reopen the Dwarf.  The Dwarf may
have been built in place in memory, or the file may have been deleted
or replaced since the Dwarf was opened.

)docstring";
}


std::unique_ptr <value_str>
op_name_die::operate (std::unique_ptr <value_die> a) const
{
  if (a->is_cooked ())
    {
      // On cooked DIE's, `name` integrates.
      const char *name = dwarf_diename (&a->get_die ());
      if (name != nullptr)
	return std::make_unique <value_str> (name, 0);
      else
	return nullptr;
    }
  // Unfortunately there's no non-integrating dwarf_diename
  // counterpart.
  else if (dwarf_hasattr (&a->get_die (), DW_AT_name))
    {
      Dwarf_Attribute attr = dwpp_attr (a->get_die (), DW_AT_name);
      return std::make_unique <value_str> (dwpp_formstring (attr), 0);
    }
  else
    return nullptr;
}

std::string
op_name_die::docstring ()
{
  return
R"docstring(

Equivalent to ``@AT_name``.

)docstring";
}


// raw

value_dwarf
op_raw_dwarf::operate (std::unique_ptr <value_dwarf> a) const
{
  return value_dwarf {a->get_fn (), a->get_dwctx (), 0, doneness::raw};
}

std::string
op_raw_dwarf::docstring ()
{
  return
R"docstring(

Takes a Dwarf on TOS and yields a raw version thereof.

|RawCookeImmutable|

.. |RawCookeImmutable| replace::

   The words ``raw`` and ``cooked`` create a new value, they do not
   change a state of an existing one.  The underlying bits that
   constitute the Dwarf data themselves are shared, but those, too,
   are immutable.

In particular, take a look at the following example::

	$ dwgrep ./tests/a1.out -e '
		(|D| D raw D (|R C| [C entry] length [R entry] length))'
	---
	13
	11

Had ``raw`` changed the value that it's applied to, the next reference
to *D* would yield a raw Dwarf.  But raw yields a new value and *D* is
kept intact (despite the sharing of underlying bits, as mentioned).

)docstring";
}


value_cu
op_raw_cu::operate (std::unique_ptr <value_cu> a) const
{
  return value_cu {a->get_dwctx (), a->get_cu (), a->get_offset (),
		   0, doneness::raw};
}

std::string
op_raw_cu::docstring ()
{
  return
R"docstring(

Takes a CU on TOS and yields a raw version thereof.

|RawCookeImmutable|

)docstring";
}


value_die
op_raw_die::operate (std::unique_ptr <value_die> a) const
{
  return value_die {a->get_dwctx (), a->get_die (), 0, doneness::raw};
}

std::string
op_raw_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields a raw version thereof.

|RawCookeImmutable|

)docstring";
}


value_attr
op_raw_attr::operate (std::unique_ptr <value_attr> a) const
{
  return value_attr {a->get_value_die (), a->get_attr (), 0, doneness::raw};
}

std::string
op_raw_attr::docstring ()
{
  return
R"docstring(

Takes an attribute on TOS and yields a raw version thereof.

|RawCookeImmutable|

)docstring";
}


// cooked

value_dwarf
op_cooked_dwarf::operate (std::unique_ptr <value_dwarf> a) const
{
  return value_dwarf {a->get_fn (), a->get_dwctx (), 0, doneness::cooked};
}

std::string
op_cooked_dwarf::docstring ()
{
  return
R"docstring(

Takes a Dwarf on TOS and yields a cooked version thereof.

|RawCookeImmutable|

)docstring";
}


value_cu
op_cooked_cu::operate (std::unique_ptr <value_cu> a) const
{
  return value_cu {a->get_dwctx (), a->get_cu (), a->get_offset (),
		   0, doneness::cooked};
}

std::string
op_cooked_cu::docstring ()
{
  return
R"docstring(

Takes a CU on TOS and yields a cooked version thereof.

|RawCookeImmutable|

)docstring";
}


value_die
op_cooked_die::operate (std::unique_ptr <value_die> a) const
{
  return value_die {a->get_dwctx (), a->get_die (), 0, doneness::cooked};
}

std::string
op_cooked_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields a cooked version thereof.

|RawCookeImmutable|

)docstring";
}


value_attr
op_cooked_attr::operate (std::unique_ptr <value_attr> a) const
{
  return value_attr {a->get_value_die (), a->get_attr (), 0, doneness::cooked};
}

std::string
op_cooked_attr::docstring ()
{
  return
R"docstring(

Takes an attribute on TOS and yields a cooked version thereof.

|RawCookeImmutable|

)docstring";
}


// @AT_*

namespace
{
  enum class find_attribute_result
    {
      not_found = 0,
      found,
      found_integrated,
    };

  // Return whether the attribute was found.
  //
  // If found or found_integrated, and if RET is non-nullptr, prime
  // the pointed-to value with found attribute
  //
  // If found_integrated, and if DWCTX is non-nullptr, a new value_die
  // with the DIE where the attribute was found is created and passed
  // in second slot of the returned pair.

  std::pair <find_attribute_result, std::unique_ptr <value_die>>
  find_attribute (Dwarf_Die die, int atname, doneness d,
		  Dwarf_Attribute *ret_at,
		  std::shared_ptr <dwfl_context> dwctx)
  {
    if (dwarf_hasattr (&die, atname))
      {
	if (ret_at != nullptr)
	  *ret_at = dwpp_attr (die, atname);
	return std::make_pair (find_attribute_result::found, nullptr);
      }
    else if (d == doneness::cooked && attr_should_be_integrated (atname))
      {
	// Find ATNAME in DIE referenced by ATNAME2.
	auto recursively_find = [&] (int atname2)
		-> std::pair <find_attribute_result,
			      std::unique_ptr <value_die>>
	  {
	    if (dwarf_hasattr (&die, atname2))
	      {
		Dwarf_Attribute at = dwpp_attr (die, atname2);
		Dwarf_Die integrated_die = dwpp_formref_die (at);
		auto ret = find_attribute (integrated_die, atname, d,
					   ret_at, nullptr);

		// If this call found anything, translate from found
		// to found_integrated and create the accompanying
		// value_die if requested.
		if (ret.first == find_attribute_result::found)
		  {
		    std::unique_ptr <value_die> vd
		      = dwctx == nullptr ? nullptr
		        : std::make_unique <value_die>
					(dwctx, integrated_die, 0, d);
		    return std::make_pair
				(find_attribute_result::found_integrated,
				 std::move (vd));
		  }
		else
		  return ret;
	      }
	    else
	      return std::make_pair (find_attribute_result::not_found, nullptr);
	  };

	auto ret = recursively_find (DW_AT_specification);
	if (ret.first != find_attribute_result::not_found)
	  return ret;
	return recursively_find (DW_AT_abstract_origin);
      }

    return std::make_pair (find_attribute_result::not_found, nullptr);
  }
}

std::unique_ptr <value_producer <value>>
op_atval_die::operate (std::unique_ptr <value_die> a) const
{
  Dwarf_Attribute attr;
  auto r = find_attribute (a->get_die (), m_atname, a->get_doneness (),
			   &attr, a->get_dwctx ());
  if (r.first == find_attribute_result::not_found)
    return nullptr;
  doneness_aspect d = *a;
  auto dv = r.second != nullptr ? std::move (r.second) : std::move (a);

  return ::at_value (d, dv->get_dwctx (), *dv, attr);
}

std::string
op_atval_die::docstring ()
{
  return
R"docstring(

Takes a DIE on TOS and yields value of attribute indicated by the
word.  Syntactic sugar for ``(attribute ?(label == AT_*) value)``::

	$ dwgrep ./tests/nullptr.o -e '
		entry (|A| "%( A @AT_name %): %( A @AT_declaration %)")'
	foo: true

The yielded value depends on whether the DIE on TOS in cooked or raw.
See the ``value`` operator for elaboration.

)docstring";
}


// ?AT_*

pred_atname_die::pred_atname_die (unsigned atname)
  : m_atname {atname}
{}

pred_result
pred_atname_die::result (value_die &a) const
{
  return find_attribute (a.get_die (), m_atname,
			 a.get_doneness (), nullptr, nullptr).first
		!= find_attribute_result::not_found
    ? pred_result::yes : pred_result::no;
}

std::string
pred_atname_die::docstring ()
{
  return
R"docstring(

Inspects a DIE on TOS and holds if that DIE has the attribute
indicated by this word.  Note that for cooked DIE's, this word
integrates attributes from abstract origins and specifications,
according to the same rules as ``attribute`` uses.

For example, in the following, a DIE 0x6e reports that it has an
``DW_AT_name`` attribute even if it in fact does not.  This is due to
``DW_AT_specification`` bringing this in::

	$ dwgrep ./tests/nullptr.o -e 'entry (offset == 0x6e) ?AT_name'
	[6e]	subprogram
		specification (ref4)	[3f];
		inline (data1)	DW_INL_declared_inlined;
		object_pointer (ref4)	[7c];
		sibling (ref4)	[8b];

	$ dwgrep ./tests/nullptr.o -e 'entry (offset == 0x3f)'
	[3f]	subprogram
		external (flag_present)	true;
		name (string)	foo;
		decl_file (data1)	tests/nullptr.cc;
		decl_line (data1)	3;
		declaration (flag_present)	true;
		object_pointer (ref4)	[4a];

)docstring";
}


pred_atname_attr::pred_atname_attr (unsigned atname)
  : m_atname {atname}
{}

pred_result
pred_atname_attr::result (value_attr &a) const
{
  return pred_result (dwarf_whatattr (&a.get_attr ()) == m_atname);
}

std::string
pred_atname_attr::docstring ()
{
  return
R"docstring(

Inspects an attribute on TOS and holds if name of that attribute is
the same as indicated by this word::

	$ dwgrep ./tests/nullptr.o -e 'entry (offset == 0x6e) attribute ?AT_name'
	name (string)	foo;

)docstring";
}


pred_atname_cst::pred_atname_cst (unsigned atname)
  : m_const {atname, &dw_attr_dom ()}
{}

pred_result
pred_atname_cst::result (value_cst &a) const
{
  return pred_result (m_const == a.get_constant ());
}

std::string
pred_atname_cst::docstring ()
{
  return
R"docstring(

Inspects a constant on TOS and holds if it's the same constant as
indicated by this word::

	$ dwgrep ./tests/nullptr.o -e '
		entry (offset == 0x4a) abbrev attribute label'
	DW_AT_type
	DW_AT_artificial

	$ dwgrep ./tests/nullptr.o -e '
		entry (offset == 0x4a) abbrev attribute label ?AT_type'
	DW_AT_type

)docstring";
}


// ?TAG_*

pred_tag_die::pred_tag_die (int tag)
  : m_tag {tag}
{}

pred_result
pred_tag_die::result (value_die &a) const
{
  return pred_result (dwarf_tag (&a.get_die ()) == m_tag);
}

std::string
pred_tag_die::docstring ()
{
  return
R"docstring(

Inspects a DIE on TOS and holds if its tag is the same as this word
indicates::

	$ dwgrep ./tests/nullptr.o -e 'entry ?TAG_unspecified_type'
	[69]	unspecified_type
		name (strp)	decltype(nullptr);

)docstring";
}


pred_tag_cst::pred_tag_cst (int tag)
  : m_const {(unsigned) tag, &dw_tag_dom ()}
{}

pred_result
pred_tag_cst::result (value_cst &a) const
{
  return pred_result (m_const == a.get_constant ());
}

std::string
pred_tag_cst::docstring ()
{
  return
R"docstring(

Inspects a constant on TOS and holds if it's the same constant as
indicated by this word::

	$ dwgrep 'DW_TAG_formal_parameter ?TAG_formal_parameter'
	DW_TAG_formal_parameter

	$ dwgrep -c 'DW_TAG_formal_parameter value ?TAG_formal_parameter'
	0

)docstring";
}


// ?FORM_*

pred_form_attr::pred_form_attr (unsigned form)
  : m_form {form}
{}

pred_result
pred_form_attr::result (value_attr &a) const
{
  return pred_result (dwarf_whatform (&a.get_attr ()) == m_form);
}

std::string
pred_form_attr::docstring ()
{
  return
R"docstring(

Inspects an attribute on TOS and holds if the form of that attribute
is the same as indicated by this word::

	$ dwgrep ./tests/nullptr.o -e 'entry ?(attribute ?FORM_data2)'
	[e3]	formal_parameter
		abstract_origin (ref4)	[a5];
		const_value (data2)	65535;

)docstring";
}


pred_form_cst::pred_form_cst (unsigned form)
  : m_const {form, &dw_form_dom ()}
{}

pred_result
pred_form_cst::result (value_cst &a) const
{
  return pred_result (m_const == a.get_constant ());
}

std::string
pred_form_cst::docstring ()
{
  return
R"docstring(

Inspects a constant on TOS and holds if it's the same as indicated by
this word.

)docstring";
}


// ?OP_*

pred_op_loclist_elem::pred_op_loclist_elem (unsigned op)
  : m_op {op}
{}

pred_result
pred_op_loclist_elem::result (value_loclist_elem &a) const
{
  for (size_t i = 0; i < a.get_exprlen (); ++i)
    if (a.get_expr ()[i].atom == m_op)
      return pred_result::yes;
  return pred_result::no;
}

std::string
pred_op_loclist_elem::docstring ()
{
  return
R"docstring(

Inspects a location expression on TOS and holds if it contains an
instruction with an opcode indicated by this word::

	$ dwgrep ./tests/bitcount.o -e 'entry @AT_location ?OP_and'
	0x10017..0x1001a:[0:breg5<0>, 2:breg1<0>, 4:and, 5:stack_value]

)docstring";
}


pred_op_loclist_op::pred_op_loclist_op (unsigned op)
  : m_op {op}
{}

pred_result
pred_op_loclist_op::result (value_loclist_op &a) const
{
  return pred_result (a.get_dwop ()->atom == m_op);
}

std::string
pred_op_loclist_op::docstring ()
{
  return
R"docstring(

Inspects a location expression instruction on TOS and holds if its
opcode is the same as indicated by this word::

	$ dwgrep ./tests/bitcount.o -e 'entry @AT_location elem ?OP_and'
	4:and

)docstring";
}


pred_op_cst::pred_op_cst (unsigned form)
  : m_const {form, &dw_locexpr_opcode_dom ()}
{}

pred_result
pred_op_cst::result (value_cst &a) const
{
  return pred_result (m_const == a.get_constant ());
}

std::string
pred_op_cst::docstring ()
{
  return
R"docstring(

Inspects a constant on TOS and holds if it is the same as indicated by
this word.

)docstring";
}
