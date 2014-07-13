#ifndef _BUILTIN_CMP_H_
#define _BUILTIN_CMP_H_

#include "builtin.hh"

struct builtin_eq
  : public pred_builtin
{
  using pred_builtin::pred_builtin;

  std::unique_ptr <pred> build_pred (dwgrep_graph::sptr q,
				     std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_lt
  : public pred_builtin
{
  using pred_builtin::pred_builtin;

  std::unique_ptr <pred> build_pred (dwgrep_graph::sptr q,
				     std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

struct builtin_gt
  : public pred_builtin
{
  using pred_builtin::pred_builtin;

  std::unique_ptr <pred> build_pred (dwgrep_graph::sptr q,
				     std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

#endif /* _BUILTIN_CMP_H_ */
