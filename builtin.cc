#include <memory>
#include "make_unique.hh"
#include "builtin.hh"
#include "op.hh"

std::unique_ptr <pred>
builtin::build_pred (dwgrep_graph::sptr q, std::shared_ptr <scope> scope) const
{
  return nullptr;
}

std::shared_ptr <op>
builtin::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
		     std::shared_ptr <scope> scope) const
{
  return nullptr;
}

std::unique_ptr <pred>
pred_builtin::maybe_invert (std::unique_ptr <pred> pred) const
{
  if (m_positive)
    return pred;
  else
    return std::make_unique <pred_not> (std::move (pred));
}

namespace
{
  std::map <std::string, builtin const &> &
  get_builtin_map ()
  {
    static std::map <std::string, builtin const &> bi_map;
    return bi_map;
  }
}

void
add_builtin (builtin &b, std::string const &name)
{
  get_builtin_map ().insert
    (std::pair <std::string, builtin const &> (name, b));
}

void
add_builtin (builtin &b)
{
  add_builtin (b, b.name ());
}

builtin const *
find_builtin (std::string const &name)
{
  auto it = get_builtin_map ().find (name);
  if (it != get_builtin_map ().end ())
    return &it->second;
  else
    return nullptr;
}
