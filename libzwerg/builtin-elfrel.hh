/*
   Copyright (C) 2019 Petr Machata
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

#ifndef BUILTIN_ELFREL_H
#define BUILTIN_ELFREL_H

#include "overload.hh"
#include "value-elf.hh"

struct op_label_elfrel
  : public op_once_overload <value_cst, value_elf_rel>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_elf_rel> a) const override;
  static std::string docstring ();
};

struct pred_label_elfrel
  : public pred_overload <value_elf_rel>
{
  unsigned m_type;
  int m_machine;

  pred_label_elfrel (unsigned type, int machine)
    : m_type {type}
    , m_machine {machine}
  {}

  pred_result result (value_elf_rel &a) const override final;
  static std::string docstring ();
};

#endif /* BUILTIN_ELFREL_H */
