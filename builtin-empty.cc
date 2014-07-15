#include <memory>
#include "make_unique.hh"

#include "builtin-empty.hh"

overload_tab &
ovl_tab_empty ()
{
  static overload_tab tab;
  return tab;
}

struct builtin_empty::p
  : public overload_pred
{
  using overload_pred::overload_pred;

  std::string
  name () const override
  {
    return "empty";
  }
};

std::unique_ptr <pred>
builtin_empty::build_pred (dwgrep_graph::sptr q,
			   std::shared_ptr <scope> scope) const
{
  return maybe_invert
    (std::make_unique <p> (ovl_tab_empty ().instantiate (q, scope)));
}

char const *
builtin_empty::name () const
{
  if (m_positive)
    return "?empty";
  else
    return "!empty";
}

