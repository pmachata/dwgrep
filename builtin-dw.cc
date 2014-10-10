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
      return std::make_unique <value_dwarf> (a->get_string (), 0);
    }
  };
}

// entry
namespace
{
  struct op_entry_dwarf
    : public op_yielding_overload <value_dwarf>
  {
    struct producer
      : public value_producer
    {
      std::shared_ptr <dwfl_context> m_dwctx;
      dwfl_module_iterator m_modit;
      all_dies_iterator m_it;
      size_t m_i;

      producer (std::shared_ptr <dwfl_context> dwctx)
	: m_dwctx {(assert (dwctx != nullptr), dwctx)}
	, m_modit {m_dwctx->get_dwfl ()}
	, m_it {all_dies_iterator::end ()}
	, m_i {0}
      {}

      std::unique_ptr <value>
      next () override
      {
	if (m_it == all_dies_iterator::end ())
	  {
	    if (m_modit == dwfl_module_iterator::end ())
	      return nullptr;

	    auto ret = *m_modit++;
	    assert (ret.first != nullptr);
	    m_it = all_dies_iterator (ret.first);
	  }

	return std::make_unique <value_die> (m_dwctx, **m_it++, m_i++);
      }
    };

    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_dwarf> a) override
    {
      return std::make_unique <producer> (a->get_dwctx ());
    }
  };

  std::pair <all_dies_iterator, all_dies_iterator>
  cu_it_range (Dwarf_Die cudie)
  {
    Dwarf *dw = dwarf_cu_getdwarf (cudie.cu);
    cu_iterator cuit {dw, cudie};
    std::cerr << "Will iterate through CU " << cuit.offset () << std::endl;
    all_dies_iterator a (cuit);
    all_dies_iterator e (++cuit);
    return std::make_pair (a, e);
  }

  bool
  inline_partial_units (std::vector <std::pair <all_dies_iterator,
						all_dies_iterator>> &stack)
  {
    Dwarf_Die *die = *stack.back ().first;
    Dwarf_Attribute at_import;
    Dwarf_Die cudie;
    if (dwarf_tag (die) == DW_TAG_imported_unit
	&& dwarf_hasattr (die, DW_AT_import)
	&& dwarf_attr (die, DW_AT_import, &at_import) != nullptr
	&& dwarf_formref_die (&at_import, &cudie) != nullptr)
      {
	// Skip DW_TAG_imported_unit.
	stack.back ().first++;

	stack.push_back (cu_it_range (cudie));

	// Skip root DIE of DW_TAG_partial_unit.
	stack.back ().first++;
	return true;
      }

    return false;
  }

  bool
  drop_finished_it_range (std::vector <std::pair <all_dies_iterator,
						  all_dies_iterator>> &stack)
  {
    assert (! stack.empty ());
    if (stack.back ().first != stack.back ().second)
      return false;

    stack.pop_back ();
    return true;
  }

  template <bool Cooked>
  struct cu_entry_producer
    : public value_producer
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    // Stack of iterator ranges.
    std::vector <std::pair <all_dies_iterator, all_dies_iterator>> m_stack;
    size_t m_i;

    cu_entry_producer (std::shared_ptr <dwfl_context> dwctx, Dwarf_CU &cu)
      : m_dwctx {dwctx}
      , m_i {0}
    {
      m_stack.push_back (cu_it_range (dwpp_cudie (cu)));
    }

    std::unique_ptr <value>
    next () override
    {
      do
	if (m_stack.empty ())
	  return nullptr;
      while (drop_finished_it_range (m_stack)
	     || (Cooked && inline_partial_units (m_stack)));

      return std::make_unique <value_die>
	(m_dwctx, **m_stack.back ().first++, m_i++);
    }
  };

  struct op_entry_cu
    : public op_yielding_overload <value_cu>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_cu> a) override
    {
      return std::make_unique <cu_entry_producer <true>>
	(a->get_dwctx (), a->get_cu ());
    }
  };

  struct op_entry_rawcu
    : public op_yielding_overload <value_rawcu>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_rawcu> a) override
    {
      return std::make_unique <cu_entry_producer <false>>
	(a->get_dwctx (), a->get_cu ());
    }
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
  unit_acceptable (bool cooked, Dwarf_Die die)
  {
    if (! cooked)
      return true;

    // In cooked mode, we reject partial units.
    // XXX Should we reject type units as well?
    return dwarf_tag (&die) != DW_TAG_partial_unit;
  }

  template <bool Cooked>
  struct dwarf_unit_producer
    : public value_producer
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    dwfl_module_iterator m_modit;
    cu_iterator m_cuit;
    size_t m_i;

    dwarf_unit_producer (std::shared_ptr <dwfl_context> dwctx)
      : m_dwctx {dwctx}
      , m_modit {m_dwctx->get_dwfl ()}
      , m_cuit {cu_iterator::end ()}
      , m_i {0}
    {}

    std::unique_ptr <value>
    next () override
    {
      do
	if (! maybe_next_module (m_modit, m_cuit))
	  return nullptr;
      while (unit_acceptable (Cooked, **m_cuit)
	     ? false : (m_cuit++, true));

      std::unique_ptr <value> ret;
      if (Cooked)
	ret = std::make_unique <value_cu>
	  (m_dwctx, *(*m_cuit)->cu, m_cuit.offset (), m_i);
      else
	ret = std::make_unique <value_rawcu>
	  (m_dwctx, *(*m_cuit)->cu, m_cuit.offset (), m_i);

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
      return std::make_unique <dwarf_unit_producer <true>> (a->get_dwctx ());
    }
  };

  struct op_unit_rawdwarf
    : public op_yielding_overload <value_rawdwarf>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_rawdwarf> a) override
    {
      return std::make_unique <dwarf_unit_producer <false>> (a->get_dwctx ());
    }
  };

  std::unique_ptr <value>
  cu_for_die (std::shared_ptr <dwfl_context> dwctx, Dwarf_Die die)
  {
    Dwarf_Die cudie;
    if (dwarf_diecu (&die, &cudie, nullptr, nullptr) == nullptr)
      throw_libdw ();

    Dwarf *dw = dwarf_cu_getdwarf (cudie.cu);
    cu_iterator cuit {dw, cudie};

    return std::make_unique <value_cu> (dwctx, *(*cuit)->cu,
					cuit.offset (), 0);
  }

  struct op_unit_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      return cu_for_die (a->get_dwctx (), a->get_die ());
    }
  };

  struct op_unit_attr
    : public op_overload <value_attr>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_attr> a) override
    {
      return cu_for_die (a->get_dwctx (), a->get_die ());
    }
  };
}

