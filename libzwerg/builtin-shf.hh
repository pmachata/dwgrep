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

#ifndef _BUILTIN_SHF_H_
#define _BUILTIN_SHF_H_

#include "op.hh"

struct op_drop
  : public inner_op
{
  using inner_op::inner_op;
  stack::uptr next (scon2 &sc) const override;

  static std::string docstring ();
};

struct op_swap
  : public inner_op
{
  using inner_op::inner_op;
  stack::uptr next (scon2 &sc) const override;

  static std::string docstring ();
};

struct op_dup
  : public inner_op
{
  using inner_op::inner_op;
  stack::uptr next (scon2 &sc) const override;

  static std::string docstring ();
};

struct op_over
  : public inner_op
{
  using inner_op::inner_op;
  stack::uptr next (scon2 &sc) const override;

  static std::string docstring ();
};

struct op_rot
  : public inner_op
{
  using inner_op::inner_op;
  stack::uptr next (scon2 &sc) const override;

  static std::string docstring ();
};

#endif /* _BUILTIN_SHF_H_ */
