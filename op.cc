#include <iostream>
#include <sstream>

#include "op.hh"
#include "dwit.hh"
#include "dwpp.hh"
#include "make_unique.hh"

op_origin::op_origin ()
  : m_done (false)
{}

valfile::uptr
op_origin::next ()
{
  if (m_done)
    return nullptr;
  m_done = true;
  return valfile::create (0);
}

std::string
op_origin::name () const
{
  return "origin";
}


struct op_sel_universe::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <Dwarf> m_dw;
  all_dies_iterator m_it;
  valfile::uptr m_vf;
  size_t m_osz;
  size_t m_nsz;
  slot_idx m_dst;

  pimpl (std::shared_ptr <op> upstream, dwgrep_graph::ptr q,
	 size_t osz, size_t nsz, slot_idx dst)
    : m_upstream (upstream)
    , m_dw (q->dwarf)
    , m_it (all_dies_iterator::end ())
    , m_osz (osz)
    , m_nsz (nsz)
    , m_dst (dst)
  {}

  valfile::uptr
  next ()
  {
    while (true)
      {
	if (m_vf == nullptr)
	  {
	    m_vf = m_upstream->next ();
	    if (m_vf == nullptr)
	      return nullptr;
	    m_it = all_dies_iterator (&*m_dw);
	  }

	if (m_it != all_dies_iterator::end ())
	  {
	    auto ret = valfile::copy (*m_vf, m_osz, m_nsz);
	    ret->set_slot (m_dst, **m_it);
	    ++m_it;
	    return ret;
	  }

	m_vf = nullptr;
      }
  }
};

op_sel_universe::op_sel_universe (std::shared_ptr <op> upstream,
				  dwgrep_graph::ptr q,
				  size_t osz, size_t nsz, slot_idx dst)
  : m_pimpl (std::make_unique <pimpl> (upstream, q, osz, nsz, dst))
{}

op_sel_universe::~op_sel_universe ()
{}

valfile::uptr
op_sel_universe::next ()
{
  return m_pimpl->next ();
}

std::string
op_sel_universe::name () const
{
  return "sel_universe";
}


valfile::uptr
op_nop::next ()
{
  return m_upstream->next ();
}

std::string
op_nop::name () const
{
  return "nop";
}


valfile::uptr
op_assert::next ()
{
  while (auto vf = m_upstream->next ())
    if (m_pred->result (*vf) == pred_result::yes)
      return vf;
  return nullptr;
}

std::string
op_assert::name () const
{
  std::stringstream ss;
  ss << "assert<" << m_pred->name () << ">";
  return ss.str ();
}

