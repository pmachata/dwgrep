#include "builtin-shf.hh"
#include "op.hh"

namespace
{
  struct op_drop
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      if (auto vf = m_upstream->next ())
	{
	  vf->pop ();
	  return vf;
	}
      return nullptr;
    }

    std::string
    name () const override
    {
      return "drop";
    }
  };

  struct op_swap
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      if (auto vf = m_upstream->next ())
	{
	  auto a = vf->pop ();
	  auto b = vf->pop ();
	  vf->push (std::move (a));
	  vf->push (std::move (b));
	  return vf;
	}
      return nullptr;
    }

    std::string
    name () const override
    {
      return "swap";
    }
  };

  struct op_dup
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      if (auto vf = m_upstream->next ())
	{
	  vf->push (vf->top ().clone ());
	  return vf;
	}
      return nullptr;
    }

    std::string
    name () const override
    {
      return "dup";
    }
  };

  struct op_over
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      if (auto vf = m_upstream->next ())
	{
	  vf->push (vf->below ().clone ());
	  return vf;
	}
      return nullptr;
    }

    std::string
    name () const override
    {
      return "over";
    }
  };
}

std::shared_ptr <op>
builtin_drop::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_drop> (upstream);
}

char const *
builtin_drop::name () const
{
  return "drop";
}

std::shared_ptr <op>
builtin_swap::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_swap> (upstream);
}

char const *
builtin_swap::name () const
{
  return "swap";
}

std::shared_ptr <op>
builtin_dup::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_dup> (upstream);
}

char const *
builtin_dup::name () const
{
  return "dup";
}

std::shared_ptr <op>
builtin_over::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_over> (upstream);
}

char const *
builtin_over::name () const
{
  return "over";
}
