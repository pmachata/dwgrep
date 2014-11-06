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

#include <memory>
#include <sstream>

#include "atval.hh"
#include "builtin-cst.hh"
#include "builtin-dw.hh"
#include "builtin.hh"
#include "dwcst.hh"
#include "dwit.hh"
#include "dwpp.hh"
#include "known-dwarf.h"
#include "op.hh"
#include "overload.hh"
#include "value-closure.hh"
#include "value-cst.hh"
#include "value-str.hh"
#include "value-dw.hh"
#include "cache.hh"

// dwopen
namespace
{
  struct op_dwopen_str
    : public op_overload <value_str>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_str> a) override
    {
      return std::make_unique <value_dwarf> (a->get_string (), 0,
					     doneness::cooked);
    }
  };
}


// unit
namespace
{
  bool
  maybe_next_module (dwfl_module_iterator &modit, cu_iterator &cuit)
  {
    while (cuit == cu_iterator::end ())
      {
	if (modit == dwfl_module_iterator::end ())
	  return false;

	Dwarf *dw = (*modit++).first;
	assert (dw != nullptr);
	cuit = cu_iterator (dw);
      }

    return true;
  }

  bool
  unit_acceptable (doneness d, Dwarf_Die die)
  {
    if (d == doneness::raw)
      return true;

    // In cooked mode, we reject partial units.
    // XXX Should we reject type units as well?
    return dwarf_tag (&die) != DW_TAG_partial_unit;
  }

  struct dwarf_unit_producer
    : public value_producer
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    dwfl_module_iterator m_modit;
    cu_iterator m_cuit;
    size_t m_i;
    doneness m_doneness;

    dwarf_unit_producer (std::shared_ptr <dwfl_context> dwctx, doneness d)
      : m_dwctx {dwctx}
      , m_modit {m_dwctx->get_dwfl ()}
      , m_cuit {cu_iterator::end ()}
      , m_i {0}
      , m_doneness {d}
    {}

    std::unique_ptr <value>
    next () override
    {
      do
	if (! maybe_next_module (m_modit, m_cuit))
	  return nullptr;
      while (unit_acceptable (m_doneness, **m_cuit)
	     ? false : (m_cuit++, true));

      std::unique_ptr <value> ret;
      ret = std::make_unique <value_cu>
	(m_dwctx, *(*m_cuit)->cu, m_cuit.offset (), m_i, m_doneness);

      m_i++;
      m_cuit++;
      return std::move (ret);
    }
  };

  struct op_unit_dwarf
    : public op_yielding_overload <value_dwarf>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_dwarf> a) override
    {
      return std::make_unique <dwarf_unit_producer> (a->get_dwctx (),
						     a->get_doneness ());
    }
  };

  std::unique_ptr <value>
  cu_for_die (std::shared_ptr <dwfl_context> dwctx, Dwarf_Die die, doneness d)
  {
    Dwarf_Die cudie;
    if (dwarf_diecu (&die, &cudie, nullptr, nullptr) == nullptr)
      throw_libdw ();

    Dwarf *dw = dwarf_cu_getdwarf (cudie.cu);
    cu_iterator cuit {dw, cudie};

    return std::make_unique <value_cu> (dwctx, *(*cuit)->cu,
					cuit.offset (), 0, d);
  }

  struct op_unit_die
    : public op_overload <value_die>
  {
    using op_overload <value_die>::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      return cu_for_die (a->get_dwctx (), a->get_die (), doneness::cooked);
    }
  };

  struct op_unit_rawdie
    : public op_overload <value_rawdie>
  {
    using op_overload <value_rawdie>::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawdie> a) override
    {
      return cu_for_die (a->get_dwctx (), a->get_die (), doneness::raw);
    }
  };

  struct op_unit_attr
    : public op_overload <value_attr>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_attr> a) override
    {
      return cu_for_die (a->get_dwctx (), a->get_die (), doneness::cooked);
    }
  };
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
	import = std::make_shared <value_die> (dwctx, import, *die, 0);

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
    : public value_producer
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

    std::unique_ptr <value>
    next () override
    {
      do
	if (m_stack.empty ())
	  return nullptr;
      while (drop_finished_imports (m_stack, m_import)
	     || (m_doneness == doneness::cooked
		 && import_partial_units (m_stack, m_dwctx, m_import)));

      if (m_doneness == doneness::cooked)
	return std::make_unique <value_die>
	  (m_dwctx, m_import, **m_stack.back ().first++, m_i++);
      else
	return std::make_unique <value_rawdie>
	  (m_dwctx, **m_stack.back ().first++, m_i++);
    }
  };

  std::unique_ptr <value_producer>
  make_cu_entry_producer (std::shared_ptr <dwfl_context> dwctx, Dwarf_CU &cu,
			  doneness d)
  {
    return std::make_unique <die_it_producer <all_dies_iterator>>
      (dwctx, dwpp_cudie (cu), d);
  }

  struct op_entry_cu
    : public op_yielding_overload <value_cu>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_cu> a) override
    {
      return make_cu_entry_producer (a->get_dwctx (), a->get_cu (),
				     a->get_doneness ());
    }
  };

  template <class A, class B>
  struct op_entry_dwarf_base
    : public stub_op
  {
    op_entry_dwarf_base (std::shared_ptr <op> upstream)
      : stub_op {std::make_shared <B> (std::make_shared <A> (upstream))}
    {}

    stack::uptr
    next () override
    { return m_upstream->next (); }
  };

  struct op_entry_dwarf
    : public op_entry_dwarf_base <op_unit_dwarf, op_entry_cu>
  {
    using op_entry_dwarf_base::op_entry_dwarf_base;

    static selector get_selector ()
    { return {value_dwarf::vtype}; }
  };

  struct op_entry_abbrev_unit
    : public op_yielding_overload <value_abbrev_unit>
  {
    using op_yielding_overload::op_yielding_overload;

    struct producer
      : public value_producer
    {
      std::shared_ptr <dwfl_context> m_dwctx;
      std::vector <Dwarf_Abbrev *> m_abbrevs;
      Dwarf_Die m_cudie;
      Dwarf_Off m_offset;
      size_t m_i;

      producer (std::unique_ptr <value_abbrev_unit> a)
	: m_dwctx {a->get_dwctx ()}
	, m_offset {0}
	, m_i {0}
      {
	if (dwarf_cu_die (&a->get_cu (), &m_cudie, nullptr, nullptr,
			  nullptr, nullptr, nullptr, nullptr) == nullptr)
	  throw_libdw ();
      }

      std::unique_ptr <value>
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

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_abbrev_unit> a) override
    {
      return std::make_unique <producer> (std::move (a));
    }
  };
}

// child
namespace
{
  std::unique_ptr <value_producer>
  make_die_child_producer (std::shared_ptr <dwfl_context> dwctx,
			   Dwarf_Die parent, doneness d)
  {
    return std::make_unique <die_it_producer <child_iterator>>
      (dwctx, parent, d);
  }