valfile::uptr
op_f_atval::next ()
{
  while (auto vf = m_upstream->next ())
    if (vf->get_slot_type (m_idx) == slot_type::DIE)
      {
	Dwarf_Die die = vf->get_slot_die (m_idx);
	vf->invalidate_slot (m_idx);
	Dwarf_Attribute attr;

	if (dwarf_attr_integrate (&die, m_name, &attr) != nullptr)
	  {
	    switch (dwarf_whatform (&attr))
	      {
	      case DW_FORM_string:
	      case DW_FORM_strp:
	      case DW_FORM_GNU_strp_alt:
		{
		  const char *str = dwarf_formstring (&attr);
		  if (str == nullptr)
		    throw_libdw ();
		  vf->set_slot (m_idx, str);
		  return vf;
		}

	      case DW_FORM_sdata:
		{
		data_signed:
		  Dwarf_Sword sval;
		  if (dwarf_formsdata (&attr, &sval) != 0)
		    throw_libdw ();
		  constant cst { (uint64_t) sval, &signed_constant_dom };
		  vf->set_slot (m_idx, std::move (cst));
		  return vf;
		}

	      case DW_FORM_udata:
		{
		data_unsigned:
		  Dwarf_Word uval;
		  if (dwarf_formudata (&attr, &uval) != 0)
		    throw_libdw ();
		  constant cst { uval, &signed_constant_dom };
		  vf->set_slot (m_idx, std::move (cst));
		  return vf;
		}

	      case DW_FORM_data1:
	      case DW_FORM_data2:
	      case DW_FORM_data4:
	      case DW_FORM_data8:
		switch (m_name)
		  {
		  case DW_AT_byte_stride:
		  case DW_AT_bit_stride:
		    // """Note that the stride can be negative."""
		  case DW_AT_binary_scale:
		  case DW_AT_decimal_scale:
		    goto data_signed;

		  case DW_AT_byte_size:
		  case DW_AT_bit_size:
		  case DW_AT_bit_offset:
		  case DW_AT_data_bit_offset:
		  case DW_AT_lower_bound:
		  case DW_AT_upper_bound:
		  case DW_AT_count:
		  case DW_AT_allocated:
		  case DW_AT_associated:
		  case DW_AT_start_scope:
		  case DW_AT_digit_count:
		  case DW_AT_GNU_odr_signature:
		    goto data_unsigned;

		  case DW_AT_ordering:
		  case DW_AT_language:
		  case DW_AT_visibility:
		  case DW_AT_inline:
		  case DW_AT_accessibility:
		  case DW_AT_address_class:
		  case DW_AT_calling_convention:
		  case DW_AT_encoding:
		  case DW_AT_identifier_case:
		  case DW_AT_virtuality:
		  case DW_AT_endianity:
		  case DW_AT_decimal_sign:
		  case DW_AT_decl_line:
		  case DW_AT_call_line:
		  case DW_AT_decl_column:
		  case DW_AT_call_column:
		    // XXX these are Dwarf constants and should have
		    // a domain associated to them.
		    goto data_unsigned;

		  case DW_AT_discr_value:
		  case DW_AT_const_value:
		    // """The number is signed if the tag type for
		    // the variant part containing this variant is a
		    // signed type."""
		    assert (! "signedness of attribute not implemented yet");
		    abort ();
		  }
		assert (! "signedness of attribute unhandled");
		abort ();

	      case DW_FORM_addr:
	      case DW_FORM_block2:
	      case DW_FORM_block4:
	      case DW_FORM_block:
	      case DW_FORM_block1:
	      case DW_FORM_flag:
	      case DW_FORM_ref_addr:
	      case DW_FORM_ref1:
	      case DW_FORM_ref2:
	      case DW_FORM_ref4:
	      case DW_FORM_ref8:
	      case DW_FORM_ref_udata:
	      case DW_FORM_indirect:
	      case DW_FORM_sec_offset:
	      case DW_FORM_exprloc:
	      case DW_FORM_flag_present:
	      case DW_FORM_ref_sig8:
	      case DW_FORM_GNU_ref_alt:
		assert (! "Form unhandled.");
		abort  ();
	      }
	    }
      }
  return nullptr;
}

std::string
op_f_atval::name () const
{
  std::stringstream ss;
  ss << "f_atval<" << m_name << ">";
  return ss.str ();
}

