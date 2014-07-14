#ifndef _BUILTIN_VALUE_H_
#define _BUILTIN_VALUE_H_

#include "builtin.hh"
#include "overload.hh"

class builtin_value
  : public builtin
{
  struct o;

public:
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct op_value_cst
  : public stub_op
{
  using stub_op::stub_op;
  valfile::uptr next () override;
};

overload_tab &ovl_tab_value ();

#endif /* _BUILTIN_VALUE_H_ */
