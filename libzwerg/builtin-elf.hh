/*
   Copyright (C) 2018 Petr Machata
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

#ifndef BUILTIN_ELF_H
#define BUILTIN_ELF_H

#include "overload.hh"
#include "value-dw.hh"
#include "value-elf.hh"
#include "value-str.hh"

struct op_elf_dwarf
  : public op_yielding_overload <value_elf, value_dwarf>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_elf>>
  operate (std::unique_ptr <value_dwarf> val) const override;

  static std::string docstring ();
};

struct op_name_elf
  : public op_once_overload <value_str, value_elf>
{
  using op_once_overload::op_once_overload;

  value_str
  operate (std::unique_ptr <value_elf> val) const override;

  static std::string docstring ();
};


template <class Def, class ValueType>
struct pred_elf_simple_value
  : public pred_overload <ValueType>
  , protected Def
{
  unsigned m_value;

  pred_elf_simple_value (unsigned value)
    : m_value {value}
  {}

  pred_result
  result (ValueType &a) const override final
  {
    unsigned value = Def::value (a.get_dwctx ()->get_dwfl ());
    return pred_result (value == m_value);
  }

  using Def::docstring;
};

template <class Def, class ValueType>
struct op_elf_simple_value
  : public op_once_overload <value_cst, ValueType>
  , protected Def
{
  using op_once_overload <value_cst, ValueType>::op_once_overload;

  value_cst
  operate (std::unique_ptr <ValueType> a) const override
  {
    unsigned value = Def::value (a->get_dwctx ()->get_dwfl ());
    return value_cst {constant {value, &Def::cdom ()}, 0};
  }

  using Def::docstring;
};



struct elf_atclass_def
{
  static unsigned value (Dwfl *dwfl);
  static zw_cdom const &cdom ();
  static std::string docstring ();
};

template <class ValueType>
struct op_elf_atclass
  : public op_elf_simple_value <elf_atclass_def, ValueType>
{
  using op_elf_simple_value <elf_atclass_def, ValueType>
		::op_elf_simple_value;
};

template <class ValueType>
struct pred_elf_atclass
  : public pred_elf_simple_value <elf_atclass_def, ValueType>
{
  using pred_elf_simple_value <elf_atclass_def, ValueType>
		::pred_elf_simple_value;
};


struct elf_attype_def
{
  static unsigned value (Dwfl *dwfl);
  static zw_cdom const &cdom ();
  static std::string docstring ();
};

template <class ValueType>
struct op_elf_attype
  : public op_elf_simple_value <elf_attype_def, ValueType>
{
  using op_elf_simple_value <elf_attype_def, ValueType>
		::op_elf_simple_value;
};

template <class ValueType>
struct pred_elf_attype
  : public pred_elf_simple_value <elf_attype_def, ValueType>
{
  using pred_elf_simple_value <elf_attype_def, ValueType>
		::pred_elf_simple_value;
};


struct elf_atmachine_def
{
  static unsigned value (Dwfl *dwfl);
  static zw_cdom const &cdom ();
  static std::string docstring ();
};

template <class ValueType>
struct op_elf_atmachine
  : public op_elf_simple_value <elf_atmachine_def, ValueType>
{
  using op_elf_simple_value <elf_atmachine_def, ValueType>
		::op_elf_simple_value;
};

template <class ValueType>
struct pred_elf_atmachine
  : public pred_elf_simple_value <elf_atmachine_def, ValueType>
{
  using pred_elf_simple_value <elf_atmachine_def, ValueType>
		::pred_elf_simple_value;
};


struct elf_atflags_def
{
  static unsigned value (Dwfl *dwfl);
  static zw_cdom const &cdom ();
  static std::string docstring ();
};

template <class ValueType>
struct op_elf_atflags
  : public op_elf_simple_value <elf_atflags_def, ValueType>
{
  using op_elf_simple_value <elf_atflags_def, ValueType>
		::op_elf_simple_value;
};

#endif /* BUILTIN_ELF_H */