  struct op_child_die
    : public op_yielding_overload <value_die>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_die> a) override
    {
      return make_die_child_producer (a->get_dwctx (), a->get_die (),
				      doneness::cooked);
    }
  };

  struct op_child_rawdie
    : public op_yielding_overload <value_rawdie>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_rawdie> a) override
    {
      return make_die_child_producer (a->get_dwctx (), a->get_die (),
				      doneness::raw);
    }
  };
}

// elem, relem
namespace
{
  struct elem_loclist_producer
    : public value_producer
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

    std::unique_ptr <value>
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

  struct op_elem_loclist_elem
    : public op_yielding_overload <value_loclist_elem>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_loclist_elem> a) override
    {
      return std::make_unique <elem_loclist_producer> (std::move (a), true);
    }
  };

  struct op_relem_loclist_elem
    : public op_yielding_overload <value_loclist_elem>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_loclist_elem> a) override
    {
      return std::make_unique <elem_loclist_producer> (std::move (a), false);
    }
  };

  struct elem_aset_producer
    : public value_producer
  {
    coverage cov;
    size_t m_idx;	// position among ranges
    uint64_t m_ai;	// iteration through a range
    size_t m_i;		// produced value counter
    bool m_forward;

    elem_aset_producer (coverage a_cov, bool forward)
      : cov {a_cov}
      , m_idx {0}
      , m_ai {0}
      , m_i {0}
      , m_forward {forward}
    {}

    std::unique_ptr <value>
    next () override
    {
      if (m_idx >= cov.size ())
	return nullptr;

      auto idx = [&] () {
	return m_forward ? m_idx : cov.size () - 1 - m_idx;
      };

      if (m_ai >= cov.at (idx ()).length)
	{
	  m_idx++;
	  if (m_idx >= cov.size ())
	    return nullptr;
	  assert (cov.at (idx ()).length > 0);
	  m_ai = 0;
	}

      uint64_t ai = m_forward ? m_ai : cov.at (idx ()).length - 1 - m_ai;
      uint64_t addr = cov.at (idx ()).start + ai;
      m_ai++;

      return std::make_unique <value_cst>
	(constant {addr, &dw_address_dom ()}, m_i++);
    }
  };

  struct op_elem_aset
    : public op_yielding_overload <value_aset>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_aset> val) override
    {
      return std::make_unique <elem_aset_producer>
	(val->get_coverage (), true);
    }
  };

  struct op_relem_aset
    : public op_yielding_overload <value_aset>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_aset> val) override
    {
      return std::make_unique <elem_aset_producer>
	(val->get_coverage (), false);
    }
  };
}

// attribute
namespace
{
  struct attribute_producer
    : public value_producer
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    Dwarf_Die m_die;
    attr_iterator m_it;
    size_t m_i;
    bool m_integrate;
    bool m_secondary;

    // Already seen attributes.
    std::vector <int> m_seen;

    // We store full DIE's to allow DW_AT_specification's in a
    // separate debug info files.
    std::vector <Dwarf_Die> m_next;

    void
    schedule (Dwarf_Attribute &at)
    {
      assert (at.code == DW_AT_abstract_origin
	      || at.code == DW_AT_specification);

      Dwarf_Die die_mem;
      if (dwarf_formref_die (&at, &die_mem) == nullptr)
	throw_libdw ();

      m_next.push_back (die_mem);
    }

    bool
    next_die ()
    {
      if (m_next.empty ())
	return false;

      m_die = m_next.back ();
      m_it = attr_iterator {&m_die};
      m_next.pop_back ();
      return true;
    }

    bool
    seen (int atname) const
    {
      return std::find (std::begin (m_seen), std::end (m_seen),
			atname) != std::end (m_seen);
    }

    attribute_producer (std::shared_ptr <dwfl_context> dwctx,
			Dwarf_Die die, bool integrate)
      : m_dwctx {dwctx}
      , m_it {attr_iterator::end ()}
      , m_i {0}
      , m_integrate {integrate}
      , m_secondary {false}
    {
      m_next.push_back (die);
      next_die ();
    }

    attribute_producer (std::unique_ptr <value_die> value)
      : attribute_producer {value->get_dwctx (), value->get_die (), true}
    {}

    attribute_producer (std::unique_ptr <value_rawdie> value)
      : attribute_producer {value->get_dwctx (), value->get_die (), false}
    {}

    std::unique_ptr <value>
    next () override
    {
      // Should DW_AT_abstract_origin and DW_AT_specification be
      // expanded inline?  (And therefore themselves skipped?)  Should
      // DW_AT_specification's DW_AT_declaration be skipped as well?
      // Should DW_AT_declaration'd DIE's be ignored altogether?
      //
      // Note: page 187 describes the hashing algorithm, which handles
      // DW_AT_specification.

      Dwarf_Attribute at;
      do
	{
	again:
	  while (m_it == attr_iterator::end ())
	    if (! m_integrate || ! next_die ())
	      return nullptr;
	    else
	      m_secondary = true;

	  at = **m_it++;
	  if (m_integrate
	      && (at.code == DW_AT_specification
		  || at.code == DW_AT_abstract_origin))
	    {
	      // Schedule this for future traversal, but still show
	      // the attribute in the output (i.e. skip the seen-check
	      // to possibly also present this several times if we
	      // went through several rounds of integration).  There's
	      // no gain in hiding this from the user.
	      schedule (at);
	      break;
	    }

	  // Some attributes only make sense at the non-defining DIE
	  // and shouldn't be brought down here.
	  //
	  // DW_AT_decl_* suite in particular is meaningful here as
	  // well as the non-defining declaration.  But then we would
	  // see local or remote set of attributes depending on
	  // whether there is any local set.  It would be impossible
	  // to distinguish in a script, which set is seen.
	  //
	  // XXX ?AT_* and @AT_* should have consistent rules for what
	  // it brings from the specification/abstract_origin DIE's.
	  if (m_secondary)
	    switch (at.code)
	    case DW_AT_sibling:
	    case DW_AT_declaration:
	    case DW_AT_decl_line:
	    case DW_AT_decl_column:
	    case DW_AT_decl_file:
	      goto again;
	}
      while (m_integrate && seen (at.code));

      m_seen.push_back (at.code);
      return std::make_unique <value_attr> (m_dwctx, at, m_die, m_i++);
    }
  };

  struct op_attribute_rawdie
    : public op_yielding_overload <value_rawdie>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_rawdie> a) override
    {
      return std::make_unique <attribute_producer> (std::move (a));
    }
  };

  struct op_attribute_die
    : public op_yielding_overload <value_die>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_die> a) override
    {
      return std::make_unique <attribute_producer> (std::move (a));
    }
  };

  struct op_attribute_abbrev
    : public op_yielding_overload <value_abbrev>
  {
    using op_yielding_overload::op_yielding_overload;

    struct producer
      : public value_producer
    {
      std::unique_ptr <value_abbrev> m_value;
      size_t m_n;
      size_t m_i;

      producer (std::unique_ptr <value_abbrev> value)
	: m_value {std::move (value)}
	, m_n {dwpp_abbrev_attrcnt (m_value->get_abbrev ())}
	, m_i {0}
      {}

      std::unique_ptr <value>
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

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_abbrev> a) override
    {
      return std::make_unique <producer> (std::move (a));
    }
  };
}

