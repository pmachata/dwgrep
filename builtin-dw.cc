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
#include "value-dw.hh"

// winfo
namespace
{
  struct op_winfo
    : public inner_op
  {
    dwgrep_graph::sptr m_gr;
    all_dies_iterator m_it;
    stack::uptr m_stk;
    size_t m_pos;

    op_winfo (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr,
	      std::shared_ptr <scope> scope)
      : inner_op {upstream, gr, scope}
      , m_gr {gr}
      , m_it {all_dies_iterator::end ()}
      , m_pos {0}
    {}

    void
    reset_me ()
    {
      m_stk = nullptr;
      m_pos = 0;
    }

    stack::uptr
    next () override
    {
      while (true)
	{
	  if (m_stk == nullptr)
	    {
	      m_stk = m_upstream->next ();
	      if (m_stk == nullptr)
		return nullptr;
	      m_it = all_dies_iterator (&*m_gr->dwarf);
	    }

	  if (m_it != all_dies_iterator::end ())
	    {
	      auto ret = std::make_unique <stack> (*m_stk);
	      auto v = std::make_unique <value_die> (m_gr, **m_it, m_pos++);
	      ret->push (std::move (v));
	      ++m_it;
	      return ret;
	    }

	  reset_me ();
	}
    }

    void
    reset () override
    {
      reset_me ();
      m_upstream->reset ();
    }
  };
}

// unit
namespace
{
  struct producer_unit_die
    : public value_producer
  {
    dwgrep_graph::sptr m_gr;
    all_dies_iterator m_it;
    all_dies_iterator m_end;
    size_t m_i;

    producer_unit_die (Dwarf_Die die, dwgrep_graph::sptr gr)
      : m_gr {gr}
      , m_it {all_dies_iterator::end ()}
      , m_end {all_dies_iterator::end ()}
      , m_i {0}
    {
      Dwarf_Die cudie;
      if (dwarf_diecu (&die, &cudie, nullptr, nullptr) == nullptr)
	throw_libdw ();

      cu_iterator cuit {&*m_gr->dwarf, cudie};
      m_it = all_dies_iterator (cuit);
      m_end = all_dies_iterator (++cuit);
    }

    std::unique_ptr <value>
    next () override
    {
      if (m_it == m_end)
	return nullptr;

      return std::make_unique <value_die> (m_gr, **m_it++, m_i++);
    }
  };

  struct op_unit_die
    : public op_yielding_overload <value_die>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_die> a) override
    {
      return std::make_unique <producer_unit_die>
	(a->get_die (), a->get_graph ());
    }
  };

  struct op_unit_attr
    : public op_yielding_overload <value_attr>
  {
    using op_yielding_overload::op_yielding_overload;

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_attr> a) override
    {
      return std::make_unique <producer_unit_die>
	(a->get_die (), a->get_graph ());
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
	      (ret->get_graph (), child, ret->get_pos () + 1);
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

	  auto value = std::make_unique <value_die> (m_gr, child, 0);
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
	  (m_value->get_graph (), T::get_expr (*m_value, idx),
	   m_value->get_attr (), idx);
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
	    (m_value->get_graph (), **m_it++, m_value->get_die (), m_i++);
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
}

// offset
namespace
{
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
      return std::make_unique <value_addr_range>
	(constant {val->get_low (), &dw_address_dom},
	 constant {val->get_high (), &dw_address_dom}, 0);
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
}

// parent
namespace
{
  struct op_parent_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> val) override
    {
      Dwarf_Off par_off = m_gr->find_parent (val->get_die ());
      if (par_off == dwgrep_graph::none_off)
	return nullptr;

      Dwarf_Die par_die;
      if (dwarf_offdie (&*m_gr->dwarf, par_off, &par_die) == nullptr)
	throw_libdw ();

      return std::make_unique <value_die> (m_gr, par_die, 0);
    }
  };

  struct op_parent_attr
    : public op_overload <value_attr>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_attr> val) override
    {
      return std::make_unique <value_die> (m_gr, val->get_die (), 0);
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

      return std::make_unique <value_die> (m_gr, *die2, 0);
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
      return pred_result (m_gr->is_root (a.get_die ()));
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
      return at_value (a->get_attr (), a->get_die (), a->get_graph ());
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
	(dwop_number (a->get_dwop (), a->get_attr (), a->get_graph ()),
	 dwop_number2 (a->get_dwop (), a->get_attr (), a->get_graph ()));
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

  struct op_low_arange
    : public op_overload <value_addr_range>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_addr_range> a) override
    {
      return std::make_unique <value_cst> (a->get_low (), 0);
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

  struct op_high_arange
    : public op_overload <value_addr_range>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_addr_range> a) override
    {
      return std::make_unique <value_cst> (a->get_high (), 0);
    }
  };
}

