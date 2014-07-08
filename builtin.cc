#include "builtin.hh"
#include "op.hh"

std::unique_ptr <pred>
builtin::build_pred (dwgrep_graph::sptr q, std::shared_ptr <scope> scope) const
{
  return nullptr;
}

std::shared_ptr <op>
builtin::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
		     std::shared_ptr <scope> scope) const
{
  return nullptr;
}


struct
  : public builtin
{
  class op_drop
    : public op
  {
    std::shared_ptr <op> m_upstream;

  public:
    explicit op_drop (std::shared_ptr <op> upstream)
      : m_upstream {upstream}
    {}

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

    void
    reset () override
    {
      m_upstream->reset ();
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <op_drop> (upstream);
  }

  char const *
  name () const
  {
    return "drop";
  }
} builtin_drop_obj;

std::map <std::string, builtin const &> builtin_map =
  {
    {builtin_drop_obj.name (), builtin_drop_obj}
  };