/*
std::unique_ptr <yielder>
op_f_child::get_yielder (std::unique_ptr <stack> stk, size_t stksz,
			 std::shared_ptr <Dwarf> &dw) const
{
  class f_child_yielder
    : public yielder
  {
    bool done;
    std::shared_ptr <Dwarf> &m_dw;
    size_t m_idx;
    Dwarf_Die m_child;

  public:
    f_child_yielder (std::shared_ptr <Dwarf> &dw, size_t idx)
      : done (false)
      , m_dw (dw)
      , m_idx (idx)
      , m_child {}
    {}

    virtual std::unique_ptr <stack>
    yield () override
    {
      if (done)
	{
	stop:
	  done = true;
	  return nullptr;
	}

      if (m_child.addr == nullptr)
	{
	  auto vd = dynamic_cast <v_die const *> (&stk.get_slot (m_idx));
	  if (vd == nullptr)
	    goto stop;

	  m_child = vd->die (m_dw);
	  if (! dwarf_haschildren (&m_child))
	    goto stop;

	  if (dwarf_child (&m_child, &m_child) != 0)
	    throw_libdw ();
	}
      else
	switch (dwarf_siblingof (&m_child, &m_child))
	  {
	  case -1:
	    throw_libdw ();
	  case 1:
	    // No more siblings.
	    goto stop;
	  case 0:
	    break;
	  }

      assert (m_child.addr != nullptr);
      auto ret = stk.clone (stksz);
      ret->invalidate_slot (m_idx);
      ret->set_slot (m_idx, std::unique_ptr <value> { new v_die { &m_child } });
      return ret;
    }
  };

  return std::make_unique <f_child_yielder> (dw, m_idx);
}

std::unique_ptr <yielder>
op_f_offset::get_yielder (std::unique_ptr <stack> stk, size_t stksz,
			  std::shared_ptr <Dwarf> &dw) const
{
  class f_offset_yielder
    : public yielder
  {
    bool done;
    std::shared_ptr <Dwarf> &m_dw;
    size_t m_idx;

  public:
    f_offset_yielder (std::shared_ptr <Dwarf> &dw, size_t idx)
      : done (false)
      , m_dw (dw)
      , m_idx (idx)
    {}

    virtual std::unique_ptr <stack>
    yield () override
    {
      if (done)
	return nullptr;

      done = true;
      if (auto vd = dynamic_cast <v_die const *> (&stk.get_slot (m_idx)))
	{
	  auto nv = std::make_unique <v_uint> (vd->offset ());
	  auto ret = stk.clone (stksz);
	  ret->invalidate_slot (m_idx);
	  ret->set_slot (m_idx, std::move (nv));
	  return ret;
	}

      return nullptr;
    }
  };

  return std::make_unique <f_offset_yielder> (dw, m_idx);
}

std::unique_ptr <yielder>
op_format::get_yielder (std::unique_ptr <stack> stk, size_t stksz,
			std::shared_ptr <Dwarf> &dw) const
{
  class format_yielder
    : public yielder
  {
    bool done;
    std::shared_ptr <Dwarf> &m_dw;
    std::string const &m_lit;
    size_t m_idx;

  public:
    format_yielder (std::shared_ptr <Dwarf> &dw,
		    std::string const &lit, size_t idx)
      : done (false)
      , m_dw (dw)
      , m_lit (lit)
      , m_idx (idx)
    {}

    virtual std::unique_ptr <stack>
    yield () override
    {
      if (done)
	return nullptr;

      done = true;
      auto ret = stk.clone (stksz);
      auto nv = std::make_unique <v_str> (m_lit.c_str ());
      ret->set_slot (m_idx, std::move (nv));
      return ret;
    }
  };

  return std::make_unique <format_yielder> (dw, m_str, m_idx);
}

std::unique_ptr <yielder>
op_drop::get_yielder (std::unique_ptr <stack> stk, size_t stksz,
		      std::shared_ptr <Dwarf> &dw) const
{
  class drop_yielder
    : public yielder
  {
    bool done;
    size_t m_idx;

  public:
    explicit drop_yielder (size_t idx)
      : done (false)
      , m_idx (idx)
    {}

    virtual std::unique_ptr <stack>
    yield () override
    {
      if (done)
	return nullptr;

      done = true;
      auto ret = stk.clone (stksz);
      ret->invalidate_slot (m_idx);
      return ret;
    }
  };

  return std::make_unique <drop_yielder> (m_idx);
}

std::unique_ptr <yielder>
op_const::get_yielder (std::unique_ptr <stack> stk, size_t stksz,
		       std::shared_ptr <Dwarf> &dw) const
{
  class const_yielder
    : public yielder
  {
    bool done;
    value const &m_val;
    size_t m_idx;

  public:
    const_yielder (value const &val, size_t idx)
      : done (false)
      , m_val (val)
      , m_idx (idx)
    {}

    virtual std::unique_ptr <stack>
    yield () override
    {
      if (done)
	return nullptr;

      done = true;
      auto ret = stk.clone (stksz);
      ret->set_slot (m_idx, m_val.clone ());
      return ret;
    }
  };

  return std::make_unique <const_yielder> (*m_val, m_idx);
}
*/


pred_result
pred_not::result (valfile &vf) const
{
  return ! m_a->result (vf);
}

