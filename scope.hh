#ifndef _SCOPE_H_
#define _SCOPE_H_

#include <algorithm>
#include "valfile.hh"

struct scope
{
  std::shared_ptr <scope> parent;
  std::vector <std::string> vars;

  scope () = default;

  explicit scope (std::shared_ptr <scope> a_parent)
    : parent {a_parent}
  {}

  bool
  has_name (std::string const &name) const
  {
    return std::find (vars.begin (), vars.end (), name) != vars.end ();
  }

  void
  add_name (std::string const &name)
  {
    assert (! has_name (name));
    vars.push_back (name);
  }

  bool
  empty () const
  {
    return vars.empty ();
  }

  var_id
  index (std::string const &name) const
  {
    assert (has_name (name));
    return static_cast <var_id>
      (std::distance (vars.begin (),
		      std::find (vars.begin (), vars.end (), name)));
  }

  size_t
  num_names () const
  {
    return vars.size ();
  }
};

#endif /* _SCOPE_H_ */
