#include "op.hh"
#include "dwit.hh"
#include "dwpp.hh"
#include <iostream>

std::unique_ptr <yielder>
op_cat::get_yielder (std::shared_ptr <Dwarf> &dw) const
{
  class cat_yielder
    : public yielder
  {
    std::shared_ptr <Dwarf> &m_dw;
    op const &m_b;
    std::unique_ptr <yielder> m_ya;
    std::unique_ptr <stack> m_stk;
    std::unique_ptr <yielder> m_yb;

  public:
    cat_yielder (std::shared_ptr <Dwarf> &dw, op const &a, op const &b)
      : m_dw (dw)
      , m_b (b)
      , m_ya (a.get_yielder (dw))
    {}

    virtual std::unique_ptr <stack>
    yield (stack const &stk, size_t stksz) override
    {
      while (true)
	{
	  assert (m_ya != nullptr);
	  if (m_yb != nullptr)
	    {
	      assert (m_stk != nullptr);
	      if (std::unique_ptr <stack> result = m_yb->yield (*m_stk, stksz))
		return std::move (result);
	      m_yb.reset ();
	      m_stk.reset ();
	    }

	  assert (m_ya != nullptr);
	  assert (m_yb == nullptr && m_stk == nullptr);
	  m_yb = m_b.get_yielder (m_dw);
	  m_stk = m_ya->yield (stk, stksz);

	  if (m_stk == nullptr)
	    return nullptr;
	}
    }
  };

  return std::unique_ptr <yielder> { new cat_yielder { dw, *m_a, *m_b } };
}

std::unique_ptr <yielder>
op_sel_universe::get_yielder (std::shared_ptr <Dwarf> &dw) const
{
  class universe_yielder
    : public yielder
  {
    all_dies_iterator m_it;
    size_t m_idx;

  public:
    universe_yielder (std::shared_ptr <Dwarf> &dw, size_t idx)
      : m_it { &*dw }
      , m_idx { idx }
    {}

    virtual std::unique_ptr <stack>
    yield (stack const &stk, size_t stksz) override
    {
      if (m_it == all_dies_iterator::end ())
	return nullptr;

      all_dies_iterator cur = m_it++;
      std::unique_ptr <stack> stk2 = stk.clone (stksz);
      stk2->set_slot
	(m_idx, std::move (std::unique_ptr <value> { new v_die { *cur } }));
      return std::move (stk2);
    }
  };

  return std::unique_ptr <yielder> { new universe_yielder { dw, m_idx } };
}

std::unique_ptr <yielder>
op_nop::get_yielder (std::shared_ptr <Dwarf> &dw) const
{
  class nop_yielder
    : public yielder
  {
    bool done;

  public:
    nop_yielder () : done { false } {}

    virtual std::unique_ptr <stack>
    yield (stack const &stk, size_t stksz) override
    {
      if (!done)
	return nullptr;

      done = true;
      return stk.clone (stksz);
    }
  };

  return std::unique_ptr <yielder> { new nop_yielder {} };
}

std::unique_ptr <yielder>
op_assert::get_yielder (std::shared_ptr <Dwarf> &dw) const
{
  class assert_yielder
    : public yielder
  {
    bool done;
    std::shared_ptr <Dwarf> &m_dw;
    pred const &m_pred;

  public:
    assert_yielder (std::shared_ptr <Dwarf> &dw, pred const &p)
      : done (false)
      , m_dw (dw)
      , m_pred (p)
    {}

    virtual std::unique_ptr <stack>
    yield (stack const &stk, size_t stksz) override
    {
      if (done)
	return nullptr;

      done = true;
      switch (m_pred.result (m_dw, stk, stksz))
	{
	case pred_result::yes:
	  return stk.clone (stksz);

	case pred_result::no:
	case pred_result::fail:
	  return nullptr;
	}
      assert (! "should never get here");
      abort ();
    }
  };

  return std::unique_ptr <yielder> { new assert_yielder { dw, *m_pred } };
}