// arange
namespace
{
  struct op_arange_cst_cst
    : public op_overload <value_cst, value_cst>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_cst> a,
	     std::unique_ptr <value_cst> b) override
    {
      return std::make_unique <value_addr_range>
	(a->get_constant (), b->get_constant (), 0);
    }
  };
}

// ?contains
namespace
{
  bool
  contains (value_addr_range &a, constant const &c)
  {
    return c >= a.get_low () && c < a.get_high ();
  }

  bool
  contains_incl (value_addr_range &a, constant const &c)
  {
    return c >= a.get_low () && c <= a.get_high ();
  }

  struct pred_containsp_arange_cst
    : public pred_overload <value_addr_range, value_cst>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_addr_range &a, value_cst &cst) override
    {
      return pred_result (contains (a, cst.get_constant ()));
    }
  };

  struct pred_containsp_arange_arange
    : public pred_overload <value_addr_range, value_addr_range>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_addr_range &a, value_addr_range &b) override
    {
      if (a.get_low () == a.get_high ())
	return pred_result (b.get_low () == a.get_low ()
			    && b.get_high () == a.get_high ());

      return pred_result (contains (a, b.get_low ())
			  && contains_incl (a, b.get_high ()));
    }
  };
}

// ?overlaps
namespace
{
  struct pred_overlapsp_arange_arange
    : public pred_overload <value_addr_range, value_addr_range>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_addr_range &a, value_addr_range &b) override
    {
      return pred_result (contains (a, b.get_low ())
			  || (contains (a, b.get_high ())
			      && b.get_high () != a.get_low ())
			  || (b.get_low () < a.get_low ()
			      && b.get_high () >= a.get_high ()));
    }
  };
}