std::string
pred_not::name () const
{
  std::stringstream ss;
  ss << "not <" << m_a->name () << ">";
  return ss.str ();
}


pred_result
pred_and::result (valfile &vf) const
{
  return m_a->result (vf) && m_b->result (vf);
}

std::string
pred_and::name () const
{
  std::stringstream ss;
  ss << "and <" << m_a->name () << "><" << m_b->name () << ">";
  return ss.str ();
}


pred_result
pred_or::result (valfile &vf) const
{
  return m_a->result (vf) || m_b->result (vf);
}

std::string
pred_or::name () const
{
  std::stringstream ss;
  ss << "or <" << m_a->name () << "><" << m_b->name () << ">";
  return ss.str ();
}

/*
pred_result
pred_at::result (std::shared_ptr <Dwarf> &dw,
		 stack const &stk, size_t stksz) const
{
  auto const &var = stk.get_slot (m_idx);
  if (auto vd = dynamic_cast <v_die const *> (&var))
    {
      Dwarf_Die die = vd->die (dw);
      return pred_result (dwarf_hasattr_integrate (&die, m_atname) != 0);
    }
  else if (auto va = dynamic_cast <v_at const *> (&var))
    return pred_result (va->atname () == m_atname);
  else
    return pred_result::fail;
}

pred_result
pred_tag::result (std::shared_ptr <Dwarf> &dw,
		  stack const &stk, size_t stksz) const
{
  auto const &var = stk.get_slot (m_idx);
  if (auto vd = dynamic_cast <v_die const *> (&var))
    {
      Dwarf_Die die = vd->die (dw);
      return pred_result (dwarf_tag (&die) == m_tag);
    }
  else
    return pred_result::fail;
}

value const &
pred_binary::get_a (stack const &stk) const
{
  return stk.get_slot (m_idx_a);
}

value const &
pred_binary::get_b (stack const &stk) const
{
  return stk.get_slot (m_idx_b);
}

value const &
pred_unary::get_a (stack const &stk) const
{
  return stk.get_slot (m_idx_a);
}

pred_result
pred_eq::result (std::shared_ptr <Dwarf> &dw,
		 stack const &stk, size_t stksz) const
{
  value const &a = get_a (stk);
  value const &b = get_b (stk);

  switch (a.lt (b))
    {
    case pred_result::fail:
      return pred_result::fail;
    case pred_result::yes:
      return pred_result::no;
    case pred_result::no:
      break;
    }

  return ! b.lt (a);
}

pred_result
pred_lt::result (std::shared_ptr <Dwarf> &dw,
		 stack const &stk, size_t stksz) const
{
  return get_a (stk).lt (get_b (stk));
}

pred_result
pred_gt::result (std::shared_ptr <Dwarf> &dw,
		 stack const &stk, size_t stksz) const
{
  value const &a = get_a (stk);
  value const &b = get_b (stk);

  switch (a.lt (b))
    {
    case pred_result::fail:
      return pred_result::fail;
    case pred_result::yes:
      return pred_result::no;
    case pred_result::no:
      break;
    }

  return b.lt (a);
}

pred_result
pred_root::result (std::shared_ptr <Dwarf> &dw,
		   stack const &stk, size_t stksz) const
{
  // XXX this is not accurate, and is fairly slow.  This should use
  // our parent/prev cache when it exists.
  if (auto vd = dynamic_cast <v_die const *> (&get_a (stk)))
    {
      Dwarf_Die die = vd->die (dw);
      for (auto it = cu_iterator { &*dw }; it != cu_iterator::end (); ++it)
	{
	  Dwarf_Die *cudiep = *it;
	  if (dwarf_dieoffset (&die) == dwarf_dieoffset (cudiep))
	    return pred_result::yes;
	}
      return pred_result::no;
    }
  else
    return pred_result::fail;
}

pred_result
pred_subx_any::result (std::shared_ptr <Dwarf> &dw,
		       stack const &stk, size_t stksz) const
{
  if (m_op->get_yielder (stk.clone (stksz), stksz, dw)->yield () != nullptr)
    return pred_result::yes;
  else
    return pred_result::no;
}
*/