std::unique_ptr <yielder>
op_f_atval::get_yielder (std::shared_ptr <Dwarf> &dw) const
{
  class f_atval_yielder
    : public yielder
  {
    bool done;
    std::shared_ptr <Dwarf> &m_dw;
    int m_name;
    size_t m_idx;

  public:
    f_atval_yielder (std::shared_ptr <Dwarf> &dw, int name, size_t idx)
      : done (false)
      , m_dw (dw)
      , m_name (name)
      , m_idx (idx)
    {}

    virtual std::unique_ptr <stack>
    yield (stack const &stk, size_t stksz) override
    {
      if (done)
	return nullptr;

      done = true;
      if (auto vd = dynamic_cast <v_die const *> (&stk.get_slot (m_idx)))
	{
	  auto ret = stk.clone (stksz);

	  Dwarf_Die die = vd->die (m_dw);
	  Dwarf_Attribute attr;
	  std::unique_ptr <value> nv;
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
		    nv = std::unique_ptr <value> { new v_str { str } };
		    break;
		  }

		case DW_FORM_sdata:
		  {
		  data_signed:
		    Dwarf_Sword sval;
		    if (dwarf_formsdata (&attr, &sval) != 0)
		      throw_libdw ();
		    nv = std::unique_ptr <value> { new v_sint { sval } };
		    break;
		  }

		case DW_FORM_udata:
		  {
		  data_unsigned:
		    Dwarf_Word uval;
		    if (dwarf_formudata (&attr, &uval) != 0)
		      throw_libdw ();
		    nv = std::unique_ptr <value> { new v_uint { uval } };
		    break;
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
	  else
	    // No such attribute.
	    return nullptr;

	  assert (nv != nullptr);
	  ret->invalidate_slot (m_idx);
	  ret->set_slot (m_idx, std::move (nv));
	  return ret;
	}

      return nullptr;
    }
  };

  return std::unique_ptr <yielder>
    { new f_atval_yielder { dw, m_name, m_idx } };
}

std::unique_ptr <yielder>
op_f_child::get_yielder (std::shared_ptr <Dwarf> &dw) const
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
    yield (stack const &stk, size_t stksz) override
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

  return std::unique_ptr <yielder> { new f_child_yielder { dw, m_idx } };
}

std::unique_ptr <yielder>
op_f_offset::get_yielder (std::shared_ptr <Dwarf> &dw) const
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
    yield (stack const &stk, size_t stksz) override
    {
      if (done)
	return nullptr;

      done = true;
      if (auto vd = dynamic_cast <v_die const *> (&stk.get_slot (m_idx)))
	{
	  auto nv = std::unique_ptr <value> { new v_uint { vd->offset () } };
	  auto ret = stk.clone (stksz);
	  ret->invalidate_slot (m_idx);
	  ret->set_slot (m_idx, std::move (nv));
	  return ret;
	}

      return nullptr;
    }
  };

  return std::unique_ptr <yielder> { new f_offset_yielder { dw, m_idx } };
}

std::unique_ptr <yielder>
op_format::get_yielder (std::shared_ptr <Dwarf> &dw) const
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
    yield (stack const &stk, size_t stksz) override
    {
      if (done)
	return nullptr;

      done = true;
      auto ret = stk.clone (stksz);
      auto nv = std::unique_ptr <value> { new v_str { m_lit.c_str () } };
      ret->set_slot (m_idx, std::move (nv));
      return ret;
    }
  };

  return std::unique_ptr <yielder> { new format_yielder { dw, m_str, m_idx } };
}

std::unique_ptr <yielder>
op_drop::get_yielder (std::shared_ptr <Dwarf> &dw) const
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
    yield (stack const &stk, size_t stksz) override
    {
      if (done)
	return nullptr;

      done = true;
      auto ret = stk.clone (stksz);
      ret->invalidate_slot (m_idx);
      return ret;
    }
  };

  return std::unique_ptr <yielder> { new drop_yielder { m_idx } };
}

std::unique_ptr <yielder>
op_const::get_yielder (std::shared_ptr <Dwarf> &dw) const
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
    yield (stack const &stk, size_t stksz) override
    {
      if (done)
	return nullptr;

      done = true;
      auto ret = stk.clone (stksz);
      ret->set_slot (m_idx, m_val.clone ());
      return ret;
    }
  };

  return std::unique_ptr <yielder> { new const_yielder { *m_val, m_idx } };
}


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
  if (std::unique_ptr <stack> result
      = m_op->get_yielder (dw)->yield (stk, stksz))
    return pred_result::yes;
  else
    return pred_result::no;
}