// child
namespace
{
  struct op_child_die
    : public op_yielding_overload <value_die>
  {
    using op_yielding_overload::op_yielding_overload;

    struct producer
      : public value_producer
    {
      std::unique_ptr <value_die> m_child;

      producer (std::unique_ptr <value_die> child)
	: m_child {std::move (child)}
      {}

      std::unique_ptr <value>
      next () override
      {
	if (m_child == nullptr)
	  return nullptr;

	std::unique_ptr <value_die> ret = std::move (m_child);

	Dwarf_Die child;
	switch (dwarf_siblingof (&ret->get_die (), &child))
	  {
	  case 0:
	    m_child = std::make_unique <value_die>
	      (ret->get_dwctx (), child, ret->get_pos () + 1);
	  case 1: // no more siblings
	    return std::move (ret);
	  }

	throw_libdw ();
      }
    };

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_die> a) override
    {
      Dwarf_Die *die = &a->get_die ();
      if (dwarf_haschildren (die))
	{
	  Dwarf_Die child;
	  if (dwarf_child (die, &child) != 0)
	    throw_libdw ();

	  auto value = std::make_unique <value_die> (a->get_dwctx (), child, 0);
	  return std::make_unique <producer> (std::move (value));
	}

      return nullptr;
    }
  };
}

// elem, relem
namespace
{
  template <class T>
  struct elem_loclist_producer
    : public value_producer
  {
    std::unique_ptr <value_loclist_elem> m_value;
    size_t m_i;

    elem_loclist_producer (std::unique_ptr <value_loclist_elem> value)
      : m_value {std::move (value)}
      , m_i {0}
    {}

    std::unique_ptr <value>
    next () override
    {
      size_t idx = m_i++;
      if (idx < m_value->get_exprlen ())
	return std::make_unique <value_loclist_op>
	  (m_value->get_dwctx (), m_value->get_attr (),
	   T::get_expr (*m_value, idx), idx);
      else
	return nullptr;
    }
  };

