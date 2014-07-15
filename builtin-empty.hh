#ifndef _BUILTIN_EMPTY_H_
#define _BUILTIN_EMPTY_H_

#include "builtin.hh"
#include "overload.hh"

class builtin_empty
  : public pred_builtin
{
  struct p;

public:
  using pred_builtin::pred_builtin;

  std::unique_ptr <pred> build_pred (dwgrep_graph::sptr q,
				     std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

overload_tab &ovl_tab_empty ();

#endif /* _BUILTIN_EMPTY_H_ */
