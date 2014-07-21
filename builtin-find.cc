#include <memory>
#include "make_unique.hh"

#include "builtin-find.hh"

overload_tab &
ovl_tab_find ()
{
  static overload_tab tab {2};
  return tab;
}

struct builtin_find::p
  : public overload_pred
{
  using overload_pred::overload_pred;

  std::string
  name () const override
  {
    return "find";
  }
};

std::unique_ptr <pred>
builtin_find::build_pred (dwgrep_graph::sptr q,
			   std::shared_ptr <scope> scope) const
{
  return maybe_invert
    (std::make_unique <p> (ovl_tab_find ().instantiate (q, scope)));
}

char const *
builtin_find::name () const
{
  if (m_positive)
    return "?find";
  else
    return "!find";
}