  struct op_elem_loclist_elem
    : public op_yielding_overload <value_loclist_elem>
  {
    using op_yielding_overload::op_yielding_overload;

    struct producer
      : public elem_loclist_producer <producer>
    {
      using elem_loclist_producer::elem_loclist_producer;

      static Dwarf_Op *
      get_expr (value_loclist_elem &v, size_t idx)
      {
	return v.get_expr () + idx;
      }
    };

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_loclist_elem> a) override
    {
      return std::make_unique <producer> (std::move (a));
    }
  };

  struct op_relem_loclist_elem
    : public op_yielding_overload <value_loclist_elem>
  {
    using op_yielding_overload::op_yielding_overload;

    struct producer
      : public elem_loclist_producer <producer>
    {
      using elem_loclist_producer::elem_loclist_producer;

      static Dwarf_Op *
      get_expr (value_loclist_elem &v, size_t idx)
      {
	return v.get_expr () + v.get_exprlen () - 1 - idx;
      }
    };

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_loclist_elem> a) override
    {
      return std::make_unique <producer> (std::move (a));
    }
  };

  template <bool Forward>
  struct op_elem_aset
    : public op_yielding_overload <value_aset>
  {
    using op_yielding_overload::op_yielding_overload;

    struct producer
      : public value_producer
    {
      coverage cov;
      size_t m_idx;	// position among ranges
      uint64_t m_ai;	// iteration through a range
      size_t m_i;	// produced value counter

      producer (coverage a_cov)
	: cov {a_cov}
	, m_idx {0}
	, m_ai {0}
	, m_i {0}
      {}

      std::unique_ptr <value>
      next () override
      {
	if (m_idx >= cov.size ())
	  return nullptr;

	auto idx = [&] () {
	  return Forward ? m_idx : cov.size () - 1 - m_idx;
	};

	if (m_ai >= cov.at (idx ()).length)
	  {
	    m_idx++;
	    if (m_idx >= cov.size ())
	      return nullptr;
	    assert (cov.at (idx ()).length > 0);
	    m_ai = 0;
	  }

	uint64_t ai = Forward ? m_ai : cov.at (idx ()).length - 1 - m_ai;
	uint64_t addr = cov.at (idx ()).start + ai;
	m_ai++;

	return std::make_unique <value_cst>
	  (constant {addr, &dw_address_dom}, m_i++);
      }
    };

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_aset> val) override
    {
      return std::make_unique <producer> (val->get_coverage ());
    }
  };
}

// attribute
namespace
{
  struct op_attribute_die
    : public op_yielding_overload <value_die>
  {
    using op_yielding_overload::op_yielding_overload;

    struct producer
      : public value_producer
    {
      std::unique_ptr <value_die> m_value;
      attr_iterator m_it;
      size_t m_i;

      producer (std::unique_ptr <value_die> value)
	: m_value {std::move (value)}
	, m_it {attr_iterator {&m_value->get_die ()}}
	, m_i {0}
      {}

      std::unique_ptr <value>
      next () override
      {
	if (m_it != attr_iterator::end ())
	  return std::make_unique <value_attr>
	    (m_value->get_dwctx (), **m_it++, m_value->get_die (), m_i++);
	else
	  return nullptr;
      }
    };

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_die> a) override
    {
      return std::make_unique <producer> (std::move (a));
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
    constant c {offset, &dw_offset_dom};
    return std::make_unique <value_cst> (c, 0);
  }

  struct op_offset_cu
    : public op_overload <value_cu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cu> a) override
    {
      return cu_offset (a->get_offset ());
    }
  };

  struct op_offset_rawcu
    : public op_overload <value_rawcu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawcu> a) override
    {
      return cu_offset (a->get_offset ());
    }
  };

  struct op_offset_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> val) override
    {
      constant c {dwarf_dieoffset (&val->get_die ()), &dw_offset_dom};
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

      constant c {abbrev_off, &dw_offset_dom};
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
      constant c {dwpp_abbrev_offset (a->get_abbrev ()), &dw_offset_dom};
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
      constant c {a->offset, &dw_offset_dom};
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
      constant c {dwop->offset, &dw_offset_dom};
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
    return std::make_unique <value_cst> (constant {addr, &dw_address_dom}, 0);
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
	    (constant {addr, &dw_address_dom}, 0);
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
      constant cst {(unsigned) tag, &dw_tag_dom};
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
      constant cst {dwarf_whatattr (&val->get_attr ()), &dw_attr_dom};
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
      constant cst {(unsigned) tag, &dw_tag_dom};
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
      constant cst {a->name, &dw_attr_dom};
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
      constant cst {val->get_dwop ()->atom, &dw_locexpr_opcode_dom,
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
      constant cst {dwarf_whatform (&val->get_attr ()), &dw_form_dom};
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
      constant cst {a->form, &dw_form_dom};
      return std::make_unique <value_cst> (cst, 0);
    }
  };
}

