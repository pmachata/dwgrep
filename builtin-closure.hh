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

#ifndef _BUILTIN_CLOSURE_H_
#define _BUILTIN_CLOSURE_H_

#include "op.hh"
#include "builtin.hh"

// Pop closure, execute it.
class op_apply
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_apply (std::shared_ptr <op> upstream);
  ~op_apply ();

  void reset () override;
  valfile::uptr next () override;
  std::string name () const override;
};

struct builtin_apply
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

#endif /* _BUILTIN_CLOSURE_H_ */
