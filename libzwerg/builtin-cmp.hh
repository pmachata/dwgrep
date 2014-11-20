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

#ifndef _BUILTIN_CMP_H_
#define _BUILTIN_CMP_H_

#include "builtin.hh"

struct builtin_eq
  : public pred_builtin
{
  using pred_builtin::pred_builtin;

  std::unique_ptr <pred> build_pred () const override;

  char const *name () const override;
  std::string docstring () const override;
};

struct builtin_lt
  : public pred_builtin
{
  using pred_builtin::pred_builtin;

  std::unique_ptr <pred> build_pred () const override;

  char const *name () const override;
  std::string docstring () const override;
};

struct builtin_gt
  : public pred_builtin
{
  using pred_builtin::pred_builtin;

  std::unique_ptr <pred> build_pred () const override;

  char const *name () const override;
  std::string docstring () const override;
};

#endif /* _BUILTIN_CMP_H_ */