// parent
namespace
{
  struct op_parent_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      Dwarf_Off par_off = a->get_dwctx ()->find_parent (a->get_die ());
      if (par_off == parent_cache::no_off)
	return nullptr;

      Dwarf_Die par_die;
      if (dwarf_offdie (dwarf_cu_getdwarf (a->get_die ().cu),
			par_off, &par_die) == nullptr)
	throw_libdw ();

      return std::make_unique <value_die> (a->get_dwctx (), par_die, 0);
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
  struct pred_rootp_die
    : public pred_overload <value_die>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_die &a) override
    {
      return pred_result (a.get_dwctx ()->is_root (a.get_die ()));
    }
  };

  struct pred_rootp_attr
    : public pred_overload <value_attr>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_attr &a) override
    {
      // By definition, attributes are never root.
      return pred_result::no;
    }
  };
}

// root
namespace
{
  std::unique_ptr <value>
  cu_root (std::shared_ptr <dwfl_context> dwctx, Dwarf_CU &cu)
  {
    Dwarf_Die cudie;
    if (dwarf_cu_die (&cu, &cudie, nullptr, nullptr,
		      nullptr, nullptr, nullptr, nullptr) == nullptr)
      throw_libdw ();
    return std::make_unique <value_die> (dwctx, cudie, 0);
  }

  struct op_root_cu
    : public op_overload <value_cu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cu> a) override
    {
      return cu_root (a->get_dwctx (), a->get_cu ());
    }
  };

  struct op_root_rawcu
    : public op_overload <value_rawcu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawcu> a) override
    {
      return cu_root (a->get_dwctx (), a->get_cu ());
    }
  };

  struct op_root_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      Dwarf_Die cudie;
      if (dwarf_diecu (&a->get_die (), &cudie, nullptr, nullptr) == nullptr)
	throw_libdw ();
      return std::make_unique <value_die> (a->get_dwctx (), cudie, 0);
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
	(constant {cov.at (0).start, &dw_address_dom}, 0);
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
	(constant {range.start + range.length, &dw_address_dom}, 0);
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

  struct op_abbrev_rawdwarf
    : public op_yielding_overload <value_rawdwarf>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_rawdwarf> a) override
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

  struct op_abbrev_rawcu
    : public op_overload <value_rawcu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawcu> a) override
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
		    &dw_abbrevcode_dom};
      return std::make_unique <value_cst> (cst, 0);
    }
  };
}

// version
namespace
{
  std::unique_ptr <value>
  cu_version (Dwarf_CU &cu)
  {
    Dwarf_Die cudie;
    Dwarf_Half version;
    if (dwarf_cu_die (&cu, &cudie, &version, nullptr,
		      nullptr, nullptr, nullptr, nullptr) == nullptr)
      throw_libdw ();

    constant c {version, &dec_constant_dom};
    return std::make_unique <value_cst> (c, 0);
  }

  struct op_version_cu
    : public op_overload <value_cu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cu> a) override
    {
      return cu_version (a->get_cu ());
    }
  };

  struct op_version_rawcu
    : public op_overload <value_rawcu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawcu> a) override
    {
      return cu_version (a->get_cu ());
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

  struct op_name_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> a) override
    {
      const char *name = dwarf_diename (&a->get_die ());
      if (name != nullptr)
	return std::make_unique <value_str> (name, 0);
      else
	return nullptr;
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
      return std::make_unique <value_rawdwarf>
	(a->get_fn (), a->get_dwctx (), 0);
    }
  };

  struct op_raw_cu
    : public op_overload <value_cu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cu> a) override
    {
      return std::make_unique <value_rawcu>
	(a->get_dwctx (), a->get_cu (), a->get_offset (), 0);
    }
  };
}

