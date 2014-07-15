#include <memory>
#include "make_unique.hh"

#include "builtin-closure.hh"
#include "value-closure.hh"

struct op_apply::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op> m_op;
  std::shared_ptr <frame> m_old_frame;

  pimpl (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  void
  reset_me ()
  {
    m_op = nullptr;
    m_old_frame = nullptr;
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (m_op == nullptr)
	  if (auto vf = m_upstream->next ())
	    {
	      if (! vf->top ().is <value_closure> ())
		{
		  std::cerr << "Error: `apply' expects a T_CLOSURE on TOS.\n";
		  continue;
		}

	      auto val = vf->pop ();
	      auto &cl = static_cast <value_closure &> (*val);

	      m_old_frame = vf->nth_frame (0);
	      vf->set_frame (cl.get_frame ());
	      auto origin = std::make_shared <op_origin> (std::move (vf));
	      m_op = cl.get_tree ().build_exec (origin, cl.get_graph (),
						cl.get_scope ());
	    }
	  else
	    return nullptr;

	if (auto vf = m_op->next ())
	  {
	    vf->set_frame (m_old_frame);
	    return vf;
	  }

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_apply::op_apply (std::shared_ptr <op> upstream)
  : m_pimpl {std::make_unique <pimpl> (upstream)}
{}

op_apply::~op_apply ()
{}

void
op_apply::reset ()
{
  m_pimpl->reset ();
}

valfile::uptr
op_apply::next ()
{
  return m_pimpl->next ();
}

std::string
op_apply::name () const
{
  return "apply";
}

std::shared_ptr <op>
builtin_apply::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			   std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_apply> (upstream);
}

char const *
builtin_apply::name () const
{
  return "apply";
}
