#ifndef _BUILTIN_CST_H_
#define _BUILTIN_CST_H_

#include <memory>

#include "builtin.hh"
#include "value.hh"

class builtin_constant
  : public builtin
{
  std::unique_ptr <value> m_value;

public:
  explicit builtin_constant (std::unique_ptr <value> &&value)
    : m_value {std::move (value)}
  {}

  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_hex
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_dec
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_oct
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_bin
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_type
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_pos
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

#endif /* _BUILTIN_CST_H_ */