// cooked
namespace
{
  struct op_cooked_rawdwarf
    : public op_overload <value_rawdwarf>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawdwarf> a) override
    {
      return std::make_unique <value_dwarf>
	(a->get_fn (), a->get_dwctx (), 0);
    }
  };

  struct op_cooked_rawcu
    : public op_overload <value_rawcu>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_rawcu> a) override
    {
      return std::make_unique <value_cu>
	(a->get_dwctx (), a->get_cu (), a->get_offset (), 0);
    }
  };
}

// @AT_*
namespace
{
  class op_atval_die
    : public op_yielding_overload <value_die>
  {
    int m_atname;

  public:
    op_atval_die (std::shared_ptr <op> upstream, int atname)
      : op_yielding_overload {upstream}
      , m_atname {atname}
    {}

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_die> a)
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
      : m_const {atname, &dw_attr_dom}
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
  struct pred_tag_die
    : public pred_overload <value_die>
  {
    int m_tag;

    pred_tag_die (int tag)
      : m_tag {tag}
    {}

    pred_result
    result (value_die &a) override
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
      : m_const {(unsigned) tag, &dw_tag_dom}
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
      : m_const {form, &dw_form_dom}
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
      : m_const {form, &dw_locexpr_opcode_dom}
    {}

    pred_result
    result (value_cst &a) override
    {
      check_constants_comparable (m_const, a.get_constant ());
      return pred_result (m_const == a.get_constant ());
    }
  };
}