// offset
namespace
{
  std::unique_ptr <value>
  cu_offset (Dwarf_Off offset)
  {
    constant c {offset, &dw_offset_dom ()};
    return std::make_unique <value_cst> (c, 0);
  }

  template <class T>
  struct op_offset_cu
    : public op_overload <T>
  {
    using op_overload <T>::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <T> a) override
    {
      return cu_offset (a->get_offset ());
    }
  };

  template <class T>
  struct op_offset_die
    : public op_overload <T>
  {
    using op_overload <T>::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <T> val) override
    {
      constant c {dwarf_dieoffset (&val->get_die ()), &dw_offset_dom ()};
      return std::make_unique <value_cst> (c, 0);
    }
  };

  struct op_offset_abbrev_unit
    : public op_overload <value_abbrev_unit>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_abbrev_unit> a) override
    {
      Dwarf_Die cudie;
      Dwarf_Off abbrev_off;
      if (dwarf_cu_die (&a->get_cu (), &cudie, nullptr, &abbrev_off,
			nullptr, nullptr, nullptr, nullptr) == nullptr)
	throw_libdw ();

      constant c {abbrev_off, &dw_offset_dom ()};
      return std::make_unique <value_cst> (c, 0);
    }
  };

  struct op_offset_abbrev
    : public op_overload <value_abbrev>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_abbrev> a) override
    {
      constant c {dwpp_abbrev_offset (a->get_abbrev ()), &dw_offset_dom ()};
      return std::make_unique <value_cst> (c, 0);
    }
  };

  struct op_offset_abbrev_attr
    : public op_overload <value_abbrev_attr>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_abbrev_attr> a) override
    {
      constant c {a->offset, &dw_offset_dom ()};
      return std::make_unique <value_cst> (c, 0);
    }
  };

  struct op_offset_loclist_op
    : public op_overload <value_loclist_op>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_loclist_op> val) override
    {
      Dwarf_Op const *dwop = val->get_dwop ();
      constant c {dwop->offset, &dw_offset_dom ()};
      return std::make_unique <value_cst> (c, 0);
    }
  };
}

// address
namespace
{
  struct op_address_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      return die_ranges (a->get_die ());
    }
  };

  static std::unique_ptr <value>
  get_die_addr (Dwarf_Die *die, int (cb) (Dwarf_Die *, Dwarf_Addr *))
  {
    Dwarf_Addr addr;
    if (cb (die, &addr) < 0)
      throw_libdw ();
    return std::make_unique <value_cst>
      (constant {addr, &dw_address_dom ()}, 0);
  }

  struct op_address_attr
    : public op_overload <value_attr>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_attr> a) override
    {
      if (dwarf_whatattr (&a->get_attr ()) == DW_AT_high_pc)
	return get_die_addr (&a->get_die (), &dwarf_highpc);

      if (dwarf_whatattr (&a->get_attr ()) == DW_AT_entry_pc)
	return get_die_addr (&a->get_die (), &dwarf_entrypc);

      if (dwarf_whatform (&a->get_attr ()) == DW_FORM_addr)
	{
	  Dwarf_Addr addr;
	  if (dwarf_formaddr (&a->get_attr (), &addr) < 0)
	    throw_libdw ();
	  return std::make_unique <value_cst>
	    (constant {addr, &dw_address_dom ()}, 0);
	}

      std::cerr << "`address' applied to non-address attribute:\n    ";
      a->show (std::cerr, brevity::brief);
      std::cerr << std::endl;

      return nullptr;
    }
  };

  struct op_address_loclist_elem
    : public op_overload <value_loclist_elem>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_loclist_elem> val) override
    {
      uint64_t low = val->get_low ();
      uint64_t len = val->get_high () - low;

      coverage cov;
      cov.add (low, len);
      return std::make_unique <value_aset> (cov, 0);
    }
  };
}

// label
namespace
{
  struct op_label_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> val) override
    {
      int tag = dwarf_tag (&val->get_die ());
      assert (tag >= 0);
      constant cst {(unsigned) tag, &dw_tag_dom ()};
      return std::make_unique <value_cst> (cst, 0);
    }
  };

  struct op_label_attr
    : public op_overload <value_attr>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_attr> val) override
    {
      constant cst {dwarf_whatattr (&val->get_attr ()), &dw_attr_dom ()};
      return std::make_unique <value_cst> (cst, 0);
    }
  };

  struct op_label_abbrev
    : public op_overload <value_abbrev>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_abbrev> a) override
    {
      unsigned int tag = dwarf_getabbrevtag (&a->get_abbrev ());
      assert (tag >= 0);
      constant cst {(unsigned) tag, &dw_tag_dom ()};
      return std::make_unique <value_cst> (cst, 0);
    }
  };

  struct op_label_abbrev_attr
    : public op_overload <value_abbrev_attr>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_abbrev_attr> a) override
    {
      constant cst {a->name, &dw_attr_dom ()};
      return std::make_unique <value_cst> (cst, 0);
    }
  };

  struct op_label_loclist_op
    : public op_overload <value_loclist_op>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_loclist_op> val) override
    {
      constant cst {val->get_dwop ()->atom, &dw_locexpr_opcode_dom (),
		    brevity::brief};
      return std::make_unique <value_cst> (cst, 0);
    }
  };
}

// form
namespace
{
  struct op_form_attr
    : public op_overload <value_attr>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_attr> val) override
    {
      constant cst {dwarf_whatform (&val->get_attr ()), &dw_form_dom ()};
      return std::make_unique <value_cst> (cst, 0);
    }
  };

  struct op_form_abbrev_attr
    : public op_overload <value_abbrev_attr>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_abbrev_attr> a) override
    {
      constant cst {a->form, &dw_form_dom ()};
      return std::make_unique <value_cst> (cst, 0);
    }
  };
}

// parent
namespace
{
  template <class T>
  bool
  get_parent (T &value, Dwarf_Die &ret)
  {
    Dwarf_Off par_off = value.get_dwctx ()->find_parent (value.get_die ());
    if (par_off == parent_cache::no_off)
      return false;

    if (dwarf_offdie (dwarf_cu_getdwarf (value.get_die ().cu),
		      par_off, &ret) == nullptr)
      throw_libdw ();

    return true;
  }

  struct op_parent_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    bool
    pop_import (std::shared_ptr <value_die> &a)
    {
      if (auto b = a->get_import ())
	{
	  a = b;
	  return true;
	}
      return false;
    }

