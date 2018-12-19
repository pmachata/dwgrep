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
#include "value-seq.hh"
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



struct elf_eclass_def
{
  static unsigned value (Dwfl *dwfl);
  static zw_cdom const &cdom ();
  static std::string docstring ();
};

template <class ValueType>
struct op_eclass_elf
  : public op_elf_simple_value <elf_eclass_def, ValueType>
{
  using op_elf_simple_value <elf_eclass_def, ValueType>
		::op_elf_simple_value;
};

template <class ValueType>
struct pred_eclass_elf
  : public pred_elf_simple_value <elf_eclass_def, ValueType>
{
  using pred_elf_simple_value <elf_eclass_def, ValueType>
		::pred_elf_simple_value;
};


struct elf_edata_def
{
  static unsigned value (Dwfl *dwfl);
  static zw_cdom const &cdom ();
  static std::string docstring ();
};

template <class ValueType>
struct op_edata_elf
  : public op_elf_simple_value <elf_edata_def, ValueType>
{
  using op_elf_simple_value <elf_edata_def, ValueType>
		::op_elf_simple_value;
};

template <class ValueType>
struct pred_edata_elf
  : public pred_elf_simple_value <elf_edata_def, ValueType>
{
  using pred_elf_simple_value <elf_edata_def, ValueType>
		::pred_elf_simple_value;
};


struct elf_etype_def
{
  static unsigned value (Dwfl *dwfl);
  static zw_cdom const &cdom ();
  static std::string docstring ();
};

template <class ValueType>
struct op_etype_elf
  : public op_elf_simple_value <elf_etype_def, ValueType>
{
  using op_elf_simple_value <elf_etype_def, ValueType>
		::op_elf_simple_value;
};

template <class ValueType>
struct pred_etype_elf
  : public pred_elf_simple_value <elf_etype_def, ValueType>
{
  using pred_elf_simple_value <elf_etype_def, ValueType>
		::pred_elf_simple_value;
};


struct elf_emachine_def
{
  static unsigned value (Dwfl *dwfl);
  static zw_cdom const &cdom ();
  static std::string docstring ();
};

template <class ValueType>
struct op_emachine_elf
  : public op_elf_simple_value <elf_emachine_def, ValueType>
{
  using op_elf_simple_value <elf_emachine_def, ValueType>
		::op_elf_simple_value;
};

template <class ValueType>
struct pred_emachine_elf
  : public pred_elf_simple_value <elf_emachine_def, ValueType>
{
  using pred_elf_simple_value <elf_emachine_def, ValueType>
		::pred_elf_simple_value;
};


struct elf_eflags_def
{
  static unsigned value (Dwfl *dwfl);
  static zw_cdom const &cdom ();
  static std::string docstring ();
};

template <class ValueType>
struct op_eflags_elf
  : public op_elf_simple_value <elf_eflags_def, ValueType>
{
  using op_elf_simple_value <elf_eflags_def, ValueType>
		::op_elf_simple_value;
};


struct elf_osabi_def
{
  static unsigned value (Dwfl *dwfl);
  static zw_cdom const &cdom ();
  static std::string docstring ();
};

template <class ValueType>
struct op_osabi_elf
  : public op_elf_simple_value <elf_osabi_def, ValueType>
{
  using op_elf_simple_value <elf_osabi_def, ValueType>
		::op_elf_simple_value;
};

template <class ValueType>
struct pred_osabi_elf
  : public pred_elf_simple_value <elf_osabi_def, ValueType>
{
  using pred_elf_simple_value <elf_osabi_def, ValueType>
		::pred_elf_simple_value;
};


template <class ValueType>
struct op_version_elf
  : public op_once_overload <value_cst, ValueType>
{
  using op_once_overload <value_cst, ValueType>::op_once_overload;

  value_cst operate (std::unique_ptr <ValueType> a) const override;
  static std::string docstring ();
};

template <class ValueType>
struct op_eversion_elf
  : public op_once_overload <value_cst, ValueType>
{
  using op_once_overload <value_cst, ValueType>::op_once_overload;

  value_cst operate (std::unique_ptr <ValueType> a) const override;
  static std::string docstring ();
};

template <class ValueType>
struct op_eentry_elf
  : public op_once_overload <value_cst, ValueType>
{
  using op_once_overload <value_cst, ValueType>::op_once_overload;

  value_cst operate (std::unique_ptr <ValueType> a) const override;
  static std::string docstring ();
};

template <class ValueType>
struct op_abiversion_elf
  : public op_once_overload <value_cst, ValueType>
{
  using op_once_overload <value_cst, ValueType>::op_once_overload;

  value_cst operate (std::unique_ptr <ValueType> a) const override;
  static std::string docstring ();
};

template <class ValueType>
struct op_eident_elf
  : public op_once_overload <value_seq, ValueType>
{
  using op_once_overload <value_seq, ValueType>::op_once_overload;

  value_seq operate (std::unique_ptr <ValueType> a) const override;
  static std::string docstring ();
};

template <class ValueType>
struct op_shstr_elf
  : public op_once_overload <value_elf_section, ValueType>
{
  using op_once_overload <value_elf_section, ValueType>::op_once_overload;

  value_elf_section operate (std::unique_ptr <ValueType> a) const override;
  static std::string docstring ();
};

template <class ValueType>
struct op_section_elf
  : public op_yielding_overload <value_elf_section, ValueType>
{
  using op_yielding_overload <value_elf_section,
			      ValueType>::op_yielding_overload;

  std::unique_ptr <value_producer <value_elf_section>>
  operate (std::unique_ptr <ValueType> val) const override;
  static std::string docstring ();
};

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


struct op_value_strtab_entry
  : public op_once_overload <value_str, value_strtab_entry>
{
  using op_once_overload::op_once_overload;

  value_str operate (std::unique_ptr <value_strtab_entry> val) const override;
  static std::string docstring ();
};

#endif /* BUILTIN_ELF_H */
