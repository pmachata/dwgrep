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

extern std::map <std::string, builtin const &> builtin_map;

#endif /* _BUILTIN_H_ */