std::unique_ptr <builtin_dict>
dwgrep_builtins_dw ()
{
  auto ret = std::make_unique <builtin_dict> ();
  builtin_dict &dict = *ret;

  add_builtin_type_constant <value_dwarf> (dict);
  add_builtin_type_constant <value_rawdwarf> (dict);
  add_builtin_type_constant <value_cu> (dict);
  add_builtin_type_constant <value_rawcu> (dict);
  add_builtin_type_constant <value_die> (dict);
  add_builtin_type_constant <value_attr> (dict);
  add_builtin_type_constant <value_abbrev_unit> (dict);
  add_builtin_type_constant <value_abbrev> (dict);
  add_builtin_type_constant <value_abbrev_attr> (dict);
  add_builtin_type_constant <value_aset> (dict);
  add_builtin_type_constant <value_loclist_elem> (dict);
  add_builtin_type_constant <value_loclist_op> (dict);

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_dwopen_str> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("dwopen", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_entry_dwarf> ();
    // xxx rawdwarf
    // entry on cooked dwarf only iterates full units, but
    // transparently includes partial ones.  Thus entries from a
    // certain partial unit may never be seen if that unit is never
    // imported.
    t->add_op_overload <op_entry_cu> ();
    t->add_op_overload <op_entry_rawcu> ();
    t->add_op_overload <op_entry_abbrev_unit> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("entry", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_unit_dwarf> ();
    t->add_op_overload <op_unit_rawdwarf> ();
    t->add_op_overload <op_unit_die> ();
    // xxx rawdie
    t->add_op_overload <op_unit_attr> ();
    // xxx rawattr

    dict.add (std::make_shared <overloaded_op_builtin> ("unit", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_attribute_die> ();
    // xxx rawdie
    // should cooked DIE yield even integrated attributes?  It seems
    // like it should, but it's somewhat surprising...
    t->add_op_overload <op_attribute_abbrev> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("attribute", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_rootp_die> ();
    // xxx rawdie
    t->add_pred_overload <pred_rootp_attr> ();
    // xxx rawattr
    // xxx actually, does ?root over attributes make sense at all?
    // This idea comes back to times where we thought about the tree
    // as one whole.  But now the traversal interface is much more
    // fine-grained, and asking for root attributes is simply
    // meaningless.

    dict.add (std::make_shared <overloaded_pred_builtin <true>> ("?root", t));
    dict.add (std::make_shared <overloaded_pred_builtin <false>> ("!root", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_root_cu> ();
    t->add_op_overload <op_root_rawcu> ();
    t->add_op_overload <op_root_die> ();
    // xxx rawdie
    // cooked child returns to root of the original unit where it came
    // from, across transparent partial unit inlines

    dict.add (std::make_shared <overloaded_op_builtin> ("root", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_child_die> ();
    // xxx rawdie
    // cooked child transparently inlines partial unit imports

    dict.add (std::make_shared <overloaded_op_builtin> ("child", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_elem_loclist_elem> ();
    t->add_op_overload <op_elem_aset <true>> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("elem", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_relem_loclist_elem> ();
    t->add_op_overload <op_elem_aset <false>> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("relem", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_value_attr> ();
    t->add_op_overload <op_value_loclist_op> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("value", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_offset_cu> ();
    t->add_op_overload <op_offset_rawcu> ();
    t->add_op_overload <op_offset_die> ();
    // xxx rawdie
    t->add_op_overload <op_offset_abbrev_unit> ();
    t->add_op_overload <op_offset_abbrev> ();
    t->add_op_overload <op_offset_abbrev_attr> ();
    t->add_op_overload <op_offset_loclist_op> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("offset", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_address_die> ();
    // xxx rawdie
    t->add_op_overload <op_address_attr> ();
    t->add_op_overload <op_address_loclist_elem> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("address", t));
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

    dict.add (std::make_shared <overloaded_op_builtin> ("label", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_form_attr> ();
    // xxx rawattr
    t->add_op_overload <op_form_abbrev_attr> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("form", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_parent_die> ();
    // xxx rawdie

    // The difference here will be that cooked DIE's will travel
    // across partial unit inlining boundaries, while raw DIE's will
    // just stop.

    dict.add (std::make_shared <overloaded_op_builtin> ("parent", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_integrate_die> ();
    // xxx rawdie
    // xxx or will the whole integrate thing be dropped?
    t->add_op_overload <op_integrate_closure> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("integrate", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_low_die> ();
    // xxx rawdie
    t->add_op_overload <op_low_aset> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("low", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_high_die> ();
    // xxx rawdie
    t->add_op_overload <op_high_aset> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("high", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_aset_cst_cst> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("aset", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_add_aset_cst> ();
    t->add_op_overload <op_add_aset_aset> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("add", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_sub_aset_cst> ();
    t->add_op_overload <op_sub_aset_aset> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("sub", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_length_aset> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("length", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_range_aset> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("range", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_containsp_aset_cst> ();
    t->add_pred_overload <pred_containsp_aset_aset> ();

    dict.add
      (std::make_shared <overloaded_pred_builtin <true>> ("?contains", t));
    dict.add
      (std::make_shared <overloaded_pred_builtin <false>> ("!contains", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_overlapsp_aset_aset> ();

    dict.add
      (std::make_shared <overloaded_pred_builtin <true>> ("?overlaps", t));
    dict.add
      (std::make_shared <overloaded_pred_builtin <false>> ("!overlaps", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_overlap_aset_aset> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("overlap", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_emptyp_aset> ();

    dict.add (std::make_shared <overloaded_pred_builtin <true>> ("?empty", t));
    dict.add (std::make_shared <overloaded_pred_builtin <false>> ("!empty", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_abbrev_dwarf> ();
    t->add_op_overload <op_abbrev_rawdwarf> ();
    t->add_op_overload <op_abbrev_cu> ();
    t->add_op_overload <op_abbrev_rawcu> ();
    t->add_op_overload <op_abbrev_die> ();
    // xxx rawdie

    dict.add (std::make_shared <overloaded_op_builtin> ("abbrev", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_haschildrenp_die> ();
    t->add_pred_overload <pred_haschildrenp_abbrev> ();

    dict.add (std::make_shared
		<overloaded_pred_builtin <true>> ("?haschildren", t));
    dict.add (std::make_shared
		<overloaded_pred_builtin <false>> ("!haschildren", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_code_abbrev> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("code", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_version_cu> ();
    t->add_op_overload <op_version_rawcu> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("version", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_name_dwarf> ();
    t->add_op_overload <op_name_die> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("name", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_raw_dwarf> ();
    t->add_op_overload <op_raw_cu> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("raw", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_cooked_rawdwarf> ();
    t->add_op_overload <op_cooked_rawcu> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("cooked", t));
  }

  auto add_dw_at = [&dict] (unsigned code,
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

	dict.add
	  (std::make_shared <overloaded_pred_builtin <true>> (qname, t));
	dict.add
	  (std::make_shared <overloaded_pred_builtin <false>> (bname, t));
	dict.add
	  (std::make_shared <overloaded_pred_builtin <true>> (lqname, t));
	dict.add
	  (std::make_shared <overloaded_pred_builtin <false>> (lbname, t));
      }

      // @AT_* etc.
      {
	auto t = std::make_shared <overload_tab> ();

	t->add_op_overload <op_atval_die> (code);
	// xxx rawdie

	dict.add (std::make_shared <overloaded_op_builtin> (atname, t));
	dict.add (std::make_shared <overloaded_op_builtin> (latname, t));
      }

      // DW_AT_*
      add_builtin_constant (dict, constant (code, &dw_attr_dom), lqname + 1);
    };

#define ONE_KNOWN_DW_AT(NAME, CODE)					\
  add_dw_at (CODE, "?AT_" #NAME, "!AT_" #NAME, "@AT_" #NAME,		\
	     "?" #CODE, "!" #CODE, "@" #CODE);
  ALL_KNOWN_DW_AT;
#undef ONE_KNOWN_DW_AT

  auto add_dw_tag = [&dict] (int code,
			     char const *qname, char const *bname,
			     char const *lqname, char const *lbname)
    {
      auto t = std::make_shared <overload_tab> ();

      t->add_pred_overload <pred_tag_die> (code);
      // xxx rawdie
      t->add_pred_overload <pred_tag_abbrev> (code);
      t->add_pred_overload <pred_tag_cst> (code);

      dict.add (std::make_shared <overloaded_pred_builtin <true>> (qname, t));
      dict.add (std::make_shared <overloaded_pred_builtin <false>> (bname, t));
      dict.add (std::make_shared <overloaded_pred_builtin <true>> (lqname, t));
      dict.add (std::make_shared <overloaded_pred_builtin <false>> (lbname, t));

      add_builtin_constant (dict, constant (code, &dw_tag_dom), lqname + 1);
    };

#define ONE_KNOWN_DW_TAG(NAME, CODE)					\
  add_dw_tag (CODE, "?TAG_" #NAME, "!TAG_" #NAME, "?" #CODE, "!" #CODE);
  ALL_KNOWN_DW_TAG;
#undef ONE_KNOWN_DW_TAG

  auto add_dw_form = [&dict] (unsigned code,
			      char const *qname, char const *bname,
			      char const *lqname, char const *lbname)
    {
      auto t = std::make_shared <overload_tab> ();

      t->add_pred_overload <pred_form_attr> (code);
      // xxx rawattr
      t->add_pred_overload <pred_form_abbrev_attr> (code);
      t->add_pred_overload <pred_form_cst> (code);

      dict.add (std::make_shared <overloaded_pred_builtin <true>> (qname, t));
      dict.add (std::make_shared <overloaded_pred_builtin <false>> (bname, t));
      dict.add (std::make_shared <overloaded_pred_builtin <true>> (lqname, t));
      dict.add (std::make_shared <overloaded_pred_builtin <false>> (lbname, t));

      add_builtin_constant (dict, constant (code, &dw_form_dom), lqname + 1);
    };

#define ONE_KNOWN_DW_FORM_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_FORM (NAME, CODE)
#define ONE_KNOWN_DW_FORM(NAME, CODE)					\
  add_dw_form (CODE, "?FORM_" #NAME, "!FORM_" #NAME, "?" #CODE, "!" #CODE);
  ALL_KNOWN_DW_FORM;
#undef ONE_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM_DESC

  auto add_dw_op = [&dict] (unsigned code,
			    char const *qname, char const *bname,
			    char const *lqname, char const *lbname)
    {
      auto t = std::make_shared <overload_tab> ();

      t->add_pred_overload <pred_op_loclist_elem> (code);
      t->add_pred_overload <pred_op_loclist_op> (code);
      t->add_pred_overload <pred_op_cst> (code);

      dict.add (std::make_shared <overloaded_pred_builtin <true>> (qname, t));
      dict.add (std::make_shared <overloaded_pred_builtin <false>> (bname, t));
      dict.add (std::make_shared <overloaded_pred_builtin <true>> (lqname, t));
      dict.add (std::make_shared <overloaded_pred_builtin <false>> (lbname, t));

      add_builtin_constant (dict, constant (code, &dw_locexpr_opcode_dom),
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
    add_builtin_constant (dict, constant (CODE, &dw_lang_dom), #CODE);	\
  }
  ALL_KNOWN_DW_LANG;
#undef ONE_KNOWN_DW_LANG_DESC

#define ONE_KNOWN_DW_MACINFO(NAME, CODE)				\
  {									\
    add_builtin_constant (dict, constant (CODE, &dw_macinfo_dom), #CODE); \
  }
  ALL_KNOWN_DW_MACINFO;
#undef ONE_KNOWN_DW_MACINFO

#define ONE_KNOWN_DW_MACRO_GNU(NAME, CODE)				\
  {									\
    add_builtin_constant (dict, constant (CODE, &dw_macro_dom), #CODE); \
  }
  ALL_KNOWN_DW_MACRO_GNU;
#undef ONE_KNOWN_DW_MACRO_GNU

#define ONE_KNOWN_DW_INL(NAME, CODE)					\
  {									\
    add_builtin_constant (dict, constant (CODE, &dw_inline_dom), #CODE); \
  }
  ALL_KNOWN_DW_INL;
#undef ONE_KNOWN_DW_INL

#define ONE_KNOWN_DW_ATE(NAME, CODE)					\
  {									\
    add_builtin_constant (dict, constant (CODE, &dw_encoding_dom), #CODE); \
  }
  ALL_KNOWN_DW_ATE;
#undef ONE_KNOWN_DW_ATE

#define ONE_KNOWN_DW_ACCESS(NAME, CODE)					\
  {									\
    add_builtin_constant (dict, constant (CODE, &dw_access_dom), #CODE); \
  }
  ALL_KNOWN_DW_ACCESS;
#undef ONE_KNOWN_DW_ACCESS

#define ONE_KNOWN_DW_VIS(NAME, CODE)					\
  {									\
    add_builtin_constant (dict, constant (CODE, &dw_visibility_dom), #CODE); \
  }
  ALL_KNOWN_DW_VIS;
#undef ONE_KNOWN_DW_VIS

#define ONE_KNOWN_DW_VIRTUALITY(NAME, CODE)				\
  {									\
    add_builtin_constant (dict, constant (CODE, &dw_virtuality_dom), #CODE); \
  }
  ALL_KNOWN_DW_VIRTUALITY;
#undef ONE_KNOWN_DW_VIRTUALITY

#define ONE_KNOWN_DW_ID(NAME, CODE)					\
  {									\
    add_builtin_constant (dict,						\
			  constant (CODE, &dw_identifier_case_dom), #CODE); \
  }
  ALL_KNOWN_DW_ID;
#undef ONE_KNOWN_DW_ID

#define ONE_KNOWN_DW_CC(NAME, CODE)					\
  {									\
    add_builtin_constant (dict,						\
			  constant (CODE, &dw_calling_convention_dom), #CODE); \
  }
  ALL_KNOWN_DW_CC;
#undef ONE_KNOWN_DW_CC

#define ONE_KNOWN_DW_ORD(NAME, CODE)					\
  {									\
    add_builtin_constant (dict, constant (CODE, &dw_ordering_dom), #CODE); \
  }
  ALL_KNOWN_DW_ORD;
#undef ONE_KNOWN_DW_ORD

#define ONE_KNOWN_DW_DSC(NAME, CODE)					\
  {									\
    add_builtin_constant (dict, constant (CODE, &dw_discr_list_dom), #CODE); \
  }
  ALL_KNOWN_DW_DSC;
#undef ONE_KNOWN_DW_DSC

#define ONE_KNOWN_DW_DS(NAME, CODE)					\
  {									\
    add_builtin_constant (dict,						\
			  constant (CODE, &dw_decimal_sign_dom), #CODE); \
  }
  ALL_KNOWN_DW_DS;
#undef ONE_KNOWN_DW_DS

  add_builtin_constant (dict, constant (DW_ADDR_none, &dw_address_class_dom),
			"DW_ADDR_none");

#define ONE_KNOWN_DW_END(NAME, CODE)					\
  {									\
    add_builtin_constant (dict, constant (CODE, &dw_endianity_dom), #CODE); \
  }
  ALL_KNOWN_DW_END;
#undef ONE_KNOWN_DW_END

  return ret;
}
