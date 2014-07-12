#ifndef _BUILTIN_ADD_H_
#define _BUILTIN_ADD_H_

#include "overload.hh"
#include "builtin.hh"

class builtin_add
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

struct op_add_cst
  : public stub_op
{
  using stub_op::stub_op;
  valfile::uptr next () override;
};

overload_tab &ovl_tab_add ();

#endif /* _BUILTIN_ADD_H_ */
