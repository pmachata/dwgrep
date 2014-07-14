#include <memory>
#include "make_unique.hh"

#include "op.hh"
#include "builtin-value.hh"

overload_tab &
ovl_tab_value ()
{
  static overload_tab tab;
  return tab;
}

struct builtin_value::o
  : public overload_op
{
  using overload_op::overload_op;

  std::string
  name () const override
  {
    return "value";
  }
};

std::shared_ptr <op>
builtin_value::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			    std::shared_ptr <scope> scope) const
{
  return std::make_shared <o> (upstream,
			       ovl_tab_value ().instantiate (q, scope));
}

char const *
builtin_value::name () const
{
  return "value";
}

valfile::uptr
op_value_cst::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop_as <value_cst> ();
      constant cst {vp->get_constant ().value (), &dec_constant_dom};
      vf->push (std::make_unique <value_cst> (cst, 0));
      return vf;
    }

  return nullptr;
}
