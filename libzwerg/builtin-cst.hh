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

#ifndef _BUILTIN_CST_H_
#define _BUILTIN_CST_H_

#include <memory>

#include "builtin.hh"
#include "op.hh"
#include "value.hh"

class builtin_constant
  : public builtin
{
  std::unique_ptr <value> m_value;

public:
  explicit builtin_constant (std::unique_ptr <value> &&value)
    : m_value {std::move (value)}
  {}

  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream)
    const override;

  char const *name () const override;

  std::string docstring () const override;
};

struct builtin_hex
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream)
    const override;

  char const *name () const override;
};

struct builtin_dec
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream)
    const override;

  char const *name () const override;
};

struct builtin_oct
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream)
    const override;

  char const *name () const override;
};

struct builtin_bin
  : public builtin
{
  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream)
    const override;

  char const *name () const override;
};

struct op_type
  : public inner_op
{
  using inner_op::inner_op;
  stack::uptr next () override;

  static std::string docstring ();
};

struct op_pos
  : public inner_op
{
  using inner_op::inner_op;
  stack::uptr next () override;

  static std::string docstring ();
};

#endif /* _BUILTIN_CST_H_ */
