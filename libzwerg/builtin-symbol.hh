/*
   Copyright (C) 2017, 2018, 2019 Petr Machata
   Copyright (C) 2014, 2015 Red Hat, Inc.
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

#ifndef BUILTIN_SYMBOL_H
#define BUILTIN_SYMBOL_H

#include "overload.hh"
#include "value-cst.hh"
#include "value-elf.hh"
#include "value-dw.hh"
#include "value-symbol.hh"
#include "value-str.hh"

struct op_symbol_dwarf
  : public op_yielding_overload <value_symbol, value_dwarf>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_symbol>>
  operate (std::unique_ptr <value_dwarf> val) const override;

  static std::string docstring ();
};

struct op_symbol_elf
  : public op_yielding_overload <value_symbol, value_elf>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_symbol>>
  operate (std::unique_ptr <value_elf> val) const override;

  static std::string docstring ();
};

struct op_dynsym_dwarf
  : public op_yielding_overload <value_symbol, value_dwarf>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_symbol>>
  operate (std::unique_ptr <value_dwarf> val) const override;

  static std::string docstring ();
};

struct op_dynsym_elf
  : public op_yielding_overload <value_symbol, value_elf>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_symbol>>
  operate (std::unique_ptr <value_elf> val) const override;

  static std::string docstring ();
};

struct op_name_symbol
  : public op_once_overload <value_str, value_symbol>
{
  using op_once_overload::op_once_overload;

  value_str operate (std::unique_ptr <value_symbol> val) const override;
  static std::string docstring ();
};

struct op_label_symbol
  : public op_once_overload <value_cst, value_symbol>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_symbol> val) const override;
  static std::string docstring ();
};

struct op_binding_symbol
  : public op_once_overload <value_cst, value_symbol>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_symbol> val) const override;
  static std::string docstring ();
};

struct op_visibility_symbol
  : public op_once_overload <value_cst, value_symbol>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_symbol> val) const override;
  static std::string docstring ();
};

struct op_address_symbol
  : public op_once_overload <value_cst, value_symbol>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_symbol> val) const override;
  static std::string docstring ();
};

struct op_size_symbol
  : public op_once_overload <value_cst, value_symbol>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_symbol> val) const override;
  static std::string docstring ();
};

#endif /* BUILTIN_SYMBOL_H */
