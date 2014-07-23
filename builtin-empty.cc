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