    std::unique_ptr <value>
    do_operate (std::shared_ptr <value_die> a)
    {
      Dwarf_Die par_die;
      do
	if (! get_parent (*a, par_die))
	  return nullptr;
      while (dwarf_tag (&par_die) == DW_TAG_partial_unit
	     && pop_import (a));

      return std::make_unique <value_die> (a->get_dwctx (), par_die, 0);
    }

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      return do_operate (std::move (a));
    }
  };

  struct op_parent_rawdie
    : public op_overload <value_rawdie>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawdie> a) override
    {
      Dwarf_Die par_die;
      if (! get_parent (*a, par_die))
	return nullptr;
      return std::make_unique <value_rawdie> (a->get_dwctx (), par_die, 0);
    }
  };
}

// integrate
namespace
{
  struct op_integrate_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> val) override
    {
      Dwarf_Attribute attr_mem, *attr
	= dwarf_attr (&val->get_die (), DW_AT_abstract_origin, &attr_mem);

      if (attr == nullptr)
	attr = dwarf_attr (&val->get_die (), DW_AT_specification, &attr_mem);

      if (attr == nullptr)
	return nullptr;

      Dwarf_Die die_mem, *die2 = dwarf_formref_die (attr, &die_mem);
      if (die2 == nullptr)
	throw_libdw ();

      return std::make_unique <value_die> (val->get_dwctx (), *die2, 0);
    }
  };

  struct op_integrate_closure
    : public op_overload <value_die, value_closure>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> die,
	     std::unique_ptr <value_closure> action) override
    {
      throw std::runtime_error ("integrate over closures not implemented");
    }
  };
}

// ?root
namespace
{
  template <class T>
  struct pred_rootp_die
    : public pred_overload <T>
  {
    using pred_overload <T>::pred_overload;

    pred_result
    result (T &a) override
    {
      // N.B. the following works the same for raw as well as cooked
      // DIE's.  The difference in behavior is in 'parent', which for
      // cooked DIE's should never get to DW_TAG_partial_unit DIE
      // unless we've actually started traversal in that partial unit.
      // In that case it's fully legitimate that ?root holds on such
      // node.
      return pred_result (a.get_dwctx ()->is_root (a.get_die ()));
    }
  };
}

// root
namespace
{
  struct op_root_cu
    : public op_overload <value_cu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cu> a) override
    {
      Dwarf_CU &cu = a->get_cu ();
      Dwarf_Die cudie;
      if (dwarf_cu_die (&cu, &cudie, nullptr, nullptr,
			nullptr, nullptr, nullptr, nullptr) == nullptr)
	throw_libdw ();

      if (a->get_doneness () == doneness::cooked)
	return std::make_unique <value_die> (a->get_dwctx (), cudie, 0);
      else
	return std::make_unique <value_rawdie> (a->get_dwctx (), cudie, 0);
    }
  };

  struct op_root_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    static std::unique_ptr <value>
    do_operate (std::shared_ptr <value_die> a)
    {
      while (a->get_import () != nullptr)
	a = a->get_import ();

      return std::make_unique <value_die>
	(a->get_dwctx (), dwpp_cudie (a->get_die ()), 0);
    }

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      return do_operate (std::move (a));
    }
  };

  struct op_root_rawdie
    : public op_overload <value_rawdie>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawdie> a) override
    {
      return std::make_unique <value_rawdie>
	(a->get_dwctx (), dwpp_cudie (a->get_die ()), 0);
    }
  };
}

// value
namespace
{
  struct op_value_attr
    : public op_yielding_overload <value_attr>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_attr> a) override
    {
      return at_value (a->get_dwctx (), a->get_die (), a->get_attr ());
    }
  };

  struct op_value_loclist_op
    : public op_yielding_overload <value_loclist_op>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_loclist_op> a) override
    {
      return std::make_unique <value_producer_cat>
	(dwop_number (a->get_dwctx (), a->get_attr (), a->get_dwop ()),
	 dwop_number2 (a->get_dwctx (), a->get_attr (), a->get_dwop ()));
    }
  };
}

// low
namespace
{
  struct op_low_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      if (! dwarf_hasattr (&a->get_die (), DW_AT_low_pc))
	return nullptr;
      return get_die_addr (&a->get_die (), &dwarf_lowpc);
    }
  };

  struct op_low_aset
    : public op_overload <value_aset>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_aset> a) override
    {
      auto &cov = a->get_coverage ();
      if (cov.empty ())
	return nullptr;

      return std::make_unique <value_cst>
	(constant {cov.at (0).start, &dw_address_dom ()}, 0);
    }
  };
}

// high
namespace
{
  struct op_high_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      if (! dwarf_hasattr (&a->get_die (), DW_AT_high_pc))
	return nullptr;
      return get_die_addr (&a->get_die (), &dwarf_highpc);
    }
  };

  struct op_high_aset
    : public op_overload <value_aset>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_aset> a) override
    {
      auto &cov = a->get_coverage ();
      if (cov.empty ())
	return nullptr;

      auto range = cov.at (cov.size () - 1);

      return std::make_unique <value_cst>
	(constant {range.start + range.length, &dw_address_dom ()}, 0);
    }
  };
}

// aset
namespace
{
  mpz_class
  addressify (constant c)
  {
    if (! c.dom ()->safe_arith ())
      std::cerr << "Warning: the constant " << c
		<< " doesn't seem to be suitable for use in address sets.\n";

    auto v = c.value ();

    if (v < 0)
      {
	std::cerr
	  << "Warning: Negative values are not allowed in address sets.\n";
	v = 0;
      }

    return v;
  }

  struct op_aset_cst_cst
    : public op_overload <value_cst, value_cst>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cst> a,
	     std::unique_ptr <value_cst> b) override
    {
      auto av = addressify (a->get_constant ());
      auto bv = addressify (b->get_constant ());
      if (av > bv)
	std::swap (av, bv);

      coverage cov;
      cov.add (av.uval (), (bv - av).uval ());
      return std::make_unique <value_aset> (cov, 0);
    }
  };
}

// add
namespace
{
  struct op_add_aset_cst
    : public op_overload <value_aset, value_cst>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_aset> a,
	     std::unique_ptr <value_cst> b) override
    {
      auto bv = addressify (b->get_constant ());
      a->get_coverage ().add (bv.uval (), 1);
      return std::move (a);
    }
  };

  struct op_add_aset_aset
    : public op_overload <value_aset, value_aset>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_aset> a,
	     std::unique_ptr <value_aset> b) override
    {
      a->get_coverage ().add_all (b->get_coverage ());
      return std::move (a);
    }
  };
}

