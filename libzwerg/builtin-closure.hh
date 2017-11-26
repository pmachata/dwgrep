/*
   Copyright (C) 2017 Petr Machata
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
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op> m_op;
  std::shared_ptr <frame> m_old_frame;
  bool m_skip_non_closures;

  void reset_me ();

public:
  // When skip_non_closures is true and the incoming stack doesn't have a
  // closure on TOS, op_apply lets it pass through. Otherwise it complains.
  op_apply (std::shared_ptr <op> upstream, bool skip_non_closures = false);
  ~op_apply ();

  void reset () override;
  stack::uptr next () override;
  std::string name () const override;
};

struct builtin_apply
  : public builtin
{
  std::shared_ptr <op> build_exec (layout &l, std::shared_ptr <op> upstream)
    const override;

  char const *name () const override;
  std::string docstring () const override;
};

#endif /* _BUILTIN_CLOSURE_H_ */
