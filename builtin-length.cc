#include "op.hh"
#include "overload.hh"
#include "builtin-length.hh"

overload_tab &
ovl_tab_length ()
{
  static overload_tab tab;
  return tab;
}

struct builtin_length::o
  : public overload_op
{
  using overload_op::overload_op;

  std::string
  name () const override
  {
    return "length";
  }
};

std::shared_ptr <op>
builtin_length::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			    std::shared_ptr <scope> scope) const
{
  return std::make_shared <o> (upstream,
			       ovl_tab_length ().instantiate (q, scope));
}

char const *
builtin_length::name () const
{
  return "length";
}
