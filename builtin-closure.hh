#ifndef _BUILTIN_CLOSURE_H_
#define _BUILTIN_CLOSURE_H_

#include "op.hh"
#include "builtin.hh"

// Pop closure, execute it.
class op_apply
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_apply (std::shared_ptr <op> upstream);
  ~op_apply ();

  void reset () override;
  valfile::uptr next () override;
  std::string name () const override;
};

struct builtin_apply
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

#endif /* _BUILTIN_CLOSURE_H_ */
