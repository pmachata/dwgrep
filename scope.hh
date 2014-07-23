/*
   Copyright (C) 2014 Red Hat, Inc.
   This file is part of dwgrep.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   dwgrep is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

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
