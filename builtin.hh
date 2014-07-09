#ifndef _BUILTIN_H_
#define _BUILTIN_H_

#include <memory>
#include <map>
#include <string>

#include "dwgrep.hh"
#include "scope.hh"

struct pred;
struct op;

class builtin
{
public:
  virtual std::unique_ptr <pred>
  build_pred (dwgrep_graph::sptr q, std::shared_ptr <scope> scope) const;

  virtual std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const;

  virtual char const *name () const = 0;
};

class pred_builtin
  : public builtin
{
protected:
  bool m_positive;

public:
  explicit pred_builtin (bool positive)
    : m_positive {positive}
  {}

  // Return either PRED, or PRED_NOT(PRED), depending on M_POSITIVE.
  std::unique_ptr <pred> maybe_invert (std::unique_ptr <pred> pred) const;
};

void add_builtin (builtin &b);
void add_builtin (builtin &b, std::string const &name);
builtin const *find_builtin (std::string const &name);

#endif /* _BUILTIN_H_ */
