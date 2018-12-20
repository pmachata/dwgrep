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

#ifndef BUILTIN_ELFSCN_H
#define BUILTIN_ELFSCN_H

#include "overload.hh"
#include "value-cst.hh"
#include "value-elf.hh"
#include "value-str.hh"

struct op_name_elfscn
  : public op_once_overload <value_str, value_elf_section>
{
  using op_once_overload::op_once_overload;

  value_str operate (std::unique_ptr <value_elf_section> a) const override;
  static std::string docstring ();
};

struct op_entry_elfscn
  : public op_yielding_overload <value, value_elf_section>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value>>
  operate (std::unique_ptr <value_elf_section> a) const override;
  static std::string docstring ();
};

struct op_link_elfscn
  : public op_overload <value_elf_section, value_elf_section>
{
  using op_overload::op_overload;

  std::unique_ptr <value_elf_section>
  operate (std::unique_ptr <value_elf_section> a) const override;
  static std::string docstring ();
};

struct op_info_elfscn
  : public op_overload <value, value_elf_section>
{
  using op_overload::op_overload;

  std::unique_ptr <value>
  operate (std::unique_ptr <value_elf_section> a) const override;
  static std::string docstring ();
};


template <class Def>
struct op_elfscn_simple_value
  : public op_once_overload <value_cst, value_elf_section>
  , protected Def
{
  using op_once_overload::op_once_overload;

  value_cst
  operate (std::unique_ptr <value_elf_section> a) const override
  {
    Dwfl *dwfl = a->get_dwctx ()->get_dwfl ();
    Elf_Scn *scn = a->get_scn ();
    auto value = Def::value (dwfl, scn);
    return value_cst {constant {value, &Def::cdom (dwfl)}, 0};
  }

  using Def::docstring;
};


struct elfscn_size_def
{
  static uint64_t value (Dwfl *dwfl, Elf_Scn *scn);
  static zw_cdom const &cdom (Dwfl *dwfl);
  static std::string docstring ();
};

struct op_size_elfscn
  : public op_elfscn_simple_value <elfscn_size_def>
{
  using op_elfscn_simple_value::op_elfscn_simple_value;
};


struct elfscn_entsize_def
{
  static uint64_t value (Dwfl *dwfl, Elf_Scn *scn);
  static zw_cdom const &cdom (Dwfl *dwfl);
  static std::string docstring ();
};

struct op_entsize_elfscn
  : public op_elfscn_simple_value <elfscn_entsize_def>
{
  using op_elfscn_simple_value::op_elfscn_simple_value;
};


struct elfscn_label_def
{
  static unsigned value (Dwfl *dwfl, Elf_Scn *scn);
  static zw_cdom const &cdom (Dwfl *dwfl);
  static std::string docstring ();
};

struct op_label_elfscn
  : public op_elfscn_simple_value <elfscn_label_def>
{
  using op_elfscn_simple_value::op_elfscn_simple_value;
};


struct elfscn_flags_def
{
  static unsigned value (Dwfl *dwfl, Elf_Scn *scn);
  static zw_cdom const &cdom (Dwfl *dwfl);
  static std::string docstring ();
};

struct op_flags_elfscn
  : public op_elfscn_simple_value <elfscn_flags_def>
{
  using op_elfscn_simple_value::op_elfscn_simple_value;
};

struct pred_flags_elfscn
  : public pred_overload <value_elf_section>
{
  unsigned m_flag;

  pred_flags_elfscn (unsigned flag)
    : m_flag {flag}
  {}

  pred_result result (value_elf_section &a) const override;
  static std::string docstring ();
};


struct elfscn_address_def
{
  static unsigned value (Dwfl *dwfl, Elf_Scn *scn);
  static zw_cdom const &cdom (Dwfl *dwfl);
  static std::string docstring ();
};

struct op_address_elfscn
  : public op_elfscn_simple_value <elfscn_address_def>
{
  using op_elfscn_simple_value::op_elfscn_simple_value;
};


struct elfscn_alignment_def
{
  static unsigned value (Dwfl *dwfl, Elf_Scn *scn);
  static zw_cdom const &cdom (Dwfl *dwfl);
  static std::string docstring ();
};

struct op_alignment_elfscn
  : public op_elfscn_simple_value <elfscn_alignment_def>
{
  using op_elfscn_simple_value::op_elfscn_simple_value;
};


struct elfscn_offset_def
{
  static unsigned value (Dwfl *dwfl, Elf_Scn *scn);
  static zw_cdom const &cdom (Dwfl *dwfl);
  static std::string docstring ();
};

struct op_offset_elfscn
  : public op_elfscn_simple_value <elfscn_offset_def>
{
  using op_elfscn_simple_value::op_elfscn_simple_value;
};

#endif /* BUILTIN_ELFSCN_H */
