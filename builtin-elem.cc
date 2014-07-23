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

#include "op.hh"
#include "overload.hh"
#include "builtin-elem.hh"

overload_tab &
ovl_tab_elem ()
{
  static overload_tab tab;
  return tab;
}

struct builtin_elem::o
  : public overload_op
{
  using overload_op::overload_op;

  std::string
  name () const override
  {
    return "elem";
  }
};

std::shared_ptr <op>
builtin_elem::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope) const
{
  return std::make_shared <o> (upstream,
			       ovl_tab_elem ().instantiate (q, scope));
}

char const *
builtin_elem::name () const
{
  return "elem";
}