// ?empty
namespace
{
  struct pred_emptyp_arange
    : public pred_overload <value_addr_range>
  {
    using pred_overload::pred_overload;

    pred_result
    result (value_addr_range &a) override
    {
      return pred_result (a.get_low () == a.get_high ());
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
    op_atval_die (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr,
		  std::shared_ptr <scope> scope, int atname)
      : op_yielding_overload {upstream, gr, scope}
      , m_atname {atname}
    {}

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_die> a)
    {
      Dwarf_Attribute attr;
      if (dwarf_attr (&a->get_die (), m_atname, &attr) == nullptr)
	return false;

      return at_value (attr, a->get_die (), a->get_graph ());
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

    pred_atname_die (dwgrep_graph::sptr gr, std::shared_ptr <scope> scope,
		     unsigned atname)
      : pred_overload {gr, scope}
      , m_atname {atname}
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

    pred_atname_attr (dwgrep_graph::sptr gr, std::shared_ptr <scope> scope,
		      unsigned atname)
      : pred_overload {gr, scope}
      , m_atname {atname}
    {}

    pred_result
    result (value_attr &a) override
    {
      return pred_result (dwarf_whatattr (&a.get_attr ()) == m_atname);
    }
  };

  struct pred_atname_cst
    : public pred_overload <value_cst>
  {
    constant m_const;

    pred_atname_cst (dwgrep_graph::sptr gr, std::shared_ptr <scope> scope,
		     unsigned atname)
      : pred_overload {gr, scope}
      , m_const {atname, &dw_attr_dom}
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

    pred_tag_die (dwgrep_graph::sptr gr, std::shared_ptr <scope> scope, int tag)
      : pred_overload {gr, scope}
      , m_tag {tag}
    {}

    pred_result
    result (value_die &a) override
    {
      return pred_result (dwarf_tag (&a.get_die ()) == m_tag);
    }
  };

  struct pred_tag_cst
    : public pred_overload <value_cst>
  {
    constant m_const;

    pred_tag_cst (dwgrep_graph::sptr gr, std::shared_ptr <scope> scope, int tag)
      : pred_overload {gr, scope}
      , m_const {(unsigned) tag, &dw_tag_dom}
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

    pred_form_attr (dwgrep_graph::sptr gr, std::shared_ptr <scope> scope,
		    unsigned form)
      : pred_overload {gr, scope}
      , m_form {form}
    {}

    pred_result
    result (value_attr &a) override
    {
      return pred_result (dwarf_whatform (&a.get_attr ()) == m_form);
    }
  };

  struct pred_form_cst
    : public pred_overload <value_cst>
  {
    constant m_const;

    pred_form_cst (dwgrep_graph::sptr gr, std::shared_ptr <scope> scope,
		   unsigned form)
      : pred_overload {gr, scope}
      , m_const {form, &dw_form_dom}
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

    pred_op_loclist_elem (dwgrep_graph::sptr gr, std::shared_ptr <scope> scope,
			  unsigned op)
      : pred_overload {gr, scope}
      , m_op {op}
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

    pred_op_loclist_op (dwgrep_graph::sptr gr, std::shared_ptr <scope> scope,
			unsigned op)
      : pred_overload {gr, scope}
      , m_op {op}
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

    pred_op_cst (dwgrep_graph::sptr gr, std::shared_ptr <scope> scope,
		 unsigned form)
      : pred_overload {gr, scope}
      , m_const {form, &dw_locexpr_opcode_dom}
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

  add_builtin_type_constant <value_die> (dict);
  add_builtin_type_constant <value_attr> (dict);
  add_builtin_type_constant <value_loclist_op> (dict);

  add_simple_exec_builtin <op_winfo> (dict, "winfo");

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_unit_die> ();
    t->add_op_overload <op_unit_attr> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("unit", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_attribute_die> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("attribute", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_rootp_die> ();
    t->add_pred_overload <pred_rootp_attr> ();

    dict.add (std::make_shared <overloaded_pred_builtin> ("?root", t));
    dict.add (std::make_shared <overloaded_pred_builtin> ("!root", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_child_die> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("child", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_elem_loclist_elem> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("elem", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_relem_loclist_elem> ();

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

    t->add_op_overload <op_offset_die> ();
    t->add_op_overload <op_offset_loclist_op> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("offset", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_address_attr> ();
    t->add_op_overload <op_address_loclist_elem> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("address", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_label_die> ();
    t->add_op_overload <op_label_attr> ();
    t->add_op_overload <op_label_loclist_op> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("label", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_form_attr> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("form", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_parent_die> ();
    t->add_op_overload <op_parent_attr> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("parent", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_integrate_die> ();
    t->add_op_overload <op_integrate_closure> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("integrate", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_low_die> ();
    t->add_op_overload <op_low_arange> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("low", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_high_die> ();
    t->add_op_overload <op_high_arange> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("high", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_arange_cst_cst> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("arange", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_containsp_arange_cst> ();
    t->add_pred_overload <pred_containsp_arange_arange> ();

    dict.add (std::make_shared <overloaded_pred_builtin> ("?contains", t));
    dict.add (std::make_shared <overloaded_pred_builtin> ("!contains", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_overlapsp_arange_arange> ();

    dict.add (std::make_shared <overloaded_pred_builtin> ("?overlaps", t));
    dict.add (std::make_shared <overloaded_pred_builtin> ("!overlaps", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_emptyp_arange> ();

    dict.add (std::make_shared <overloaded_pred_builtin> ("?empty", t));
    dict.add (std::make_shared <overloaded_pred_builtin> ("!empty", t));
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
	t->add_pred_overload <pred_atname_attr> (code);
	t->add_pred_overload <pred_atname_cst> (code);

	dict.add (std::make_shared <overloaded_pred_builtin> (qname, t));
	dict.add (std::make_shared <overloaded_pred_builtin> (bname, t));
	dict.add (std::make_shared <overloaded_pred_builtin> (lqname, t));
	dict.add (std::make_shared <overloaded_pred_builtin> (lbname, t));
      }

      // @AT_* etc.
      {
	auto t = std::make_shared <overload_tab> ();

	t->add_op_overload <op_atval_die> (code);

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
      t->add_pred_overload <pred_tag_cst> (code);

      dict.add (std::make_shared <overloaded_pred_builtin> (qname, t));
      dict.add (std::make_shared <overloaded_pred_builtin> (bname, t));
      dict.add (std::make_shared <overloaded_pred_builtin> (lqname, t));
      dict.add (std::make_shared <overloaded_pred_builtin> (lbname, t));

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
      t->add_pred_overload <pred_form_cst> (code);

      dict.add (std::make_shared <overloaded_pred_builtin> (qname, t));
      dict.add (std::make_shared <overloaded_pred_builtin> (bname, t));
      dict.add (std::make_shared <overloaded_pred_builtin> (lqname, t));
      dict.add (std::make_shared <overloaded_pred_builtin> (lbname, t));

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

      dict.add (std::make_shared <overloaded_pred_builtin> (qname, t));
      dict.add (std::make_shared <overloaded_pred_builtin> (bname, t));
      dict.add (std::make_shared <overloaded_pred_builtin> (lqname, t));
      dict.add (std::make_shared <overloaded_pred_builtin> (lbname, t));

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
