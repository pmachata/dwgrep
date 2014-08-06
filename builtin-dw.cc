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

namespace
{
  struct op_winfo
    : public inner_op
  {
    dwgrep_graph::sptr m_gr;
    all_dies_iterator m_it;
    valfile::uptr m_vf;
    size_t m_pos;

    op_winfo (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr,
	   std::shared_ptr <scope> scope)
      : inner_op {upstream}
      , m_gr {gr}
      , m_it {all_dies_iterator::end ()}
      , m_pos {0}
    {}

    void
    reset_me ()
    {
      m_vf = nullptr;
      m_pos = 0;
    }

    valfile::uptr
    next () override
    {
      while (true)
	{
	  if (m_vf == nullptr)
	    {
	      m_vf = m_upstream->next ();
	      if (m_vf == nullptr)
		return nullptr;
	      m_it = all_dies_iterator (&*m_gr->dwarf);
	    }

	  if (m_it != all_dies_iterator::end ())
	    {
	      auto ret = std::make_unique <valfile> (*m_vf);
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

namespace
{
  struct op_unit
    : public inner_op
  {
    dwgrep_graph::sptr m_gr;
    valfile::uptr m_vf;
    all_dies_iterator m_it;
    all_dies_iterator m_end;
    size_t m_pos;

    op_unit (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr,
	     std::shared_ptr <scope> scope)
      : inner_op {upstream}
      , m_gr {gr}
      , m_it {all_dies_iterator::end ()}
      , m_end {all_dies_iterator::end ()}
      , m_pos {0}
    {}

    void
    init_from_die (Dwarf_Die die)
    {
      Dwarf_Die cudie;
      if (dwarf_diecu (&die, &cudie, nullptr, nullptr) == nullptr)
	throw_libdw ();

      cu_iterator cuit {&*m_gr->dwarf, cudie};
      m_it = all_dies_iterator (cuit);
      m_end = all_dies_iterator (++cuit);
    }

    void
    reset_me ()
    {
      m_vf = nullptr;
      m_it = all_dies_iterator::end ();
      m_pos = 0;
    }

    valfile::uptr
    next () override
    {
      while (true)
	{
	  while (m_vf == nullptr)
	    {
	      if (auto vf = m_upstream->next ())
		{
		  auto vp = vf->pop ();
		  if (auto v = value::as <value_die> (&*vp))
		    {
		      init_from_die (v->get_die ());
		      m_vf = std::move (vf);
		    }
		  else if (auto v = value::as <value_attr> (&*vp))
		    {
		      init_from_die (v->get_die ());
		      m_vf = std::move (vf);
		    }
		  else
		    show_expects (name (),
				  {value_die::vtype, value_attr::vtype});
		}
	      else
		return nullptr;
	    }

	  if (m_it != m_end)
	    {
	      auto ret = std::make_unique <valfile> (*m_vf);
	      ret->push (std::make_unique <value_die>
			 (m_gr, **m_it, m_pos++));
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

namespace
{
  struct op_child_die
    : public op_yielding_overload <value_die>
  {
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

    op_child_die (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr,
		  std::shared_ptr <scope> scope)
      : op_yielding_overload {upstream, gr, scope}
    {}

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

  struct op_child_loclist_elem
    : public op_yielding_overload <value_loclist_elem>
  {
    using op_yielding_overload::op_yielding_overload;

    struct producer
      : public value_producer
    {
      std::unique_ptr <value_loclist_elem> m_value;
      size_t m_i;

      producer (std::unique_ptr <value_loclist_elem> value)
	: m_value {std::move (value)}
	, m_i {0}
      {}

      std::unique_ptr <value>
      next () override
      {
	size_t idx = m_i++;
	if (idx < m_value->get_exprlen ())
	  return std::make_unique <value_loclist_op>
	    (m_value->get_graph (), m_value->get_expr () + idx,
	     m_value->get_attr (), idx);
	else
	  return nullptr;
      }
    };

    std::unique_ptr <value_producer>
    operate (std::unique_ptr <value_loclist_elem> a) override
    {
      return std::make_unique <producer> (std::move (a));
    }
  };
}

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

namespace
{
  class die_op_f
    : public inner_op
  {
  protected:
    dwgrep_graph::sptr m_gr;

  public:
    die_op_f (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
      : inner_op {upstream}
      , m_gr {gr}
    {}

    valfile::uptr
    next () override final
    {
      while (auto vf = m_upstream->next ())
	{
	  auto vp = vf->pop ();
	  if (auto v = value::as <value_die> (&*vp))
	    {
	      if (operate (*vf, v->get_die ()))
		return vf;
	    }

	  else
	    show_expects (name (), {value_die::vtype});
	}

      return nullptr;
    }

    void reset () override final
    { m_upstream->reset (); }

    virtual std::string name () const override = 0;

    virtual bool operate (valfile &vf, Dwarf_Die &die)
    { return false; }
  };
}

namespace
{
  struct op_offset_die
    : public op_overload <value_die>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_die> val) override
    {
      constant c {dwarf_dieoffset (&val->get_die ()), &hex_constant_dom};
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
      constant c {dwop->offset, &hex_constant_dom};
      return std::make_unique <value_cst> (c, 0);
    }
  };
}

namespace
{
  struct op_address_loclist_elem
    : public op_overload <value_loclist_elem>
  {
    using op_overload::op_overload;

    std::unique_ptr <value>
    operate (std::unique_ptr <value_loclist_elem> val) override
    {
      return std::make_unique <value_addr_range>
	(constant {val->get_low (), &hex_constant_dom},
	 constant {val->get_high (), &hex_constant_dom}, 0);
    }
  };
}

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

namespace
{
  struct builtin_rootp
    : public pred_builtin
  {
    using pred_builtin::pred_builtin;

    struct p
      : public pred
    {
      dwgrep_graph::sptr m_gr;

      explicit p (dwgrep_graph::sptr g)
	: m_gr {g}
      {}

      pred_result
      result (valfile &vf) override
      {
	if (auto v = vf.top_as <value_die> ())
	  return pred_result (m_gr->is_root (v->get_die ()));

	else if (vf.top ().is <value_attr> ())
	  // By definition, attributes are never root.
	  return pred_result::no;

	else
	  {
	    show_expects (name (), {value_die::vtype, value_attr::vtype});
	    return pred_result::fail;
	  }
      }

      void
      reset () override
      {}

      std::string
      name () const override
      {
	return "?root";
      }
    };

    std::unique_ptr <pred>
    build_pred (dwgrep_graph::sptr q,
		std::shared_ptr <scope> scope) const override
    {
      return maybe_invert (std::make_unique <p> (q));
    }

    char const *
    name () const override
    {
      if (m_positive)
	return "?root";
      else
	return "!root";
    }
  };
}

namespace
{
  struct op_value_attr
    : public stub_op
  {
    dwgrep_graph::sptr m_gr;
    std::unique_ptr <value_producer> m_vpr;
    valfile::uptr m_vf;

    op_value_attr (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr,
		   std::shared_ptr <scope> scope)
      : stub_op {upstream, gr, scope}
      , m_gr {gr}
    {}

    void
    reset_me ()
    {
      m_vf = nullptr;
      m_vpr = nullptr;
    }

    valfile::uptr
    next () override
    {
      while (true)
	{
	  while (m_vpr == nullptr)
	    if (m_vf = m_upstream->next ())
	      {
		auto vp = m_vf->pop_as <value_attr> ();
		m_vpr = at_value (vp->get_attr (), vp->get_die (), m_gr);
	      }
	    else
	      return nullptr;

	  if (auto v = m_vpr->next ())
	    {
	      auto vf = std::make_unique <valfile> (*m_vf);
	      vf->push (std::move (v));
	      return vf;
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

    static selector get_selector ()
    { return {value_attr::vtype}; }
  };

  struct op_value_loclist_op
    : public stub_op
  {
    dwgrep_graph::sptr m_gr;
    std::unique_ptr <value_producer> m_vpr1;
    std::unique_ptr <value_producer> m_vpr2;
    valfile::uptr m_vf;

    op_value_loclist_op (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr,
			 std::shared_ptr <scope> scope)
      : stub_op {upstream, gr, scope}
      , m_gr {gr}
    {}

    void
    reset_me ()
    {
      m_vf = nullptr;
      m_vpr1 = nullptr;
      m_vpr2 = nullptr;
    }

    valfile::uptr
    next () override
    {
      while (true)
	{
	  while (m_vpr1 == nullptr)
	    if (m_vf = m_upstream->next ())
	      {
		auto vp = m_vf->pop_as <value_loclist_op> ();
		m_vpr1 = dwop_number (vp->get_dwop (), vp->get_attr (), m_gr);
		m_vpr2 = dwop_number2 (vp->get_dwop (), vp->get_attr (), m_gr);
	      }
	    else
	      return nullptr;

	  while (m_vpr1 != nullptr)
	    if (auto v = m_vpr1->next ())
	      {
		auto vf = std::make_unique <valfile> (*m_vf);
		vf->push (std::move (v));
		return vf;
	      }
	    else
	      m_vpr1 = std::move (m_vpr2);

	  reset_me ();
	}
    }

    void
    reset () override
    {
      reset_me ();
      m_upstream->reset ();
    }

    static selector get_selector ()
    { return {value_loclist_op::vtype}; }
  };
}

namespace
{
  struct builtin_attr_named
    : public builtin
  {
    int m_atname;
    explicit builtin_attr_named (int atname)
      : m_atname {atname}
    {}

    struct o
      : public die_op_f
    {
      int m_atname;

    public:
      o (std::shared_ptr <op> upstream, dwgrep_graph::sptr g, int atname)
	: die_op_f {upstream, g}
	, m_atname {atname}
      {}

      bool
      operate (valfile &vf, Dwarf_Die &die) override
      {
	Dwarf_Attribute attr;
	if (dwarf_attr (&die, m_atname, &attr) == nullptr)
	  return false;

	vf.push (std::make_unique <value_attr> (m_gr, attr, die, 0));
	return true;
      }

      std::string
      name () const override
      {
	std::stringstream ss;
	ss << "@" << constant {m_atname, &dw_attr_dom, brevity::brief};
	return ss.str ();
      }
    };

    std::shared_ptr <op>
    build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
		std::shared_ptr <scope> scope) const override
    {
      auto t = std::make_shared <o> (upstream, q, m_atname);
      return std::make_shared <op_value_attr> (t, q, scope);
    }

    char const *
    name () const override
    {
      return "@attr";
    }
  };
}

namespace
{
  struct builtin_pred_attr
    : public pred_builtin
  {
    int m_atname;
    builtin_pred_attr (int atname, bool positive)
      : pred_builtin {positive}
      , m_atname {atname}
    {}

    struct p
      : public pred
    {
      unsigned m_atname;
      constant m_const;

      explicit p (unsigned atname)
	: m_atname {atname}
	, m_const {atname, &dw_attr_dom}
      {}

      pred_result
      result (valfile &vf) override
      {
	if (auto v = vf.top_as <value_die> ())
	  {
	    Dwarf_Die *die = &v->get_die ();
	    return pred_result (dwarf_hasattr (die, m_atname) != 0);
	  }

	else if (auto v = vf.top_as <value_cst> ())
	  {
	    check_constants_comparable (m_const, v->get_constant ());
	    return pred_result (m_const == v->get_constant ());
	  }

	else if (auto v = vf.top_as <value_attr> ())
	  return pred_result (dwarf_whatattr (&v->get_attr ()) == m_atname);

	else
	  {
	    show_expects (name (),
			  {value_die::vtype,
			   value_attr::vtype, value_cst::vtype});
	    return pred_result::fail;
	  }
      }

      void reset () override {}

      std::string
      name () const override
      {
	std::stringstream ss;
	ss << "?AT_" << constant {m_atname, &dw_attr_dom};
	return ss.str ();
      }
    };

    std::unique_ptr <pred>
    build_pred (dwgrep_graph::sptr q,
		std::shared_ptr <scope> scope) const override
    {
      return maybe_invert (std::make_unique <p> (m_atname));
    }

    char const *
    name () const override
    {
      if (m_positive)
	return "?attr";
      else
	return "!attr";
    }
  };
}

namespace
{
  struct builtin_pred_tag
    : public pred_builtin
  {
    int m_tag;
    builtin_pred_tag (int tag, bool positive)
      : pred_builtin {positive}
      , m_tag {tag}
    {}

    struct p
      : public pred
    {
      int m_tag;
      constant m_const;

      explicit p (int tag)
	: m_tag {tag}
	, m_const {(unsigned) tag, &dw_tag_dom}
      {}

      pred_result
      result (valfile &vf) override
      {
	if (auto v = vf.top_as <value_die> ())
	  return pred_result (dwarf_tag (&v->get_die ()) == m_tag);

	else if (auto v = vf.top_as <value_cst> ())
	  {
	    check_constants_comparable (m_const, v->get_constant ());
	    return pred_result (m_const == v->get_constant ());
	  }

	else
	  {
	    show_expects (name (), {value_die::vtype, value_cst::vtype});
	    return pred_result::fail;
	  }
      }

      void reset () override {}

      std::string
      name () const override
      {
	std::stringstream ss;
	ss << "?" << m_const;
	return ss.str ();
      }
    };

    std::unique_ptr <pred>
    build_pred (dwgrep_graph::sptr q,
		std::shared_ptr <scope> scope) const override
    {
      return maybe_invert (std::make_unique <p> (m_tag));
    }

    char const *
    name () const override
    {
      if (m_positive)
	return "?tag";
      else
	return "!tag";
    }
  };
}

namespace
{
  struct builtin_pred_form
    : public pred_builtin
  {
    unsigned m_form;
    builtin_pred_form (unsigned form, bool positive)
      : pred_builtin {positive}
      , m_form {form}
    {}

    struct p
      : public pred
    {
      unsigned m_form;
      constant m_const;

      explicit p (unsigned form)
	: m_form {form}
	, m_const {m_form, &dw_form_dom}
      {}

      pred_result
      result (valfile &vf) override
      {
	if (auto v = vf.top_as <value_attr> ())
	  return pred_result (dwarf_whatform (&v->get_attr ()) == m_form);

	else if (auto v = vf.top_as <value_cst> ())
	  {
	    check_constants_comparable (m_const, v->get_constant ());
	    return pred_result (m_const == v->get_constant ());
	  }

	else
	  {
	    show_expects (name (), {value_attr::vtype, value_cst::vtype});
	    return pred_result::fail;
	  }
      }

      void reset () override {}

      std::string
      name () const override
      {
	std::stringstream ss;
	ss << "?" << m_const;
	return ss.str ();
      }
    };

    std::unique_ptr <pred>
    build_pred (dwgrep_graph::sptr q,
		std::shared_ptr <scope> scope) const override
    {
      return maybe_invert (std::make_unique <p> (m_form));
    }

    char const *
    name () const override
    {
      if (m_positive)
	return "?form";
      else
	return "!form";
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
  add_simple_exec_builtin <op_unit> (dict, "unit");

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_attribute_die> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("attribute", t));
  }

  dict.add (std::make_shared <builtin_rootp> (true));
  dict.add (std::make_shared <builtin_rootp> (false));

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_child_die> ();
    t->add_simple_op_overload <op_child_loclist_elem> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("child", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_value_attr> ();
    t->add_simple_op_overload <op_value_loclist_op> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("value", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_offset_die> ();
    t->add_simple_op_overload <op_offset_loclist_op> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("offset", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_address_loclist_elem> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("address", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_label_die> ();
    t->add_simple_op_overload <op_label_attr> ();
    t->add_simple_op_overload <op_label_loclist_op> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("label", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_form_attr> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("form", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_parent_die> ();
    t->add_simple_op_overload <op_parent_attr> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("parent", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_integrate_die> ();
    t->add_simple_op_overload <op_integrate_closure> ();

    dict.add (std::make_shared <overloaded_op_builtin> ("integrate", t));
  }

#define ONE_KNOWN_DW_AT(NAME, CODE)					\
  {									\
    auto b1 = std::make_shared <builtin_attr_named> (CODE);		\
    dict.add (b1, "@AT_" #NAME);					\
    dict.add (b1, "@" #CODE);						\
    auto b2 = std::make_shared <builtin_pred_attr> (CODE, true);	\
    auto nb2 = std::make_shared <builtin_pred_attr> (CODE, false);	\
    dict.add (b2, "?AT_" #NAME);					\
    dict.add (nb2, "!AT_" #NAME);					\
    dict.add (b2, "?" #CODE);						\
    dict.add (nb2, "!" #CODE);						\
    add_builtin_constant (dict, constant (CODE, &dw_attr_dom), #CODE);	\
  }
  ALL_KNOWN_DW_AT;
#undef ONE_KNOWN_DW_AT

#define ONE_KNOWN_DW_TAG(NAME, CODE)					\
  {									\
    auto b1 = std::make_shared <builtin_pred_tag> (CODE, true);		\
    auto nb1 = std::make_shared <builtin_pred_tag> (CODE, false);	\
    dict.add (b1, "?TAG_" #NAME);					\
    dict.add (nb1, "!TAG_" #NAME);					\
    dict.add (b1, "?" #CODE);						\
    dict.add (nb1, "!" #CODE);						\
    add_builtin_constant (dict, constant (CODE, &dw_tag_dom), #CODE);	\
  }
  ALL_KNOWN_DW_TAG;
#undef ONE_KNOWN_DW_TAG

#define ONE_KNOWN_DW_FORM_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_FORM (NAME, CODE)
#define ONE_KNOWN_DW_FORM(NAME, CODE)					\
  {									\
    auto b1 = std::make_shared <builtin_pred_form> (CODE, true);	\
    auto nb1 = std::make_shared <builtin_pred_form> (CODE, false);	\
    dict.add (b1, "?FORM_" #NAME);					\
    dict.add (nb1, "!FORM_" #NAME);					\
    dict.add (b1, "?" #CODE);						\
    dict.add (nb1, "!" #CODE);						\
    add_builtin_constant (dict, constant (CODE, &dw_form_dom), #CODE);	\
  }
  ALL_KNOWN_DW_FORM;
#undef ONE_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM_DESC

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

#define ONE_KNOWN_DW_OP_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_OP (NAME, CODE)
#define ONE_KNOWN_DW_OP(NAME, CODE)					\
  {									\
    add_builtin_constant (dict,						\
			  constant (CODE, &dw_locexpr_opcode_dom), #CODE); \
  }
  ALL_KNOWN_DW_OP;
#undef ONE_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP_DESC

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