// sub
namespace
{
  struct op_sub_aset_cst
    : public op_overload <value_aset, value_cst>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_aset> a,
	     std::unique_ptr <value_cst> b) override
    {
      auto bv = addressify (b->get_constant ());
      a->get_coverage ().remove (bv.uval (), 1);
      return std::move (a);
    }
  };

  struct op_sub_aset_aset
    : public op_overload <value_aset, value_aset>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_aset> a,
	     std::unique_ptr <value_aset> b) override
    {
      a->get_coverage ().remove_all (b->get_coverage ());
      return std::move (a);
    }
  };
}

// length
namespace
{
  struct op_length_aset
    : public op_overload <value_aset>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_aset> a) override
    {
      uint64_t length = 0;
      for (size_t i = 0; i < a->get_coverage ().size (); ++i)
	length += a->get_coverage ().at (i).length;

      return std::make_unique <value_cst>
	(constant {length, &dec_constant_dom}, 0);
    }
  };
}

// range
namespace
{
  struct op_range_aset
    : public op_yielding_overload <value_aset>
  {
    using op_yielding_overload::op_yielding_overload;

    struct producer
      : public value_producer
    {
      std::unique_ptr <value_aset> m_a;
      size_t m_i;

      producer (std::unique_ptr <value_aset> a)
	: m_a {std::move (a)}
	, m_i {0}
      {}

      std::unique_ptr <value>
      next () override
      {
	size_t sz = m_a->get_coverage ().size ();
	if (m_i >= sz)
	  return nullptr;

	coverage ret;
	auto range = m_a->get_coverage ().at (m_i);
	ret.add (range.start, range.length);

	return std::make_unique <value_aset> (ret, m_i++);
      }
    };

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_aset> a) override
    {
      return std::make_unique <producer> (std::move (a));
    }
  };
}

// ?contains
namespace
{
  struct pred_containsp_aset_cst
    : public pred_overload <value_aset, value_cst>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_aset &a, value_cst &b) override
    {
      auto av = addressify (b.get_constant ());
      return pred_result (a.get_coverage ().is_covered (av.uval (), 1));
    }
  };

  struct pred_containsp_aset_aset
    : public pred_overload <value_aset, value_aset>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_aset &a, value_aset &b) override
    {
      // ?contains holds if A contains all of B.
      for (size_t i = 0; i < b.get_coverage ().size (); ++i)
	{
	  auto range = b.get_coverage ().at (i);
	  if (! a.get_coverage ().is_covered (range.start, range.length))
	    return pred_result::no;
	}
      return pred_result::yes;
    }
  };
}

// ?overlaps
namespace
{
  struct pred_overlapsp_aset_aset
    : public pred_overload <value_aset, value_aset>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_aset &a, value_aset &b) override
    {
      for (size_t i = 0; i < b.get_coverage ().size (); ++i)
	{
	  auto range = b.get_coverage ().at (i);
	  if (a.get_coverage ().is_overlap (range.start, range.length))
	    return pred_result::yes;
	}
      return pred_result::no;
    }
  };
}

// overlap
namespace
{
  struct op_overlap_aset_aset
    : public op_overload <value_aset, value_aset>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_aset> a,
	     std::unique_ptr <value_aset> b) override
    {
      coverage ret;
      for (size_t i = 0; i < b->get_coverage ().size (); ++i)
	{
	  auto range = b->get_coverage ().at (i);
	  auto cov = a->get_coverage ().intersect (range.start, range.length);
	  ret.add_all (cov);
	}
      return std::make_unique <value_aset> (ret, 0);
    }
  };
}

// ?empty
namespace
{
  struct pred_emptyp_aset
    : public pred_overload <value_aset>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_aset &a) override
    {
      return pred_result (a.get_coverage ().empty ());
    }
  };
}

// abbrev
namespace
{
  struct abbrev_producer
    : public value_producer
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    dwfl_module_iterator m_modit;
    cu_iterator m_cuit;
    size_t m_i;

    abbrev_producer (std::shared_ptr <dwfl_context> dwctx)
      : m_dwctx {(assert (dwctx != nullptr), dwctx)}
      , m_modit {m_dwctx->get_dwfl ()}
      , m_cuit {cu_iterator::end ()}
      , m_i {0}
    {}

    std::unique_ptr <value>
    next () override
    {
      if (! maybe_next_module (m_modit, m_cuit))
	return nullptr;

      return std::make_unique <value_abbrev_unit>
	(m_dwctx, *(*m_cuit++)->cu, m_i++);
    }
  };

  struct op_abbrev_dwarf
    : public op_yielding_overload <value_dwarf>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_dwarf> a) override
    {
      return std::make_unique <abbrev_producer> (a->get_dwctx ());
    }
  };

  struct op_abbrev_cu
    : public op_overload <value_cu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cu> a) override
    {
      return std::make_unique <value_abbrev_unit>
	(a->get_dwctx (), a->get_cu (), 0);
    }
  };

  struct op_abbrev_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      // If the DIE doesn't have an abbreviation yet, force its
      // look-up.
      if (a->get_die ().abbrev == nullptr)
	dwarf_haschildren (&a->get_die ());
      assert (a->get_die ().abbrev != nullptr);

      return std::make_unique <value_abbrev>
	(a->get_dwctx (), *a->get_die ().abbrev, 0);
    }
  };
}

// ?haschildren
namespace
{
  struct pred_haschildrenp_die
    : public pred_overload <value_die>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_die &a) override
    {
      return pred_result (dwarf_haschildren (&a.get_die ()));
    }
  };

  struct pred_haschildrenp_abbrev
    : public pred_overload <value_abbrev>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_abbrev &a) override
    {
      return pred_result (dwarf_abbrevhaschildren (&a.get_abbrev ()));
    }
  };
}

// code
namespace
{
  struct op_code_abbrev
    : public op_overload <value_abbrev>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_abbrev> a) override
    {
      constant cst {dwarf_getabbrevcode (&a->get_abbrev ()),
		    &dw_abbrevcode_dom ()};
      return std::make_unique <value_cst> (cst, 0);
    }
  };
}

// version
namespace
{
  struct op_version_cu
    : public op_overload <value_cu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cu> a) override
    {
      Dwarf_CU &cu = a->get_cu ();
      Dwarf_Die cudie;
      Dwarf_Half version;
      if (dwarf_cu_die (&cu, &cudie, &version, nullptr,
			nullptr, nullptr, nullptr, nullptr) == nullptr)
	throw_libdw ();

      constant c {version, &dec_constant_dom};
      return std::make_unique <value_cst> (c, 0);
    }
  };
}

// name
namespace
{
  struct op_name_dwarf
    : public op_overload <value_dwarf>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_dwarf> a) override
    {
      return std::make_unique <value_str> (std::string {a->get_fn ()}, 0);
    }
  };

  std::unique_ptr <value>
  diename (Dwarf_Die &die)
  {
    const char *name = dwarf_diename (&die);
    if (name != nullptr)
      return std::make_unique <value_str> (name, 0);
    else
      return nullptr;
  }

  struct op_name_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      return diename (a->get_die ());
    }
  };

  struct op_name_rawdie
    : public op_overload <value_rawdie>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawdie> a) override
    {
      return diename (a->get_die ());
    }
  };
}

