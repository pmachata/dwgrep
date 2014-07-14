#ifndef _BUILTIN_SHF_H_
#define _BUILTIN_SHF_H_

#include "builtin.hh"

struct builtin_drop
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

char const *name () const override;
};

struct builtin_swap
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_dup
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_over
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_rot
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

#endif /* _BUILTIN_SHF_H_ */
