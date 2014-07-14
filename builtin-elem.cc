#include "op.hh"
#include "overload.hh"
#include "builtin-elem.hh"

overload_tab &
ovl_tab_elem ()
{
  static overload_tab tab;
  return tab;
}

struct builtin_elem::o
  : public overload_op
{
  using overload_op::overload_op;

  std::string
  name () const override
  {
    return "elem";
  }
};

std::shared_ptr <op>
builtin_elem::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope) const
{
  return std::make_shared <o> (upstream,
			       ovl_tab_elem ().instantiate (q, scope));
}

char const *
builtin_elem::name () const
{
  return "elem";
}