// raw
namespace
{
  struct op_raw_dwarf
    : public op_overload <value_dwarf>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_dwarf> a) override
    {
      return std::make_unique <value_dwarf>
	(a->get_fn (), a->get_dwctx (), 0, doneness::raw);
    }
  };

  struct op_raw_cu
    : public op_overload <value_cu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cu> a) override
    {
      return std::make_unique <value_cu>
	(a->get_dwctx (), a->get_cu (), a->get_offset (), 0, doneness::raw);
    }
  };

  struct op_raw_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      return std::make_unique <value_rawdie>
	(a->get_dwctx (), a->get_die (), 0);
    }
  };
}

// cooked
namespace
{
  struct op_cooked_dwarf
    : public op_overload <value_dwarf>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_dwarf> a) override
    {
      return std::make_unique <value_dwarf>
	(a->get_fn (), a->get_dwctx (), 0, doneness::cooked);
    }
  };

  struct op_cooked_cu
    : public op_overload <value_cu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cu> a) override
    {
      return std::make_unique <value_cu>
	(a->get_dwctx (), a->get_cu (), a->get_offset (), 0, doneness::cooked);
    }
  };

  struct op_cooked_rawdie
    : public op_overload <value_rawdie>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawdie> a) override
    {
      return std::make_unique <value_die>
	(a->get_dwctx (), a->get_die (), 0);
    }
  };
}

// @AT_*
namespace
{
  template <class T>
  class op_atval_die
    : public op_yielding_overload <T>
  {
    int m_atname;

  public:
    op_atval_die (std::shared_ptr <op> upstream, int atname)
      : op_yielding_overload <T> {upstream}
      , m_atname {atname}
    {}

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <T> a)
    {
      Dwarf_Attribute attr;
      if (dwarf_attr (&a->get_die (), m_atname, &attr) == nullptr)
	return false;

      return at_value (a->get_dwctx (), a->get_die (), attr);
    }
  };
}

// ?AT_*
namespace
{
  struct pred_atname_die
    : public pred_overload <value_die>
  {
    unsigned m_atname;

    pred_atname_die (unsigned atname)
      : m_atname {atname}
    {}

    pred_result
    result (value_die &a) override
    {
      Dwarf_Die *die = &a.get_die ();
      return pred_result (dwarf_hasattr (die, m_atname) != 0);
    }
  };

  struct pred_atname_attr
    : public pred_overload <value_attr>
  {
    unsigned m_atname;

    pred_atname_attr (unsigned atname)
      : m_atname {atname}
    {}

    pred_result
    result (value_attr &a) override
    {
      return pred_result (dwarf_whatattr (&a.get_attr ()) == m_atname);
    }
  };

  struct pred_atname_abbrev
    : public pred_overload <value_abbrev>
  {
    unsigned m_atname;

    pred_atname_abbrev (unsigned atname)
      : m_atname {atname}
    {}

    pred_result
    result (value_abbrev &a) override
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
  };

  struct pred_atname_abbrev_attr
    : public pred_overload <value_abbrev_attr>
  {
    unsigned m_atname;

    pred_atname_abbrev_attr (unsigned atname)
      : m_atname {atname}
    {}

    pred_result
    result (value_abbrev_attr &a) override
    {
      return pred_result (a.name == m_atname);
    }
  };

  struct pred_atname_cst
    : public pred_overload <value_cst>
  {
    constant m_const;

    pred_atname_cst (unsigned atname)
      : m_const {atname, &dw_attr_dom ()}
    {}

    pred_result
    result (value_cst &a) override
    {
      check_constants_comparable (m_const, a.get_constant ());
      return pred_result (m_const == a.get_constant ());
    }
  };
}

// ?TAG_*
namespace
{
  template <class T>
  struct pred_tag_die
    : public pred_overload <T>
  {
    int m_tag;

    pred_tag_die (int tag)
      : m_tag {tag}
    {}

    pred_result
    result (T &a) override
    {
      return pred_result (dwarf_tag (&a.get_die ()) == m_tag);
    }
  };

  struct pred_tag_abbrev
    : public pred_overload <value_abbrev>
  {
    unsigned int m_tag;

    pred_tag_abbrev (unsigned int tag)
      : m_tag {tag}
    {}

    pred_result
    result (value_abbrev &a) override
    {
      return pred_result (dwarf_getabbrevtag (&a.get_abbrev ()) == m_tag);
    }
  };

  struct pred_tag_cst
    : public pred_overload <value_cst>
  {
    constant m_const;

    pred_tag_cst (int tag)
      : m_const {(unsigned) tag, &dw_tag_dom ()}
    {}

    pred_result
    result (value_cst &a) override
    {
      check_constants_comparable (m_const, a.get_constant ());
      return pred_result (m_const == a.get_constant ());
    }
  };
}

// ?FORM_*
namespace
{
  struct pred_form_attr
    : public pred_overload <value_attr>
  {
    unsigned m_form;

    pred_form_attr (unsigned form)
      : m_form {form}
    {}

    pred_result
    result (value_attr &a) override
    {
      return pred_result (dwarf_whatform (&a.get_attr ()) == m_form);
    }
  };

  struct pred_form_abbrev_attr
    : public pred_overload <value_abbrev_attr>
  {
    unsigned m_form;

    pred_form_abbrev_attr (unsigned form)
      : m_form {form}
    {}

    pred_result
    result (value_abbrev_attr &a) override
    {
      return pred_result (a.form == m_form);
    }
  };

  struct pred_form_cst
    : public pred_overload <value_cst>
  {
    constant m_const;

    pred_form_cst (unsigned form)
      : m_const {form, &dw_form_dom ()}
    {}

    pred_result
    result (value_cst &a) override
    {
      check_constants_comparable (m_const, a.get_constant ());
      return pred_result (m_const == a.get_constant ());
    }
  };
}

// ?OP_*
namespace
{
  struct pred_op_loclist_elem
    : public pred_overload <value_loclist_elem>
  {
    unsigned m_op;

    pred_op_loclist_elem (unsigned op)
      : m_op {op}
    {}

    pred_result
    result (value_loclist_elem &a) override
    {
      for (size_t i = 0; i < a.get_exprlen (); ++i)
	if (a.get_expr ()[i].atom == m_op)
	  return pred_result::yes;
      return pred_result::no;
    }
  };

  struct pred_op_loclist_op
    : public pred_overload <value_loclist_op>
  {
    unsigned m_op;

    pred_op_loclist_op (unsigned op)
      : m_op {op}
    {}

    pred_result
    result (value_loclist_op &a) override
    {
      return pred_result (a.get_dwop ()->atom == m_op);
    }
  };

