#include <memory>
#include "make_unique.hh"

#include "op.hh"
#include "builtin-add.hh"

valfile::uptr
op_add_cst::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop_as <value_cst> ();
      auto wp = vf->pop_as <value_cst> ();

      constant const &cst_v = vp->get_constant ();
      constant const &cst_w = wp->get_constant ();
      check_arith (cst_v, cst_w);

      constant_dom const *dom = cst_v.dom ()->plain ()
	? cst_w.dom () : cst_v.dom ();

      constant result {cst_v.value () + cst_w.value (), dom};
      vf->push (std::make_unique <value_cst> (result, 0));
      return vf;
    }

  return nullptr;
}

overload_tab &
ovl_tab_add ()
{
  static overload_tab tab;
  return tab;
}

struct builtin_add::o
  : public overload_op
{
  using overload_op::overload_op;

  std::string
  name () const override
  {
    return "add";
  }
};

std::shared_ptr <op>
builtin_add::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <o> (upstream,
			       ovl_tab_add ().instantiate (q, scope));
}

char const *
builtin_add::name () const
{
  return "add";
}
