#include "op.hh"
#include "dwit.hh"
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
	{
	  done = true;
	  return stk.clone (stksz);
	}
      return nullptr;
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
    explicit assert_yielder (std::shared_ptr <Dwarf> &dw, pred const &p)
      : done (false)
      , m_dw (dw)
      , m_pred (p)
    {}

    virtual std::unique_ptr <stack>
    yield (stack const &stk, size_t stksz) override
    {
      if (!done)
	{
	  done = true;
	  switch (m_pred.result (m_dw, stk))
	    {
	    case pred_result::yes:
	      return stk.clone (stksz);

	    case pred_result::no:
	    case pred_result::fail:
	      return nullptr;
	    }
	}
      return nullptr;
    }
  };

  return std::unique_ptr <yielder> { new assert_yielder { dw, *m_pred } };
}

pred_result
pred_at::result (std::shared_ptr <Dwarf> &dw, stack const &stk) const
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
pred_tag::result (std::shared_ptr <Dwarf> &dw, stack const &stk) const
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