  struct pred_op_cst
    : public pred_overload <value_cst>
  {
    constant m_const;

    pred_op_cst (unsigned form)
      : m_const {form, &dw_locexpr_opcode_dom ()}
    {}

    pred_result
    result (value_cst &a) override
    {
      check_constants_comparable (m_const, a.get_constant ());
      return pred_result (m_const == a.get_constant ());
    }
  };
}

std::unique_ptr <vocabulary>
dwgrep_vocabulary_dw ()
{
  auto ret = std::make_unique <vocabulary> ();
  vocabulary &voc = *ret;

  add_builtin_type_constant <value_dwarf> (voc);
  add_builtin_type_constant <value_cu> (voc);
  add_builtin_type_constant <value_die> (voc);
  add_builtin_type_constant <value_rawdie> (voc);
  add_builtin_type_constant <value_attr> (voc);
  add_builtin_type_constant <value_abbrev_unit> (voc);
  add_builtin_type_constant <value_abbrev> (voc);
  add_builtin_type_constant <value_abbrev_attr> (voc);
  add_builtin_type_constant <value_aset> (voc);
  add_builtin_type_constant <value_loclist_elem> (voc);
  add_builtin_type_constant <value_loclist_op> (voc);

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_dwopen_str> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("dwopen", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_unit_dwarf> ();
    t->add_op_overload <op_unit_die> ();
    t->add_op_overload <op_unit_rawdie> ();
    t->add_op_overload <op_unit_attr> ();
    // xxx rawattr

    voc.add (std::make_shared <overloaded_op_builtin> ("unit", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_entry_dwarf> ();
    t->add_op_overload <op_entry_cu> ();
    t->add_op_overload <op_entry_abbrev_unit> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("entry", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_attribute_die> ();
    t->add_op_overload <op_attribute_rawdie> ();
    t->add_op_overload <op_attribute_abbrev> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("attribute", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_rootp_die <value_die>> ();
    t->add_pred_overload <pred_rootp_die <value_rawdie>> ();

    voc.add (std::make_shared <overloaded_pred_builtin> ("?root", t, true));
    voc.add (std::make_shared <overloaded_pred_builtin> ("!root", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_root_cu> ();
    t->add_op_overload <op_root_die> ();
    t->add_op_overload <op_root_rawdie> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("root", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_child_die> ();
    t->add_op_overload <op_child_rawdie> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("child", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_elem_loclist_elem> ();
    t->add_op_overload <op_elem_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("elem", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_relem_loclist_elem> ();
    t->add_op_overload <op_relem_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("relem", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_value_attr> ();
    t->add_op_overload <op_value_loclist_op> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("value", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_offset_cu <value_cu>> ();
    t->add_op_overload <op_offset_die <value_die>> ();
    t->add_op_overload <op_offset_die <value_rawdie>> ();
    t->add_op_overload <op_offset_abbrev_unit> ();
    t->add_op_overload <op_offset_abbrev> ();
    t->add_op_overload <op_offset_abbrev_attr> ();
    t->add_op_overload <op_offset_loclist_op> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("offset", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_address_die> ();
    // xxx rawdie
    t->add_op_overload <op_address_attr> ();
    t->add_op_overload <op_address_loclist_elem> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("address", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_label_die> ();
    // xxx rawdie
    t->add_op_overload <op_label_attr> ();
    // xxx rawattr
    t->add_op_overload <op_label_abbrev> ();
    t->add_op_overload <op_label_abbrev_attr> ();
    t->add_op_overload <op_label_loclist_op> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("label", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_form_attr> ();
    // xxx rawattr
    t->add_op_overload <op_form_abbrev_attr> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("form", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_parent_die> ();
    t->add_op_overload <op_parent_rawdie> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("parent", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_integrate_die> ();
    // xxx rawdie
    // xxx or will the whole integrate thing be dropped?
    t->add_op_overload <op_integrate_closure> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("integrate", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_low_die> ();
    // xxx rawdie
    t->add_op_overload <op_low_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("low", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_high_die> ();
    // xxx rawdie
    t->add_op_overload <op_high_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("high", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_aset_cst_cst> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("aset", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_add_aset_cst> ();
    t->add_op_overload <op_add_aset_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("add", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_sub_aset_cst> ();
    t->add_op_overload <op_sub_aset_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("sub", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_length_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("length", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_range_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("range", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_containsp_aset_cst> ();
    t->add_pred_overload <pred_containsp_aset_aset> ();

    voc.add
      (std::make_shared <overloaded_pred_builtin> ("?contains", t, true));
    voc.add
      (std::make_shared <overloaded_pred_builtin> ("!contains", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_overlapsp_aset_aset> ();

    voc.add
      (std::make_shared <overloaded_pred_builtin> ("?overlaps", t, true));
    voc.add
      (std::make_shared <overloaded_pred_builtin> ("!overlaps", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_overlap_aset_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("overlap", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_emptyp_aset> ();

    voc.add (std::make_shared <overloaded_pred_builtin> ("?empty", t, true));
    voc.add (std::make_shared <overloaded_pred_builtin> ("!empty", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_abbrev_dwarf> ();
    t->add_op_overload <op_abbrev_cu> ();
    t->add_op_overload <op_abbrev_die> ();
    // xxx rawdie

    voc.add (std::make_shared <overloaded_op_builtin> ("abbrev", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_haschildrenp_die> ();
    t->add_pred_overload <pred_haschildrenp_abbrev> ();

    voc.add (std::make_shared
	     <overloaded_pred_builtin> ("?haschildren", t, true));
    voc.add (std::make_shared
	     <overloaded_pred_builtin> ("!haschildren", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_code_abbrev> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("code", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_version_cu> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("version", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_name_dwarf> ();
    t->add_op_overload <op_name_die> ();
    t->add_op_overload <op_name_rawdie> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("name", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_raw_dwarf> ();
    t->add_op_overload <op_raw_cu> ();
    t->add_op_overload <op_raw_die> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("raw", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_cooked_dwarf> ();
    t->add_op_overload <op_cooked_cu> ();
    t->add_op_overload <op_cooked_rawdie> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("cooked", t));
  }

  auto add_dw_at = [&voc] (unsigned code,
			    char const *qname, char const *bname,
			    char const *atname,
			    char const *lqname, char const *lbname,
			    char const *latname)
    {
      // ?AT_* etc.
      {
	auto t = std::make_shared <overload_tab> ();

	t->add_pred_overload <pred_atname_die> (code);
	// xxx rawdie
	t->add_pred_overload <pred_atname_attr> (code);
	// xxx rawattr
	t->add_pred_overload <pred_atname_abbrev> (code);
	t->add_pred_overload <pred_atname_abbrev_attr> (code);
	t->add_pred_overload <pred_atname_cst> (code);

	voc.add
	(std::make_shared <overloaded_pred_builtin> (qname, t, true));
	voc.add
	(std::make_shared <overloaded_pred_builtin> (bname, t, false));
	voc.add
	(std::make_shared <overloaded_pred_builtin> (lqname, t, true));
	voc.add
	(std::make_shared <overloaded_pred_builtin> (lbname, t, false));
      }

      // @AT_* etc.
      {
	auto t = std::make_shared <overload_tab> ();

	t->add_op_overload <op_atval_die <value_die>> (code);
	t->add_op_overload <op_atval_die <value_rawdie>> (code);

	voc.add (std::make_shared <overloaded_op_builtin> (atname, t));
	voc.add (std::make_shared <overloaded_op_builtin> (latname, t));
      }

      // DW_AT_*
      add_builtin_constant (voc, constant (code, &dw_attr_dom ()), lqname + 1);
    };

#define ONE_KNOWN_DW_AT(NAME, CODE)					\
  add_dw_at (CODE, "?AT_" #NAME, "!AT_" #NAME, "@AT_" #NAME,		\
	     "?" #CODE, "!" #CODE, "@" #CODE);
  ALL_KNOWN_DW_AT;
#undef ONE_KNOWN_DW_AT

  auto add_dw_tag = [&voc] (int code,
			     char const *qname, char const *bname,
			     char const *lqname, char const *lbname)
    {
      auto t = std::make_shared <overload_tab> ();

      t->add_pred_overload <pred_tag_die <value_die>> (code);
      t->add_pred_overload <pred_tag_die <value_rawdie>> (code);
      t->add_pred_overload <pred_tag_abbrev> (code);
      t->add_pred_overload <pred_tag_cst> (code);

      voc.add (std::make_shared <overloaded_pred_builtin> (qname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (bname, t, false));
      voc.add (std::make_shared <overloaded_pred_builtin> (lqname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (lbname, t, false));

      add_builtin_constant (voc, constant (code, &dw_tag_dom ()), lqname + 1);
    };

#define ONE_KNOWN_DW_TAG(NAME, CODE)					\
  add_dw_tag (CODE, "?TAG_" #NAME, "!TAG_" #NAME, "?" #CODE, "!" #CODE);
  ALL_KNOWN_DW_TAG;
#undef ONE_KNOWN_DW_TAG

  auto add_dw_form = [&voc] (unsigned code,
			      char const *qname, char const *bname,
			      char const *lqname, char const *lbname)
    {
      auto t = std::make_shared <overload_tab> ();

      t->add_pred_overload <pred_form_attr> (code);
      // xxx rawattr
      t->add_pred_overload <pred_form_abbrev_attr> (code);
      t->add_pred_overload <pred_form_cst> (code);

      voc.add (std::make_shared <overloaded_pred_builtin> (qname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (bname, t, false));
      voc.add (std::make_shared <overloaded_pred_builtin> (lqname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (lbname, t, false));

      add_builtin_constant (voc, constant (code, &dw_form_dom ()), lqname + 1);
    };

#define ONE_KNOWN_DW_FORM_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_FORM (NAME, CODE)
#define ONE_KNOWN_DW_FORM(NAME, CODE)					\
  add_dw_form (CODE, "?FORM_" #NAME, "!FORM_" #NAME, "?" #CODE, "!" #CODE);
  ALL_KNOWN_DW_FORM;
#undef ONE_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM_DESC

  auto add_dw_op = [&voc] (unsigned code,
			    char const *qname, char const *bname,
			    char const *lqname, char const *lbname)
    {
      auto t = std::make_shared <overload_tab> ();

      t->add_pred_overload <pred_op_loclist_elem> (code);
      t->add_pred_overload <pred_op_loclist_op> (code);
      t->add_pred_overload <pred_op_cst> (code);

      voc.add (std::make_shared <overloaded_pred_builtin> (qname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (bname, t, false));
      voc.add (std::make_shared <overloaded_pred_builtin> (lqname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (lbname, t, false));

      add_builtin_constant (voc, constant (code, &dw_locexpr_opcode_dom ()),
			    lqname + 1);
    };

#define ONE_KNOWN_DW_OP_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_OP (NAME, CODE)
#define ONE_KNOWN_DW_OP(NAME, CODE)					\
  add_dw_op (CODE, "?OP_" #NAME, "!OP_" #NAME, "?" #CODE, "!" #CODE);
  ALL_KNOWN_DW_OP;
#undef ONE_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP_DESC

#define ONE_KNOWN_DW_LANG_DESC(NAME, CODE, DESC)			\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_lang_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_LANG;
#undef ONE_KNOWN_DW_LANG_DESC

#define ONE_KNOWN_DW_MACINFO(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_macinfo_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_MACINFO;
#undef ONE_KNOWN_DW_MACINFO

#define ONE_KNOWN_DW_MACRO_GNU(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_macro_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_MACRO_GNU;
#undef ONE_KNOWN_DW_MACRO_GNU

#define ONE_KNOWN_DW_INL(NAME, CODE)					\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_inline_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_INL;
#undef ONE_KNOWN_DW_INL

#define ONE_KNOWN_DW_ATE(NAME, CODE)					\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_encoding_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_ATE;
#undef ONE_KNOWN_DW_ATE

#define ONE_KNOWN_DW_ACCESS(NAME, CODE)					\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_access_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_ACCESS;
#undef ONE_KNOWN_DW_ACCESS

#define ONE_KNOWN_DW_VIS(NAME, CODE)					\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_visibility_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_VIS;
#undef ONE_KNOWN_DW_VIS

#define ONE_KNOWN_DW_VIRTUALITY(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_virtuality_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_VIRTUALITY;
#undef ONE_KNOWN_DW_VIRTUALITY

#define ONE_KNOWN_DW_ID(NAME, CODE)					\
  {									\
    add_builtin_constant (voc,						\
			  constant (CODE, &dw_identifier_case_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_ID;
#undef ONE_KNOWN_DW_ID

#define ONE_KNOWN_DW_CC(NAME, CODE)					\
  {									\
    add_builtin_constant (voc,						\
			  constant (CODE, &dw_calling_convention_dom ()), \
			  #CODE);					\
  }
  ALL_KNOWN_DW_CC;
#undef ONE_KNOWN_DW_CC

#define ONE_KNOWN_DW_ORD(NAME, CODE)					\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_ordering_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_ORD;
#undef ONE_KNOWN_DW_ORD

#define ONE_KNOWN_DW_DSC(NAME, CODE)					\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_discr_list_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_DSC;
#undef ONE_KNOWN_DW_DSC

#define ONE_KNOWN_DW_DS(NAME, CODE)					\
  {									\
    add_builtin_constant (voc,						\
			  constant (CODE, &dw_decimal_sign_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_DS;
#undef ONE_KNOWN_DW_DS

  add_builtin_constant (voc, constant (DW_ADDR_none, &dw_address_class_dom ()),
			"DW_ADDR_none");

#define ONE_KNOWN_DW_END(NAME, CODE)					\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_endianity_dom ()), #CODE); \
  }
  ALL_KNOWN_DW_END;
#undef ONE_KNOWN_DW_END

  return ret;
}
