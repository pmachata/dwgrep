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

